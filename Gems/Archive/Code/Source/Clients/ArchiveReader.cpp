/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveReader.h"

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/OpenMode.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/Task/TaskGraph.h>

#include <Archive/ArchiveTypeIds.h>

#include <Compression/DecompressionInterfaceAPI.h>

namespace Archive
{
    // Implement TypeInfo, Rtti and Allocator support
    AZ_TYPE_INFO_WITH_NAME_IMPL(ArchiveReader, "ArchiveReader", ArchiveReaderTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(ArchiveReader, IArchiveReader);
    AZ_CLASS_ALLOCATOR_IMPL(ArchiveReader, AZ::SystemAllocator);

    ArchiveReader::ArchiveReader() = default;

    ArchiveReader::~ArchiveReader()
    {
        UnmountArchive();
    }

    ArchiveReader::ArchiveReader(const ArchiveReaderSettings& writerSettings)
        : m_settings(writerSettings)
    {}

    ArchiveReader::ArchiveReader(AZ::IO::PathView archivePath, const ArchiveReaderSettings& writerSettings)
        : m_settings(writerSettings)
    {
        MountArchive(archivePath);
    }

    ArchiveReader::ArchiveReader(ArchiveStreamPtr archiveStream, const ArchiveReaderSettings& writerSettings)
        : m_settings(writerSettings)
    {
        MountArchive(AZStd::move(archiveStream));
    }

    bool ArchiveReader::ReadArchiveHeader(ArchiveHeader& archiveHeader, AZ::IO::GenericStream& archiveStream)
    {
        // Read the Archive header into memory
        AZStd::scoped_lock archiveLock(m_archiveStreamMutex);
        archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        AZ::IO::SizeType bytesRead = archiveStream.Read(sizeof(archiveHeader), &archiveHeader);
        archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        if (bytesRead != sizeof(archiveHeader))
        {
            m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorReadingHeader, ArchiveReaderErrorString::format(
                "Archive header should have size %zu, but only %llu bytes were read from the beginning of the archive",
                sizeof(archiveHeader), bytesRead) });
        }

        return true;
    }

    // Define the Archive TableOfContentReader constructors
    ArchiveReader::ArchiveTableOfContentsReader::ArchiveTableOfContentsReader() = default;

    // Stores the buffer containing the Table of Contents raw data
    // and an ArchiveTableOfContentsView instance that points into that raw data
    ArchiveReader::ArchiveTableOfContentsReader::ArchiveTableOfContentsReader(AZStd::vector<AZStd::byte> tocBuffer,
        ArchiveTableOfContentsView tocView)
        : m_tocBuffer(AZStd::move(tocBuffer))
        , m_tocView(AZStd::move(tocView))
    {}

    bool ArchiveReader::ReadArchiveTOC(ArchiveTableOfContentsReader& archiveToc, AZ::IO::GenericStream& archiveStream,
        const ArchiveHeader& archiveHeader)
    {
        // RAII structure which resets the archive stream to offset 0
        // when it goes out of scope
        struct SeekStreamToBeginRAII
        {
            ~SeekStreamToBeginRAII()
            {
                m_stream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            }
            AZ::IO::GenericStream& m_stream;
        };

        if (archiveHeader.m_tocOffset > archiveStream.GetLength())
        {
            // The TOC offset is invalid since it is after the end of the stream
            m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorReadingTableOfContents,
                    ArchiveReaderErrorString::format("TOC offset is invalid. It is pass the end of the stream."
                        " Offset value %llu, archive stream size %llu",
                        static_cast<AZ::u64>(archiveHeader.m_tocOffset), archiveStream.GetLength()) });
            return false;
        }

        // Buffer which stores the raw table of contents data from the archive file
        AZStd::vector<AZStd::byte> tocBuffer;

        // Seek to the location of the Table of Contents
        {
            AZStd::scoped_lock archiveLock(m_archiveStreamMutex);
            // Make sure the archive offset is reset to 0 on return
            SeekStreamToBeginRAII seekToBeginScope{ archiveStream };
            archiveStream.Seek(archiveHeader.m_tocOffset, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

            tocBuffer.resize_no_construct(archiveHeader.GetTocStoredSize());
            AZ::IO::SizeType bytesRead = archiveStream.Read(tocBuffer.size(), tocBuffer.data());
            if (bytesRead != tocBuffer.size())
            {
                m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorReadingTableOfContents,
                    ArchiveReaderErrorString::format("Unable to read all TOC bytes from the archive."
                        " The TOC size is %zu, but only %llu bytes were read",
                        tocBuffer.size(), bytesRead) });
                return false;
            }
        }

        // Check if the archive table of contents is compressed
        if (archiveHeader.m_tocCompressionAlgoIndex < UncompressedAlgorithmIndex)
        {
            auto decompressionRegistrar = Compression::DecompressionRegistrar::Get();
            if (decompressionRegistrar == nullptr)
            {
                // The decompression registrar does not exist
                m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorReadingTableOfContents,
                    ArchiveReaderErrorString("The Decompression Registry is not available"
                        " Is the Compression gem active?") });
                return false;
            }

            Compression::CompressionAlgorithmId tocCompressionAlgorithmId =
                archiveHeader.m_compressionAlgorithmsIds[archiveHeader.m_tocCompressionAlgoIndex];

            Compression::IDecompressionInterface* decompressionInterface =
                decompressionRegistrar->FindDecompressionInterface(tocCompressionAlgorithmId);
            if (decompressionInterface == nullptr)
            {
                // Compression algorithm isn't registered with the decompression registrar
                m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorReadingTableOfContents,
                    ArchiveReaderErrorString::format("Compression Algorithm %u used by TOC"
                        " isn't registered with decompression registrar",
                        AZStd::to_underlying(tocCompressionAlgorithmId)) });
                return false;
            }

            // Resize the uncompressed TOC buffer to be the size of the uncompressed Table of Contents
            AZStd::vector<AZStd::byte> uncompressedTocBuffer;
            uncompressedTocBuffer.resize_no_construct(archiveHeader.GetUncompressedTocSize());

            // Run the compressed toc data through the decompressor
            if (Compression::DecompressionResultData decompressionResultData =
                decompressionInterface->DecompressBlock(uncompressedTocBuffer, tocBuffer, Compression::DecompressionOptions{});
                decompressionResultData)
            {

                // If decompression succeed, move the uncompressed buffer to the tocBuffer variable
                tocBuffer = AZStd::move(uncompressedTocBuffer);
                if (decompressionResultData.GetUncompressedByteCount() != tocBuffer.size())
                {
                    // The size of uncompressed size of the data does not match the total uncompressed
                    // TOC size read from the ArchiveHeader::GetUncompressedTocSize() function
                    m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorReadingTableOfContents,
                    ArchiveReaderErrorString::format("The uncompressed TOC size %llu does not match the total uncompressed size %zu"
                        " read from the archive header",
                        decompressionResultData.GetUncompressedByteCount(), tocBuffer.size()) });
                    return false;
                }
            }
        }

        // Wrap the table of contents in an reader structure that encapsulates the raw tocBuffer data on disk
        // and a view into the Table of Contents memory
        if (auto tocView = ArchiveTableOfContentsView::CreateFromArchiveHeaderAndBuffer(archiveHeader, tocBuffer);
            tocView)
        {
            archiveToc = ArchiveTableOfContentsReader{ AZStd::move(tocBuffer), AZStd::move(tocView).value() };
        }
        else
        {
            // Invoke the error callback indicating an error reading the table of contents
            m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorReadingTableOfContents, tocView.error().m_errorMessage });
            return false;
        }

        return true;
    }

    bool ArchiveReader::BuildFilePathMap(const ArchiveTableOfContentsView& tocView)
    {
        m_pathMap.clear();

        // Build a map of file path view to within the FilePathIndex array of the TOC View
        auto BuildViewOfFilePaths = [this, filePathBlobTable = &tocView.m_filePathBlob, filePathIndex = 0]
        (AZ::u64 filePathBlobOffset, AZ::u16 filePathSize) mutable
        {
            AZ::IO::PathView contentPathView(filePathBlobTable->substr(filePathBlobOffset, filePathSize));
            m_pathMap.emplace(contentPathView, filePathIndex++);
        };
        EnumerateFilePathIndexOffsets(BuildViewOfFilePaths, tocView);

        return true;
    }

    bool ArchiveReader::MountArchive(AZ::IO::PathView archivePath)
    {
        UnmountArchive();
        AZ::IO::FixedMaxPath mountPath{ archivePath };
        constexpr AZ::IO::OpenMode openMode =
            AZ::IO::OpenMode::ModeRead
            | AZ::IO::OpenMode::ModeBinary;

        m_archiveStream.reset(aznew AZ::IO::SystemFileStream(mountPath.c_str(), openMode));

        // Early return if the archive is not open
        if (!m_archiveStream->IsOpen())
        {
            m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorOpeningArchive, ArchiveReaderErrorString::format(
                "Archive with filename %s could not be open",
                mountPath.c_str()) });
            return false;
        }

        // If the Archive header and TOC could not be read
        // then unmount the archive and return false
        if (!ReadArchiveHeaderAndToc())
        {
            // UnmountArchive is invoked to reset
            // the Archive Header, TOC and the path map structures
            UnmountArchive();
            return false;
        }
        return true;
    }

    bool ArchiveReader::MountArchive(ArchiveStreamPtr archiveStream)
    {
        UnmountArchive();
        m_archiveStream = AZStd::move(archiveStream);

        if (m_archiveStream == nullptr || !m_archiveStream->IsOpen())
        {
            m_settings.m_errorCallback({ ArchiveReaderErrorCode::ErrorOpeningArchive,
                ArchiveReaderErrorString("Archive stream pointer is nullptr or not open") });
            return false;
        }

        if (!ReadArchiveHeaderAndToc())
        {
            // UnmountArchive is invoked to reset
            // the Archive Header, TOC and the path map structures
            UnmountArchive();
            return false;
        }
        return true;
    }

    bool ArchiveReader::ReadArchiveHeaderAndToc()
    {
        if (m_archiveStream == nullptr)
        {
            return false;
        }

        const bool mountResult = ReadArchiveHeader(m_archiveHeader, *m_archiveStream)
            && ReadArchiveTOC(m_archiveToc, *m_archiveStream, m_archiveHeader)
            && BuildFilePathMap(m_archiveToc.m_tocView);

        return mountResult;
    }

    void ArchiveReader::UnmountArchive()
    {
        if (m_archiveStream != nullptr && m_archiveStream->IsOpen())
        {
            // Clear the path mount on unmount as it has pointers
            // into the table of contents reader
            m_pathMap.clear();
            // Now clear the table of contents reader
            m_archiveToc = {};
            // Finally clear the archive header
            m_archiveHeader = {};
        }

        m_archiveStream.reset();
    }

    bool ArchiveReader::IsMounted() const
    {
        return m_archiveStream != nullptr && m_archiveStream->IsOpen();
    }

    ArchiveExtractFileResult ArchiveReader::ExtractFileFromArchive(AZStd::span<AZStd::byte> outputSpan,
        const ArchiveReaderFileSettings& fileSettings)
    {
        ArchiveListFileResult listResult;
        if (auto filePathString = AZStd::get_if<AZ::IO::PathView>(&fileSettings.m_filePathIdentifier);
            filePathString != nullptr)
        {
            listResult = ListFileInArchive(*filePathString);
        }
        else
        {
            // The only remaining alternative is the ArchiveFileToken
            // so use AZStd::get is used on a reference to the variant
            // Make sure the filePathToken points to file within the TOC
            const ArchiveFileToken archiveFileToken = AZStd::get<ArchiveFileToken>(fileSettings.m_filePathIdentifier);
            listResult = ListFileInArchive(archiveFileToken);
        }

        // Copy the result of listing the file in the archive to the extract result structure
        ArchiveExtractFileResult extractResult;
        extractResult.m_relativeFilePath = listResult.m_relativeFilePath;
        extractResult.m_filePathToken = listResult.m_filePathToken;
        extractResult.m_compressionAlgorithm = listResult.m_compressionAlgorithm;
        extractResult.m_uncompressedSize = listResult.m_uncompressedSize;
        extractResult.m_compressedSize = listResult.m_compressedSize;
        extractResult.m_offset = listResult.m_offset;
        extractResult.m_crc32 = listResult.m_crc32;
        extractResult.m_resultOutcome = listResult.m_resultOutcome;

        // If querying of the file within the archive failed,
        // then return the extract file result which copied the error
        // state from the list file result
        if (!extractResult)
        {
            return extractResult;
        }

        // Determine if the file is compressed
        const bool isFileCompressed = extractResult.m_compressionAlgorithm != Compression::Uncompressed
            && extractResult.m_compressionAlgorithm != Compression::Invalid;
        // Check if the file should be decompressed
        const bool shouldDecompressFile = fileSettings.m_decompressFile
            && isFileCompressed;

        // if the should be decompressed, decompress it
        if (shouldDecompressFile)
        {
            // If the decompressFile option is true, then decompress the file into the output buffer
            if (fileSettings.m_decompressFile)
            {
                // Decompress the data into the output span
                if (ReadCompressedFileOutcome readFileOutcome = ReadCompressedFileIntoBuffer(outputSpan,
                    fileSettings, extractResult);
                    readFileOutcome)
                {
                    // On success populate a span with the exact size of the file data read
                    // from the archive
                    extractResult.m_fileSpan = readFileOutcome.value();
                }
                else
                {
                    extractResult.m_resultOutcome = AZStd::unexpected(AZStd::move(readFileOutcome.error()));
                }

            }
        }
        else
        {
            // When performing a raw read, use the knowledge of the file being compressed
            // to decide the file size to read
            AZ::u64 fileSize = isFileCompressed ? extractResult.m_compressedSize : extractResult.m_uncompressedSize;

            // Read the raw file data directly into the output span if possible
            if (ReadRawFileOutcome readFileOutcome = ReadRawFileIntoBuffer(outputSpan, extractResult.m_offset,
                fileSize, fileSettings);
                readFileOutcome)
            {
                // On success populate a span with the exact size of the file data read
                // from the archive
                extractResult.m_fileSpan = readFileOutcome.value();
            }
            else
            {
                extractResult.m_resultOutcome = AZStd::unexpected(AZStd::move(readFileOutcome.error()));
            }
        }

        return extractResult;
    }

    auto ArchiveReader::ReadRawFileIntoBuffer(AZStd::span<AZStd::byte> fileBuffer, AZ::u64 offset,
        AZ::u64 fileSize,
        const ArchiveReaderFileSettings& fileSettings)
        -> ReadRawFileOutcome
    {
        // Calculate the start offset where to read the file content from
        // It must be within the the range of [offset, offset + size)
        AZ::IO::OffsetType readOffset = AZStd::clamp(offset, offset + fileSettings.m_startOffset, offset + fileSize);
        // Next clamp the bytesToRead to not read pass the end of the file
        AZ::IO::SizeType bytesAvailableForRead = (offset + fileSize) - readOffset;

        // Set the amount of bytes to read to be the minimum of the file size and the amount of bytes to read
        AZ::IO::SizeType bytesToRead = AZStd::min(bytesAvailableForRead, fileSettings.m_bytesToRead);
        if (fileBuffer.size() < bytesToRead)
        {
            return AZStd::unexpected(ResultString::format("Buffer size is not large enough to read the raw file data at"
                " archive file offset %lld."
                " Buffer size is %zu, while %llu is required.", readOffset, fileBuffer.size(), bytesToRead));
        }

        AZStd::scoped_lock archiveReadLock(m_archiveStreamMutex);
        if (AZ::IO::SizeType bytesRead = m_archiveStream->ReadAtOffset(bytesToRead, fileBuffer.data(), readOffset);
            bytesRead < bytesToRead)
        {
            return AZStd::unexpected(ResultString::format("Attempted to read %llu bytes from the archive at offset %lld."
                " But only %llu bytes were able to be read.", bytesToRead, readOffset, bytesRead));
        }

        // Make a span with the exact amount of data read
        return fileBuffer.first(bytesToRead);
    }

    auto ArchiveReader::ReadCompressedFileIntoBuffer(AZStd::span<AZStd::byte> decompressionResultSpan,
        const ArchiveReaderFileSettings& fileSettings,
        const ArchiveExtractFileResult& extractFileResult) -> ReadCompressedFileOutcome
    {
        // If the file is empty, there is nothing to decompress
        if (extractFileResult.m_uncompressedSize == 0)
        {
            // Return a successful expectation with an empty span
            return {};
        }

        auto decompressionRegistrar = Compression::DecompressionRegistrar::Get();
        if (decompressionRegistrar == nullptr)
        {
            return AZStd::unexpected(ResultString("Decompression Registrar is not available. File cannot be decompressed"));
        }

        Compression::IDecompressionInterface* decompressionInterface =
            decompressionRegistrar->FindDecompressionInterface(extractFileResult.m_compressionAlgorithm);
        if (decompressionInterface == nullptr)
        {
            return AZStd::unexpected(ResultString::format("Compression Algorithm with ID %x is not registered"
                " with the decompression registrar.",
                static_cast<AZ::u32>(extractFileResult.m_compressionAlgorithm)));
        }

        // Retrieve a subspan of the block lines for the file being extracted
        // The file path token doubles as the index into the table of contents FileMetadataTable and FilePathIndexTable vector
        auto fileMetadataTableIndex = static_cast<AZ::u64>(extractFileResult.m_filePathToken);
        auto blockLineSpanOutcome = GetBlockLineSpanForFile(m_archiveToc.m_tocView, fileMetadataTableIndex);
        if (!blockLineSpanOutcome)
        {
            // Return the error for retrieving the block line for the file
            return AZStd::unexpected(AZStd::move(blockLineSpanOutcome.error()));
        }

        AZStd::span<const ArchiveBlockLineUnion> fileBlockLineSpan = blockLineSpanOutcome.value();

        // Determine the range of compressed blocks within the file to read
        // The cap is uncompressed size of the file
        if (fileSettings.m_startOffset > extractFileResult.m_uncompressedSize)
        {
            return AZStd::unexpected(ResultString::format("Start offset %llu to read file data from is larger."
                " than the size of the file %llu for file %s", fileSettings.m_startOffset,
                extractFileResult.m_compressedSize,
                extractFileResult.m_relativeFilePath.c_str()));
        }

        // Clamp the bytes that can read for the file to be at
        // most the difference in uncompressed size and the start offset
        AZ::u64 maxBytesToReadForFile = AZStd::min(fileSettings.m_bytesToRead,
            extractFileResult.m_uncompressedSize - fileSettings.m_startOffset);

        // Set the amount of bytes to read to be the minimum of the file size and the amount of bytes to read
        auto blockRange = GetBlockRangeToRead(fileSettings.m_startOffset, maxBytesToReadForFile);

        // Get the number of 2-MiB blocks for the file
        AZ::u32 blockCount = GetBlockCountIfCompressed(extractFileResult.m_uncompressedSize);

        // First calculate if the first block line is a jump entry
        // If there are more than 3 blocks lines, then the file contains at least a jump from
        // block line[0] -> block line[3] and the file contains at least 10 blocks of data
        // If the file only contains 3 block lines, then there would not be a jump entry
        // and the file would contain at most 9 blocks
        // See the ArchiveInterfaceStructs.h header for more information

        // The aligned seek offset where to read to start reading the compressed
        // data will calculated by adding up the 512-byte aligned sizes of each compressed block
        AZ::IO::SizeType alignedFirstSeekOffset{};

        for (AZ::u64 blockIndex{}; blockIndex < blockRange.first;)
        {
            // The internal archive code will never trigger the error case of (blockIndex >= blockCount),
            // so checking it will be skipped
            size_t blockLineIndex = GetBlockLineIndexFromBlockIndex(blockCount, blockIndex).m_blockLineIndex;
            // Block line indices which are multiples of 3 all have jump entries unless they are part of the final 3 block
            // lines of a file
            const bool blockLineContainsJump = (blockLineIndex % BlockLinesToSkipWithJumpEntry == 0)
                && (fileBlockLineSpan.size() - blockLineIndex) > BlockLinesToSkipWithJumpEntry;
            if (blockLineContainsJump)
            {
                const ArchiveBlockLineJump& blockLineWithJump = fileBlockLineSpan[blockLineIndex].m_blockLineWithJump;
                // there is a jump entry for the file, so the logic gets a bit more trickier
                // First check if the blockIndex + BlocksToSkipWithJumpEntry is less than blockRange.first
                if (blockIndex + BlocksToSkipWithJumpEntry < blockRange.first)
                {
                    // In this case the jump entry can be used to skip the next 8 blocks(3 block lines)
                    // The jump entry contains the number of 512-byte sectors the next 8 blocks
                    // take in the raw file section of the archive
                    // The value is multiplied by ArchiveDefaultBlockAlignment to get the correct value
                    alignedFirstSeekOffset += blockLineWithJump.m_blockJump * ArchiveDefaultBlockAlignment;

                    // Increment the block index by 8, as it was the number of blocks that were skipped
                    blockIndex += BlocksToSkipWithJumpEntry;
                }
                else
                {
                    // Otherwise process up to the remaining two block entries in this block line if possible
                    alignedFirstSeekOffset += AZ_SIZE_ALIGN_UP(blockLineWithJump.m_block0, ArchiveDefaultBlockAlignment);
                    ++blockIndex;

                    // If the blockIndex is still less than the beginning of the block range to read
                    // then grab the second and final block from the block line
                    if ((blockIndex + 1) < blockRange.first)
                    {
                        alignedFirstSeekOffset += AZ_SIZE_ALIGN_UP(blockLineWithJump.m_block1, ArchiveDefaultBlockAlignment);
                        ++blockIndex;
                    }
                }
            }
            else
            {
                // There aren't anymore jump entry for the file so accumulate the aligned compressed block offsets
                const ArchiveBlockLine& blockLine = fileBlockLineSpan[blockLineIndex].m_blockLine;
                // Try to process up to 3 block lines per for loop iteration
                // This allows skipping the blockIndex / BlocksPerBlockLine division twice
                // If the blockIndex is within 3 of the blockRange.first value then up to that amount of blocks are processed
                const AZ::u64 blocksToProcess = AZStd::min(BlocksPerBlockLine, blockRange.first - blockIndex);

                // Align all the compressed sizes up to 512-byte alignment to get the correct
                // seek offset for the file
                // blocksToProcess is >=1 due to the for loop condition
                alignedFirstSeekOffset += AZ_SIZE_ALIGN_UP(blockLine.m_block0, ArchiveDefaultBlockAlignment);
                alignedFirstSeekOffset += blocksToProcess >= BlocksPerBlockLine - 1
                    ? AZ_SIZE_ALIGN_UP(blockLine.m_block1, ArchiveDefaultBlockAlignment)
                    : 0;
                alignedFirstSeekOffset += blocksToProcess >= BlocksPerBlockLine
                    ? AZ_SIZE_ALIGN_UP(blockLine.m_block2, ArchiveDefaultBlockAlignment)
                    : 0;

                // Increment the block index by the blocks that were processed
                blockIndex += blocksToProcess;
            }
        }

        // Stores the list of compressed blocks to decompress
        AZStd::vector<AZStd::byte> compressedBlocks;
        compressedBlocks.resize_no_construct((blockRange.second - blockRange.first) * ArchiveBlockSizeForCompression);
        AZStd::span<AZStd::byte> compressedBlockRemainingSpan = compressedBlocks;

        AZ::IO::SizeType fileRelativeSeekOffset = alignedFirstSeekOffset;
        for (AZ::u64 blockIndex = blockRange.first; blockIndex != blockRange.second; ++blockIndex)
        {
            const AZ::u64 blockCompressedSize = GetCompressedSizeForBlock(fileBlockLineSpan, blockCount, blockIndex);
            // Get the next 2 MiB block (or less if in the final block) of memory to store the compressed block data
            const auto availableBytesInCompressedBlock = AZStd::min<size_t>(compressedBlockRemainingSpan.size(),
                ArchiveBlockSizeForCompression);
            const AZStd::span<AZStd::byte> compressedBlockToReadInto = compressedBlockRemainingSpan.first(
                availableBytesInCompressedBlock);
            // Slide the compressed block remaining span view ahead by the 2 MiB that is being used for the read span
            compressedBlockRemainingSpan = compressedBlockRemainingSpan.subspan(availableBytesInCompressedBlock);
            const AZ::u64 absoluteSeekOffset = extractFileResult.m_offset + fileRelativeSeekOffset;
            if (AZ::IO::SizeType bytesRead = m_archiveStream->ReadAtOffset(blockCompressedSize,
                compressedBlockToReadInto.data(), absoluteSeekOffset);
                bytesRead != blockCompressedSize)
            {
                return AZStd::unexpected(ResultString::format("Cannot read all of compressed block for"
                    " block %llu. The compressed block size is %llu, but only %llu was able to be read",
                    blockIndex, blockCompressedSize, bytesRead));
            }

            // As the read was successful add the aligned compressed size to the fileRelativeSeekOffset
            // The value is the read offset where the next block data starts
            fileRelativeSeekOffset += AZ_SIZE_ALIGN_UP(blockCompressedSize, ArchiveDefaultBlockAlignment);
        }

        // Reset the compressed block remaining to the start of the compressedBlocks vector
        compressedBlockRemainingSpan = compressedBlocks;
        // The span below is used to slide a 2 MiB window for storing decompressed file contents
        AZStd::span<AZStd::byte> decompressionRemainingSpan = decompressionResultSpan;

        // Get a reference to the the caller supplied decompression options if available
        const auto& decompressionOptions = fileSettings.m_decompressionOptions != nullptr
            ? *fileSettings.m_decompressionOptions
            : Compression::DecompressionOptions{};

        // m_maxDecompressTasks has a minimum value of 1
        // This makes sure there is never a scenario where the there are blocks to decompress
        // but the decompress task count is 0
        const AZ::u32 maxDecompressTasks = AZStd::min(
            AZStd::max(1U, m_settings.m_maxDecompressTasks),
            static_cast<AZ::u32>(blockRange.second - blockRange.first));
        AZStd::vector<Compression::DecompressionResultData> decompressedBlockResults(maxDecompressTasks);

        for (AZ::u64 blockIndex = blockRange.first; blockIndex < blockRange.second;)
        {
            // Determine the number of decompression task that can be run in parallel
            const AZ::u32 decompressTaskCount = AZStd::min(static_cast<AZ::u32>(blockRange.second - blockIndex),
                maxDecompressTasks);

            // Task graph event used to block decompressing blocks in parallel
            auto taskDecompressGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>("Content File Decompress Sync");
            AZ::TaskGraph taskGraph{ "Archive Decompress Tasks" };
            AZ::TaskDescriptor decompressTaskDescriptor{ "Decompress Block", "Archive Content File Decompression" };

            // Increment the block index as part of the inner loop that creates the decompression task
            for (AZ::u32 decompressTaskSlot = 0; decompressTaskSlot < decompressTaskCount; ++decompressTaskSlot,
                ++blockIndex)
            {
                const AZ::u64 blockCompressedSize = GetCompressedSizeForBlock(fileBlockLineSpan, blockCount, blockIndex);
                // Downsize the 2 MiB span that was used to read the compressed data to the exact compressed size
                const auto availableBytesInCompressedBlock = AZStd::min<size_t>(compressedBlockRemainingSpan.size(),
                    ArchiveBlockSizeForCompression);
                AZStd::span<const AZStd::byte> compressedDataForBlock = compressedBlockRemainingSpan
                    .first(availableBytesInCompressedBlock)
                    .first(blockCompressedSize);
                // Slide the compressed block remaining span by 2 MiB
                compressedBlockRemainingSpan = compressedBlockRemainingSpan.subspan(availableBytesInCompressedBlock);

                // Get the block span for storing the decompressed block
                // As the uncompressed size is 2 MiB for all blocks except the last
                // the entire contiguous file sequence will be available in the decompressedResultSpan after the loop
                const auto remainingBytesInBlockSpan = AZStd::min<size_t>(decompressionRemainingSpan.size(),
                    ArchiveBlockSizeForCompression);
                AZStd::span<AZStd::byte> decompressionBlockSpan = decompressionRemainingSpan
                    .first(remainingBytesInBlockSpan);
                // Slide the remaining decompressed span by 2 MiB as well
                decompressionRemainingSpan = decompressionRemainingSpan.subspan(remainingBytesInBlockSpan);

                //! Decompress Task to execute in task executor
                auto decompressTask = [decompressionInterface, &decompressionOptions, decompressionBlockSpan, compressedDataForBlock,
                    &decompressedBlockResult = decompressedBlockResults[decompressTaskSlot]]()
                {
                    // Decompressed the compressed block
                    decompressedBlockResult = decompressionInterface->DecompressBlock(
                        decompressionBlockSpan, compressedDataForBlock, decompressionOptions);
                };

                taskGraph.AddTask(decompressTaskDescriptor, AZStd::move(decompressTask));
            }

            taskGraph.SubmitOnExecutor(m_taskExecutor, taskDecompressGraphEvent.get());
            // Sync on the task completion
            taskDecompressGraphEvent->Wait();

            // Validate the decompression for all blocks
            for (AZ::u32 decompressTaskSlot = 0; decompressTaskSlot < decompressTaskCount; ++decompressTaskSlot)
            {
                const auto& decompressedBlockResult = decompressedBlockResults[decompressTaskSlot];
                if (!decompressedBlockResult)
                {
                    // If one of the decompression task fails, early return with the error message
                    return AZStd::unexpected(AZStd::move(decompressedBlockResult.m_decompressionOutcome.m_resultString));
                }
            }
        }

        // Return a subspan that accounts for the start offset within the compressed file to start
        // reading from, up to the bytes read amount
        // Due to the logic in the function only reading the set of 2 MiB blocks that are needed
        // the start offset for reading is calculated by a modulo operation
        // with the ArchiveBlockSizeForCompression (2 MiB).
        // The start offset will always be in the first read block
        size_t startOffset = fileSettings.m_startOffset % ArchiveBlockSizeForCompression;
        size_t endOffset = AZStd::min<size_t>(decompressionResultSpan.size() - startOffset, maxBytesToReadForFile);
        return decompressionResultSpan.subspan(startOffset, endOffset);
    }

    ArchiveListFileResult ArchiveReader::ListFileInArchive(ArchiveFileToken archiveFileToken) const
    {
        if (static_cast<AZ::u64>(archiveFileToken) > m_archiveToc.m_tocView.m_filePathIndexTable.size())
        {
            ArchiveListFileResult errorResult;
            errorResult.m_filePathToken = archiveFileToken;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString::format(R"(A file token "%llu" is being used to extract the file and that token does not point)"
                    " to a file within the archive TOC.", static_cast<AZ::u64>(archiveFileToken)));
            return errorResult;
        }

        // Populate the path view from the Table of Contents View
        ArchiveTocFilePathIndex filePathOffsetSize = m_archiveToc.m_tocView.m_filePathIndexTable[
            static_cast<AZ::u64>(archiveFileToken)];

        if (filePathOffsetSize.m_size == 0)
        {
            ArchiveListFileResult errorResult;
            errorResult.m_filePathToken = archiveFileToken;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString::format(R"(A file token "%llu" is being used to extract the file,)"
                    " but the file path stored in the TOC is empty."
                    "This indicates that the token is referring to a deleted file.",
                    static_cast<AZ::u64>(archiveFileToken)));
            return errorResult;
        }

        // The file has been found and has a non-empty path
        // Populate the ArchiveListFileResult structure
        ArchiveListFileResult listResult;
        // Extract the path stored in the file path blob into the extract result
        listResult.m_relativeFilePath = AZ::IO::PathView(
            m_archiveToc.m_tocView.m_filePathBlob.substr(filePathOffsetSize.m_offset, filePathOffsetSize.m_size));
        listResult.m_filePathToken = archiveFileToken;

        // Gather the file metadata
        const ArchiveTocFileMetadata& fileMetadata = m_archiveToc.m_tocView.m_fileMetadataTable[
            static_cast<AZ::u64>(archiveFileToken)];

        // Use the compression algorithm index to lookup the compression algorithm ID
        // if the file the value is less than the size of the compression AlgorithmIds array
        if (fileMetadata.m_compressionAlgoIndex < m_archiveHeader.m_compressionAlgorithmsIds.size())
        {
            listResult.m_compressionAlgorithm = m_archiveHeader.m_compressionAlgorithmsIds[
                fileMetadata.m_compressionAlgoIndex];
        }

        listResult.m_uncompressedSize = fileMetadata.m_uncompressedSize;
        if (auto rawFileSizeResult = GetRawFileSize(fileMetadata, m_archiveToc.m_tocView.m_blockOffsetTable);
            !rawFileSizeResult)
        {
            ArchiveListFileResult errorResult;
            errorResult.m_filePathToken = archiveFileToken;
            // Take the error from GetRawFileSize call and return that
            errorResult.m_resultOutcome = AZStd::unexpected(AZStd::move(rawFileSizeResult.error()));
            return errorResult;
        }
        else
        {
            listResult.m_compressedSize = rawFileSizeResult.value();
        }
        listResult.m_offset = fileMetadata.m_offset;
        listResult.m_crc32 = fileMetadata.m_crc32;

        return listResult;
    }

    ArchiveListFileResult ArchiveReader::ListFileInArchive(AZ::IO::PathView relativePath) const
    {
        if (relativePath.empty())
        {
            ArchiveListFileResult errorResult;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString("An empty file path has been supplied and cannot be found in the archive."));
            return errorResult;
        }
        auto foundIt = m_pathMap.find(relativePath);
        if (foundIt == m_pathMap.end())
        {
            ArchiveListFileResult errorResult;
            errorResult.m_relativeFilePath = AZStd::move(relativePath);
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString::format(R"(The file path "%.*s")"
                    " does not exist in the archive.", AZ_PATH_ARG(errorResult.m_relativeFilePath)));
            return errorResult;
        }

        // Now that the file has been found, pass in the ArchiveFileToken to the
        // the other overload
        return ListFileInArchive(static_cast<ArchiveFileToken>(foundIt->second));
    }

    bool ArchiveReader::ContainsFile(AZ::IO::PathView relativePath) const
    {
        return static_cast<bool>(ListFileInArchive(relativePath));
    }

    EnumerateArchiveResult ArchiveReader::EnumerateFilesInArchive(ListFileCallback listFileCallback) const
    {
        ResultOutcome fileResultOutcome;
        // Lambda is marked mutable to allow the filePathIndex variable to be incremented each call
        auto EnumerateAllFiles = [&listFileCallback, &fileResultOutcome, this, filePathIndex = 0]
        (AZ::u64 filePathBlobOffset, AZ::u16 filePathSize) mutable
            {
                // Invoke callback on each file with a non-empty path
                if (AZ::IO::PathView contentPathView(m_archiveToc.m_tocView.m_filePathBlob.substr(filePathBlobOffset, filePathSize));
                    !contentPathView.empty())
                {
                    ArchiveListFileResult listResult;
                    listResult.m_relativeFilePath = contentPathView;
                    listResult.m_filePathToken = static_cast<ArchiveFileToken>(filePathIndex);

                    // Gather the file metadata
                    const ArchiveTocFileMetadata& fileMetadata = m_archiveToc.m_tocView.m_fileMetadataTable[filePathIndex];

                    // Use the compression algorithm index to lookup the compression algorithm ID
                    if (fileMetadata.m_compressionAlgoIndex < m_archiveHeader.m_compressionAlgorithmsIds.size())
                    {
                        listResult.m_compressionAlgorithm = m_archiveHeader.m_compressionAlgorithmsIds[
                            fileMetadata.m_compressionAlgoIndex];
                    }

                    listResult.m_uncompressedSize = fileMetadata.m_uncompressedSize;
                    if (auto rawFileSizeResult = GetRawFileSize(fileMetadata, m_archiveToc.m_tocView.m_blockOffsetTable);
                        !rawFileSizeResult)
                    {
                        fileResultOutcome = AZStd::unexpected(AZStd::move(rawFileSizeResult.error()));
                        return;
                    }
                    else
                    {
                        listResult.m_compressedSize = rawFileSizeResult.value();
                    }
                    listResult.m_offset = fileMetadata.m_offset;
                    listResult.m_crc32 = fileMetadata.m_crc32;

                    listFileCallback(AZStd::move(listResult));
                }
                ++filePathIndex;
            };
        EnumerateFilePathIndexOffsets(EnumerateAllFiles, m_archiveToc.m_tocView);

        // There are currently no error messages that enumerate file path sets
        // So a default constructed instance which converts to boolean true is returned
        return {};
    }

    bool ArchiveReader::DumpArchiveMetadata(AZ::IO::GenericStream& metadataStream,
        const ArchiveMetadataSettings& metadataSettings) const
    {
        using MetadataString = AZStd::fixed_string<256>;
        if (metadataSettings.m_writeFileCount)
        {
            auto fileCountString = MetadataString::format("Total File Count: %u\n", m_archiveHeader.m_fileCount);
            metadataStream.Write(fileCountString.size(), fileCountString.data());
        }

        if (metadataSettings.m_writeFilePaths)
        {
            // Validate the file path and file metadata tables are in sync
            if (m_archiveToc.m_tocView.m_filePathIndexTable.size() != m_archiveToc.m_tocView.m_fileMetadataTable.size())
            {
                auto errorString = MetadataString::format("Error: The Archive TOC of contents has a mismatched size between"
                    " the file path index vector (size=%zu) and the file metadata vector (size=%zu).\n"
                    "This indicates a code error in the ArchiveReader.",
                    m_archiveToc.m_tocView.m_filePathIndexTable.size(), m_archiveToc.m_tocView.m_fileMetadataTable.size());
                metadataStream.Write(errorString.size(), errorString.data());
                return false;
            }

            // Trackes the index of the file being output
            size_t activeFileOffset{};

            for (size_t filePathIndexTableIndex{}; filePathIndexTableIndex < m_archiveToc.m_tocView.m_filePathIndexTable.size();
                ++filePathIndexTableIndex)
            {
                // Use the FilePathIndex entry to lookup the offset and size of the file path within the FilePath blob
                const auto& contentFilePathIndex = m_archiveToc.m_tocView.m_filePathIndexTable[filePathIndexTableIndex];
                AZ::IO::PathView contentFilePath(m_archiveToc.m_tocView.m_filePathBlob.substr(contentFilePathIndex.m_offset,
                    contentFilePathIndex.m_size));
                // An empty file path is used to track removed files from the archive,
                // therefore only non-empty paths are iterated
                if (!contentFilePath.empty())
                {
                    const ArchiveTocFileMetadata& contentFileMetadata = m_archiveToc.m_tocView.m_fileMetadataTable[filePathIndexTableIndex];
                    auto fileMetadataString = MetadataString::format(R"(File %zu: path="%.*s")", activeFileOffset, AZ_PATH_ARG(contentFilePath));
                    if (metadataSettings.m_writeFileOffsets)
                    {
                        fileMetadataString += MetadataString::format(R"(, offset=%llu)", contentFileMetadata.m_offset);
                    }
                    if (metadataSettings.m_writeFileSizesAndCompression)
                    {
                        fileMetadataString += MetadataString::format(R"(, uncompressed_size=%llu)", contentFileMetadata.m_uncompressedSize);
                        // Only output compressed size if an compression that compresses data is being used
                        if (contentFileMetadata.m_compressionAlgoIndex < UncompressedAlgorithmIndex)
                        {
                            if (auto rawFileSizeResult = GetRawFileSize(contentFileMetadata, m_archiveToc.m_tocView.m_blockOffsetTable);
                                rawFileSizeResult)
                            {
                                AZ::u64 compressedSize = rawFileSizeResult.value();
                                fileMetadataString += MetadataString::format(R"(, compressed_size=%llu)",
                                    compressedSize);
                            }
                            fileMetadataString += MetadataString::format(R"(, compression_algorithm_id=%u)",
                                AZStd::to_underlying(m_archiveHeader.m_compressionAlgorithmsIds[contentFileMetadata.m_compressionAlgoIndex]));
                        }
                    }

                    // Append a newline before writing to the stream
                    fileMetadataString.push_back('\n');
                    metadataStream.Write(fileMetadataString.size(), fileMetadataString.data());

                    // Increment the active file offset for non-removed files
                    ++activeFileOffset;
                }
            }
        }
        return true;
    }
} // namespace Archive

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveWriter.h"

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/OpenMode.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Task/TaskGraph.h>

#include <Archive/ArchiveTypeIds.h>

#include <Compression/CompressionInterfaceAPI.h>
#include <Compression/DecompressionInterfaceAPI.h>

namespace Archive
{
    // Implement TypeInfo, Rtti and Allocator support
    AZ_TYPE_INFO_WITH_NAME_IMPL(ArchiveWriter, "ArchiveWriter", ArchiveWriterTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(ArchiveWriter, IArchiveWriter);
    AZ_CLASS_ALLOCATOR_IMPL(ArchiveWriter, AZ::SystemAllocator);

    ArchiveWriter::ArchiveWriter() = default;

    ArchiveWriter::~ArchiveWriter()
    {
        UnmountArchive();
    }

    ArchiveWriter::ArchiveWriter(const ArchiveWriterSettings& writerSettings)
        : m_settings(writerSettings)
    {}

    ArchiveWriter::ArchiveWriter(AZ::IO::PathView archivePath, const ArchiveWriterSettings& writerSettings)
        : m_settings(writerSettings)
    {
        MountArchive(archivePath);
    }

    ArchiveWriter::ArchiveWriter(ArchiveStreamPtr archiveStream, const ArchiveWriterSettings& writerSettings)
        : m_settings(writerSettings)
    {
        MountArchive(AZStd::move(archiveStream));
    }

    bool ArchiveWriter::ReadArchiveHeader(ArchiveHeader& archiveHeader, AZ::IO::GenericStream& archiveStream)
    {
        // Read the Archive header into memory
        AZStd::scoped_lock archiveLock(m_archiveStreamMutex);
        archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        AZ::IO::SizeType bytesRead = archiveStream.Read(sizeof(archiveHeader), &archiveHeader);
        archiveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        if (bytesRead != sizeof(archiveHeader))
        {
            m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingHeader, ArchiveWriterErrorString::format(
                "Archive header should have size %zu, but only %llu bytes were read from the beginning of the archive",
                sizeof(archiveHeader), bytesRead) });
        }
        else if (auto archiveValidationResult = ValidateHeader(archiveHeader);
            archiveValidationResult)
        {
            m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingHeader,
                archiveValidationResult.m_errorMessage });
        }

        return true;
    }

    bool ArchiveWriter::ReadArchiveTOC(ArchiveTableOfContents& archiveToc, AZ::IO::GenericStream& archiveStream,
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
            m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingTableOfContents,
                    ArchiveWriterErrorString::format("TOC offset is invalid. It is pass the end of the stream."
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
                m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingTableOfContents,
                    ArchiveWriterErrorString::format("Unable to read all TOC bytes from the archive."
                        " The TOC size is %zu, but only %llu bytes were read",
                        tocBuffer.size(), bytesRead)});
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
                m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingTableOfContents,
                    ArchiveWriterErrorString("The Decompression Registry is not available"
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
                m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingTableOfContents,
                    ArchiveWriterErrorString::format("Compression Algorithm %u used by TOC"
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
                    m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingTableOfContents,
                    ArchiveWriterErrorString::format("The uncompressed TOC size %llu does not match the total uncompressed size %zu"
                        " read from the archive header",
                        decompressionResultData.GetUncompressedByteCount(), tocBuffer.size()) });
                    return false;
                }
            }
        }

        // Map the Table of Contents to a structure that makes it easier to maintain
        // the Archive TOC state in memory and allows adding to the existing tables
        if (auto tocView = ArchiveTableOfContentsView::CreateFromArchiveHeaderAndBuffer(archiveHeader, tocBuffer);
            tocView)
        {
            if (auto archiveTocCreateOutcome = ArchiveTableOfContents::CreateFromTocView(tocView.value());
                archiveTocCreateOutcome)
            {
                archiveToc = AZStd::move(archiveTocCreateOutcome.value());
            }
            else
            {
                m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingTableOfContents, archiveTocCreateOutcome.error() });
                return false;
            }
        }
        else
        {
            // Invoke the error callback indicating an error reading the table of contents
            m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorReadingTableOfContents, tocView.error().m_errorMessage });
            return false;
        }

        return true;
    }

    bool ArchiveWriter::BuildDeletedFileBlocksMap(const ArchiveHeader& archiveHeader,
        AZ::IO::GenericStream& archiveStream)
    {
        // Build the deleted block map using the the first deleted block offset
        // in the Archive Header
        AZ::u64 nextBlock{ DeletedBlockOffsetSentinel };
        for (AZ::u64 deletedBlockOffset = archiveHeader.m_firstDeletedBlockOffset;
            deletedBlockOffset != DeletedBlockOffsetSentinel; deletedBlockOffset = nextBlock)
        {
            // initialize the next block value to the deleted block offset sentinel value
            // of 0xffff'ffff'ffff'ffff

            if (archiveStream.Read(sizeof(nextBlock), &nextBlock) != sizeof(nextBlock))
            {
                // If the next block offset cannot be read in force the deletedBlockOffset
                // to be the deleted block offset sentinel value to exit the loop
                nextBlock = DeletedBlockOffsetSentinel;
            }

            // Read in the size of the deleted block
            if (AZ::u64 blockSize{};
                archiveStream.Read(sizeof(blockSize), &blockSize) == sizeof(blockSize))
            {
                // If the block size has been successfully read, update the deleted block offset map
                // with a key of the block size which maps to a sorted set which contains
                // the current iterated block
                // Make sure any block size is aligned UP to a 512-byte boundary
                // and any block offset is aligned UP to a 512-byte boundary
                // This prevents issues with writing block data to a non-aligned block
                AZ::u64 alignedBlockSize = AZ_SIZE_ALIGN_UP(blockSize, ArchiveDefaultBlockAlignment);
                AZ::u64 alignedBlockOffset = AZ_SIZE_ALIGN_UP(deletedBlockOffset, ArchiveDefaultBlockAlignment);

                if (alignedBlockSize > 0)
                {
                    auto& deletedBlockOffsetSet = m_deletedBlockSizeToOffsetMap[alignedBlockSize];
                    deletedBlockOffsetSet.emplace(alignedBlockOffset);
                }
            }
        }

        MergeContiguousDeletedBlocks();

        return true;
    }

    void ArchiveWriter::MergeContiguousDeletedBlocks()
    {
        AZStd::map<AZ::u64, AZ::u64> deletedBlockOffsetRangeMap;
        for (auto deletedBlockIt = m_deletedBlockSizeToOffsetMap.begin(); deletedBlockIt != m_deletedBlockSizeToOffsetMap.end(); ++deletedBlockIt)
        {
            const AZ::u64 deletedBlockSize = deletedBlockIt->first;
            auto& deletedBlockOffsetSet = deletedBlockIt->second;
            for (const AZ::u64 deletedBlockOffset : deletedBlockOffsetSet)
            {
                // Stores an iterator to the block offset range to update
                auto updateBlockRangeIter = deletedBlockOffsetRangeMap.end();
                // Locate the last block before the block offset
                auto nextBlockRangeIt = deletedBlockOffsetRangeMap.lower_bound(deletedBlockOffset);
                if (nextBlockRangeIt != deletedBlockOffsetRangeMap.begin())
                {
                    // Check if the block range entry for the previous block
                    // ends at the beginning of the current block
                    if (auto previousBlockRangeIt = --nextBlockRangeIt;
                        previousBlockRangeIt->second == deletedBlockOffset)
                    {
                        updateBlockRangeIter = previousBlockRangeIt;
                        // Update the entry for the previous block offset range
                        // to have its seconded value point to the end of the current block
                        updateBlockRangeIter->second += deletedBlockSize;
                    }
                }

                typename decltype(deletedBlockOffsetRangeMap)::node_type nextBlockRangeNodeHandle;
                // if the iterator is before the end it represents the next block offset range to check
                // the condition is if it's begin offset is at end of the current deleted block
                if (nextBlockRangeIt != deletedBlockOffsetRangeMap.end()
                    && nextBlockRangeIt->first == deletedBlockSize + deletedBlockSize)
                {
                    // Extract the next entry out of the block offset range map
                    nextBlockRangeNodeHandle = deletedBlockOffsetRangeMap.extract(nextBlockRangeIt);
                }

                // There are four different scenarios here one of which has already been taken care of "last block before"
                // logic above
                // 1. The deleted block exist between two existing deleted block offset range entries
                // i.e <current block metadata> = (offset = 2 MiB, size = 2 MiB)
                //     <entry 1> = (0-2) MiB
                //     <entry 2> = (4-6) MiB
                // In this case the number of entries should be collapsed to 1 by
                //     <entry 1> = (0-6) MiB
                // This can be done by updating <entry 1> end range to be <entry 2> end range and removing <entry 2>
                //
                // 2. The deleted block exist after another deleted block, but the next block is in use
                // i.e <current block metadata> = (offset = 2 MiB, size = 2 MiB)
                //     <entry 1> = (0-2) MiB
                // In this case the existing entry is updated, to increment the current block size
                //     <entry 1> = (0-4) MiB
                // NOTE: This was already done up above by updating updateBlockRange iterator
                //
                // 3. The deleted block exist before another deleted block, but the previous block is in use
                // i.e <current block metadata> = (offset = 2 MiB, size = 2 MiB)
                //     <entry 1> = (4-6) MiB
                // In this case the next block offset range node handle is extracted from the block range offset map,
                // its key is updated to be the current block offset and finally that node handle is re-inserted
                // back into the block range offset map
                //     <entry 1> = (2-6) MiB
                //
                // 4. The deleted block is not surrounded by any deleted blocks
                // i.e <current block metadata> = (offset = 2 MiB, size = 2 MiB)
                // In this case, a new entry is added with the current block offset with an end of that is its offset + range
                //     <entry 1> = (2-4) MiB

                // Scenario 1
                if (updateBlockRangeIter != deletedBlockOffsetRangeMap.end()
                    && nextBlockRangeNodeHandle)
                {
                    // Updated the existing prev block range iterator end entry
                    updateBlockRangeIter->second = nextBlockRangeNodeHandle.mapped();
                }
                // Scenario 2
                else if (updateBlockRangeIter != deletedBlockOffsetRangeMap.end())
                {
                    // No-op - Already handled at the beginning of the for loop
                }
                // Scenario 3
                else if (nextBlockRangeNodeHandle)
                {
                    // Update the beginning of the next block range node handle and reinsert it
                    nextBlockRangeNodeHandle.key() = deletedBlockOffset;
                    deletedBlockOffsetRangeMap.insert(AZStd::move(nextBlockRangeNodeHandle));
                }
                // Scenario 4
                else
                {
                    // Insert a new element
                    deletedBlockOffsetRangeMap.emplace(deletedBlockOffset, deletedBlockOffset + deletedBlockSize);
                }
            }
        }

        // Now create a local block size -> block offset set by iterating over the
        // deleted block offset range map
        decltype(m_deletedBlockSizeToOffsetMap) mergeDeletedBlockSizeToOffsetMap;
        for (const auto& [deletedBlockFirst, deletedBlockLast] : deletedBlockOffsetRangeMap)
        {
            // Create/update the block offset set by using the difference in offset values as the deleted block size
            auto& mergeDeletedBlockOffsetSet = mergeDeletedBlockSizeToOffsetMap[deletedBlockLast - deletedBlockFirst];
            // Add the block offset first to the set
            mergeDeletedBlockOffsetSet.emplace(deletedBlockFirst);
        }

        // Swap the local block size -> block offset set with the member variable
        m_deletedBlockSizeToOffsetMap.swap(mergeDeletedBlockSizeToOffsetMap);
    }

    bool ArchiveWriter::BuildFilePathMap(const ArchiveTableOfContents& archiveToc)
    {
        // Clear any old entries from the path map
        m_pathMap.clear();
        // Build a map of file path to the index offset within the Archive TOC
        for (size_t filePathIndex{}; filePathIndex < archiveToc.m_filePaths.size(); ++filePathIndex)
        {
            m_pathMap.emplace(archiveToc.m_filePaths[filePathIndex], filePathIndex);
        }

        return true;
    }

    bool ArchiveWriter::MountArchive(AZ::IO::PathView archivePath)
    {
        UnmountArchive();
        AZ::IO::FixedMaxPath mountPath{ archivePath };
        constexpr AZ::IO::OpenMode openMode =
            AZ::IO::OpenMode::ModeCreatePath
            | AZ::IO::OpenMode::ModeAppend
            | AZ::IO::OpenMode::ModeUpdate;

        m_archiveStream.reset(aznew AZ::IO::SystemFileStream(mountPath.c_str(), openMode));

        // Early return if the archive is not open
        if (!m_archiveStream->IsOpen())
        {
            m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorOpeningArchive, ArchiveWriterErrorString::format(
                "Archive with filename %s could not be open",
                mountPath.c_str()) });
            return false;
        }

        if (!ReadArchiveHeaderAndToc())
        {
            // UnmountArchive is invoked to reset
            // the Archive Header, TOC,
            // file path -> file index map and entries
            // and the deleted block offset table
            UnmountArchive();
            return false;
        }
        return true;
    }

    bool ArchiveWriter::MountArchive(ArchiveStreamPtr archiveStream)
    {
        UnmountArchive();
        m_archiveStream = AZStd::move(archiveStream);

        if (m_archiveStream == nullptr || !m_archiveStream->IsOpen())
        {
            m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorOpeningArchive,
                ArchiveWriterErrorString("Archive stream pointer is nullptr or not open") });
            return false;
        }

        if (!ReadArchiveHeaderAndToc())
        {
            // UnmountArchive is invoked to reset
            // the Archive Header, TOC,
            // file path -> file index map and entries
            // and the deleted block offset table
            UnmountArchive();
            return false;
        }
        return true;
    }

    bool ArchiveWriter::ReadArchiveHeaderAndToc()
    {
        if (m_archiveStream == nullptr)
        {
            return false;
        }

        // An empty file is valid to use for writing a new archive therefore return true
        if (m_archiveStream->GetLength() == 0)
        {
            return true;
        }
        const bool mountResult = ReadArchiveHeader(m_archiveHeader, *m_archiveStream)
            && ReadArchiveTOC(m_archiveToc, *m_archiveStream, m_archiveHeader)
            && BuildDeletedFileBlocksMap(m_archiveHeader, *m_archiveStream)
            && BuildFilePathMap(m_archiveToc);

        return mountResult;
    }

    void ArchiveWriter::UnmountArchive()
    {
        if (m_archiveStream != nullptr && m_archiveStream->IsOpen())
        {
            if (CommitResult commitResult = Commit();
                !commitResult)
            {
                // Signal the error callback if the commit operation fails
                m_settings.m_errorCallback({ ArchiveWriterErrorCode::ErrorWritingTableOfContents,
                    AZStd::move(commitResult.error()) });
            }

            // Clear out the path map, the archive TOC and the archive header
            // deleted block size -> raw file offset
            // and the removed file index into TOC vector set
            // on unmount
            m_pathMap.clear();
            m_removedFileIndices.clear();
            m_deletedBlockSizeToOffsetMap.clear();
            m_archiveToc = {};
            m_archiveHeader = {};
        }

        m_archiveStream.reset();
    }

    bool ArchiveWriter::IsMounted() const
    {
        return m_archiveStream != nullptr && m_archiveStream->IsOpen();
    }

    auto ArchiveWriter::Commit() -> CommitResult
    {
        if (m_archiveToc.m_filePaths.size() != m_archiveToc.m_fileMetadataTable.size())
        {
            CommitResult result;
            result = AZStd::unexpected(ResultString::format("The Archive TOC of contents has a mismatched count of "
                " file paths (size=%zu) and the file metadata entries (size=%zu).\n"
                "Cannot commit archive.",
                m_archiveToc.m_filePaths.size(), m_archiveToc.m_fileMetadataTable.size()));
            return result;
        }

        if (AZStd::scoped_lock archiveLock(m_archiveStreamMutex);
            !IsMounted())
        {
            CommitResult result;
            result = AZStd::unexpected(ResultString::format("The stream to commit the archive data is not mounted.\n"
                "Cannot commit archive."));
            return result;
        }

        // Update the Archive uncompressed TOC file sizes
        m_archiveHeader.m_tocFileMetadataTableUncompressedSize = 0;
        m_archiveHeader.m_tocPathIndexTableUncompressedSize = 0;
        m_archiveHeader.m_tocPathBlobUncompressedSize = 0;
        for (size_t fileIndex{}; fileIndex < m_archiveToc.m_filePaths.size(); ++fileIndex)
        {
            const ArchiveTableOfContents::Path& filePath = m_archiveToc.m_filePaths[fileIndex];
            const ArchiveTocFileMetadata& fileMetadata = m_archiveToc.m_fileMetadataTable[fileIndex];
            // An empty file path represents a removed file from the archive, so skip it
            if (!filePath.empty())
            {
                m_archiveHeader.m_tocFileMetadataTableUncompressedSize += sizeof(fileMetadata);
                // The ArchiveTocFilePathIndex isn't stored in the Table of Contents directly
                // It is composed later in this function
                // That is why sizeof is being used on a struct instead of a member
                m_archiveHeader.m_tocPathIndexTableUncompressedSize += sizeof(ArchiveTocFilePathIndex);
                m_archiveHeader.m_tocPathBlobUncompressedSize += static_cast<AZ::u32>(filePath.Native().size());
            }
        }

        // 1. Write out any deleted blocks to the Archive file if there are any
        if (!m_deletedBlockSizeToOffsetMap.empty())
        {
            // First merge any contiguous deleted blocks into a single deleted block entry
            MergeContiguousDeletedBlocks();

            // As the deleted blockSizeToOffsetMap is keyed on block size
            // The deleted offset linked list would store blocks that are increasingly large in size
            // That is not a problem, however what could be a problem is that the deleted block offsets
            // are most likely not sequential and that could make reading the archive take longer
            // when building the deleted block table the next time the archive writer is used
            // therefore a set of deleted block offsets are created and then written
            // to the deleted blocks within the offset in file order
            struct SortByOffset
            {
                constexpr bool operator()(const BlockOffsetSizePair& left, const BlockOffsetSizePair& right) const
                {
                    return left.m_offset < right.m_offset;
                }
            };
            AZStd::set<BlockOffsetSizePair, SortByOffset> m_deletedBlockOffsetSet;

            // Populated the sorted deleted block offset set
            for (const auto& [blockSize, blockOffsetSet] : m_deletedBlockSizeToOffsetMap)
            {
                for (const AZ::u64 blockOffset : blockOffsetSet)
                {
                    m_deletedBlockOffsetSet.insert({ blockOffset, blockSize });
                }
            }

            // Update the first deleted block offset entry to point to the beginning of the block offset set
            m_archiveHeader.m_firstDeletedBlockOffset = m_deletedBlockOffsetSet.begin()->m_offset;

            // Iterate over the deleted block offset and write the block offset and block size to the first 16-bytes in the block
            // It is guaranteed that any deleted block has at least a size of 512 due to the ArchiveDefaultBlockAlignment
            AZStd::scoped_lock archiveLock(m_archiveStreamMutex);
            for (auto currentDeletedBlockIt = m_deletedBlockOffsetSet.begin(); currentDeletedBlockIt != m_deletedBlockOffsetSet.end();
                    ++currentDeletedBlockIt)
            {
                // Seek to the current deleted block offset and write the next deleted block offset value and the size
                // of the current deleted block
                m_archiveStream->Seek(currentDeletedBlockIt->m_offset, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

                if (auto nextDeletedBlockIt = AZStd::next(currentDeletedBlockIt);
                    nextDeletedBlockIt != m_deletedBlockOffsetSet.end())
                {
                    m_archiveStream->Write(sizeof(nextDeletedBlockIt->m_offset), &nextDeletedBlockIt->m_offset);
                }
                else
                {
                    // For the final block write the deleted block offset sentinel value of 0xffff'ffff'ffff'ffff
                    m_archiveStream->Write(sizeof(DeletedBlockOffsetSentinel), &DeletedBlockOffsetSentinel);
                }
                m_archiveStream->Write(sizeof(currentDeletedBlockIt->m_size), &currentDeletedBlockIt->m_size);
            }
        }

        // Update the Archive uncompressed TOC block offset table size
        // As removed files block offset table entries aren't removed from the archive
        // It can be calculated using multiplication of <number of block line entries> * sizeof(ArchiveBlockLineUnion)
        // The simplest approach is to convert it to a span and use the size_byte function on it
        m_archiveHeader.m_tocBlockOffsetTableUncompressedSize = static_cast<AZ::u32>(
            AZStd::span(m_archiveToc.m_blockOffsetTable).size_bytes());

        // 2. Write the Archive Table of Contents
        // Both buffers lifetime must be encompass the tocWriteSpan below
        // to make sure the span points to a valid buffer
        AZStd::vector<AZStd::byte> tocRawBuffer;
        AZStd::vector<AZStd::byte> tocCompressBuffer;

        WriteTocRawResult rawTocResult = WriteTocRaw(tocRawBuffer);
        if (!rawTocResult)
        {
            CommitResult result;
            result = AZStd::unexpected(AZStd::move(rawTocResult.m_errorString));
            return result;
        }

        // Initialize the tocWriteSpan to the tocRawBuffer above
        AZStd::span<const AZStd::byte> tocWriteSpan = rawTocResult.m_tocSpan;

        // Check if the table of contents should be compressed
        Compression::CompressionAlgorithmId tocCompressionAlgorithmId =
            m_archiveHeader.m_tocCompressionAlgoIndex < UncompressedAlgorithmIndex
            ? m_archiveHeader.m_compressionAlgorithmsIds[m_archiveHeader.m_tocCompressionAlgoIndex]
            : Compression::Uncompressed;
        if (m_settings.m_tocCompressionAlgorithm.has_value())
        {
            tocCompressionAlgorithmId = m_settings.m_tocCompressionAlgorithm.value();
        }

        if (tocCompressionAlgorithmId != Compression::Invalid && tocCompressionAlgorithmId != Compression::Uncompressed)
        {
            CompressTocRawResult compressResult = CompressTocRaw(tocCompressBuffer, tocRawBuffer,
                tocCompressionAlgorithmId);
            if (!compressResult)
            {
                CommitResult result;
                result = AZStd::unexpected(AZStd::move(compressResult.m_errorString));
                return result;
            }

            // The tocWriteSpan now points to the tocCompressBufer
            // via the CompressTocRawResult span
            tocWriteSpan = compressResult.m_compressedTocSpan;

            // Update the archive header compressed toc metadata
            m_archiveHeader.m_tocCompressedSize = static_cast<AZ::u32>(tocWriteSpan.size());
            m_archiveHeader.m_tocCompressionAlgoIndex = static_cast<AZ::u32>(FindCompressionAlgorithmId(
                tocCompressionAlgorithmId, m_archiveHeader));
        }
        else
        {
            // The table of contents is not compressed
            // so store a size of 0
            m_archiveHeader.m_tocCompressedSize = 0;
        }


        // Performs writing of the raw table of contents table
        if (!tocWriteSpan.empty())
        {
            AZStd::scoped_lock archiveLock(m_archiveStreamMutex);
            // Seek to the Table of Contents toc offset index
            m_archiveStream->Seek(m_archiveHeader.m_tocOffset, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            m_archiveStream->Write(tocWriteSpan.size(), tocWriteSpan.data());
        }

        // 3. Write out the updated Archive Header
        {
            AZStd::scoped_lock archiveLock(m_archiveStreamMutex);
            // Seek to the beginning of the stream and write the archive header overtop any previous header
            m_archiveStream->Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            if (size_t writeSize = m_archiveStream->Write(sizeof(m_archiveHeader), &m_archiveHeader);
                writeSize != sizeof(m_archiveHeader))
            {
                CommitResult result;
                result = AZStd::unexpected(ResultString::format("Failed to write Archive Header to the beginning of stream.\n"
                    "Cannot commit archive."));
                return result;
            }

            // Write padding bytes to stream until the 512-byte alignment is reached
            AZStd::array<AZStd::byte, ArchiveDefaultBlockAlignment - sizeof(m_archiveHeader)> headerPadding{};
            m_archiveStream->Write(headerPadding.size(), headerPadding.data());
        }

        return CommitResult{};
    }

    ArchiveWriter::WriteTocRawResult::operator bool() const
    {
        return m_errorString.empty();
    }

    auto ArchiveWriter::WriteTocRaw(AZStd::vector<AZStd::byte>& tocOutputBuffer) -> WriteTocRawResult
    {
        tocOutputBuffer.reserve(m_archiveHeader.GetUncompressedTocSize());

        AZ::IO::ByteContainerStream tocOutputStream(&tocOutputBuffer);

        tocOutputStream.Write(sizeof(ArchiveTocMagicBytes), &ArchiveTocMagicBytes);
        // Write padding bytes to ensure that the file metadata entries start on a 32-byte boundary
        AZStd::byte fileMetadataAlignmentBytes[sizeof(ArchiveTocFileMetadata) - sizeof(ArchiveTocMagicBytes)]{};
        tocOutputStream.Write(AZStd::size(fileMetadataAlignmentBytes), fileMetadataAlignmentBytes);

        // Write out the file metadata table first to the table of contents
        // The m_fileMetadataTable and m_filePaths should have the same size
        for (size_t fileIndex = 0; fileIndex < m_archiveToc.m_filePaths.size(); ++fileIndex)
        {
            if (!m_archiveToc.m_filePaths.empty())
            {
                const ArchiveTocFileMetadata& fileMetadata = m_archiveToc.m_fileMetadataTable[fileIndex];
                tocOutputStream.Write(sizeof(fileMetadata), &fileMetadata);
            }
        }

        // Write out each file path index table entry
        // They are created on the fly here
        AZ::u64 filePathOffset{};
        for (size_t fileIndex = 0; fileIndex < m_archiveToc.m_filePaths.size(); ++fileIndex)
        {
            if (!m_archiveToc.m_filePaths.empty())
            {
                ArchiveTocFilePathIndex filePathIndex;
                filePathIndex.m_size = m_archiveToc.m_filePaths[fileIndex].Native().size();
                filePathIndex.m_offset = filePathOffset;
                // Move the file path table offset forward by the size of the path
                filePathOffset += filePathIndex.m_size;
                tocOutputStream.Write(sizeof(filePathIndex), &filePathIndex);
            }
        }

        // Write out the file path blob table
        // Consecutive paths are not separated by null terminating characters
        for (size_t fileIndex = 0; fileIndex < m_archiveToc.m_filePaths.size(); ++fileIndex)
        {
            if (!m_archiveToc.m_filePaths.empty())
            {
                // Write path from the AZ::IO::Path
                tocOutputStream.Write(m_archiveToc.m_filePaths[fileIndex].Native().size(),
                    m_archiveToc.m_filePaths[fileIndex].c_str());
            }
        }

        // If the file path blob is not aligned on a 8 byte boundary
        // then write 0 bytes until it is aligned
        constexpr AZ::u64 FilePathBlobAlignment = 8;
        if (const AZ::u64 filePathCurAlignment = filePathOffset % FilePathBlobAlignment;
            filePathCurAlignment > 0)
        {
            // Fill an array of size 8 with '\0' bytes
            AZStd::byte paddingBytes[FilePathBlobAlignment]{};
            // Writes the remaining bytes to pad to an 8 byte boundary
            tocOutputStream.Write(FilePathBlobAlignment - filePathCurAlignment, paddingBytes);
        }

        // Write out the block offset table
        // Since it can contain entries to deleted block lines, the logic is simple here
        // It is just the size of the table * sizeof(ArchiveBlockLineUnion)
        AZStd::span<ArchiveBlockLineUnion> blockOffsetTableView = m_archiveToc.m_blockOffsetTable;
        tocOutputStream.Write(blockOffsetTableView.size_bytes(), blockOffsetTableView.data());

        WriteTocRawResult result;
        result.m_tocSpan = tocOutputBuffer;
        return result;
    }

    ArchiveWriter::CompressTocRawResult::operator bool() const
    {
        return m_errorString.empty();
    }

    auto ArchiveWriter::CompressTocRaw(AZStd::vector<AZStd::byte>& tocCompressionBuffer,
        AZStd::span<const AZStd::byte> uncompressedTocInputSpan,
        Compression::CompressionAlgorithmId compressionAlgorithmId) -> CompressTocRawResult
    {
        CompressTocRawResult result;
        // Initialize the compressedTocSpan to the uncompressed data
        result.m_compressedTocSpan = uncompressedTocInputSpan;

        auto compressionRegistrar = Compression::CompressionRegistrar::Get();
        if (compressionRegistrar == nullptr)
        {
            result.m_errorString = "Compression Registrar is not available to compress the raw Table of Contents data";
            return result;
        }
        Compression::ICompressionInterface* compressionInterface =
            compressionRegistrar->FindCompressionInterface(compressionAlgorithmId);
        if (compressionInterface == nullptr)
        {
            result.m_errorString = ResultString::format(
                "Compression algorithm with ID %u is not registered with the Compression Registrar",
                AZStd::to_underlying(compressionAlgorithmId));
            return result;
        }

        // Add the Compression Algorithm ID to the archive header compression algorithm array
        AddCompressionAlgorithmId(compressionAlgorithmId, m_archiveHeader);

        // Resize the TOC compression Buffer to be able to fit the compressed content
        tocCompressionBuffer.resize_no_construct(compressionInterface->CompressBound(uncompressedTocInputSpan.size()));

        if (auto compressionResultData = compressionInterface->CompressBlock(
            tocCompressionBuffer, uncompressedTocInputSpan);
            compressionResultData)
        {
            result.m_compressedTocSpan = compressionResultData.m_compressedBuffer;
        }
        else
        {
            result.m_errorString = compressionResultData.m_compressionOutcome.m_resultString;
        }

        return result;
    }

    ArchiveAddFileResult ArchiveWriter::AddFileToArchive(AZ::IO::GenericStream& inputStream,
        const ArchiveWriterFileSettings& fileSettings)
    {
        AZStd::vector<AZStd::byte> fileData;
        fileData.resize_no_construct(inputStream.GetLength());
        AZ::IO::SizeType bytesRead = inputStream.Read(fileData.size(), fileData.data());
        // Unable to read entire stream data into memory
        if (bytesRead != fileData.size())
        {
            ArchiveAddFileResult errorResult;
            errorResult.m_relativeFilePath = fileSettings.m_relativeFilePath;
            errorResult.m_compressionAlgorithm = fileSettings.m_compressionAlgorithm;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString::format(R"(Unable to successfully read all bytes(%zu) from input stream.)"
                    " %llu bytes were read.",
                    fileData.size(), bytesRead));
            return errorResult;
        }

        return AddFileToArchive(fileData, fileSettings);
    }

    ArchiveAddFileResult ArchiveWriter::AddFileToArchive(AZStd::span<const AZStd::byte> inputSpan,
        const ArchiveWriterFileSettings& fileSettings)
    {
        if (fileSettings.m_relativeFilePath.empty())
        {
            ArchiveAddFileResult errorResult;
            errorResult.m_compressionAlgorithm = fileSettings.m_compressionAlgorithm;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString(R"(The file path is empty. File will not be added to the archive.)"));
            return errorResult;
        }

        // Update the file case based on the ArchiveFilePathCase enum
        AZ::IO::Path filePath(fileSettings.m_relativeFilePath);
        switch (fileSettings.m_fileCase)
        {
        case ArchiveFilePathCase::Lowercase:
            AZStd::to_lower(filePath.Native());
            break;
        case ArchiveFilePathCase::Uppercase:
            AZStd::to_upper(filePath.Native());
            break;
        }

        // Check if a file being added is already in the archive
        // If the ArchiveWriterFileMode is set to only add new files
        // return an ArchiveAddFileResult with an invalid file token
        if (fileSettings.m_fileMode == ArchiveWriterFileMode::AddNew
            && ContainsFile(filePath))
        {
            ArchiveAddFileResult errorResult;
            errorResult.m_relativeFilePath = AZStd::move(filePath);
            errorResult.m_compressionAlgorithm = fileSettings.m_compressionAlgorithm;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString::format(R"(The file with relative path "%s" already exist in the archive.)"
                    " The FileMode::AddNew option was specified.",
                    errorResult.m_relativeFilePath.c_str()));
            return errorResult;
        }

        ArchiveAddFileResult result;
        // Supply the file path with the case changed
        result.m_relativeFilePath = AZStd::move(filePath);

        // Storage buffer used to store the file data if it s compressed
        // It's lifetime must outlive the CompressContentOutcome
        AZStd::vector<AZStd::byte> compressionBuffer;

        CompressContentOutcome compressOutcome = CompressContentFileAsync(compressionBuffer, fileSettings, inputSpan);
        if (!compressOutcome)
        {
            result.m_compressionAlgorithm = fileSettings.m_compressionAlgorithm;
            result.m_resultOutcome = AZStd::unexpected(AZStd::move(compressOutcome.error()));
            return result;
        }

        // Populate the compression algorithm used in the result structure
        if (compressOutcome->m_compressionAlgorithmIndex < m_archiveHeader.m_compressionAlgorithmsIds.size())
        {
            result.m_compressionAlgorithm = m_archiveHeader.m_compressionAlgorithmsIds[compressOutcome->m_compressionAlgorithmIndex];
        }
        // Update the archive stream
        ContentFileData contentFileData;
        contentFileData.m_relativeFilePath = result.m_relativeFilePath;
        contentFileData.m_uncompressedSpan = inputSpan;
        contentFileData.m_contentFileBlocks = AZStd::move(*compressOutcome);

        // Write the file content to the archive stream and store the archive file path token
        // which is used to lookup the file for removal
        result.m_filePathToken = WriteContentFileToArchive(fileSettings, contentFileData);
        return result;
    }


    auto ArchiveWriter::CompressContentFileAsync(AZStd::vector<AZStd::byte>& compressionDataBuffer,
        const ArchiveWriterFileSettings& fileSettings,
        AZStd::span<const AZStd::byte> inputDataSpan) -> CompressContentOutcome
    {
        // If the file is empty, there is nothing to compress
        if (inputDataSpan.empty())
        {
            return ContentFileBlocks{};
        }

        // Try to register the compression algorithm id with the Archive Header compression algorithm id array
        // if has not already been registered
        AddCompressionAlgorithmId(fileSettings.m_compressionAlgorithm, m_archiveHeader);

        // Now lookup the compression algorithm id to make sure it corresponds to a valid entry
        // in the compression algorithm id array
        size_t compressionAlgorithmIndex = FindCompressionAlgorithmId(fileSettings.m_compressionAlgorithm, m_archiveHeader);

        // If a valid compression algorithm Id is not found in the compression algorithm id array
        // then an invalid file token is returned
        if (compressionAlgorithmIndex == InvalidAlgorithmIndex)
        {
            AZStd::unexpected<ResultString> errorString = AZStd::unexpected(
                ResultString::format(R"(Unable to locate compression algorithm registered with id %u in the archive.)",
                    static_cast<AZ::u32>(fileSettings.m_compressionAlgorithm)));
            return errorString;
        }

        ContentFileBlocks contentFileBlocks;
        contentFileBlocks.m_writeSpan = inputDataSpan;

        if (compressionAlgorithmIndex >= UncompressedAlgorithmIndex)
        {
            contentFileBlocks.m_blockOffsetSizePairs = AZStd::vector<BlockOffsetSizePair>{ { 0, inputDataSpan.size() } };
            return contentFileBlocks;
        }

        auto compressionRegistrar = Compression::CompressionRegistrar::Get();
        if (compressionRegistrar == nullptr)
        {
            contentFileBlocks.m_blockOffsetSizePairs = AZStd::vector<BlockOffsetSizePair>{ { 0, inputDataSpan.size() } };
            return contentFileBlocks;
        }
        Compression::ICompressionInterface* compressionInterface =
            compressionRegistrar->FindCompressionInterface(fileSettings.m_compressionAlgorithm);
        if (compressionInterface == nullptr)
        {
            contentFileBlocks.m_blockOffsetSizePairs = AZStd::vector<BlockOffsetSizePair>{ { 0, inputDataSpan.size() } };
            return contentFileBlocks;
        }

        const Compression::CompressionOptions& compressionOptions = fileSettings.m_compressionOptions != nullptr
            ? *fileSettings.m_compressionOptions
            : Compression::CompressionOptions{};

        // Due to check earlier validating that the inputDataSpan is not empty,
        // the compressedBlockCount will be at least 1 due to rounding up to the nearest block
        AZ::u32 compressedBlockCount = GetBlockCountIfCompressed(inputDataSpan.size());

        // Resize the compress block buffer to be be a multiple of ArchiveBlockSizeForCompression
        // times the block count
        AZStd::vector<AZStd::byte> compressBlocksBuffer;
        compressBlocksBuffer.resize_no_construct(compressedBlockCount * ArchiveBlockSizeForCompression);

        // Make sure there is at least one task that runs to make sure that progress
        // with compression is always being made
        const AZ::u32 maxCompressTasks = AZStd::max(1U, m_settings.m_maxCompressTasks);

        // Stores the compressed size for all blocks without alignment
        size_t compressedBlockSizeForAllBlocks{};
        // allocated slots for each blocks CompressionResultData
        AZStd::vector<Compression::CompressionResultData> compressedBlockResults(maxCompressTasks);
        while (compressedBlockCount > 0)
        {
            const AZ::u32 compressionThresholdInBytes = m_archiveHeader.m_compressionThreshold;
            const AZ::u32 iterationTaskCount = AZStd::min(compressedBlockCount, maxCompressTasks);
            // decrease the compressedBlockCount by the number of compressed tasks to be executed
            compressedBlockCount -= iterationTaskCount;

            {
                // Task graph event used to block when writing compressed blocks in parallel
                auto taskWriteGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>("Content File Compress Sync");
                AZ::TaskGraph taskGraph{ "Archive Compress Tasks" };
                AZ::TaskDescriptor compressTaskDescriptor{"Compress Block", "Archive Content File Compression"};

                for (size_t compressedTaskSlot = 0; compressedTaskSlot < iterationTaskCount; ++compressedTaskSlot)
                {
                    AZStd::span compressBlocksSpan(compressBlocksBuffer);
                    const size_t blockStartOffset = compressedTaskSlot * ArchiveBlockSizeForCompression;

                    // Cap the input block span size to the minimum of the ArchiveBlockSizeForCompression(2 MiB) and the remaining size
                    // left in the input buffer via subspan
                    const size_t inputBlockSize = AZStd::min(
                        inputDataSpan.size() - blockStartOffset,
                        static_cast<size_t>(ArchiveBlockSizeForCompression));
                    auto inputBlockSpan = inputDataSpan.subspan(blockStartOffset,
                        inputBlockSize);

                    // Span that is segmented in up to ArchiveBlockSize(2MiB) blocks to store compressed data
                    const size_t compressBlockSize = AZStd::min(
                        compressBlocksSpan.size() - blockStartOffset,
                        static_cast<size_t>(ArchiveBlockSizeForCompression));
                    auto compressBlockSpan = compressBlocksSpan.subspan(blockStartOffset,
                        compressBlockSize);

                    //! Compress Task to execute in task executor
                    auto compressTask = [
                        compressionInterface, &compressionOptions, inputBlockSpan, compressBlockSpan,
                        &compressedBlockResult = compressedBlockResults[compressedTaskSlot]]()
                    {
                        // Run the input data through the compressor
                        compressedBlockResult = compressionInterface->CompressBlock(
                            compressBlockSpan, inputBlockSpan, compressionOptions);
                    };
                    taskGraph.AddTask(compressTaskDescriptor, AZStd::move(compressTask));
                }

                taskGraph.SubmitOnExecutor(m_taskWriteExecutor, taskWriteGraphEvent.get());
                // Sync on the task completion
                taskWriteGraphEvent->Wait();
            }

            size_t alignedCompressedBlockSizeForAllBlocks{};

            for (size_t compressedTaskSlot = 0; compressedTaskSlot < iterationTaskCount; ++compressedTaskSlot)
            {
                Compression::CompressionResultData& compressedBlockResult = compressedBlockResults[compressedTaskSlot];
                if (!compressedBlockResult || compressedBlockResult.GetCompressedByteCount() > compressionThresholdInBytes)
                {
                    // If compression fails for a block or it is higher than the compression threshold
                    // then return the contentFileData that has the compression algorithm set to uncompressed
                    // and points to the input data span
                    // The entire file is stored uncompressed in this scenario
                    // Return a successful outcome with a single block offset size pair that references the entire
                    // input buffer
                    contentFileBlocks.m_blockOffsetSizePairs.assign({ BlockOffsetSizePair{0, inputDataSpan.size()} });
                    contentFileBlocks.m_totalUnalignedSize = inputDataSpan.size();
                    return contentFileBlocks;
                }
                else
                {
                    AZ::u64 compressedBlockSize = compressedBlockResult.GetCompressedByteCount();
                    compressedBlockSizeForAllBlocks += compressedBlockSize;
                    alignedCompressedBlockSizeForAllBlocks += AZ_SIZE_ALIGN_UP(compressedBlockSize, ArchiveDefaultBlockAlignment);
                }
            }

            // Determine the amount of additional bytes to resize the compression data output buffer
            // This takes into account the alignment of blocks as how it would be written on disk
            compressionDataBuffer.reserve(compressionDataBuffer.size() + alignedCompressedBlockSizeForAllBlocks);
            for (size_t compressedTaskSlot = 0; compressedTaskSlot < iterationTaskCount; ++compressedTaskSlot)
            {
                const Compression::CompressionResultData& compressedBlockResult = compressedBlockResults[compressedTaskSlot];

                // Calculated the number of additional bytes to store to pad the block to 512-byte alignment
                const AZ::u64 alignmentBytes = AZ_SIZE_ALIGN_UP(compressedBlockResult.GetCompressedByteCount(), ArchiveDefaultBlockAlignment)
                    - compressedBlockResult.GetCompressedByteCount();

                // Copy the bytes from block into the data buffer
                auto insertIt = compressionDataBuffer.insert(compressionDataBuffer.end(), compressedBlockResult.m_compressedBuffer.begin(),
                    compressedBlockResult.m_compressedBuffer.end());
                auto compressedBlockStartOffset = size_t(AZStd::distance(compressionDataBuffer.begin(), insertIt));
                // fill the buffer with padding bytes
                compressionDataBuffer.insert(compressionDataBuffer.end(), alignmentBytes, AZStd::byte{});

                // Populate the block offset pairs with the offset within compressionDataBuffer where the compressed block is written
                // plus the size of the compressed data
                contentFileBlocks.m_blockOffsetSizePairs.push_back({ compressedBlockStartOffset,
                    compressedBlockResult.GetCompressedByteCount() });
            }
        }

        // Set the compression algorithm index once compression has completed successfully for all blocks of the file
        contentFileBlocks.m_compressionAlgorithmIndex = static_cast<AZ::u8>(compressionAlgorithmIndex);
        // The file has been successfully compressed, so store a span to the buffer
        contentFileBlocks.m_writeSpan = compressionDataBuffer;
        // Store the compressed size of each block without taking any alignment into account
        // This is the exact total compressed size of the "file" as stored in blocks
        contentFileBlocks.m_totalUnalignedSize = compressedBlockSizeForAllBlocks;

        return contentFileBlocks;
    }

    ArchiveFileToken ArchiveWriter::WriteContentFileToArchive(const ArchiveWriterFileSettings& fileSettings,
        const ContentFileData& contentFileData)
    {
        if (fileSettings.m_fileMode == ArchiveWriterFileMode::AddNew)
        {
            // Update the file count in the archive
            ++m_archiveHeader.m_fileCount;
        }

        // Locate the location within the Archive to write the file data
        // First any deleted blocks are located to see if the file data can be written to it
        // otherwise the content data is written at the current table of contents offset
        // and the table of contents offset is then shifted by that amount

        // The m_relativeFilePath is guaranteed to not be empty due to the check at the top of AddFileToArchive
        // If the file path already exist in the archive locate it
        auto findArchiveTokenIt = m_pathMap.find(contentFileData.m_relativeFilePath);

        // Insert the file path to the end of the file path index table if the file path is not in the archive
        size_t archiveFileIndex{};
        if (findArchiveTokenIt != m_pathMap.end())
        {
            // If the file exist in the archive, store its index
            archiveFileIndex = findArchiveTokenIt->second;
        }
        else if (!m_removedFileIndices.empty())
        {
            // If the removedFileIndices set is not empty, store that index
            auto firstDeletedFileIt = m_removedFileIndices.begin();
            archiveFileIndex = *firstDeletedFileIt;
            // Pop off the index of the from the removed file indices sets
            m_removedFileIndices.erase(firstDeletedFileIt);
        }
        else
        {
            // In this case, the file path does not exist as part of the existing archive
            // and the removed file indices set is empty
            // Get the current size of the file path index table
            archiveFileIndex = m_archiveToc.m_fileMetadataTable.size();

            // Append a new entry to each of the Archive TOC file metadata containers
            m_archiveToc.m_fileMetadataTable.emplace_back();
            m_archiveToc.m_filePaths.emplace_back();
        }

        // Get reference to the FileMetadata entry in the Archive
        ArchiveTocFileMetadata& fileMetadata = m_archiveToc.m_fileMetadataTable[archiveFileIndex];
        fileMetadata.m_uncompressedSize = contentFileData.m_uncompressedSpan.size();
        // Divide by the ArchiveDefaultBlockAlignment(512) to convert the compressedSize to sectors
        const AZ::u64 alignedFileSize = AZ_SIZE_ALIGN_UP(contentFileData.m_contentFileBlocks.m_writeSpan.size(), ArchiveDefaultBlockAlignment);
        fileMetadata.m_compressedSizeInSectors = alignedFileSize / ArchiveDefaultBlockAlignment;
        fileMetadata.m_compressionAlgoIndex = contentFileData.m_contentFileBlocks.m_compressionAlgorithmIndex;
        fileMetadata.m_offset = ExtractWriteBlockOffset(alignedFileSize);
        fileMetadata.m_crc32 = AZ::Crc32(contentFileData.m_uncompressedSpan);

        ArchiveTableOfContents::Path& filePath = m_archiveToc.m_filePaths[archiveFileIndex];
        filePath = contentFileData.m_relativeFilePath;

        {
            AZStd::span<const AZStd::byte> contiguousWriteSpan = contentFileData.m_contentFileBlocks.m_writeSpan;
            // Write out the blocks to the stream
            AZStd::scoped_lock writeLock(m_archiveStreamMutex);
            m_archiveStream->Seek(fileMetadata.m_offset, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            m_archiveStream->Write(contiguousWriteSpan.size(), contiguousWriteSpan.data());
        }

        // Update the block offset table if the file is compressed
        if (contentFileData.m_contentFileBlocks.m_compressionAlgorithmIndex < UncompressedAlgorithmIndex)
        {
            fileMetadata.m_blockLineTableFirstIndex = UpdateBlockOffsetEntryForFile(contentFileData);
        }

        m_pathMap[filePath] = archiveFileIndex;
        return static_cast<ArchiveFileToken>(archiveFileIndex);
    }

    AZ::u64 ArchiveWriter::UpdateBlockOffsetEntryForFile(const ContentFileData& contentFileData)
    {
        // Index into the block offset table first entry for the file
        AZ::u64 blockLineFirstIndex = m_archiveToc.m_blockOffsetTable.size();

        // Reserve space for the number of block line entries stored
        AZ::u64 remainingUncompressedSize = contentFileData.m_uncompressedSpan.size();
        size_t blockLineCount = GetBlockLineCountIfCompressed(remainingUncompressedSize);
        m_archiveToc.m_blockOffsetTable.reserve(m_archiveToc.m_blockOffsetTable.size() + blockLineCount);

        size_t blockOffsetIndex{};

        // Three block lines which is up to 18MiB of uncompressed data is handled
        // each iteration of the loop.
        // If the remaining uncompressed size is <=18MiB then the last iteration of the loop
        // handles the remaining block lines for which there can be 1(<= 6 MiB) to 3 (> 12 MiB && <= 18 MiB)
        while (blockLineCount > 0 && blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
        {
            if (remainingUncompressedSize > MaxRemainingFileSizeNoJumpEntry)
            {
                // Stores the jump offset which can be used to skip 16 MiB of uncompressed content
                // This is calculated by summing the 512-bit aligned compressed sizes of each block
                AZ::u16 jumpOffset{};

                size_t blockLineWithJumpIndex = AZStd::numeric_limits<size_t>::max();

                BlockOffsetSizePair blockOffsetSizePair;
                // Stores sizes for compressed data from offsets (0-4MiB]
                {
                    ArchiveBlockLineUnion& blockLine1 = m_archiveToc.m_blockOffsetTable.emplace_back();
                    // Tracks the index of the block line vector element with a jump entry
                    // The jump entry offset needs to be updated after the next 8 block line aligned compressed sizes
                    // have been calculated
                    // NOTE: The `blockLine1` is not safe to use later on, as further emplace_back calls could invalidate
                    // the reference via reallocation, so index into the vector is being used
                    blockLineWithJumpIndex = m_archiveToc.m_blockOffsetTable.size();

                    // Set the first block as used
                    blockLine1.m_blockLineWithJump.m_blockUsed = 1;

                    // Write out the first block offset entry
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine1.m_blockLineWithJump.m_block0 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                    // Write out the second block offset entry
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine1.m_blockLineWithJump.m_block1 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                }

                // The second block line represents the offsets of (4MiB-10MiB]
                {
                    ArchiveBlockLineUnion& blockLine2 = m_archiveToc.m_blockOffsetTable.emplace_back();
                    // Set the second block as used
                    blockLine2.m_blockLine.m_blockUsed = 1;
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        // Write out the third block offset entry
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine2.m_blockLine.m_block0 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        // Write out the fourth block offset entry
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine2.m_blockLine.m_block1 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        // Write out the fifth block offset entry
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine2.m_blockLine.m_block2 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                }

                // The third block line represents the offsets of (10MiB-16MiB]
                {
                    ArchiveBlockLineUnion& blockLine3 = m_archiveToc.m_blockOffsetTable.emplace_back();
                    // Set the third block as used
                    blockLine3.m_blockLine.m_blockUsed = 1;
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        // Write out the sixth block offset entry
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine3.m_blockLine.m_block0 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        // Write out the seventh block offset entry
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine3.m_blockLine.m_block1 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        // Write out the eighth block offset entry
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine3.m_blockLine.m_block2 = blockOffsetSizePair.m_size;
                        jumpOffset += aznumeric_cast<AZ::u16>(AZ_SIZE_ALIGN_UP(blockOffsetSizePair.m_size, ArchiveDefaultBlockAlignment)
                            / ArchiveDefaultBlockAlignment);
                    }
                }

                // Now update the jump offset value with the amount of bytes that can be skipped in the C data section of the archive
                // from the beginning of the file
                m_archiveToc.m_blockOffsetTable[blockLineWithJumpIndex].m_blockLineWithJump.m_blockJump = jumpOffset;

                // The first block offset entry is a jump table entry
                // while the next 8 block offset entries store compressed sizes
                // for 2 MiB chunks which total 16 MiB
                // The three block lines are represented by 3 G4-bit integers
                //
                // 64-bit block line #1 (57-bits used)
                //   Jump Entry : 16-bits
                //   Block #0 : 21-bits
                //   Block #1 : 21-bits
                // 64-bit block line #2 (63-bits used)
                //   Block #2 : 21-bits
                //   Block #3 : 21-bits
                //   Block #4 : 21-bits
                // 64-bit block line #3 (63-bits used)
                //   Block #5 : 21-bits
                //   Block #6 : 21-bits
                //   Block #7 : 21-bits
                remainingUncompressedSize -= FileSizeToSkipWithJumpEntry;
                // As 3 blocks are being processed at a time decrement the block line count by 3
                blockLineCount = blockLineCount > 2 ? blockLineCount - 3 : 0;
            }
            else
            {
                BlockOffsetSizePair blockOffsetSizePair;
                // Store each 6 MiB block line
                for (; remainingUncompressedSize > 0; remainingUncompressedSize -= AZStd::min(remainingUncompressedSize, MaxBlockLineSize))
                {
                    ArchiveBlockLineUnion& blockLine1 = m_archiveToc.m_blockOffsetTable.emplace_back();
                    // Set the first block as used
                    blockLine1.m_blockLine.m_blockUsed = 1;

                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        // Write out the first block offset entry
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine1.m_blockLine.m_block0 = blockOffsetSizePair.m_size;
                    }
                    // Write out the second block offset entry
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine1.m_blockLine.m_block1 = blockOffsetSizePair.m_size;
                    }
                    // Write out the third block offset entry
                    if (blockOffsetIndex < contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs.size())
                    {
                        blockOffsetSizePair = contentFileData.m_contentFileBlocks.m_blockOffsetSizePairs[blockOffsetIndex++];
                        blockLine1.m_blockLine.m_block2 = blockOffsetSizePair.m_size;
                    }
                }

                // The remaining uncompressed size <= 18 MiB, so a block jump entry is not used
                // Up to the next 9 blocks(3 block lines) if needed will encode the 2 MiB chunks
                // which can total up to 18 MiB
                //
                // 64-bit block line #1 (63-bits used)
                //   Block #0 : 21-bits
                //   Block #1 : 21-bits
                //   Block #2 : 21-bits
                // 64-bit block line #2 (63-bits used)
                //   Block #3 : 21-bits
                //   Block #4 : 21-bits
                //   Block #5 : 21-bits
                // 64-bit block line #3 (63-bits used)
                //   Block #6 : 21-bits
                //   Block #7 : 21-bits
                //   Block #8 : 21-bits
                remainingUncompressedSize = 0;
                // There are no more block lines to process after this loop
                blockLineCount = 0;
            }
        }

        return blockLineFirstIndex;
    }

    AZ::u64 ArchiveWriter::ExtractWriteBlockOffset(AZ::u64 alignedFileSizeToWrite)
    {
        // If the file size is 0, then the offset value doesn't matter
        // return 0 in this case
        if (alignedFileSizeToWrite == 0)
        {
            return 0ULL;
        }

        if (auto deletedBlockIt = m_deletedBlockSizeToOffsetMap.lower_bound(alignedFileSizeToWrite);
            deletedBlockIt != m_deletedBlockSizeToOffsetMap.end())
        {
            // Get the full size of the deleted block
            const AZ::u64 deletedBlockSize = deletedBlockIt->first;
            // gets reference to AZStd::set of deleted blocks
            if (auto& blockOffsetSet = deletedBlockIt->second; !blockOffsetSet.empty())
            {
                // extract the first element from the deleted block offset set
                // for a specific block size
                auto blockOffsetHandle = blockOffsetSet.extract(blockOffsetSet.begin());

                // If the block offset set is now empty for a specific block size,
                // erase it from the deleted block size offset map
                if (blockOffsetSet.empty())
                {
                    m_deletedBlockSizeToOffsetMap.erase(deletedBlockIt);
                }

                // copy the deleted block offset from the extracted value from the set
                const AZ::u64 deletedBlockWriteOffset = blockOffsetHandle.value();

                // insert a smaller deleted block back into the deleted block size offset map
                // if the entire deleted block is not used
                // Since blocks are 512-byte aligned, the newDeletedBlockSize is rounded down to the nearest
                // the 512-byte boundary and checked if it is >0
                if(AZ::u64 newAlignedDeletedBlockSize = deletedBlockSize - alignedFileSizeToWrite;
                    newAlignedDeletedBlockSize > 0)
                {
                    // Calculate the new deleted block offset by aligning up to the nearest 512-byte boundary
                    const AZ::u64 newDeletedBlockOffset = AZ_SIZE_ALIGN_UP(deletedBlockWriteOffset + alignedFileSizeToWrite,
                        ArchiveDefaultBlockAlignment);

                    auto& deletedBlockSetForSize = m_deletedBlockSizeToOffsetMap[newAlignedDeletedBlockSize];
                    deletedBlockSetForSize.emplace(newDeletedBlockOffset);
                }

                return deletedBlockWriteOffset;
            }
        }

        // Fall back to returning the archive header TOC offset as the write block offset
        AZ::u64 writeBlockOffset = m_archiveHeader.m_tocOffset;
        // Move the archive header TOC offset forward by the file size that will be written
        m_archiveHeader.m_tocOffset = writeBlockOffset + alignedFileSizeToWrite;
        return writeBlockOffset;
    }

    ArchiveFileToken ArchiveWriter::FindFile(AZ::IO::PathView relativePath) const
    {
        auto foundIt = m_pathMap.find(relativePath);
        return foundIt != m_pathMap.end() ? static_cast<ArchiveFileToken>(foundIt->second) : InvalidArchiveFileToken;
    }

    bool ArchiveWriter::ContainsFile(AZ::IO::PathView relativePath) const
    {
        return m_pathMap.contains(relativePath);
    }

    ArchiveRemoveFileResult ArchiveWriter::RemoveFileFromArchive(ArchiveFileToken filePathToken)
    {
        ArchiveRemoveFileResult result;
        const size_t archiveFileIndex = static_cast<size_t>(filePathToken);
        if (archiveFileIndex < m_archiveToc.m_fileMetadataTable.size()
            && !m_removedFileIndices.contains(archiveFileIndex))
        {
            // Add the archive file index to the set of removed file indices
            m_removedFileIndices.emplace(archiveFileIndex);

            // Get a reference to the table of contents entry being remove and add its blocks to the deleted block map
            ArchiveTocFileMetadata& fileMetadata = m_archiveToc.m_fileMetadataTable[archiveFileIndex];
            AZ::u64 blockSize = fileMetadata.m_compressionAlgoIndex == UncompressedAlgorithmIndex
                ? fileMetadata.m_uncompressedSize
                : fileMetadata.m_compressedSizeInSectors * ArchiveDefaultBlockAlignment;
            // FYI: The compressed size in sectors is the aggregate represents the total size of the compressed
            // 2-MiB blocks as stored in the raw data
            // See `ArchiveTocFileMetadata` structure for more info
            AZ::u64 alignedBlockSize = AZ_SIZE_ALIGN_UP(blockSize, ArchiveDefaultBlockAlignment);
            AZ::u64 alignedBlockOffset = AZ_SIZE_ALIGN_UP(fileMetadata.m_offset, ArchiveDefaultBlockAlignment);

            // If the new block size aligned down to nearest 512-byte boundary is 0
            // then there deleted blocks
            if (alignedBlockSize > 0)
            {
                auto& deletedBlockOffsetSet = m_deletedBlockSizeToOffsetMap[alignedBlockSize];
                deletedBlockOffsetSet.emplace(alignedBlockOffset);
            }

            // Update the result structure with the metadata about the removed file
            result.m_uncompressedSize = fileMetadata.m_uncompressedSize;

            // Get the actual size that the compressed data takes on disk
            if (auto rawFileSizeResult = GetRawFileSize(fileMetadata, m_archiveToc.m_blockOffsetTable);
                rawFileSizeResult)
            {
                result.m_compressedSize = rawFileSizeResult.value();
            }
            result.m_offset = fileMetadata.m_offset;

            // If the was compressed, retrieve the compression algorithm Id associated with the index
            if (fileMetadata.m_compressionAlgoIndex < UncompressedAlgorithmIndex)
            {
                result.m_compressionAlgorithm = m_archiveHeader.m_compressionAlgorithmsIds[fileMetadata.m_compressionAlgoIndex];
            }

            // Clear out the FileMetadata entry from Accelerating Table of Contents structure
            fileMetadata = {};

            // Move the file path stored in the table of contents into the result structure
            result.m_relativeFilePath = static_cast<AZ::IO::Path&&>(AZStd::move(m_archiveToc.m_filePaths[archiveFileIndex]));

            // Remove the file path -> file token store for this ArchiveWriter
            if (const size_t erasedPathCount = m_pathMap.erase(result.m_relativeFilePath);
                erasedPathCount == 0)
            {
                result.m_resultOutcome = AZStd::unexpected(ResultString::format("Removing mapping of file path from the Archive Writer"
                    R"(file path -> archive file token map failed to locate path "%s")", result.m_relativeFilePath.c_str()));
            }

            // Decrement the file count in the header
            --m_archiveHeader.m_fileCount;

        }

        return result;
    }

    ArchiveRemoveFileResult ArchiveWriter::RemoveFileFromArchive(AZ::IO::PathView relativePath)
    {
        const auto pathIt = m_pathMap.find(relativePath);
        return pathIt != m_pathMap.end() ? RemoveFileFromArchive(static_cast<ArchiveFileToken>(pathIt->second))
            : ArchiveRemoveFileResult{};
    }

    bool ArchiveWriter::DumpArchiveMetadata(AZ::IO::GenericStream& metadataStream,
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
            if (m_archiveToc.m_filePaths.size() != m_archiveToc.m_fileMetadataTable.size())
            {
                auto errorString = MetadataString::format("Error: The Archive TOC of contents has a mismatched size between"
                    " the file path vector (size=%zu) and the file metadata vector (size=%zu).\n"
                    "This indicates a code error in the ArchiveWriter.",
                    m_archiveToc.m_filePaths.size(), m_archiveToc.m_fileMetadataTable.size());
                metadataStream.Write(errorString.size(), errorString.data());
                return false;
            }

            // Tracks the offset of current non-deleted file entry in the table of contents
            // Deleted entries in the table of contents are skipped
            size_t activeFileOffset{};


            for (size_t fileMetadataTableIndex{}; fileMetadataTableIndex < m_archiveToc.m_filePaths.size(); ++fileMetadataTableIndex)
            {
                const ArchiveTableOfContents::Path& contentFilePath = m_archiveToc.m_filePaths[fileMetadataTableIndex];
                // An empty file path is used to track removed files from the archive,
                // therefore only non-empty paths are iterated
                if (!contentFilePath.empty())
                {
                    const ArchiveTocFileMetadata& contentFileMetadata = m_archiveToc.m_fileMetadataTable[fileMetadataTableIndex];
                    auto fileMetadataString = MetadataString::format(R"(File %zu: path="%s")", activeFileOffset, contentFilePath.c_str());
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
                            if (auto rawFileSizeResult = GetRawFileSize(contentFileMetadata, m_archiveToc.m_blockOffsetTable);
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

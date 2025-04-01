/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>

namespace Archive
{
    //! ArchiveTableOfContentsView member implementation
    inline ArchiveTableOfContentsView::ArchiveTableOfContentsView() = default;

    inline auto ArchiveTableOfContentsView::CreateFromArchiveHeaderAndBuffer(const ArchiveHeader& archiveHeader,
        AZStd::span<AZStd::byte> tocBuffer) -> CreateTOCViewOutcome
    {
        // A valid table of contents must have at least 8 bytes to store the Magic Bytes
        if (tocBuffer.size() < sizeof(ArchiveTocMagicBytes))
        {
            ArchiveTocValidationResult tocValidationResult;
            tocValidationResult.m_errorCode = ArchiveTocErrorCode::InvalidMagicBytes;
            tocValidationResult.m_errorMessage = ArchiveTocValidationResult::ErrorString::format(
                "The Archive TOC is empty. It must be at least %zu bytes to store the Magic Bytes",
                sizeof(ArchiveTocMagicBytes));
            return CreateTOCViewOutcome(AZStd::unexpected(AZStd::move(tocValidationResult)));
        }

        ArchiveTableOfContentsView tocView;

        // magic bytes offset
        constexpr size_t MagicBytesOffset = 0;

        // Round up the File metadata offset as it is 32-byte aligned
        constexpr size_t FileMetadataTableOffset = AZ_SIZE_ALIGN_UP(
            MagicBytesOffset + sizeof(ArchiveTocMagicBytes),
            sizeof(ArchiveTocFileMetadata));

        // The file path metadata entries is 32 bytes aligned
        // so round up to the nearest multiple of alignment before reading the file path index entries
        const size_t FilePathIndexTableOffset = AZ_SIZE_ALIGN_UP(
            FileMetadataTableOffset + archiveHeader.m_tocFileMetadataTableUncompressedSize,
            sizeof(ArchiveTocFileMetadata));
        // The file path index entries are 8 bytes aligned
        // so round up to the nearest multiple of alignment before reading the file path blob
        const size_t FilePathBlobOffset = AZ_SIZE_ALIGN_UP(
            FilePathIndexTableOffset + archiveHeader.m_tocPathIndexTableUncompressedSize,
            sizeof(ArchiveTocFilePathIndex));

        // The block offset table starts on an address aligned at a multiple of 8 bytes
        const size_t BlockOffsetTableOffset = AZ_SIZE_ALIGN_UP(
            FilePathBlobOffset + archiveHeader.m_tocPathBlobUncompressedSize,
            sizeof(ArchiveBlockLineUnion));

        // Cast the first 8 of the TOC buffer
        tocView.m_magicBytes = *reinterpret_cast<decltype(tocView.m_magicBytes)*>(tocBuffer.data() + MagicBytesOffset);
        // create a span to the file metadata entries
        tocView.m_fileMetadataTable = AZStd::span(
            reinterpret_cast<const ArchiveTocFileMetadata*>(tocBuffer.data() + FileMetadataTableOffset),
            archiveHeader.m_fileCount);

        // create a span to the file path index entries
        tocView.m_filePathIndexTable = AZStd::span(
            reinterpret_cast<const ArchiveTocFilePathIndex*>(tocBuffer.data() + FilePathIndexTableOffset),
            archiveHeader.m_fileCount);

        // create a string view to the file path blob
        tocView.m_filePathBlob = AZStd::string_view(
            reinterpret_cast<const char*>(tocBuffer.data() + FilePathBlobOffset),
            archiveHeader.m_tocPathBlobUncompressedSize);

        // create a span to the block offset entries
        const auto blockOffsetTableBegin = reinterpret_cast<const ArchiveBlockLineUnion*>(tocBuffer.data() + BlockOffsetTableOffset);
        const auto blockOffsetTableEnd = reinterpret_cast<const ArchiveBlockLineUnion*>(reinterpret_cast<const AZStd::byte*>(blockOffsetTableBegin)
            + archiveHeader.m_tocBlockOffsetTableUncompressedSize);
        tocView.m_blockOffsetTable = AZStd::span(
            blockOffsetTableBegin,
            blockOffsetTableEnd);

        ArchiveTocValidationOptions validationSettings;
        // Skip over validating the block Offset table has that is a potentially slow operation
        validationSettings.m_validateBlockOffsetTable = false;
        ArchiveTocValidationResult validationResult = ValidateTableOfContents(tocView, archiveHeader,
            validationSettings);
        return validationResult ? CreateTOCViewOutcome{ tocView } : CreateTOCViewOutcome{ AZStd::unexpected(AZStd::move(validationResult)) };
    }

    constexpr ArchiveTocValidationResult::operator bool() const
    {
        return m_errorCode == ArchiveTocErrorCode{};
    }

    inline ArchiveTocValidationResult ValidateTableOfContents(const ArchiveTableOfContentsView& tocView,
        const ArchiveHeader& archiveHeader,
        const ArchiveTocValidationOptions& validationOptions)
    {
        if (tocView.m_magicBytes != ArchiveTocMagicBytes)
        {
            ArchiveTocValidationResult tocValidationResult;
            tocValidationResult.m_errorCode = ArchiveTocErrorCode::InvalidMagicBytes;
            tocValidationResult.m_errorMessage = ArchiveTocValidationResult::ErrorString::format(
                "The Archive TOC has an invalid magic byte sequence of %llx", tocView.m_magicBytes);
            return tocValidationResult;
        }

        if (validationOptions.m_validateFileMetadataTable
            && tocView.m_fileMetadataTable.size_bytes() != archiveHeader.m_tocFileMetadataTableUncompressedSize)
        {
            ArchiveTocValidationResult tocValidationResult;
            tocValidationResult.m_errorCode = ArchiveTocErrorCode::FileMetadataTableSizeMismatch;
            tocValidationResult.m_errorMessage = ArchiveTocValidationResult::ErrorString::format(
                "The Archive TOC File Metadata table size %zu does not match the uncompressed size %u",
                tocView.m_fileMetadataTable.size_bytes(),
                archiveHeader.m_tocFileMetadataTableUncompressedSize);
            return tocValidationResult;
        }

        if (validationOptions.m_validateFileIndexTable
            && tocView.m_filePathIndexTable.size_bytes() != archiveHeader.m_tocPathIndexTableUncompressedSize)
        {
            ArchiveTocValidationResult tocValidationResult;
            tocValidationResult.m_errorCode = ArchiveTocErrorCode::FileIndexTableSizeMismatch;
            tocValidationResult.m_errorMessage = ArchiveTocValidationResult::ErrorString::format(
                "The Archive TOC File Path Index table size %zu does not match the uncompressed size %u",
                tocView.m_filePathIndexTable.size_bytes(),
                archiveHeader.m_tocPathIndexTableUncompressedSize);
            return tocValidationResult;
        }

        if (validationOptions.m_validateBlockOffsetTable)
        {
            // Validate the number blocks line offset table entries matches the total number of block
            // lines used per file
            // that should be in the table of contents based on the size
            // of each file uncompressed size
            AZ::u64 expectedBlockLineCount{};
            for (const ArchiveTocFileMetadata& archiveFileMetadata : tocView.m_fileMetadataTable)
            {
                // Uncompressed files are stored in contiguous memory without any blocks
                if (archiveFileMetadata.m_compressionAlgoIndex == UncompressedAlgorithmIndex)
                {
                    continue;
                }

                // The number of blocks a file contains is based on how many 2 MiB chunks can be extracted
                // using it's uncompressed size
                expectedBlockLineCount += GetBlockLineCountIfCompressed(archiveFileMetadata.m_uncompressedSize);
            }

            if (expectedBlockLineCount == tocView.m_blockOffsetTable.size())
            {
                ArchiveTocValidationResult tocValidationResult;
                tocValidationResult.m_errorCode = ArchiveTocErrorCode::BlockOffsetTableCountMismatch;
                tocValidationResult.m_errorMessage = ArchiveTocValidationResult::ErrorString::format(
                    "The count of blocks lines used by each compressed file is %llu, which does not match the count"
                    " of entries in the block offset table %zu",
                    expectedBlockLineCount,
                    tocView.m_blockOffsetTable.size());
                return tocValidationResult;
            }
        }

        // No failure has occurred so return a default initialized validation result instance
        return {};
    }

    inline size_t EnumerateFilePathIndexOffsets(FilePathIndexEntryVisitor callback,
        const ArchiveTableOfContentsView& tocView)
    {
        size_t filePathsVisited{};
        for (const auto& filePathIndexEntry : tocView.m_filePathIndexTable)
        {
            // Invoke the visitor callback if the file path size > 0
            if (filePathIndexEntry.m_size > 0)
            {
                callback(filePathIndexEntry.m_offset, static_cast<AZ::u16>(filePathIndexEntry.m_size));
                ++filePathsVisited;
            }
        }

        return filePathsVisited;
    }

    // Implementation of function which returns a span that encompass the block lines
    // for a content file provided that the first block line index for that file is supplied
    // along with that file uncompressed size
    inline FileBlockLineOutcome GetBlockLineSpanForFile(const ArchiveTableOfContentsView& tocView,
        size_t indexInFileMetadataTable)
    {
        if (indexInFileMetadataTable >= tocView.m_fileMetadataTable.size())
        {
            return AZStd::unexpected(ResultString::format("The index %zu into the Table of Contents file metadata table"
                " is is out of range", indexInFileMetadataTable));
        }

        // Query the uncompressed size and block line start index for the file from the TOC
        const AZ::u64 uncompressedSize = tocView.m_fileMetadataTable[indexInFileMetadataTable].m_uncompressedSize;
        const AZ::u64 blockLineFirstIndex = tocView.m_fileMetadataTable[indexInFileMetadataTable].m_blockLineTableFirstIndex;

        // Determine the number of blocks lines a file uses based on the uncompressed size
        const AZ::u64 fileBlockLineCount = GetBlockLineCountIfCompressed(uncompressedSize);
        if (blockLineFirstIndex >= tocView.m_blockOffsetTable.size()
            && (blockLineFirstIndex + fileBlockLineCount) >= tocView.m_blockOffsetTable.size())
        {
            return AZStd::unexpected(ResultString::format("Either the first block index for the file with value %llu"
                "is incorrect or the uncompressed size (%llu) for the file is incorrect", blockLineFirstIndex,
                uncompressedSize));
        }

        // returns a subspan that starts at the first block line for the file
        // and continues for the count of the block lines based on the uncompressed size
        return tocView.m_blockOffsetTable.subspan(blockLineFirstIndex, fileBlockLineCount);
    }

    // Retrieves the compressed size for the given block
    inline AZ::u64 GetCompressedSizeForBlock(AZStd::span<const ArchiveBlockLineUnion> fileBlockLineSpan,
        AZ::u64 blockCount, AZ::u64 blockIndex)
    {
        // Get the index of the block line corresponding to the index of the block
        auto blockLineResult = GetBlockLineIndexFromBlockIndex(blockCount, blockIndex);
        if (!blockLineResult)
        {
            return 0;
        }

        // Block line indices which are multiples of 3 all have jump entries unless they are part of the final 3 block
        // lines of a file
        const bool blockLineContainsJump = (blockLineResult.m_blockLineIndex % BlockLinesToSkipWithJumpEntry == 0)
            && (fileBlockLineSpan.size() - blockLineResult.m_blockLineIndex) > BlockLinesToSkipWithJumpEntry;
        if (blockLineContainsJump)
        {
            const ArchiveBlockLineJump& blockLineWithJump = fileBlockLineSpan[
                blockLineResult.m_blockLineIndex].m_blockLineWithJump;
            switch (blockLineResult.m_offsetInBlockLine)
            {
            case 0:
                return blockLineWithJump.m_block0;
            case 1:
                return blockLineWithJump.m_block1;
            default:
                return 0;
            }
        }
        else
        {
            const ArchiveBlockLine& blockLine = fileBlockLineSpan[
                blockLineResult.m_blockLineIndex].m_blockLine;
            switch (blockLineResult.m_offsetInBlockLine)
            {
            case 0:
                return blockLine.m_block0;
            case 1:
                return blockLine.m_block1;
            case 2:
                return blockLine.m_block2;
            default:
                return 0;
            }
        }
    }

    // Returns the summation of the compressed sizes of each block of the file
    inline GetRawFileSizeOutcome GetRawFileSize(const ArchiveTocFileMetadata& fileMetadata,
        AZStd::span<const ArchiveBlockLineUnion> tocBlockOffsetTable)
    {
        // Query the uncompressed size TOC
        const AZ::u64 uncompressedSize = fileMetadata.m_uncompressedSize;

        // If the uncompressed, then return the uncompressed size as the raw file size
        if (fileMetadata.m_compressionAlgoIndex >= UncompressedAlgorithmIndex)
        {
            return uncompressedSize;
        }

        // Determine the number of blocks lines a file uses based on the uncompressed size
        const AZ::u64 blockLineFirstIndex = fileMetadata.m_blockLineTableFirstIndex;
        const AZ::u64 fileBlockLineCount = GetBlockLineCountIfCompressed(uncompressedSize);
        if (blockLineFirstIndex >= tocBlockOffsetTable.size()
            && (blockLineFirstIndex + fileBlockLineCount) >= tocBlockOffsetTable.size())
        {
            return AZStd::unexpected(ResultString::format("The first block offset entry for the file with value %llu"
                "is incorrect or the uncompressed size (%llu) is incorrect", blockLineFirstIndex,
                uncompressedSize));
        }

        // Get a subspan of only the blocks associated with the file
        AZStd::span<const ArchiveBlockLineUnion> fileBlockLineSpan = tocBlockOffsetTable.subspan(blockLineFirstIndex, fileBlockLineCount);

        AZ::u64 compressedSize{};
        size_t blockCount = GetBlockCountIfCompressed(uncompressedSize);
        for (size_t blockLineIndex{}, blockIndex{}; blockLineIndex < fileBlockLineSpan.size();)
        {
            // Determine if the block line contains a jump entry
            // This can be used to determine the compressed block sizes for the next 8 blocks (3 block lines)
            const bool blockLineContainsJump = (blockLineIndex % BlockLinesToSkipWithJumpEntry == 0)
                && (fileBlockLineSpan.size() - blockLineIndex) > BlockLinesToSkipWithJumpEntry;
            if (blockLineContainsJump)
            {
                const ArchiveBlockLineJump& blockLineWithJump = fileBlockLineSpan[blockLineIndex].m_blockLineWithJump;
                // A jump entry contains the aligned compressed size for the next 8 blocks
                compressedSize = blockLineWithJump.m_blockJump * ArchiveDefaultBlockAlignment;
                blockLineIndex += BlockLinesToSkipWithJumpEntry;
                blockIndex += BlocksToSkipWithJumpEntry;
            }
            else
            {
                // When this logic is reached, there are no more blocks with jump entries in the file
                // therefore add the aligned size of compressed block EXCEPT the last block
                // where it's actual size is used.
                const ArchiveBlockLine& blockLine = fileBlockLineSpan[blockLineIndex].m_blockLine;

                // Check if the current blockIndex corresponds to the last block of the compressed
                // file section
                if (bool isLastBlock = ++blockIndex == blockCount;
                    isLastBlock)
                {
                    compressedSize += blockLine.m_block0;
                    break;
                }
                compressedSize += AZ_SIZE_ALIGN_UP(blockLine.m_block0, ArchiveDefaultBlockAlignment);

                if (bool isLastBlock = ++blockIndex == blockCount;
                    isLastBlock)
                {
                    compressedSize += blockLine.m_block1;
                    break;
                }
                compressedSize += AZ_SIZE_ALIGN_UP(blockLine.m_block2, ArchiveDefaultBlockAlignment);
                if (bool isLastBlock = ++blockIndex == blockCount;
                    isLastBlock)
                {
                    compressedSize += blockLine.m_block2;
                    break;
                }
                compressedSize += AZ_SIZE_ALIGN_UP(blockLine.m_block2, ArchiveDefaultBlockAlignment);

                // Increment the block line index by 1 to move to the next block
                ++blockLineIndex;
            }
        }

        return compressedSize;
    }

} // namespace Archive

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
    ArchiveTableOfContentsView::ArchiveTableOfContentsView() = default;

    auto ArchiveTableOfContentsView::CreateFromArchiveHeaderAndBuffer(const ArchiveHeader& archiveHeader,
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

        // Round up the File metadata offset as it is 16-byte aligned
        constexpr size_t FileMetadataTableOffset = AZ_SIZE_ALIGN_UP(
            MagicBytesOffset + sizeof(ArchiveTocMagicBytes),
            16);

        // The file path metadata entries is 16 bytes aligned
        // so round up to the nearest 16th byte before reading the file path index entries
        const size_t FilePathIndexTableOffset = AZ_SIZE_ALIGN_UP(
            FileMetadataTableOffset + archiveHeader.m_tocFileMetadataTableUncompressedSize,
            16);
        // The file path index entries are 8 bytes aligned
        // so round up to the nearest 8th byte before reading the file path blob
        const size_t FilePathBlobOffset = AZ_SIZE_ALIGN_UP(
            FilePathIndexTableOffset + archiveHeader.m_tocPathIndexTableUncompressedSize,
            8);

        // The block offset table starts on an address aligned at a multiple of 8 bytes
        const size_t BlockOffsetTableOffset = AZ_SIZE_ALIGN_UP(
            FilePathBlobOffset + archiveHeader.m_tocPathBlobUncompressedSize,
            8);

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

    ArchiveTocValidationResult::operator bool() const
    {
        return m_errorCode == ArchiveTocErrorCode{};
    }

    ArchiveTocValidationResult ValidateTableOfContents(const ArchiveTableOfContentsView& tocView,
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

    size_t EnumerateFilePathIndexOffsets(FilePathIndexEntryVisitor callback,
        const ArchiveTableOfContentsView& tocView)
    {
        size_t filePathsVisited{};
        for (const auto& filePathIndexEntry : tocView.m_filePathIndexTable)
        {
            // Invoke the visitor callback if the file path size > 0
            if (filePathIndexEntry.m_size > 0)
            {
                callback(filePathIndexEntry.m_offset, filePathIndexEntry.m_size);
                ++filePathsVisited;
            }
        }

        return filePathsVisited;
    }

} // namespace Archive


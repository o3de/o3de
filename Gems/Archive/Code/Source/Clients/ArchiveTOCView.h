/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_fwd.h>

#include <Archive/Clients/ArchiveInterfaceStructs.h>

namespace Archive
{
    //! Stores the error code resulting from the validation of the table of contents
    //! against the ArchiveHeader
    enum class ArchiveTocErrorCode
    {
        InvalidMagicBytes = 1,
        FileMetadataTableSizeMismatch,
        FileIndexTableSizeMismatch,
        BlockOffsetTableCountMismatch
    };

    //! Stores the error code and any error messages related to failing
    //! to validate the archive table of contents against its header
    struct ArchiveTocValidationResult
    {
        explicit operator bool() const;
        ArchiveTocErrorCode m_errorCode{};
        using ErrorString = AZStd::basic_fixed_string<char, 512, AZStd::char_traits<char>>;
        ErrorString m_errorMessage;
    };

    //! View Structure for viewing the Table of Contents at the end of the archive file
    //! This structure does not own the table of contents data in memory
    struct ArchiveTableOfContentsView
    {
        ArchiveTableOfContentsView();

        //! Initializes a Table of Contents view using the archive header
        //! and a buffer containing the uncompressed table of contents data from storage
        using CreateTOCViewOutcome = AZStd::expected<ArchiveTableOfContentsView, ArchiveTocValidationResult>;
        static CreateTOCViewOutcome CreateFromArchiveHeaderAndBuffer(const ArchiveHeader& archiveHeader,
            AZStd::span<AZStd::byte> tocBuffer);

        //! 8-byte magic bytes entry used to indicate that the read table of contents is valid
        AZ::u64 m_magicBytes = ArchiveTocMagicBytes;
        //! pointer to the beginning of the Archive File Metadata Table
        //! It's length is based on the file count value in the Archive Header Section
        AZStd::span<ArchiveTocFileMetadata const> m_fileMetadataTable{};
        //! pointer to the beginning of the Archive File Path Index Table
        //! It's length is based on the file count value in the Archive Header Section
        AZStd::span<ArchiveTocFilePathIndex const> m_filePathIndexTable{};

        //! ArchiveFilePathTable is a view into the start of the blob for file paths
        using ArchiveFilePathTable = AZStd::string_view;
        ArchiveFilePathTable m_filePathBlob{};

        //! pointer to block offset table which stores the compressed size of all blocks within the archive
        AZStd::span<ArchiveBlockLineUnion const> m_blockOffsetTable{};
    };

    //! Options which allows configuring which sections of the table of contents
    //! should be validated.
    //! NOTE: The Block Offset table takes the longest time to validate
    //! as it verifies the number of block lines in the table of contents
    //! is equivalent to the number of block lines each file should have
    //! It does this by calculating the number of block lines a file should
    //! have by examining each file uncompressed size
    struct ArchiveTocValidationOptions
    {
        bool m_validateFileMetadataTable{ true };
        bool m_validateFileIndexTable{ true };
        bool m_validateBlockOffsetTable{ true };
    };
    //! Validates the Table of Contents data
    //! This is a potentially lengthy operation as it verifies
    //! That the each archived file in the Table of Contents
    //! has the correct number of blocks based on whether it is
    //! uncompressed(file is stored contiguously with no entry in the block table)
    //! or compressed(the number of blocks stored is based on the uncompressed size of the file split into 2 MiB chunks);
    ArchiveTocValidationResult ValidateTableOfContents(const ArchiveTableOfContentsView& tocView,
        const ArchiveHeader& archiveHeader,
        const ArchiveTocValidationOptions& validationOptions = {});

    //! Visitor invoked for each deleted file path index entry
    //! It gets passed in the offset within the file path blob table where the deleted file path resides
    //! and the length of that file path
    //! @param filePathBlobOffset Offset into the raw archive TOC file path blob table where the deleted file path starts
    //! @param filePathSize length of the deleted file path. The value is guaranteed to be >0
    using FilePathIndexEntryVisitor = AZStd::function<void(AZ::u32 filePathBlobOffset, AZ::u16 filePathSize)>;

    //! Enumerates each file path index found in the TOC View
    //! @param callback to invoke when a deleted path index entry is found
    //! @param tocView view structure overlaying the raw table of contents data
    //! @return the number of file path index entries visited
    size_t EnumerateFilePathIndexOffsets(FilePathIndexEntryVisitor callback,
        const ArchiveTableOfContentsView& tocView);
} // namespace Archive

// Implementation for any struct functions
#include "ArchiveTOCView.inl"

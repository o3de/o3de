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
        explicit constexpr operator bool() const;
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
    using FilePathIndexEntryVisitor = AZStd::function<void(AZ::u64 filePathBlobOffset, AZ::u16 filePathSize)>;

    //! Enumerates each file path index found in the TOC View
    //! @param callback to invoke when a deleted path index entry is found
    //! @param tocView view structure overlaying the raw table of contents data
    //! @return the number of file path index entries visited
    size_t EnumerateFilePathIndexOffsets(FilePathIndexEntryVisitor callback,
        const ArchiveTableOfContentsView& tocView);
} // namespace Archive

// Separate namespace block to create visual spacing around utility functions
// for querying the the Table of Contents View
namespace Archive
{
   //! Retrieves a subspan from the archive TOC BlockLine Offset Table
   //! that contains the block lines associated with a specific file
   //! @param tocView readonly view into the Archive TOC
   //!        The file metadata table and block offset table is accessed using the TOC view
   //! @param fileMetadataTableIndex index into the file metadata table
   //!        This is used to lookup the uncompressed size and the first block line offset entry
   //!        for a file
   //! @return On success return a span over a view of each block line associated with the file
   //!         at the provided file metadata table index.
   //!         On failure returns an error message providing the reason the span could not be created
    using FileBlockLineOutcome = AZStd::expected<AZStd::span<const ArchiveBlockLineUnion>, ResultString>;
    FileBlockLineOutcome GetBlockLineSpanForFile(const ArchiveTableOfContentsView& tocView,
        size_t fileMetadataTableIndex);

    //! Queries the compressed size at the block index in the block line span for the compressed file
    //! @param fileBlockLineSpan span of the block lines associated with a single file
    //!       The entire TOC block line span should NOT be passed to this function
    //!       The span that is passed in should be from a call of GetBlockLineSpanForFile()
    //! @param blockCount The number of 2-MiB blocks for the file
    //! @param blockIndex index of block entry in the block line span to read the compressed size
    //! @return the compressed size value for the block if block index corresponds to a block in the file
    //!         otherwise 0 is returned
    AZ::u64 GetCompressedSizeForBlock(AZStd::span<const ArchiveBlockLineUnion> fileBlockLineSpan,
        AZ::u64 blockCount, AZ::u64 blockIndex);

   //! Gets the raw size for the file in the archive
   //! If the file is uncompressed then the uncompressed size is returned from the file metadata
   //! If the file is compressed, then this returns the size needed to read the contiguous
   //! sequence of compressed blocks exact
   //!
   //! @param fileMetadata reference to the TOC file metadata entry which contains the start offset into
   //!        the block offset table for the file.
   //!        The TOC file metadata structure m_uncompressedSize member is used to determine
   //! @param tocBlockOffsetTable span for the entire TOC block line offset table
   //! @return On success return the exact size for the file as stored in the archive
   //!         On failure returns an error message with the failure reason
    using GetRawFileSizeOutcome = AZStd::expected<AZ::u64, ResultString>;
    GetRawFileSizeOutcome GetRawFileSize(const ArchiveTocFileMetadata& fileMetadata,
        AZStd::span<const ArchiveBlockLineUnion> tocBlockOffsetTable);
}

// Implementation for any struct functions
#include "ArchiveTOCView.inl"

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_fwd.h>

#include <Archive/ArchiveInterfaceStructs.h>

namespace Archive
{
    struct ArchiveTableOfContentsView;

    //! String type which stores the error message when enumerating archived files
    using EnumerateErrorString = AZStd::fixed_string<512>;

    //! Structure for which owns the Table of Contents data
    //! It contains data structure which makes it easier to dynamically add/remove/update
    //! files to the table of contents while in memory
    struct ArchiveTableOfContents
    {
        //! The Archive TOC is either uncompressed
        //! or compressed based on the compression algorithm index
        //! set within the Archive Header
        ArchiveTableOfContents();

        //! Initializes a Table of Contents in-memory structure from a Table of Contents view
        using CreateFromTocViewOutcome = AZStd::expected<ArchiveTableOfContents, EnumerateErrorString>;
        static CreateFromTocViewOutcome CreateFromTocView(const ArchiveTableOfContentsView& tocView);

        //! vector storing a copy of each file metadata entry in memory
        //! It's length matches the value of m_fileCount
        AZStd::vector<ArchiveTocFileMetadata> m_fileMetadataTable;

        //! vector storing a copy of each file path in memory
        //! It's length matches the value of m_fileCount
        using Path = AZ::IO::Path;
        using ArchiveFilePathTable = AZStd::vector<Path>;
        ArchiveFilePathTable m_filePaths;

        //! vector storing the block offset table for each file
        AZStd::vector<ArchiveBlockLineUnion> m_blockOffsetTable{};
    };
} // namespace Archive

// Implementation for any struct functions
#include "ArchiveTOC.inl"

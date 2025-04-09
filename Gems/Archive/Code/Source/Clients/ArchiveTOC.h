/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_fwd.h>

#include <Archive/Clients/ArchiveInterfaceStructs.h>

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


        //! Wrapper path structure to ensure the Table of Contents only contain paths that
        //! uses the Posix Path Separator '/'
        //! This is used to normalize how the paths within the Table of Contents
        //! are stored across platforms(Linux/MacOS vs Windows)
        struct Path
        {
            Path() = default;

            Path(const Path&) = default;
            Path(Path&&) = default;
            //! Implicit conversion constructor to store move an AZ::IO::Path
            Path(AZ::IO::Path filePath);
            Path(AZ::IO::PathView filePath);

            Path& operator=(const Path&) = default;
            Path& operator=(Path&&) = default;
            //! Add support for storing an AZ::IO::Path into the table of contents
            //! as a path with only Posix Path Separators of '/'
            Path& operator=(AZ::IO::Path filePath);
            Path& operator=(AZ::IO::PathView filePath);

            //! implicit AZ::IO::Path operator that returns a reference
            //! to the underlying filesystem path
            operator AZ::IO::Path& () &;
            operator const AZ::IO::Path& () const&;
            operator AZ::IO::Path&& ()&&;
            operator const AZ::IO::Path&& () const&&;

            [[nodiscard]] bool empty() const;
            void clear();
            const typename AZ::IO::Path::string_type& Native() const& noexcept;
            const typename AZ::IO::Path::string_type&& Native() const&& noexcept;
            typename AZ::IO::Path::string_type& Native() & noexcept;
            typename AZ::IO::Path::string_type&& Native() && noexcept;
            const typename AZ::IO::Path::value_type* c_str() const noexcept;

        private:
            AZ::IO::Path m_posixPath{ AZ::IO::PosixPathSeparator };
        };

        //! vector storing a copy of each file path in memory
        //! Its length matches the value of m_fileCount
        using ArchiveFilePathTable = AZStd::vector<Path>;
        ArchiveFilePathTable m_filePaths;

        //! vector storing the block offset table for each file
        AZStd::vector<ArchiveBlockLineUnion> m_blockOffsetTable{};
    };
} // namespace Archive

// Implementation for any struct functions
#include "ArchiveTOC.inl"

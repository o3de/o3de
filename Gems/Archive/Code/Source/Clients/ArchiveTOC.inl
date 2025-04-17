/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ArchiveTOCView.h"

namespace Archive
{
    inline ArchiveTableOfContents::ArchiveTableOfContents() = default;

    // Archive Table of Contents file path wrapper
    inline ArchiveTableOfContents::Path::Path(AZ::IO::Path filePath)
        : m_posixPath{ AZStd::move(filePath.Native()), AZ::IO::PosixPathSeparator }
    {
        // Convert the path to use Posix path separators
        m_posixPath.MakePreferred();
    }

    inline ArchiveTableOfContents::Path::Path(AZ::IO::PathView filePath)
        : m_posixPath{ filePath.Native(), AZ::IO::PosixPathSeparator }
    {
        // Convert the path to use Posix path separators
        m_posixPath.MakePreferred();
    }

    inline auto ArchiveTableOfContents::Path::operator=(AZ::IO::Path filePath) -> Path&
    {
        m_posixPath = AZ::IO::Path(AZStd::move(filePath.Native()), AZ::IO::PosixPathSeparator).MakePreferred();
        return *this;
    }

    inline auto ArchiveTableOfContents::Path::operator=(AZ::IO::PathView filePath) -> Path&
    {
        m_posixPath = AZ::IO::Path(filePath.Native(), AZ::IO::PosixPathSeparator).MakePreferred();
        return *this;
    }

    inline ArchiveTableOfContents::Path::operator AZ::IO::Path&() &
    {
        return m_posixPath;
    }

    inline ArchiveTableOfContents::Path::operator const AZ::IO::Path&() const&
    {
        return m_posixPath;
    }

    inline ArchiveTableOfContents::Path::operator AZ::IO::Path&&() &&
    {
        return AZStd::move(m_posixPath);
    }

    inline ArchiveTableOfContents::Path::operator const AZ::IO::Path&&() const&&
    {
        return AZStd::move(m_posixPath);
    }

    inline bool ArchiveTableOfContents::Path::empty() const
    {
        return m_posixPath.empty();
    }
    inline void ArchiveTableOfContents::Path::clear()
    {
        m_posixPath.clear();
    }

    inline const typename AZ::IO::Path::string_type& ArchiveTableOfContents::Path::Native() const& noexcept
    {
        return m_posixPath.Native();
    }

    inline const typename AZ::IO::Path::string_type&& ArchiveTableOfContents::Path::Native() const&& noexcept
    {
        return AZStd::move(m_posixPath.Native());
    }
    inline typename AZ::IO::Path::string_type& ArchiveTableOfContents::Path::Native() & noexcept
    {
        return m_posixPath.Native();
    }
    inline typename AZ::IO::Path::string_type&& ArchiveTableOfContents::Path::Native() && noexcept
    {
        return AZStd::move(m_posixPath.Native());
    }
    inline const typename AZ::IO::Path::value_type* ArchiveTableOfContents::Path::c_str() const noexcept
    {
        return m_posixPath.c_str();
    }

    inline auto ArchiveTableOfContents::CreateFromTocView(
        const ArchiveTableOfContentsView& tocView) -> CreateFromTocViewOutcome
    {
        ArchiveTableOfContents tableOfContents;
        tableOfContents.m_fileMetadataTable = { tocView.m_fileMetadataTable.begin(), tocView.m_fileMetadataTable.end() };
        tableOfContents.m_blockOffsetTable = { tocView.m_blockOffsetTable.begin(), tocView.m_blockOffsetTable.end() };
        size_t fileCount = tocView.m_filePathIndexTable.size();

        // Populate the file path table using the file path index offset entries from the raw TOC view
        tableOfContents.m_filePaths.reserve(fileCount);
        for (const auto& filePathIndexEntry : tocView.m_filePathIndexTable)
        {
            // add an empty path entry and populate it with the file path
            auto& filePath = tableOfContents.m_filePaths.emplace_back();
            AZ::IO::PathView pathView{ AZStd::string_view(
                tocView.m_filePathBlob.data() + filePathIndexEntry.m_offset,
                filePathIndexEntry.m_size) };
            filePath = pathView.LexicallyNormal();
        }

        return tableOfContents;
    }
} // namespace Archive


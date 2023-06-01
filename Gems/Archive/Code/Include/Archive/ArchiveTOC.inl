/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>

#include <Archive/ArchiveTOCView.h>

namespace Archive
{
    ArchiveTableOfContents::ArchiveTableOfContents() = default;

    auto ArchiveTableOfContents::CreateFromTocView(
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


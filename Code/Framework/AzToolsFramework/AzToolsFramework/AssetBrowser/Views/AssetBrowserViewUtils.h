/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QString>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        class AssetBrowserViewUtils
        {
        public:
            static bool RenameEntry(const AZStd::vector<const AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void AfterRename(QString newVal, const AZStd::vector<const AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void DeleteEntries(const AZStd::vector<const AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void MoveEntries(const AZStd::vector<const AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void DuplicateEntries(const AZStd::vector<const AssetBrowserEntry*>& entries);
            static void MoveEntry(AZStd::string_view fromPath, AZStd::string_view toPath, bool isFolder, QWidget* parent = nullptr);
            static void CopyEntry(AZStd::string_view fromPath, AZStd::string_view toPath, bool isFolder);

            static bool IsFolderEmpty(AZStd::string_view path);
            static bool IsEngineOrProjectFolder(AZStd::string_view path);

            static Qt::DropAction SelectDropActionForEntries(const AZStd::vector<const AssetBrowserEntry*>& entries);

            // Returns the custom image or default icon for a given asset browser entry
            // @param returnIcon - when set to true, always returns the default icon for a given entry
            // @param isFavorite - used for folders in the AssetBrowser, will return the favoriteFolder ison rather than a standard folder.
            static QVariant GetThumbnail(const AssetBrowserEntry* entry, bool returnIcon = false, bool isFavorite = false);

            //! @return the most common flags for an asset browser entry, included drag and drop.
            static Qt::ItemFlags GetAssetBrowserEntryCommonItemFlags(const AssetBrowserEntry* entry, Qt::ItemFlags defaultFlags);

            //! @return the asset browser entry name with highlighted text
            static QString GetAssetBrowserEntryNameWithHighlighting(const AssetBrowserEntry* entry, const QString& highlightedText);
        };
    } // namespace AssetBrowser

} // namespace AzToolsFramework

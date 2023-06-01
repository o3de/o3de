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

            static bool IsFolderEmpty(AZStd::string_view path);
            static bool IsEngineOrProjectFolder(AZStd::string_view path);

            // Returns the custom image or default icon for a given asset browser entry
            // @param returnIcon - when set to true, always returns the default icon for a given entry
            static QVariant GetThumbnail(const AssetBrowserEntry* entry, bool returnIcon = false);
        };
    } // namespace AssetBrowser

} // namespace AzToolsFramework

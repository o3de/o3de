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
            static void RenameEntry(const AZStd::vector<AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void AfterRename(QString newVal, AZStd::vector<AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void DeleteEntries(const AZStd::vector<AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void MoveEntries(const AZStd::vector<AssetBrowserEntry*>& entries, QWidget* callingWidget);
            static void DuplicateEntries(const AZStd::vector<AssetBrowserEntry*>& entries);
            static void MoveEntry(AZStd::string_view fromPath, AZStd::string_view toPath, bool isFolder, QWidget* parent = nullptr);
        private:
            static bool IsFolderEmpty(AZStd::string_view path);
            static void EditName(QWidget* callingWidget);
        };
    } // namespace AssetBrowser

} // namespace AzToolsFramework

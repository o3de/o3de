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
            static bool IsFolderEmpty(AZStd::string_view path);
        private:
            static void EditName(QWidget* callingWidget);
        };
    } // namespace AssetBrowser

} // namespace AzToolsFramework

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        using ShownColumnsSet = AZStd::fixed_unordered_set<int, 3, aznumeric_cast<int>(AssetBrowserEntry::Column::Count)>;

        class AssetBrowserExpandedFilterModel
            : public AssetBrowserFilterModel
         {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserExpandedFilterModel, AZ::SystemAllocator);
            explicit AssetBrowserExpandedFilterModel(QObject* parent = nullptr);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

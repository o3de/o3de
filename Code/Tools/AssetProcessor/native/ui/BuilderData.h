/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AZCore/std/containers/vector.h>
#include <AZCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class AssetDatabaseConnection;
    }
} // namespace AzToolsFramework

namespace AssetProcessor
{
    class BuilderInfoMetricsItem;

    class BuilderData
    {
    public:
        BuilderData(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> dbConnection);
        void Reset();

        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_dbConnection;
        AZStd::shared_ptr<BuilderInfoMetricsItem> m_root;
        AZStd::shared_ptr<BuilderInfoMetricsItem> m_allBuildersMetrics;
        AZStd::vector<AZStd::shared_ptr<BuilderInfoMetricsItem>> m_singleBuilderMetrics;
        AZStd::unordered_map<AZStd::string, int> m_builderNameToIndex;
        AZStd::unordered_map<AZ::Uuid, int> m_builderGuidToIndex;

        //! This value, when being non-negative, refers to index of m_singleBuilderMetrics.
        //! When it is -1, currently selects m_allBuildersMetrics.
        //! When it is -2, it means BuilderInfoMetricsModel cannot find the selected builder in m_builderGuidToIndex.
        int m_currentSelectedBuilderIndex = -1;
    };
}

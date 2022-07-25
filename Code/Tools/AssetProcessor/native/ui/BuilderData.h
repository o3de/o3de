/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <native/assetprocessor.h>
#include <AZCore/std/containers/vector.h>
#include <AZCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <QObject>
#endif

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

    class BuilderData : public QObject
    {
        Q_OBJECT
    public:
        BuilderData(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> dbConnection, QObject* parent = nullptr);
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
    Q_SIGNALS:
        void DurationChanged(BuilderInfoMetricsItem* itemChanged);
    public Q_SLOTS:
        void OnProcessJobDurationChanged(JobEntry jobEntry, int value);
        void OnCreateJobsDurationChanged(QString sourceName);
    };
}

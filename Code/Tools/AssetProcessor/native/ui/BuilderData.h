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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
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
    class BuilderDataItem;

    //! BuilderData is a class that contains all jobs' metrics, categorized by builders. It is shared by BuilderInfoMetricsModel and
    //! BuilderListModel as the source of data.
    class BuilderData : public QObject
    {
        Q_OBJECT
    public:
        BuilderData(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> dbConnection, QObject* parent = nullptr);
        //! This method runs when this model is initialized. It gets the list of builders, gets existing stats about analysis jobs and
        //! processing jobs, and matches stats with builders and save them appropriately for future use.
        void Reset();

        enum class BuilderSelection: int
        {
            Invalid = -2,
            AllBuilders = -1,
            FirstBuilderIndex = 0
        };

        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_dbConnection;
        AZStd::shared_ptr<BuilderDataItem> m_root;
        AZStd::shared_ptr<BuilderDataItem> m_allBuildersMetrics;
        AZStd::vector<AZStd::shared_ptr<BuilderDataItem>> m_singleBuilderMetrics;
        AZStd::unordered_map<AZStd::string, int> m_builderNameToIndex;
        AZStd::unordered_map<AZ::Uuid, int> m_builderGuidToIndex;
        //! This value, when being non-negative, refers to index of m_singleBuilderMetrics.
        //! When it is BuilderSelection::AllBuilders, currently selects m_allBuildersMetrics.
        //! When it is BuilderSelection::Invalid, it means BuilderInfoMetricsModel cannot find the selected builder in m_builderGuidToIndex.
        int m_currentSelectedBuilderIndex = aznumeric_cast<int>(BuilderSelection::AllBuilders);
    Q_SIGNALS:
        void DurationChanged(BuilderDataItem* itemChanged);
    public Q_SLOTS:
        void OnProcessJobDurationChanged(JobEntry jobEntry, int value);
        void OnCreateJobsDurationChanged(QString sourceName);
    };
}

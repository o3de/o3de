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
        //! This method runs when this model is initialized. It gets the list of builders, gets existing stats about CreateJobs and
        //! ProcessJob, and matches stats with builders and save them appropriately for future use.
        void Reset();

        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_dbConnection;

        //! tree and index lookup hash tables
        //!   Tree Structure:
        //!   m_root (ItemType::InvisibleRoot)
        //!   +-- "All Builders" invisible root (ItemType::InvisibleRoot)
        //!   |   +-- "All Builders" (ItemType::Builder)
        //!   |       +-- "CreateJobs" (ItemType::TaskType)
        //!   |       |   +-- entry... (ItemType::Entry)
        //!   |       |   +-- entry... (ItemType::Entry)
        //!   |       +-- "ProcessJob" (ItemType::TaskType)
        //!   |           +-- entry... (ItemType::Entry)
        //!   |           +-- entry... (ItemType::Entry)
        //!   +-- "XXX Builder" invisible root (ItemType::InvisibleRoot)
        //!   |   +-- "XXX Builder" (ItemType::Builder)
        //!   |       +-- "CreateJobs" (ItemType::TaskType)
        //!   |       |   +-- entry... (ItemType::Entry)
        //!   |       |   +-- entry... (ItemType::Entry)
        //!   |       +-- "ProcessJob" (ItemType::TaskType)
        //!   |           +-- entry... (ItemType::Entry)
        //!   |           +-- entry... (ItemType::Entry)
        //!   ...
        //!   The "XXX builder" invisible root is served as the Qt TreeView root.
        AZStd::shared_ptr<BuilderDataItem> m_root;
        AZStd::unordered_map<AZStd::string, int> m_builderNameToIndex;
        AZStd::unordered_map<AZ::Uuid, int> m_builderGuidToIndex;
    Q_SIGNALS:
        void DurationChanged(BuilderDataItem* itemChanged);
    public Q_SLOTS:
        void OnProcessJobDurationChanged(JobEntry jobEntry, int value);
        void OnCreateJobsDurationChanged(QString sourceName, AZ::s64 scanFolderID);
    };
}

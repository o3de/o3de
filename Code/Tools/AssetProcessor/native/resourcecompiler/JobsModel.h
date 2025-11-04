/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <native/resourcecompiler/RCCommon.h>
#include <QAbstractItemModel>
#include <QDateTime>
#include <QHash>
#include <QIcon>
#include <QVector>

// Don't reorder above RCCommon
#include <native/assetprocessor.h>
#endif

// Do this here, rather than EditorAssetSystemAPI.h so that we don't have to link against Qt5Core to
// use EditorAssetSystemAPI.h
Q_DECLARE_METATYPE(AzToolsFramework::AssetSystem::JobStatus);

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class ProductDatabaseEntry;
        class AssetDatabaseConnection;
    }
}
namespace AssetProcessor
{
    //! CachedJobInfo stores all the necessary information needed for showing a particular job including its log
    struct CachedJobInfo
    {
        QueueElementID m_elementId;
        QDateTime m_completedTime;
        AzToolsFramework::AssetSystem::JobStatus m_jobState;
        AZ::u32 m_warningCount;
        AZ::u32 m_errorCount;
        unsigned int m_jobRunKey;
        AZ::Uuid m_builderGuid;
        QTime m_processDuration;
    };
    /**
    * The JobsModel class contains list of jobs from both the Database and the RCController
    */
    class JobsModel
        : public QAbstractItemModel
    {
        Q_OBJECT
    public:

        enum DataRoles
        {
            logRole = Qt::UserRole + 1,
            statusRole,
            logFileRole,
            SortRole,
        };

        enum Column
        {
            ColumnStatus,
            ColumnSource,
            ColumnCompleted,
            ColumnPlatform,
            ColumnJobKey,
            ColumnProcessDuration,
            Max
        };

        explicit JobsModel(QObject* parent = nullptr);
        virtual ~JobsModel();
        QModelIndex parent(const QModelIndex& index) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        int itemCount() const;
        CachedJobInfo* getItem(int index) const;
        static QString GetStatusInString(const AzToolsFramework::AssetSystem::JobStatus& state, AZ::u32 warningCount, AZ::u32 errorCount);
        void PopulateJobsFromDatabase();

        QModelIndex GetJobFromProduct(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry, AzToolsFramework::AssetDatabase::AssetDatabaseConnection& assetDatabaseConnection);
        QModelIndex GetJobFromSourceAndJobInfo(const SourceAssetReference& source, const AZStd::string& platform, const AZStd::string& jobKey);

public Q_SLOTS:
        void OnJobStatusChanged(JobEntry entry, AzToolsFramework::AssetSystem::JobStatus status);
        void OnJobProcessDurationChanged(JobEntry jobEntry, int durationMs);
        void OnJobRemoved(AzToolsFramework::AssetSystem::JobInfo jobInfo);
        void OnSourceRemoved(const SourceAssetReference& sourceAsset);

    protected:
        QIcon m_pendingIcon;
        QIcon m_errorIcon;
        QIcon m_failureIcon;
        QIcon m_warningIcon;
        QIcon m_okIcon;
        QIcon m_processingIcon;
        AZStd::vector<CachedJobInfo*> m_cachedJobs;
        QHash<AssetProcessor::QueueElementID, int> m_cachedJobsLookup; // QVector uses int as type of index.

        void RemoveJob(const AssetProcessor::QueueElementID& elementId);
    };

} //namespace AssetProcessor

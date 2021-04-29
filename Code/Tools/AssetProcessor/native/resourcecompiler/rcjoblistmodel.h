/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef RCQUEUEMODEL_H
#define RCQUEUEMODEL_H

#if !defined(Q_MOC_RUN)
#include "RCCommon.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QQueue>
#include <QMultiMap>

#include "rcjob.h"
#endif

namespace AssetProcessor
{
    class QueueElementID;
}

class RCcontrollerUnitTests;

namespace AssetProcessor
{
    /**
     * The RCJobListModel class contains lists of RC jobs
     */
    class RCJobListModel
        : public QAbstractItemModel
    {
        friend class ::RCcontrollerUnitTests;
        Q_OBJECT

    public:
        enum DataRoles
        {
            jobIndexRole = Qt::UserRole + 1,
            stateRole,
            displayNameRole,
            timeCreatedRole,
            timeLaunchedRole,
            timeCompletedRole,
            jobDataRole,
        };

        enum Column
        {
            ColumnState,
            ColumnJobId,
            ColumnCommand,
            ColumnCompleted,
            ColumnPlatform,
            Max
        };

        explicit RCJobListModel(QObject* parent = 0);


        QModelIndex parent(const QModelIndex&) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const;

        void markAsProcessing(RCJob* rcJob);
        void markAsStarted(RCJob* rcJob);
        void markAsCompleted(RCJob* rcJob);
        void markAsCataloged(const AssetProcessor::QueueElementID& check);
        unsigned int jobsInFlight() const;

        void UpdateJobEscalation(AssetProcessor::RCJob* rcJob, int jobPrioririty);
        void UpdateRow(int jobIndex);

        bool isEmpty();
        void addNewJob(RCJob* rcJob);

        bool isInFlight(const QueueElementID& check) const;
        bool isInQueue(const QueueElementID& check) const;
        bool isWaitingOnCatalog(const QueueElementID& check) const;

        void PerformHeuristicSearch(QString searchTerm, QString platform, QSet<QueueElementID>& found, AssetProcessor::JobIdEscalationList& escalationList, bool isStatusRequest, int searchRules = 0);
        void PerformUUIDSearch(AZ::Uuid searchUuid, QString platform, QSet<QueueElementID>& found, AssetProcessor::JobIdEscalationList& escalationList, bool isStatusRequest);

        int itemCount() const;
        RCJob* getItem(int index) const;
        int GetIndexOfProcessingJob(const QueueElementID& elementId);

        ///! EraseJobs expects the database name of the source file.
        void EraseJobs(QString sourceFileDatabaseName, AZStd::vector<RCJob*>& pendingJobs);

    private:

        AZStd::vector<RCJob*> m_jobs;
        QSet<RCJob*> m_jobsInFlight;

        // Keeps track of jobs waiting on the APM thread to finish writing out to the catalog
        // This prevents job dependencies from starting before the dependent job is actually done
        // Since the jobs aren't uniquely identified, and the APM thread can fall behind, we keep track of how many have finished
        QHash<QueueElementID, int> m_finishedJobsNotInCatalog;

        // profiler showed much of our time was spent in IsInQueue.
        QMultiMap<QueueElementID, RCJob*> m_jobsInQueueLookup;
    };
} // namespace AssetProcessor


#endif // RCQUEUEMODEL_H

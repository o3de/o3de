/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETPROCESSOR_RCQUEUESORTMODEL_H
#define ASSETPROCESSOR_RCQUEUESORTMODEL_H

#if !defined(Q_MOC_RUN)
#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>


#include "native/utilities/AssetUtilEBusHelper.h"
#include <AzCore/std/containers/unordered_map.h>
#include "native/assetprocessor.h"
#endif

class RCcontrollerUnitTests;

namespace AssetProcessor
{
    class QueueElementID;
    class RCJobListModel;
    class RCJob;

    //! This sort and filtering proxy model attaches to the raw RC job list
    //! And presents it in the optimal order for processing rather than display.
    //! The current desired order is
    //!  * Critical (currently Copy) jobs for currently connected platforms
    //!  * Jobs in Sync Compile Requests for currently connected platforms (with most recent requests first)
    //!  * Jobs in Async Compile Lists for currently connected platforms
    //!  * Remaining jobs in currently connected platforms, in priority order
    //!  (The same, repeated, for unconnected platforms).
    class RCQueueSortModel
        : public QSortFilterProxyModel
        , protected AssetProcessorPlatformBus::Handler
    {
        Q_OBJECT
        friend class ::RCcontrollerUnitTests;
    public:
        explicit RCQueueSortModel(QObject* parent = 0);

        void AttachToModel(RCJobListModel* target);
        RCJob* GetNextPendingJob();

        void AddJobIdEntry(AssetProcessor::RCJob* rcJob);
        void RemoveJobIdEntry(AssetProcessor::RCJob* rcJob);

        void SetQueueSortOnDBSourceName()
        {
            m_sortQueueOnDBSourceName = true;
        }

        // implement QSortFilteRProxyModel:
        bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    public Q_SLOTS:
        void OnEscalateJobs(AssetProcessor::JobIdEscalationList jobIdEscalationList);


    protected:

        typedef AZStd::unordered_map<AZ::s64, AssetProcessor::RCJob*> JobRunKeyToRCJobMap;

        JobRunKeyToRCJobMap m_currentJobRunKeyToJobEntries;

        QSet<QString> m_currentlyConnectedPlatforms;
        bool m_dirtyNeedsResort = false; // instead of constantly resorting, we resort only when someone wants to pull an element from us

        // By default, jobs with equal priority and escalation sort on the job run key. This flag changes
        // jobs to sort on the database source name. This is used for testing, to guarantee jobs run in the same
        // order for those tests each time they are run.
        bool m_sortQueueOnDBSourceName = false;

        // ---------------------------------------------------------
        // AssetProcessorPlatformBus::Handler
        void AssetProcessorPlatformConnected(const AZStd::string platform) override;
        void AssetProcessorPlatformDisconnected(const AZStd::string platform) override;
        // -----------

        RCJobListModel* m_sourceModel;
    private Q_SLOTS:

        void ProcessPlatformChangeMessage(QString platformName, bool connected);
    };
} // namespace AssetProcessor

#endif //ASSETPROCESSOR_RCQUEUESORTMODEL_H

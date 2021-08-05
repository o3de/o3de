/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef RCCONTROLLER_H
#define RCCONTROLLER_H

#if !defined(Q_MOC_RUN)
#include "RCCommon.h"

#include <QObject>
#include <QProcess>
#include <QDir>
#include <QList>
#include "native/utilities/AssetUtilEBusHelper.h"

#include "rcjoblistmodel.h"
#include "RCQueueSortModel.h"

#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#endif

class RCcontrollerUnitTests;

namespace AssetProcessor
{
    /**
     * The RCController class controls the receiving of job requests, adding them to the model,
     * running RC and sending of responses
     */
    class RCController
        : public QObject
        , public AssetProcessorPlatformBus::Handler
    {
        friend class ::RCcontrollerUnitTests;
        Q_OBJECT
    public:
        enum CommandType
        {
            cmdUnknown = 0,
            cmdExecute,
            cmdTerminate
        };
        RCController() = default;
        explicit RCController(int minJobs, int maxJobs, QObject* parent = 0);
        virtual ~RCController();

        AssetProcessor::RCJobListModel* GetQueueModel();

        void StartJob(AssetProcessor::RCJob* rcJob);
        int NumberOfPendingCriticalJobsPerPlatform(QString platform);

        int NumberOfPendingJobsPerPlatform(QString platform);
        bool IsIdle();

        void SetQueueSortOnDBSourceName();

    Q_SIGNALS:
        void FileCompiled(JobEntry entry, AssetBuilderSDK::ProcessJobResponse response);
        void FileFailed(JobEntry entry);
        void FileCancelled(JobEntry entry);
        //void AssetStatus(JobEntry jobEntry, AzFramework::AssetProcessor::AssetStatus status);
        void RcError(QString error);
        void ReadyToQuit(QObject* source); //After receiving QuitRequested, you must send this when its safe

        ///! JobStarted will notify with a path name relative to the watch folder it was found in (not the database sourcename column)
        void JobStarted(QString inputFile, QString platform);
        void JobStatusChanged(JobEntry entry, AzToolsFramework::AssetSystem::JobStatus status);
        void JobsInQueuePerPlatform(QString platform, int jobs);
        void ActiveJobsCountChanged(unsigned int jobs); // This is the count of jobs which are either queued or inflight 

        void BecameIdle();

        //! This will be signalled upon compile group creation - or failure to do so (in which case status will be unknown)
        void CompileGroupCreated(AssetProcessor::NetworkRequestID groupID, AzFramework::AssetSystem::AssetStatus status);
        //! Once a compile group has an error or finished, this will be invoked.
        void CompileGroupFinished(AssetProcessor::NetworkRequestID groupID, AzFramework::AssetSystem::AssetStatus status);

        void EscalateJobs(AssetProcessor::JobIdEscalationList jobIdEscalationList);

    public Q_SLOTS:
        void JobSubmitted(JobDetails details);
        void QuitRequested();

        //! This will be called in order to create a compile group and start tracking it.
        void OnRequestCompileGroup(AssetProcessor::NetworkRequestID groupID, QString platform, QString searchTerm, AZ::Data::AssetId assetId, bool isStatusRequest = true, int searchType = 0);

        void OnEscalateJobsBySearchTerm(QString platform, QString searchTerm);
        void OnEscalateJobsBySourceUUID(QString platform, AZ::Uuid sourceUuid);

        void DispatchJobs();
        void DispatchJobsImpl();

        //! Pause or unpause dispatching, only necessary on startup to avoid thrashing and make sure no jobs jump the gun.
        void SetDispatchPaused(bool pause);

        //! All jobs which match this source will be cancelled or removed.  Note that relSourceFile should have any applicable output prefixes!
        void RemoveJobsBySource(QString relSourceFileDatabaseName);

        // when the AP is truly done with a particular job and its going to be deleted and nothing more cares about it,
        // this function is called. this allows us to synchronize the various threads (catalog, queue, etc) to know that
        // its completely done.
        void OnJobComplete(JobEntry completeEntry, AzToolsFramework::AssetSystem::JobStatus status);
        void OnAddedToCatalog(JobEntry jobEntry);

        //! See AssetSystemRequestBus::AppendAssetsToPrioritySet for details.
        void OnAppendAssetsToPrioritySet(
            AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> databaseConnection, QString prioritySetName,
            AZStd::vector<AZ::Uuid> assetList, uint32_t priorityBoost);
        void OnRemoveAssetsFromPrioritySet(
            AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> databaseConnection, QString prioritySetName,
            AZStd::vector<AZ::Uuid> assetList);

    private:
        void FinishJob(AssetProcessor::RCJob* rcJob);

        //! Returns 0 if the given job is NOT for the current host platform, or the asset is not present in any
        //! of the priority sets.
        //! Otherwise returns the priority escalation value.
        int GetJobAssetPrioritySetEscalation(const RCJob& rcJob) const;

        unsigned int m_maxJobs;

        bool m_dispatchingJobs = false;
        bool m_shuttingDown = false;
        bool m_dispatchingPaused = true;// dispatching starts out paused.
        bool m_dispatchJobsQueued = false;

        QMap<QString, int> m_jobsCountPerPlatform;// This stores the count of jobs per platform in the RC Queue
        QMap<QString, int> m_pendingCriticalJobsPerPlatform;// This stores the count of pending critical jobs per platform in the RC Queue

        struct PriorityRefCount
        {
            uint32_t m_priority; // A value from AssetProcessor::PriotitySetEscalation up to
                                 // AssetProcessor::ProcessAssetRequestSyncEscalation - 1
            int32_t m_refCount;
        };
        // The key is the name of the priority set, the value is another map, where:
        //     Key is the Uuid of an asset
        //     Value is a tuple with the priority and the ref count of how many times such asset has been requested to be appended to the
        //     working set.
        QMap<QString, QMap<AZ::Uuid, PriorityRefCount>> m_prioritySets; 

        AssetProcessor::RCJobListModel m_RCJobListModel;
        AssetProcessor::RCQueueSortModel m_RCQueueSortModel;

        //! An Asset Compile Group is a set of assets that we're tracking the compilation of
        //! It consists of a whole bunch of assets and is considered to be "complete" when either one of the assets in the group fails
        //! Or all assets in the group have finished.
        class AssetCompileGroup
        {
        public:
            AssetProcessor::NetworkRequestID m_requestID;
            QSet<AssetProcessor::QueueElementID> m_groupMembers;
        };

        QList<AssetCompileGroup> m_activeCompileGroups;
        
    };
} // namespace AssetProcessor


#endif // RCCONTROLLER_H

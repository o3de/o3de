/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "rccontroller.h"
#include <native/resourcecompiler/RCCommon.h>
#include <QTimer>
#include <QThreadPool>



namespace AssetProcessor
{
    RCController::RCController(int cfg_minJobs, int cfg_maxJobs, QObject* parent)
        : QObject(parent)
        , m_dispatchingJobs(false)
        , m_shuttingDown(false)
    {
        AssetProcessorPlatformBus::Handler::BusConnect();

        // Determine a good starting value for max jobs
        int maxJobs = QThread::idealThreadCount();
        if (maxJobs == -1)
        {
            maxJobs = 3;
        }

        maxJobs = qMax<int>(maxJobs - 1, 1);

        // if the user has specified max jobs in the cfg file, then we obey their request
        // regardless of whether they have chosen something bad or not - they would have had to explicitly
        // pick this value (we ship with default 0 meaning auto), so if they've changed it, they intend it that way
        m_maxJobs = cfg_maxJobs ? qMax(cfg_minJobs, cfg_maxJobs) :  maxJobs;

        m_RCQueueSortModel.AttachToModel(&m_RCJobListModel);

        // make sure that the global thread pool has enough slots to accomidate your request though, since
        // by default, the global thread pool has idealThreadCount() slots only.
        // leave an extra slot for non-job work.
        int currentMaxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
        int newMaxThreadCount = qMax<int>(currentMaxThreadCount, m_maxJobs + 1);
        QThreadPool::globalInstance()->setMaxThreadCount(newMaxThreadCount);

        QObject::connect(this, &RCController::EscalateJobs, &m_RCQueueSortModel, &AssetProcessor::RCQueueSortModel::OnEscalateJobs);
    }

    RCController::~RCController()
    {
        AssetProcessorPlatformBus::Handler::BusDisconnect();
        m_RCQueueSortModel.AttachToModel(nullptr);
    }

    RCJobListModel* RCController::GetQueueModel()
    {
        return &m_RCJobListModel;
    }

    void RCController::StartJob(RCJob* rcJob)
    {
        Q_ASSERT(rcJob);
        // request to be notified when job is done
        QObject::connect(rcJob, &RCJob::Finished, this, [this, rcJob]()
        {
            FinishJob(rcJob);
        }, Qt::QueuedConnection);

        // Mark as "being processed" by moving to Processing list
        m_RCJobListModel.markAsProcessing(rcJob);
        m_RCJobListModel.markAsStarted(rcJob);
        Q_EMIT JobStatusChanged(rcJob->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::InProgress);
        rcJob->Start();
        Q_EMIT JobStarted(rcJob->GetJobEntry().m_sourceAssetReference.RelativePath().c_str(), QString::fromUtf8(rcJob->GetPlatformInfo().m_identifier.c_str()));
    }

    void RCController::QuitRequested()
    {
        m_shuttingDown = true;

        // cancel all jobs:
        AssetBuilderSDK::JobCommandBus::Broadcast(&AssetBuilderSDK::JobCommandBus::Events::Cancel);

        if (m_RCJobListModel.jobsInFlight() == 0)
        {
            Q_EMIT ReadyToQuit(this);
            return;
        }

        QTimer::singleShot(10, this, SLOT(QuitRequested()));
    }

    int RCController::NumberOfPendingCriticalJobsPerPlatform(QString platform)
    {
        return m_pendingCriticalJobsPerPlatform[platform.toLower()];
    }

    int RCController::NumberOfPendingJobsPerPlatform(QString platform)
    {
        return m_jobsCountPerPlatform[platform.toLower()];
    }

    void RCController::FinishJob(RCJob* rcJob)
    {
        m_RCQueueSortModel.RemoveJobIdEntry(rcJob);
        QString platform = rcJob->GetPlatformInfo().m_identifier.c_str();
        auto found = m_jobsCountPerPlatform.find(platform);
        if (found != m_jobsCountPerPlatform.end())
        {
            int prevCount = found.value();
            if (prevCount > 0)
            {
                int newCount = prevCount - 1;
                m_jobsCountPerPlatform[platform] = newCount;
                Q_EMIT JobsInQueuePerPlatform(platform, newCount);
            }
        }

        if (rcJob->IsCritical())
        {
            int criticalJobsCount = m_pendingCriticalJobsPerPlatform[platform.toLower()] - 1;
            m_pendingCriticalJobsPerPlatform[platform.toLower()] = criticalJobsCount;
        }

        if (rcJob->GetState() == RCJob::cancelled)
        {
            Q_EMIT FileCancelled(rcJob->GetJobEntry());
        }
        else if (rcJob->GetState() != RCJob::completed)
        {
            Q_EMIT FileFailed(rcJob->GetJobEntry());
            Q_EMIT JobStatusChanged(rcJob->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Failed);
        }
        else
        {
            Q_EMIT FileCompiled(rcJob->GetJobEntry(), AZStd::move(rcJob->GetProcessJobResponse()));
            Q_EMIT JobStatusChanged(rcJob->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Completed);
        }

        // Move to Completed list which will mark as "completed"
        // unless a different state has been set.
        m_RCJobListModel.markAsCompleted(rcJob);

        if (!m_dispatchingPaused)
        {
            Q_EMIT ActiveJobsCountChanged(aznumeric_cast<unsigned int>(m_RCJobListModel.itemCount()));
        }

        if (!m_shuttingDown)
        {
            // Start next job only if we are not shutting down
            DispatchJobs();

            // if there is no next job, and nothing is in flight, we are done.
            if (IsIdle())
            {
                Q_EMIT BecameIdle();
            }
        }
    }

    bool RCController::IsIdle()
    {
        return ((!m_RCQueueSortModel.GetNextPendingJob()) && (m_RCJobListModel.jobsInFlight() == 0));
    }

    void RCController::JobSubmitted(JobDetails details)
    {
        AssetProcessor::QueueElementID checkFile(details.m_jobEntry.m_sourceAssetReference, details.m_jobEntry.m_platformInfo.m_identifier.c_str(), details.m_jobEntry.m_jobKey);

        if (m_RCJobListModel.isInQueue(checkFile))
        {
            AZ_TracePrintf(
                AssetProcessor::DebugChannel,
                "Job is already in queue and has not started yet - ignored [%s, %s, %s]\n",
                checkFile.GetSourceAssetReference().AbsolutePath().c_str(),
                checkFile.GetPlatform().toUtf8().data(),
                checkFile.GetJobDescriptor().toUtf8().data());

            // Don't just discard the job, we need to let APM know so it can keep track of the number of jobs that are pending/finished
            AssetBuilderSDK::JobCommandBus::Event(details.m_jobEntry.m_jobRunKey, &AssetBuilderSDK::JobCommandBus::Events::Cancel);
            Q_EMIT FileCancelled(details.m_jobEntry);
            return;
        }

        if (m_RCJobListModel.isInFlight(checkFile))
        {
            // if the computed fingerprint is the same as the fingerprint of the in-flight job, this is okay.
            int existingJobIndex = m_RCJobListModel.GetIndexOfProcessingJob(checkFile);
            if (existingJobIndex != -1)
            {
                RCJob* job = m_RCJobListModel.getItem(existingJobIndex);
                bool cancelJob = false;

                if (job->GetJobEntry().m_computedFingerprint != details.m_jobEntry.m_computedFingerprint)
                {
                    AZ_TracePrintf(
                        AssetProcessor::DebugChannel,
                        "Cancelling Job [%s, %s, %s] with old FP %u, replacing with new FP %u \n",
                        checkFile.GetSourceAssetReference().AbsolutePath().c_str(),
                        checkFile.GetPlatform().toUtf8().data(),
                        checkFile.GetJobDescriptor().toUtf8().data(),
                        job->GetJobEntry().m_computedFingerprint,
                        details.m_jobEntry.m_computedFingerprint);
                    cancelJob = true;
                }
                else if(!job->GetJobDependencies().empty())
                {
                    // If a job has dependencies, it's very likely it was re-queued as a result of a dependency being changed
                    // The in-flight job is probably going to fail at best, or use old data at worst, so cancel the in-flight job

                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Cancelling Job with dependencies [%s, %s, %s], replacing with re-queued job\n",
                        checkFile.GetSourceAssetReference().AbsolutePath().c_str(), checkFile.GetPlatform().toUtf8().data(), checkFile.GetJobDescriptor().toUtf8().data());
                    cancelJob = true;
                }
                else
                {
                    AZ_TracePrintf(
                        AssetProcessor::DebugChannel,
                        "Job is already in progress but has the same computed fingerprint (%u) - ignored [%s, %s, %s]\n",
                        details.m_jobEntry.m_computedFingerprint,
                        checkFile.GetSourceAssetReference().AbsolutePath().c_str(),
                        checkFile.GetPlatform().toUtf8().data(),
                        checkFile.GetJobDescriptor().toUtf8().data());

                    // Don't just discard the job, we need to let APM know so it can keep track of the number of jobs that are
                    // pending/finished
                    AssetBuilderSDK::JobCommandBus::Event(details.m_jobEntry.m_jobRunKey, &AssetBuilderSDK::JobCommandBus::Events::Cancel);
                    Q_EMIT FileCancelled(details.m_jobEntry);
                    return;
                }

                if(cancelJob)
                {
                    job->SetState(RCJob::JobState::cancelled);
                    AssetBuilderSDK::JobCommandBus::Event(job->GetJobEntry().m_jobRunKey, &AssetBuilderSDK::JobCommandBus::Events::Cancel);
                    m_RCJobListModel.UpdateRow(existingJobIndex);
                }
            }
        }

        RCJob* rcJob = new RCJob(&m_RCJobListModel);
        rcJob->Init(details); // note - move operation.  From this point on you must use the job details to refer to it.
        m_RCQueueSortModel.AddJobIdEntry(rcJob);
        m_RCJobListModel.addNewJob(rcJob);
        QString platformName = rcJob->GetPlatformInfo().m_identifier.c_str();// we need to get the actual platform from the rcJob
        if (rcJob->IsCritical())
        {
            int criticalJobsCount = m_pendingCriticalJobsPerPlatform[platformName.toLower()] + 1;
            m_pendingCriticalJobsPerPlatform[platformName.toLower()] = criticalJobsCount;
        }
        auto found = m_jobsCountPerPlatform.find(platformName);
        if (found != m_jobsCountPerPlatform.end())
        {
            int newCount = found.value() + 1;
            m_jobsCountPerPlatform[platformName] = newCount;
        }
        else
        {
            m_jobsCountPerPlatform[platformName] = 1;
        }
        Q_EMIT JobsInQueuePerPlatform(platformName, m_jobsCountPerPlatform[platformName]);
        Q_EMIT JobStatusChanged(rcJob->GetJobEntry(), AzToolsFramework::AssetSystem::JobStatus::Queued);

        if (!m_dispatchingPaused)
        {
            Q_EMIT ActiveJobsCountChanged(aznumeric_cast<unsigned int>(m_RCJobListModel.itemCount()));
        }

        // Start the job we just received if no job currently running
        if ((!m_shuttingDown) && (!m_dispatchingJobs))
        {
            DispatchJobs();
        }
    }

    void RCController::SetDispatchPaused(bool pause)
    {
        if (m_dispatchingPaused != pause)
        {
            m_dispatchingPaused = pause;
            if (!pause)
            {
                if ((!m_shuttingDown) && (!m_dispatchingJobs))
                {
                    DispatchJobs();
                    Q_EMIT ActiveJobsCountChanged(aznumeric_cast<unsigned int>(m_RCJobListModel.itemCount()));
                }
            }
        }
    }

    void RCController::DispatchJobsImpl()
    {
        m_dispatchJobsQueued = false;
        if (!m_dispatchingJobs)
        {
            m_dispatchingJobs = true;
            RCJob* rcJob = m_RCQueueSortModel.GetNextPendingJob();

            while (m_RCJobListModel.jobsInFlight() < m_maxJobs && rcJob && !m_shuttingDown)
            {
                if (m_dispatchingPaused)
                {
                    // note, even if dispatching is "paused" we start all "auto fail jobs" so that user gets instant feedback on failure.
                    if (!rcJob->IsAutoFail())
                    {
                        break;
                    }
                }

                StartJob(rcJob);
                rcJob = m_RCQueueSortModel.GetNextPendingJob();
            }
            m_dispatchingJobs = false;
        }
    }
    void RCController::DispatchJobs()
    {
        if (!m_dispatchJobsQueued)
        {
            m_dispatchJobsQueued = true;
            QMetaObject::invokeMethod(this, "DispatchJobsImpl", Qt::QueuedConnection);
        }
    }

    void RCController::OnRequestCompileGroup(AssetProcessor::NetworkRequestID groupID, QString platform, QString searchTerm, AZ::Data::AssetId assetId, bool isStatusRequest, int searchType)
    {
        // someone has asked for a compile group to be created that conforms to that search term.
        // the goal here is to use a heuristic to find any assets that match the search term and place them in a new group
        // then respond with the appropriate response.

        // lets do some minimal processing on the search term
        AssetProcessor::JobIdEscalationList escalationList;
        QSet<AssetProcessor::QueueElementID> results;

        if (assetId.IsValid())
        {
            m_RCJobListModel.PerformUUIDSearch(assetId.m_guid, platform, results, escalationList, isStatusRequest);
        }
        else
        {
            m_RCJobListModel.PerformHeuristicSearch(AssetUtilities::NormalizeAndRemoveAlias(searchTerm), platform, results, escalationList, isStatusRequest, searchType);
        }

        if (results.isEmpty())
        {
            // nothing found
            Q_EMIT CompileGroupCreated(groupID, AzFramework::AssetSystem::AssetStatus_Unknown);

            AZ_TracePrintf(AssetProcessor::DebugChannel, "OnRequestCompileGroup:  %s - %s requested, but no matching source assets found.\n", searchTerm.toUtf8().constData(), assetId.ToString<AZStd::string>().c_str());
        }
        else
        {
            // it is not necessary to denote the search terms or list of results here because
            // PerformHeursticSearch already prints out the results.
            m_RCQueueSortModel.OnEscalateJobs(escalationList);

            m_activeCompileGroups.push_back(AssetCompileGroup());
            m_activeCompileGroups.back().m_groupMembers.swap(results);
            m_activeCompileGroups.back().m_requestID = groupID;

            Q_EMIT CompileGroupCreated(groupID, AzFramework::AssetSystem::AssetStatus_Queued);
        }
    }

    void RCController::OnEscalateJobsBySearchTerm(QString platform, QString searchTerm)
    {
        AssetProcessor::JobIdEscalationList escalationList;
        QSet<AssetProcessor::QueueElementID> results;
        m_RCJobListModel.PerformHeuristicSearch(AssetUtilities::NormalizeAndRemoveAlias(searchTerm), platform, results, escalationList, true);

        if (!results.isEmpty())
        {
            // it is not necessary to denote the search terms or list of results here because
            // PerformHeursticSearch already prints out the results.
            m_RCQueueSortModel.OnEscalateJobs(escalationList);
        }
        // do not print a warning out when this fails, its fine for things to escalate jobs as a matter of course just to "make sure" they are escalated
        // and its fine if none are in the build queue.
    }

    void RCController::OnEscalateJobsBySourceUUID(QString platform, AZ::Uuid sourceUuid)
    {
        AssetProcessor::JobIdEscalationList escalationList;
        QSet<AssetProcessor::QueueElementID> results;
        m_RCJobListModel.PerformUUIDSearch(sourceUuid, platform, results, escalationList, true);

        if (!results.isEmpty())
        {
#if defined(AZ_ENABLE_TRACING)
            for (const AssetProcessor::QueueElementID& result : results)
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "OnEscalateJobsBySourceUUID:  %s --> %s\n", sourceUuid.ToString<AZStd::string>().c_str(), result.GetSourceAssetReference().AbsolutePath().c_str());
            }
#endif
            m_RCQueueSortModel.OnEscalateJobs(escalationList);
        }
        // do not print a warning out when this fails, its fine for things to escalate jobs as a matter of course just to "make sure" they are escalated
        // and its fine if none are in the build queue.
    }

    void RCController::OnJobComplete(JobEntry completeEntry, AzToolsFramework::AssetSystem::JobStatus state)
    {
        if (m_activeCompileGroups.empty())
        {
            return;
        }

        QueueElementID jobQueueId(completeEntry.m_sourceAssetReference, completeEntry.m_platformInfo.m_identifier.c_str(), completeEntry.m_jobKey);

        // only the 'completed' status means success:
        bool statusSucceeded = (state == AzToolsFramework::AssetSystem::JobStatus::Completed);

        // start at the end so that we can actually erase the compile groups and not skip any:
        for (int groupIdx = m_activeCompileGroups.size() - 1; groupIdx >= 0; --groupIdx)
        {
            AssetCompileGroup& compileGroup = m_activeCompileGroups[groupIdx];
            auto it = compileGroup.m_groupMembers.find(jobQueueId);
            if (it != compileGroup.m_groupMembers.end())
            {
                compileGroup.m_groupMembers.erase(it);
                if ((compileGroup.m_groupMembers.isEmpty()) || (!statusSucceeded))
                {
                    // if we get here, we're either empty (and succeeded) or we failed one and have now failed
                    Q_EMIT CompileGroupFinished(compileGroup.m_requestID, statusSucceeded ? AzFramework::AssetSystem::AssetStatus_Compiled: AzFramework::AssetSystem::AssetStatus_Failed);
                    m_activeCompileGroups.removeAt(groupIdx);
                }
            }
        }
    }

    void RCController::RemoveJobsBySource(const SourceAssetReference& sourceAsset)
    {
        // some jobs may have not been started yet, these need to be removed manually
        AZStd::vector<RCJob*> pendingJobs;

        m_RCJobListModel.EraseJobs(sourceAsset, pendingJobs);

        // force finish all pending jobs
        for (auto* rcJob : pendingJobs)
        {
            FinishJob(rcJob);
        }
    }

    void RCController::OnAddedToCatalog(JobEntry jobEntry)
    {
        AssetProcessor::QueueElementID checkFile(jobEntry.m_sourceAssetReference, jobEntry.m_platformInfo.m_identifier.c_str(), jobEntry.m_jobKey);

        m_RCJobListModel.markAsCataloged(checkFile);

        DispatchJobs();
    }

} // Namespace AssetProcessor


/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/resourcecompiler/RCQueueSortModel.h>
#include <native/AssetDatabase/AssetDatabase.h>
#include "rcjoblistmodel.h"

namespace AssetProcessor
{
    RCQueueSortModel::RCQueueSortModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        // jobs assigned to "all" platforms are always active.
        m_currentlyConnectedPlatforms.insert(QString("all"));
    }

    void RCQueueSortModel::AttachToModel(RCJobListModel* target)
    {
        if (target)
        {
            setDynamicSortFilter(true);
            BusConnect();
            m_sourceModel = target;

            setSourceModel(target);
            setSortRole(RCJobListModel::jobIndexRole);
            sort(0);
        }
        else
        {
            BusDisconnect();
            setSourceModel(nullptr);
            m_sourceModel = nullptr;
        }
    }

    RCJob* RCQueueSortModel::GetNextPendingJob()
    {
        if (m_dirtyNeedsResort)
        {
            setDynamicSortFilter(false);
            QSortFilterProxyModel::sort(0);
            setDynamicSortFilter(true);
            m_dirtyNeedsResort = false;
        }
        RCJob* anyPendingJob = nullptr;
        bool waitingOnCatalog = false; // If we find an asset thats waiting on the catalog, don't assume there's a cyclic dependency.  We'll wait until the catalog is updated and then check again.

        for (int idx = 0; idx < rowCount(); ++idx)
        {
            QModelIndex parentIndex = mapToSource(index(idx, 0));
            RCJob* actualJob = m_sourceModel->getItem(parentIndex.row());
            if ((actualJob) && (actualJob->GetState() == RCJob::pending))
            {
                // If this job has a missing dependency, and there are any jobs in flight,
                // don't queue it until those jobs finish, in case they resolve the dependency.
                // This does mean that if there are multiple queued jobs with missing dependencies,
                // they'll run one at a time instead of in parallel, while waiting for the missing dependency
                // to be potentially resolved.
                if (actualJob->HasMissingSourceDependency() &&
                    (m_sourceModel->jobsInFlight() > 0 || m_sourceModel->jobsInQueueWithoutMissingDependencies() > 0 ||
                     m_sourceModel->jobsPendingCatalog() > 0))
                {
                    // There is a race condition where this can fail:
                    // Asset A generates an intermediate asset.
                    // Asset B has a source dependency on that intermediate asset. Asset B's "HasMissingSourceDependency" flag is true.
                    // Asset A is the last job in the queue without a missing job/source dependency, so it runs.
                    // Asset A finishes processing job, and outputs the product.
                    // Intermediate A has not yet been scanned and discovered by Asset Processor, so it's not in flight or in the queue yet.
                    // Asset Processor goes to pull the next job in the queue. Asset B still technically has a missing job dependency on the intermediate asset output.
                    // Asset B gets pulled from the queue to process here, even though Intermediate A hasn't run yet, because Intermediate A hasn't gone into the queue yet.
                    // This happened with FBX files and a dependency on an intermediate asset materialtype, before Common platform jobs were made higher priority than host platform jobs.
                    // Why not just check if the target file exists at this point? Because the job key has to match up.
                    // When a check was added here to see if all the dependency files existed, other material jobs started to end up in the queue indefinitely
                    // because they had a dependency on a file that existed but a job key that did not exist for that file.
                    continue;
                }

                if (actualJob->HasMissingSourceDependency())
                {
                    AZ_Warning(
                        AssetProcessor::ConsoleChannel,
                        false,
                        "No job was found to match the job dependency criteria declared by file %s.\n"
                        "This may be due to a mismatched job key.\n"
                        "Job ordering will not be guaranteed and could result in errors or unexpected output.",
                        actualJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData());
                }
                bool canProcessJob = true;
                for (const JobDependencyInternal& jobDependencyInternal : actualJob->GetJobDependencies())
                {
                    if (jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::Order ||
                        jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::OrderOnce ||
                        jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::OrderOnly)
                    {
                        const AssetBuilderSDK::JobDependency& jobDependency = jobDependencyInternal.m_jobDependency;
                        AZ_Assert(
                            AZ::IO::PathView(jobDependency.m_sourceFile.m_sourceFileDependencyPath).IsAbsolute(),
                            "Dependency path %s is not an absolute path",
                            jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str());
                        QueueElementID elementId(
                            SourceAssetReference(jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str()),
                            jobDependency.m_platformIdentifier.c_str(),
                            jobDependency.m_jobKey.c_str());

                        if (m_sourceModel->isInFlight(elementId) || m_sourceModel->isInQueue(elementId))
                        {
                            canProcessJob = false;
                            if (!anyPendingJob || (anyPendingJob->HasMissingSourceDependency() && !actualJob->HasMissingSourceDependency()))
                            {
                                anyPendingJob = actualJob;
                            }
                        }
                        else if(m_sourceModel->isWaitingOnCatalog(elementId))
                        {
                            canProcessJob = false;
                            waitingOnCatalog = true;
                        }
                    }
                }

                if (canProcessJob)
                {
                    return actualJob;
                }
            }
        }

        // Either there are no jobs to do or there is a cyclic order job dependency.
        if (anyPendingJob && m_sourceModel->jobsInFlight() == 0 && !waitingOnCatalog)
        {
            AZ_Warning(AssetProcessor::DebugChannel, false, " Cyclic job order dependency detected. Processing job (%s, %s, %s, %s) to unblock.",
                anyPendingJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(), anyPendingJob->GetJobKey().toUtf8().data(),
                anyPendingJob->GetJobEntry().m_platformInfo.m_identifier.c_str(), anyPendingJob->GetBuilderGuid().ToString<AZStd::string>().c_str());
            return anyPendingJob;
        }
        else
        {
            return nullptr;
        }
    }

    bool RCQueueSortModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        (void)source_parent;
        RCJob* actualJob = m_sourceModel->getItem(source_row);
        if (!actualJob)
        {
            return false;
        }

        if (actualJob->GetState() != RCJob::pending)
        {
            return false;
        }

        return true;
    }

    bool RCQueueSortModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        RCJob* leftJob = m_sourceModel->getItem(left.row());
        RCJob* rightJob = m_sourceModel->getItem(right.row());

        // auto fail jobs always take priority to give user feedback asap.
        bool autoFailLeft = leftJob->IsAutoFail();
        bool autoFailRight = rightJob->IsAutoFail();
        if (autoFailLeft)
        {
            if (!autoFailRight)
            {
                return true; // left before right
            }
        }
        else if (autoFailRight)
        {
            return false; // right before left.
        }

        // Check if either job was marked as having a missing source dependency.
        // This means that the job is looking for another source asset and job to exist before it runs, but
        // that source doesn't exist yet. Those jobs are deferred to run later, in case the dependency eventually shows up.
        // The dependency may be on an intermediate asset that will be generated later in asset processing.
        if (leftJob->HasMissingSourceDependency() != rightJob->HasMissingSourceDependency())
        {
            if (rightJob->HasMissingSourceDependency())
            {
                return true; // left does not have a missing source dependency, but right does, so left wins.
            }
            return false; // Right does not have a missing source dependency, but left does, so right wins.
        }

        // Common platform jobs generate intermediate assets. These generate additional jobs, and the intermediate assets
        // can be source and/or job dependencies for other, queued assets.
        // Run intermediate assets before active platform and host platform jobs.
        // Critical jobs should run first, so skip the comparison here if either job is critical, to allow criticality to come in first.
        bool platformsMatch = leftJob->GetPlatformInfo().m_identifier == rightJob->GetPlatformInfo().m_identifier;

        // first thing to check is in platform.
        if (!platformsMatch)
        {
            if (leftJob->GetPlatformInfo().m_identifier == AssetBuilderSDK::CommonPlatformName)
            {
                return true;
            }
            if (rightJob->GetPlatformInfo().m_identifier == AssetBuilderSDK::CommonPlatformName)
            {
                return false;
            }

            bool leftActive = m_currentlyConnectedPlatforms.contains(leftJob->GetPlatformInfo().m_identifier.c_str());
            bool rightActive = m_currentlyConnectedPlatforms.contains(rightJob->GetPlatformInfo().m_identifier.c_str());

            if (leftActive)
            {
                if (!rightActive)
                {
                    return true; // left before right
                }
            }
            else if (rightActive)
            {
                return false; // right before left.
            }
        }

        // critical jobs take priority
        if (leftJob->IsCritical())
        {
            if (!rightJob->IsCritical())
            {
                return true; // left wins.
            }
        }
        else if (rightJob->IsCritical())
        {
            return false; // right wins
        }

        int leftJobEscalation = leftJob->JobEscalation();
        int rightJobEscalation = rightJob->JobEscalation();

        // This function, even though its called lessThan(), really is asking, does LEFT come before RIGHT
        // The higher the escalation, the more important the request, and thus the sooner we want to process the job
        // Which means if left has a higher escalation number than right, its LESS THAN right.
        if (leftJobEscalation != rightJobEscalation)
        {
            return leftJobEscalation > rightJobEscalation;
        }

        // arbitrarily, lets have PC get done first since pc-format assets are what the editor uses.
        if (!platformsMatch)
        {
            if (leftJob->GetPlatformInfo().m_identifier == AzToolsFramework::AssetSystem::GetHostAssetPlatform())
            {
                return true; // left wins.
            }
            if (rightJob->GetPlatformInfo().m_identifier == AzToolsFramework::AssetSystem::GetHostAssetPlatform())
            {
                return false; // right wins
            }
        }

        int priorityLeft = leftJob->GetPriority();
        int priorityRight = rightJob->GetPriority();

        if (priorityLeft != priorityRight)
        {
            return priorityLeft > priorityRight;
        }

        if (leftJob->GetJobEntry().m_sourceAssetReference == rightJob->GetJobEntry().m_sourceAssetReference)
        {
            // If there are two jobs for the same source, then sort by job run key.
            return leftJob->GetJobEntry().m_jobRunKey < rightJob->GetJobEntry().m_jobRunKey;
        }

        // if we get all the way down here it means we're dealing with two assets which are not
        // in any compile groups, not a priority platform, not a priority type, priority platform, etc.
        // we can arrange these any way we want, but must pick at least a stable order.
        return leftJob->GetJobEntry().GetAbsoluteSourcePath() < rightJob->GetJobEntry().GetAbsoluteSourcePath();
    }

    void RCQueueSortModel::AssetProcessorPlatformConnected(const AZStd::string platform)
    {
        QMetaObject::invokeMethod(this, "ProcessPlatformChangeMessage", Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(platform.c_str())), Q_ARG(bool, true));
    }

    void RCQueueSortModel::AssetProcessorPlatformDisconnected(const AZStd::string platform)
    {
        QMetaObject::invokeMethod(this, "ProcessPlatformChangeMessage", Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(platform.c_str())), Q_ARG(bool, false));
    }

    void RCQueueSortModel::ProcessPlatformChangeMessage(QString platformName, bool connected)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "RCQueueSortModel: Platform %s has %s.", platformName.toUtf8().data(), connected ? "connected" : "disconnected");
        m_dirtyNeedsResort = true;
        if (connected)
        {
            m_currentlyConnectedPlatforms.insert(platformName);
        }
        else
        {
            m_currentlyConnectedPlatforms.remove(platformName);
        }
    }
    void RCQueueSortModel::AddJobIdEntry(AssetProcessor::RCJob* rcJob)
    {
        m_currentJobRunKeyToJobEntries[rcJob->GetJobEntry().m_jobRunKey] = rcJob;
    }

    void RCQueueSortModel::RemoveJobIdEntry(AssetProcessor::RCJob* rcJob)
    {
        m_currentJobRunKeyToJobEntries.erase(rcJob->GetJobEntry().m_jobRunKey);
    }

    void RCQueueSortModel::OnEscalateJobs(AssetProcessor::JobIdEscalationList jobIdEscalationList)
    {
        for (const auto& jobIdEscalationPair : jobIdEscalationList)
        {
            auto found = m_currentJobRunKeyToJobEntries.find(jobIdEscalationPair.first);

            if (found != m_currentJobRunKeyToJobEntries.end())
            {
                m_sourceModel->UpdateJobEscalation(found->second, jobIdEscalationPair.second);
            }
        }
    }

} // end namespace AssetProcessor


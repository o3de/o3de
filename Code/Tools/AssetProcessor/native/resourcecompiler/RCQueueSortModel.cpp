/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/resourcecompiler/RCQueueSortModel.h>
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
                bool canProcessJob = true;
                for (const JobDependencyInternal& jobDepedencyInternal : actualJob->GetJobDependencies())
                {
                    if (jobDepedencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::Order || jobDepedencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::OrderOnce)
                    {
                        const AssetBuilderSDK::JobDependency& jobDependency = jobDepedencyInternal.m_jobDependency;
                        QueueElementID elementId(jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str(), jobDependency.m_platformIdentifier.c_str(), jobDependency.m_jobKey.c_str());

                        if (m_sourceModel->isInFlight(elementId) || m_sourceModel->isInQueue(elementId))
                        {
                            canProcessJob = false;
                            if (!anyPendingJob)
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
                anyPendingJob->GetJobEntry().m_pathRelativeToWatchFolder.toUtf8().data(), anyPendingJob->GetJobKey().toUtf8().data(),
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

        // first thing to check is in platform.
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
        if (leftJob->GetPlatformInfo().m_identifier != rightJob->GetPlatformInfo().m_identifier)
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

        // Optionally stabilize queue order on the source name.
        // This is used in automated tests, to allow tests to have a stable
        // order that jobs with otherwise equal priority run, so tests process
        // assets in the same order each time they are run.
        if (m_sortQueueOnDBSourceName)
        {
            return leftJob->GetJobEntry().m_databaseSourceName < rightJob->GetJobEntry().m_databaseSourceName;
        }
        
        // if we get all the way down here it means we're dealing with two assets which are not
        // in any compile groups, not a priority platform, not a priority type, priority platform, etc.
        // we can arrange these any way we want, but must pick at least a stable order.

        return leftJob->GetJobEntry().m_jobRunKey < rightJob->GetJobEntry().m_jobRunKey;
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


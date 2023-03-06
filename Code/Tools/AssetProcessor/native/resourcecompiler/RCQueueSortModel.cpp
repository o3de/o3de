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
                bool skip = false;

                if (actualJob->GetHasMissingSourceDependency() &&
                    (m_sourceModel->jobsInFlight() > 0 || m_sourceModel->jobsInQueueWithoutMissingDependencies() > 0 ||
                     m_sourceModel->jobsPendingCatalog() > 0))
                {
                    skip = true;
                }
                // If this job has a missing dependency, and there are any jobs in flight,
                // don't queue it until those jobs finish, in case they resolve the dependency.
                // This does mean that if there are multiple queued jobs with missing dependencies,
                // they'll run one at a time instead of in parallel, while waiting for the missing dependency
                // to be potentially resolved.
                if (actualJob->GetHasMissingSourceDependency() &&
                    (m_sourceModel->jobsInFlight() > 0 || m_sourceModel->jobsInQueueWithoutMissingDependencies() > 0 ||
                     m_sourceModel->jobsPendingCatalog() > 0))
                {
                    continue;
                }
                else if (actualJob->GetHasMissingSourceDependency())
                {
                    // One last try - see if the pending source dependency exists. There can be a race condition where
                    // the only jobs left in the queue have missing dependencies, and the missing dependency file job hasn't yet been created because the file was just emitted.
                    bool allDependenciesExistNow = true;
                    for (const JobDependencyInternal& jobDependencyInternal : actualJob->GetJobDependencies())
                    {
                        if (jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::Order ||
                            jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::OrderOnce)
                        {
                            const AssetBuilderSDK::JobDependency& jobDependency = jobDependencyInternal.m_jobDependency;
                            if (AZ::IO::PathView(jobDependency.m_sourceFile.m_sourceFileDependencyPath).IsAbsolute())
                            {
                                if (!AZ::IO::FileIOBase::GetInstance()->Exists(
                                        jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str()))
                                {
                                    allDependenciesExistNow = false;
                                    break;
                                }

                                // TODO: This works for FBX files, but it doesn't work for material files
                                // This is specifically looking for files newly created that haven't been processed yet, so skip
                                // files that were completed already.
                                // This is because a dependency may be on a specific job and not just a source asset.
                                // For example, a material might have a dependency on the job key "Material Type Builder (Final Stage)" for a material type,
                                // but the job dependency might be on the key "Material Type Builder (Pipeline Stage)", which may not exist.
                                // In that case, don't block this job from running if it's the last thing in the queue, just provide a warning and run it.

                                // TODO : only set the has missing dependencies flag if the absolute path is in the intermediate assets folder

                                // How can I verify that the source dependency has already been scanned at this point?
                                // In check source, when jobs are created and dependencies are updated,
                                // I can flip this job to mark it has all dependencies and put in a warning "Job keys didn't match"

                                QString normalizedFullFile =
                                    AssetUtilities::NormalizeFilePath(jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str());
                                AZ::IO::FixedMaxPath absolutePath(normalizedFullFile.toUtf8().constData());
                                QString relativePath, scanfolderPath;
                                m_platformConfig->ConvertToRelativePath(absolutePath.c_str(), relativePath, scanfolderPath);

                                AssetDatabaseConnection dbConnection;
                                dbConnection.OpenDatabase();
                                AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
                                #if 0
                                bool foundInDatabase = dbConnection.GetJobsBySourceName(
                                    actualJob->GetJobEntry().m_sourceAssetReference,
                                    jobs,
                                    actualJob->GetJobEntry().m_builderGuid,
                                    actualJob->GetJobEntry().m_jobKey,
                                    actualJob->GetJobEntry().m_platformInfo.m_identifier.c_str());
                                #endif
                                    
                                }
                            }
                        }
                    }
                    // The dependency exists, but has not been queued yet. Delay processing this job until Asset Processor picks it up.
                    // Otherwise continue and just process the asset with the missing dependency: It may still resolve if
                    // another asset with a missing dependency is in the same place and that other asset emits the file to fill this dependency.
                    // In that situation, this job would run once with the missing depedency, and get re-queued to run again because the dependency resolves.
                    if (allDependenciesExistNow)
                    {
                        AZ_Error(
                            "AssetProcessor",
                            false,
                            "Waiting to start job for %s because dependency exists and hasn't been processed.",
                            actualJob->GetJobEntry().GetAbsoluteSourcePath().toUtf8().constData());
                        for (const JobDependencyInternal& jobDependencyInternal : actualJob->GetJobDependencies())
                        {
                            if (jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::Order ||
                                jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::OrderOnce)
                            {
                                const AssetBuilderSDK::JobDependency& jobDependency = jobDependencyInternal.m_jobDependency;
                                if (AZ::IO::PathView(jobDependency.m_sourceFile.m_sourceFileDependencyPath).IsAbsolute())
                                {
                                    if (AZ::IO::FileIOBase::GetInstance()->Exists(
                                            jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str()))
                                    {
                                        AZ_Error(
                                            "AssetProcessor",
                                            false,
                                            "\t%d - %s - %s",
                                            AZ::IO::FileIOBase::GetInstance()->Exists(jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str()),
                                            jobDependency.m_sourceFile.m_sourceFileDependencyPath.c_str(),
                                            jobDependency.m_jobKey.c_str());
                                    }
                                }
                            }
                        }
                        continue;
                    }
                }
                if (!skip && actualJob->GetHasMissingSourceDependency())
                {
                    AZ_Error(
                        "AssetProcessor",
                        false,
                        "%d - Job with missing dependency is running. %s. %d, %d, %d",
                        skip,
                        actualJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(),
                        m_sourceModel->jobsInFlight(),
                        m_sourceModel->jobsInQueueWithoutMissingDependencies(),
                        m_sourceModel->jobsPendingCatalog());
                }
                bool canProcessJob = true;
                for (const JobDependencyInternal& jobDependencyInternal : actualJob->GetJobDependencies())
                {
                    if (jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::Order || jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::OrderOnce)
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
                            if (!anyPendingJob || (anyPendingJob->GetHasMissingSourceDependency() && !actualJob->GetHasMissingSourceDependency()))
                            {
                                anyPendingJob = actualJob;
                            }
                        }
                        else if(m_sourceModel->isWaitingOnCatalog(elementId))
                        {
                            canProcessJob = false;
                            waitingOnCatalog = true;
                        }

                        // TODO: Tomorrow, pause FBX jobs here if there's still a dependency gap.
                        //
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
        if (leftJob->GetHasMissingSourceDependency() != rightJob->GetHasMissingSourceDependency())
        {
            bool printOut = false;
            if (AZStd::string(leftJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str()).ends_with("fbx") ||
                AZStd::string(rightJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str()).ends_with("fbx"))
            {
                //printOut = true;
                /* AZ_Error(
                    "AssetProcessor",
                    false,
                    "Something has a missing source dependency: %d %s: %d %s",
                    leftJob->GetHasMissingSourceDependency(),
                    leftJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(),
                    rightJob->GetHasMissingSourceDependency(),
                    rightJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str());*/
            }
            if (rightJob->GetHasMissingSourceDependency())
            {
                if (printOut)
                {
                    for (auto& dep : rightJob->GetJobDependencies())
                    {
                        AZ_Error("AssetProcessor", false, "\t%s", dep.ToString().c_str());
                    }
                }
                return true; // left does not have a missing source dependency, but right does, so left wins.
            }
            if (printOut)
            {
                for (auto& dep : leftJob->GetJobDependencies())
                {
                    AZ_Error("AssetProcessor", false, "\t%s", dep.ToString().c_str());
                }
            }
            return false; // Right does not have a missing source dependency, but left does, so right wins.
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


/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h> // for AssetSystemJobRequestBus
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneUI/CommonWidgets/JobWatcher.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            const int JobWatcher::s_jobQueryInterval = 750; // ms

            JobWatcher::JobWatcher(const AZStd::string& sourceAssetFullPath, Uuid traceTag)
                : m_jobQueryTimer(new QTimer(this))
                , m_sourceAssetFullPath(sourceAssetFullPath)
                , m_traceTag(traceTag)
            {
                connect(m_jobQueryTimer, &QTimer::timeout, this, &JobWatcher::OnQueryJobs);
            }

            void JobWatcher::StartMonitoring()
            {
                m_jobQueryTimer->start(s_jobQueryInterval);
            }

            void JobWatcher::OnQueryJobs()
            {
                using namespace AzToolsFramework;
                using namespace AzToolsFramework::AssetSystem;

                AZ_TraceContext("Tag", m_traceTag);
                
                // Query for the relevant jobs
                Outcome<AssetSystem::JobInfoContainer> result = Failure();
                AssetSystemJobRequestBus::BroadcastResult(result, &AssetSystemJobRequestBus::Events::GetAssetJobsInfo, m_sourceAssetFullPath, true);
 
                if (!result.IsSuccess())
                {
                    m_jobQueryTimer->stop();
                    emit JobQueryFailed("Failed to retrieve job information from Asset Processor.");
                    return;
                }

                JobInfoContainer& allJobs = result.GetValue();
                if (allJobs.empty())
                {
                    m_jobQueryTimer->stop();
                    emit JobQueryFailed("Queued file didn't produce any jobs.");
                    return;
                }

                bool allFinished = true;
                for (const JobInfo& job : allJobs)
                {
                    AZ_Assert(job.m_status != JobStatus::Any,
                        "The 'Any' status for job should be exclusive to the database and never be a result to a query.");
                    if (job.m_status != JobStatus::Queued && job.m_status != JobStatus::InProgress)
                    {
                        if (AZStd::find(m_reportedJobs.begin(), m_reportedJobs.end(), job.m_jobRunKey) == m_reportedJobs.end())
                        {
                            bool wasSuccessful = job.m_status == AzToolsFramework::AssetSystem::JobStatus::Completed;

                            Outcome<AZStd::string> logFetchResult = Failure();
                            AssetSystemJobRequestBus::BroadcastResult(logFetchResult, &AssetSystemJobRequestBus::Events::GetJobLog, job.m_jobRunKey);
                            emit JobProcessingComplete(job.m_platform, job.m_jobRunKey, wasSuccessful,
                                logFetchResult.IsSuccess() ? logFetchResult.GetValue() : "");

                            m_reportedJobs.push_back(job.m_jobRunKey);
                        }
                    }
                    else
                    {
                        allFinished = false;
                    }
                }
                if (allFinished)
                {
                    m_jobQueryTimer->stop();
                    emit AllJobsComplete();
                }
            }
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ

#include <CommonWidgets/moc_JobWatcher.cpp>

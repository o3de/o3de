/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/AssetStatusReporter.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AtomToolsFramework
{
    AssetStatusReporter::AssetStatusReporter(const AZStd::vector<AZStd::string>& sourcePaths)
        : m_sourcePaths(sourcePaths)
    {
    }

    AssetStatusReporterState AssetStatusReporter::Update()
    {
        if (GetCurrentState() == AssetStatusReporterState::Processing)
        {
            const AZStd::string& sourcePath = GetCurrentPath();

            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
            AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
                jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfo, sourcePath, false);

            if (jobOutcome.IsSuccess())
            {
                for (const auto& job : jobOutcome.GetValue())
                {
                    switch (job.m_status)
                    {
                    case AzToolsFramework::AssetSystem::JobStatus::Failed:
                    case AzToolsFramework::AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:
                        // If any of the asset jobs failed then the entire operation is a failure.
                        m_failed = true;
                        return GetCurrentState();
                    }
                }

                for (const auto& job : jobOutcome.GetValue())
                {
                    switch (job.m_status)
                    {
                    case AzToolsFramework::AssetSystem::JobStatus::Queued:
                    case AzToolsFramework::AssetSystem::JobStatus::InProgress:
                        // If any of the asset jobs are queued or in progress then return early until the next status request.
                        return GetCurrentState();
                    }
                }
            }

            ++m_index;
        }

        return GetCurrentState();
    }

    AssetStatusReporterState AssetStatusReporter::GetCurrentState() const
    {
        if (m_failed)
        {
            return AssetStatusReporterState::Failed;
        }
        return m_index < m_sourcePaths.size() ? AssetStatusReporterState::Processing : AssetStatusReporterState::Succeeded;
    }

    AZStd::string AssetStatusReporter::GetCurrentStateName() const
    {
        switch (GetCurrentState())
        {
        case AssetStatusReporterState::Failed:
            return "Failed";
        case AssetStatusReporterState::Processing:
            return "Processing";
        case AssetStatusReporterState::Succeeded:
            return "Succeeded";
        }
        return "Invalid";
    }

    AZStd::string AssetStatusReporter::GetCurrentStatusMessage() const
    {
        return AZStd::string::format("%s (%s)", GetCurrentPath().c_str(), GetCurrentStateName().c_str());
    }

    AZStd::string AssetStatusReporter::GetCurrentPath() const
    {
        return m_index < m_sourcePaths.size() ? m_sourcePaths[m_index] : AZStd::string();
    }
} // namespace AtomToolsFramework

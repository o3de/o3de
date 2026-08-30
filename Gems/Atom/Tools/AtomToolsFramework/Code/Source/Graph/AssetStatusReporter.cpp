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
        // Advance past every path that has already settled instead of stepping a single path per call. AssetStatusReporterSystem drives
        // this from a thread that sleeps 10ms between iterations, so stepping one path at a time imposed a floor of 10ms multiplied by the
        // number of reported paths on every graph compile, even when all of the asset jobs had already finished. This loop always
        // terminates: each iteration either returns or increments m_index, and GetCurrentState() stops returning Processing once m_index
        // reaches the end of m_sourcePaths.
        while (GetCurrentState() == AssetStatusReporterState::Processing)
        {
            const AZStd::string sourcePath = GetCurrentPath();

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
                        // This path still has work outstanding. Stop draining here and re-check it on the next status request.
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

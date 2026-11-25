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
        AZStd::string sourcePath;
        {
            AZStd::scoped_lock lock(m_mutex);
            if (GetCurrentStateUnlocked() != AssetStatusReporterState::Processing)
            {
                return GetCurrentStateUnlocked();
            }
            sourcePath = m_sourcePaths[m_index];
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
        AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
            jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfo, sourcePath, false);

        if (!jobOutcome.IsSuccess() || jobOutcome.GetValue().empty())
        {
            return GetCurrentState();
        }

        for (const auto& job : jobOutcome.GetValue())
        {
            switch (job.m_status)
            {
            case AzToolsFramework::AssetSystem::JobStatus::Failed:
            case AzToolsFramework::AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:
                AZStd::scoped_lock lock(m_mutex);
                m_failed = true;
                return GetCurrentStateUnlocked();
            }
        }

        for (const auto& job : jobOutcome.GetValue())
        {
            switch (job.m_status)
            {
            case AzToolsFramework::AssetSystem::JobStatus::Queued:
            case AzToolsFramework::AssetSystem::JobStatus::InProgress:
                return GetCurrentState();
            }
        }

        {
            AZStd::scoped_lock lock(m_mutex);
            ++m_index;
            return GetCurrentStateUnlocked();
        }
    }

    AssetStatusReporterState AssetStatusReporter::GetCurrentState() const
    {
        AZStd::scoped_lock lock(m_mutex);
        return GetCurrentStateUnlocked();
    }

    AssetStatusReporterState AssetStatusReporter::GetCurrentStateUnlocked() const
    {
        if (m_failed)
        {
            return AssetStatusReporterState::Failed;
        }
        if (m_index < m_sourcePaths.size())
        {
            return AssetStatusReporterState::Processing;
        }
        return AssetStatusReporterState::Succeeded;
    }

    AZStd::string AssetStatusReporter::GetCurrentStateName() const
    {
        AZStd::scoped_lock lock(m_mutex);
        return GetCurrentStateNameUnlocked();
    }

    const char* AssetStatusReporter::GetCurrentStateNameUnlocked() const
    {
        switch (GetCurrentStateUnlocked())
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
        AZStd::scoped_lock lock(m_mutex);
        AZStd::string currentPath;
        if (m_index < m_sourcePaths.size())
        {
            currentPath = m_sourcePaths[m_index];
        }
        return AZStd::string::format("%s (%s)", currentPath.c_str(), GetCurrentStateNameUnlocked());
    }

    AZStd::string AssetStatusReporter::GetCurrentPath() const
    {
        AZStd::scoped_lock lock(m_mutex);
        if (m_index < m_sourcePaths.size())
        {
            return m_sourcePaths[m_index];
        }
        return {};
    }
} // namespace AtomToolsFramework

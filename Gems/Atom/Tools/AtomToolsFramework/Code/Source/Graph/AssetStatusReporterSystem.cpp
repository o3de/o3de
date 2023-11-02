/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/AssetStatusReporterSystem.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AtomToolsFramework
{
    AssetStatusReporterSystem::AssetStatusReporterSystem(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        AssetStatusReporterSystemRequestBus::Handler::BusConnect(m_toolId);

        // Create a thread that continuously processes a queue of incoming asset status requests.
        m_threadRunning = true;
        m_threadDesc.m_name = "AssetStatusReporterSystem";
        m_thread = AZStd::thread(
            m_threadDesc,
            [this]()
            {
                while (m_threadRunning)
                {
                    Update();

                    // Sleep briefly to give AP time to update and other possible threads time to make AssetSystemJobRequestBus requests
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                }
            });

    }

    AssetStatusReporterSystem::~AssetStatusReporterSystem()
    {
        StopReportingAll();
        m_threadRunning = false;
        m_thread.join();
        AssetStatusReporterSystemRequestBus::Handler::BusDisconnect();
    }

    void AssetStatusReporterSystem::StartReporting(const AZ::Uuid& requestId, const AZStd::vector<AZStd::string>& sourcePaths)
    {
        StopReporting(requestId);
        AZStd::scoped_lock lock(m_requestMutex);
        m_activeReporterTable.emplace_back(requestId, AZStd::make_shared<AssetStatusReporter>(sourcePaths));
    }

    void AssetStatusReporterSystem::StopReporting(const AZ::Uuid& requestId)
    {
        AZStd::scoped_lock lock(m_requestMutex);
        AZStd::erase_if(m_activeReporterTable, [&requestId](const auto& reporterPair) {
            return reporterPair.first == requestId;
        });
        AZStd::erase_if(m_inactiveReporterTable, [&requestId](const auto& reporterPair) {
            return reporterPair.first == requestId;
        });
    }

    void AssetStatusReporterSystem::StopReportingAll()
    {
        AZStd::scoped_lock lock(m_requestMutex);
        m_activeReporterTable.clear();
        m_inactiveReporterTable.clear();
    }

    AssetStatusReporterState AssetStatusReporterSystem::GetStatus(const AZ::Uuid& requestId) const
    {
        AZStd::scoped_lock lock(m_requestMutex);
        if (auto reporterIt = AZStd::find_if(
                m_activeReporterTable.begin(),
                m_activeReporterTable.end(),
                [&requestId](const auto& reporterPair)
                {
                    return reporterPair.first == requestId;
                });
            reporterIt != m_activeReporterTable.end())
        {
            return reporterIt->second->GetCurrentState();
        }

        if (auto reporterIt = AZStd::find_if(
                m_inactiveReporterTable.begin(),
                m_inactiveReporterTable.end(),
                [&requestId](const auto& reporterPair)
                {
                    return reporterPair.first == requestId;
                });
            reporterIt != m_inactiveReporterTable.end())
        {
            return reporterIt->second->GetCurrentState();
        }

        return AssetStatusReporterState::Invalid;
    }

    void AssetStatusReporterSystem::Update()
    {
        AZStd::scoped_lock lock(m_requestMutex);
        if (!m_activeReporterTable.empty())
        {
            // Retrieve and update the status for the current active request.
            auto reporterIt = m_activeReporterTable.begin();
            reporterIt->second->Update();

            // Create a string message from the current status.
            const AZStd::string statusMessage = reporterIt->second->GetCurrentStatusMessage();

            // If the message has not changed since the last update then send it to the main windows status bar.
            if (m_lastStatusMessage != statusMessage)
            {
                m_lastStatusMessage = statusMessage;

                // Queuing the notification on the system take bus so that it triggers on the main thread.
                AZ::SystemTickBus::QueueFunction([toolId = m_toolId, statusMessage]() {
                    // This should be generalized with a status reporter notification bus so the message can be handled by other systems
                    // or UI than the status bar.
                    AtomToolsMainWindowRequestBus::Event(
                        toolId, &AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);
                });
            }

            // Any complete or canceled requests will get moved to the inactive list.
            if (reporterIt->second->GetCurrentState() != AssetStatusReporterState::Processing)
            {
                m_inactiveReporterTable.emplace_back(AZStd::move(*reporterIt));
                m_activeReporterTable.erase(reporterIt);
                m_lastStatusMessage.clear();
            }
        }
    }
} // namespace AtomToolsFramework

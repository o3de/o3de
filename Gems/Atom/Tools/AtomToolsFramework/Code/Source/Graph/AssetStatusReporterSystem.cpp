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
                }
            });

    }

    AssetStatusReporterSystem::~AssetStatusReporterSystem()
    {
        AssetStatusReporterSystemRequestBus::Handler::BusDisconnect();
        {
            AZStd::scoped_lock lock(m_requestMutex);
            m_threadRunning = false;
        }
        m_requestCondition.notify_all();
        m_thread.join();
        StopReportingAll();
    }

    void AssetStatusReporterSystem::StartReporting(const AZ::Uuid& requestId, const AZStd::vector<AZStd::string>& sourcePaths)
    {
        {
            AZStd::scoped_lock lock(m_requestMutex);
            AZStd::erase_if(m_activeReporterTable, [&requestId](const auto& reporterPair)
            {
                return reporterPair.first == requestId;
            });
            AZStd::erase_if(m_inactiveReporterTable, [&requestId](const auto& reporterPair)
            {
                return reporterPair.first == requestId;
            });
            m_activeReporterTable.emplace_back(requestId, AZStd::make_shared<AssetStatusReporter>(sourcePaths));
        }
        m_requestCondition.notify_one();
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
        AZ::Uuid requestId;
        AZStd::shared_ptr<AssetStatusReporter> reporter;
        {
            AZStd::unique_lock lock(m_requestMutex);
            m_requestCondition.wait(lock, [this]() { return !m_threadRunning || !m_activeReporterTable.empty(); });
            if (!m_threadRunning)
            {
                return;
            }
            requestId = m_activeReporterTable.front().first;
            reporter = m_activeReporterTable.front().second;
        }

        const AssetStatusReporterState state = reporter->Update();
        const AZStd::string statusMessage = reporter->GetCurrentStatusMessage();
        bool publishStatus = false;

        {
            AZStd::unique_lock lock(m_requestMutex);
            const auto reporterIt = AZStd::find_if(
                m_activeReporterTable.begin(),
                m_activeReporterTable.end(),
                [&requestId, &reporter](const auto& reporterPair)
                {
                    return reporterPair.first == requestId && reporterPair.second == reporter;
                });
            if (reporterIt != m_activeReporterTable.end())
            {
                if (m_lastStatusMessage != statusMessage)
                {
                    m_lastStatusMessage = statusMessage;
                    publishStatus = true;
                }

                if (state != AssetStatusReporterState::Processing)
                {
                    m_inactiveReporterTable.emplace_back(AZStd::move(*reporterIt));
                    m_activeReporterTable.erase(reporterIt);
                    m_lastStatusMessage.clear();
                }
                else if (m_activeReporterTable.size() > 1)
                {
                    AZStd::rotate(reporterIt, AZStd::next(reporterIt), m_activeReporterTable.end());
                }
            }

            m_requestCondition.wait_for(lock, AZStd::chrono::milliseconds(10), [this]() { return !m_threadRunning; });
        }

        if (publishStatus)
        {
            AZ::SystemTickBus::QueueFunction([toolId = m_toolId, statusMessage]()
            {
                AtomToolsMainWindowRequestBus::Event(toolId, &AtomToolsMainWindowRequestBus::Events::SetStatusMessage, statusMessage);
            });
        }
    }
} // namespace AtomToolsFramework

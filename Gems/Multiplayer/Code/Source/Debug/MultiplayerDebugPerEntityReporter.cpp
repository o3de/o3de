/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerDebugPerEntityReporter.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/ToString.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Multiplayer/IMultiplayer.h>

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#endif

AZ_CVAR(float, net_DebugEntities_ShowAboveKbps, 1.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Prints bandwidth on network entities with higher kpbs than this value");

AZ_CVAR(float, net_DebugEntities_WarnAboveKbps, 10.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Prints bandwidth on network entities with higher kpbs than this value");

AZ_CVAR(AZ::Color, net_DebugEntities_WarningColor, AZ::Colors::Red, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If true, prints debug text over entities that use a considerable amount of network traffic");

AZ_CVAR(AZ::Color, net_DebugEntities_BelowWarningColor, AZ::Colors::Grey, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If true, prints debug text over entities that use a considerable amount of network traffic");

namespace Multiplayer
{
#if defined(IMGUI_ENABLED)
    static const ImVec4 k_ImGuiTomato = ImVec4(1.0f, 0.4f, 0.3f, 1.0f);
    static const ImVec4 k_ImGuiKhaki = ImVec4(0.9f, 0.8f, 0.5f, 1.0f);
    static const ImVec4 k_ImGuiCyan = ImVec4(0.5f, 1.0f, 1.0f, 1.0f);
    static const ImVec4 k_ImGuiDusk = ImVec4(0.7f, 0.7f, 1.0f, 1.0f);
    static const ImVec4 k_ImGuiWhite = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    // --------------------------------------------------------------------------------------------
    template <typename Reporter>
    bool ReplicatedStateTreeNode(const AZStd::string& name, Reporter& report, const ImVec4& color, int depth = 0)
    {
        const int defaultPadAmount = 55;
        const int depthReduction = 3;
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        const bool expanded = ImGui::TreeNode(name.c_str(),
            "%-*s %7.2f kbps %7.2f B Avg. %4zu B Max %10zu B Payload",
            defaultPadAmount - depthReduction * depth,
            name.c_str(),
            report.GetKbitsPerSecond(),
            report.GetAverageBytes(),
            report.GetMaxBytes(),
            report.GetTotalBytes());
        ImGui::PopStyleColor();
        return expanded;
    }

    // --------------------------------------------------------------------------------------------
    void DisplayReplicatedStateReport(AZStd::map<AZStd::string, MultiplayerDebugComponentReporter>& componentReports, float kbpsWarn, float maxWarn)
    {
        for (auto& componentPair : componentReports)
        {
            ImGui::Separator();
            MultiplayerDebugComponentReporter& componentReport = componentPair.second;

            if (ReplicatedStateTreeNode(componentPair.first, componentReport, k_ImGuiCyan, 1))
            {
                ImGui::Separator();
                ImGui::Columns(6, "replicated_field_columns");
                ImGui::NextColumn();
                ImGui::Text("kbps");
                ImGui::NextColumn();
                ImGui::Text("Avg. Bytes");
                ImGui::NextColumn();
                ImGui::Text("Min Bytes");
                ImGui::NextColumn();
                ImGui::Text("Max Bytes");
                ImGui::NextColumn();
                ImGui::Text("Total Bytes");
                ImGui::NextColumn();

                auto fieldReports = componentReport.GetFieldReports();
                for (auto& fieldPair : fieldReports)
                {
                    MultiplayerDebugByteReporter& fieldReport = *fieldPair.second;
                    const float kbitsLastSecond = fieldReport.GetKbitsPerSecond();

                    const ImVec4* textColor = &k_ImGuiWhite;
                    if (aznumeric_cast<float>(fieldReport.GetMaxBytes()) > maxWarn)
                    {
                        textColor = &k_ImGuiKhaki;
                    }

                    if (kbitsLastSecond > kbpsWarn)
                    {
                        textColor = &k_ImGuiTomato;
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, *textColor);

                    ImGui::Text("%s", fieldPair.first.c_str());
                    ImGui::NextColumn();
                    ImGui::Text("%.2f", kbitsLastSecond);
                    ImGui::NextColumn();
                    ImGui::Text("%.2f", fieldReport.GetAverageBytes());
                    ImGui::NextColumn();
                    ImGui::Text("%zu", fieldReport.GetMinBytes());
                    ImGui::NextColumn();
                    ImGui::Text("%zu", fieldReport.GetMaxBytes());
                    ImGui::NextColumn();
                    ImGui::Text("%zu", fieldReport.GetTotalBytes());
                    ImGui::NextColumn();

                    ImGui::PopStyleColor();
                }

                ImGui::Columns(1);
                ImGui::TreePop();
            }
        }
    }
#endif

    MultiplayerDebugPerEntityReporter::MultiplayerDebugPerEntityReporter()
        : m_updateDebugOverlay([this]() { UpdateDebugOverlay(); }, AZ::Name("UpdateDebugPerEntityOverlay"))
    {
        m_updateDebugOverlay.Enqueue(AZ::Time::ZeroTimeMs, true);

        m_eventHandlers.m_entitySerializeStart = decltype(m_eventHandlers.m_entitySerializeStart)([this](AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName)
            {
                RecordEntitySerializeStart(mode, entityId, entityName);
            });
        m_eventHandlers.m_componentSerializeEnd = decltype(m_eventHandlers.m_componentSerializeEnd)([this](AzNetworking::SerializerMode mode,
            NetComponentId netComponentId)
            {
                RecordComponentSerializeEnd(mode, netComponentId);
            });
        m_eventHandlers.m_entitySerializeStop = decltype(m_eventHandlers.m_entitySerializeStop)([this](AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName)
            {
                RecordEntitySerializeStop(mode, entityId, entityName);
            });
        m_eventHandlers.m_propertySent = decltype(m_eventHandlers.m_propertySent)([this](NetComponentId netComponentId,
                                                                                         PropertyIndex propertyId, uint32_t totalBytes)
            {
                RecordPropertySent(netComponentId, propertyId, totalBytes);
            });
        m_eventHandlers.m_propertyReceived = decltype(m_eventHandlers.m_propertyReceived)([this](NetComponentId netComponentId,
            PropertyIndex propertyId, uint32_t totalBytes)
            {
                RecordPropertyReceived(netComponentId, propertyId, totalBytes);
            });
        m_eventHandlers.m_rpcSent = decltype(m_eventHandlers.m_rpcSent)([this](AZ::EntityId entityId, const char* entityName,
                                                                               NetComponentId netComponentId,
                                                                               RpcIndex rpcId, uint32_t totalBytes)
            {
                RecordRpcSent(entityId, entityName, netComponentId, rpcId, totalBytes);
            });
        m_eventHandlers.m_rpcReceived = decltype(m_eventHandlers.m_rpcReceived)([this](AZ::EntityId entityId, const char* entityName,
                                                                                       NetComponentId netComponentId,
                                                                                       RpcIndex rpcId, uint32_t totalBytes)
            {
                RecordRpcSent(entityId, entityName, netComponentId, rpcId, totalBytes);
            });

        GetMultiplayer()->GetStats().ConnectHandlers(m_eventHandlers);
    }

    // --------------------------------------------------------------------------------------------
    void MultiplayerDebugPerEntityReporter::OnImGuiUpdate()
    {
#if defined(IMGUI_ENABLED)
        static ImGuiTextFilter filter;
        filter.Draw();

        if (ImGui::CollapsingHeader("Receiving Entities"))
        {
            for (AZStd::pair<AZ::EntityId, MultiplayerDebugEntityReporter>& entityPair : m_receivingEntityReports)
            {
                if (!filter.PassFilter(entityPair.second.GetEntityName()))
                {
                    continue;
                }

                ImGui::Separator();
                if (ReplicatedStateTreeNode(entityPair.second.GetEntityName(), entityPair.second, k_ImGuiDusk))
                {
                    DisplayReplicatedStateReport(entityPair.second.GetComponentReports(), m_replicatedStateKbpsWarn, m_replicatedStateMaxSizeWarn);
                    ImGui::TreePop();
                }
            }
        }

        if (ImGui::CollapsingHeader("Sending Entities"))
        {
            for (AZStd::pair<AZ::EntityId, MultiplayerDebugEntityReporter>& entityPair : m_sendingEntityReports)
            {
                const char* name = entityPair.second.GetEntityName();
                if (!filter.PassFilter(name))
                {
                    continue;
                }

                ImGui::Separator();
                if (ReplicatedStateTreeNode(name, entityPair.second, k_ImGuiDusk))
                {
                    DisplayReplicatedStateReport(entityPair.second.GetComponentReports(), m_replicatedStateKbpsWarn, m_replicatedStateMaxSizeWarn);
                    ImGui::TreePop();
                }
            }
        }
#endif
    }

    void MultiplayerDebugPerEntityReporter::RecordEntitySerializeStart(AzNetworking::SerializerMode mode,
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const char* entityName)
    {
        switch (mode)
        {
        case AzNetworking::SerializerMode::ReadFromObject:
            m_currentSendingEntityReport.Reset();
            m_currentSendingEntityReport.SetEntityName(entityName);
            break;
        case AzNetworking::SerializerMode::WriteToObject:
            m_currentReceivingEntityReport.Reset();
            m_currentReceivingEntityReport.SetEntityName(entityName);
            break;
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordComponentSerializeEnd(AzNetworking::SerializerMode mode, [[maybe_unused]] NetComponentId
                                                                        netComponentId)
    {
        switch (mode)
        {
        case AzNetworking::SerializerMode::ReadFromObject:
            m_currentSendingEntityReport.ReportFragmentEnd();
            break;
        case AzNetworking::SerializerMode::WriteToObject:
            m_currentReceivingEntityReport.ReportFragmentEnd();
            break;
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordEntitySerializeStop(AzNetworking::SerializerMode mode,
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const char* entityName)
    {
        switch (mode)
        {
        case AzNetworking::SerializerMode::ReadFromObject:
            m_sendingEntityReports[entityId].Combine(m_currentSendingEntityReport);
            break;
        case AzNetworking::SerializerMode::WriteToObject:
            m_receivingEntityReports[entityId].Combine(m_currentReceivingEntityReport);
            break;
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordPropertySent(
        NetComponentId netComponentId,
        PropertyIndex propertyId,
        uint32_t totalBytes)
    {
        if (const MultiplayerComponentRegistry* componentRegistry = GetMultiplayerComponentRegistry())
        {
            m_currentSendingEntityReport.ReportField(static_cast<AZ::u32>(netComponentId),
                componentRegistry->GetComponentName(netComponentId),
                componentRegistry->GetComponentPropertyName(netComponentId, propertyId), totalBytes);
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordPropertyReceived(
        NetComponentId netComponentId,
        PropertyIndex propertyId,
        uint32_t totalBytes)
    {
        if (const MultiplayerComponentRegistry* componentRegistry = GetMultiplayerComponentRegistry())
        {
            m_currentReceivingEntityReport.ReportField(static_cast<AZ::u32>(netComponentId),
                componentRegistry->GetComponentName(netComponentId),
                componentRegistry->GetComponentPropertyName(netComponentId, propertyId), totalBytes);
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordRpcSent(AZ::EntityId entityId, const char* entityName, NetComponentId netComponentId,
                                                          RpcIndex rpcId, uint32_t totalBytes)
    {
        if (const MultiplayerComponentRegistry* componentRegistry = GetMultiplayerComponentRegistry())
        {
            // MultiplayerDebugByteReporter requires a
            RecordEntitySerializeStart(AzNetworking::SerializerMode::ReadFromObject, entityId, entityName);

            m_currentSendingEntityReport.ReportField(static_cast<AZ::u32>(netComponentId),
                componentRegistry->GetComponentName(netComponentId),
                componentRegistry->GetComponentRpcName(netComponentId, rpcId), totalBytes);

            RecordComponentSerializeEnd(AzNetworking::SerializerMode::ReadFromObject, netComponentId);
            RecordEntitySerializeStop(AzNetworking::SerializerMode::ReadFromObject, entityId, entityName);
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordRpcReceived(
        AZ::EntityId entityId, const char* entityName,
        NetComponentId netComponentId,
        RpcIndex rpcId,
        uint32_t totalBytes)
    {
        if (const MultiplayerComponentRegistry* componentRegistry = GetMultiplayerComponentRegistry())
        {
            RecordEntitySerializeStart(AzNetworking::SerializerMode::WriteToObject, entityId, entityName);

            m_currentReceivingEntityReport.ReportField(static_cast<AZ::u32>(netComponentId),
                componentRegistry->GetComponentName(netComponentId),
                componentRegistry->GetComponentRpcName(netComponentId, rpcId), totalBytes);

            RecordComponentSerializeEnd(AzNetworking::SerializerMode::WriteToObject, netComponentId);
            RecordEntitySerializeStop(AzNetworking::SerializerMode::WriteToObject, entityId, entityName);
        }
    }

    void MultiplayerDebugPerEntityReporter::UpdateDebugOverlay()
    {
        m_networkEntitiesTraffic.clear();

        // Merging up and down traffic to provide a unified debug text per entity
        for (AZStd::pair<AZ::EntityId, MultiplayerDebugEntityReporter>& entityPair : m_receivingEntityReports)
        {
            m_networkEntitiesTraffic[entityPair.first].m_name = entityPair.second.GetEntityName();
            m_networkEntitiesTraffic[entityPair.first].m_down = entityPair.second.GetKbitsPerSecond();
        }
        for (AZStd::pair<AZ::EntityId, MultiplayerDebugEntityReporter>& entityPair : m_sendingEntityReports)
        {
            m_networkEntitiesTraffic[entityPair.first].m_name = entityPair.second.GetEntityName();
            m_networkEntitiesTraffic[entityPair.first].m_up = entityPair.second.GetKbitsPerSecond();
        }

        if (m_debugDisplay == nullptr)
        {
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
            m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        }

        const AZ::u32 stateBefore = m_debugDisplay->GetState();

        for (const AZStd::pair<AZ::EntityId, NetworkEntityTraffic>& networkEntity : m_networkEntitiesTraffic)
        {
            if (networkEntity.second.m_down < net_DebugEntities_ShowAboveKbps && networkEntity.second.m_up < net_DebugEntities_ShowAboveKbps)
            {
                continue;
            }

            if (networkEntity.second.m_down > net_DebugEntities_WarnAboveKbps || networkEntity.second.m_up > net_DebugEntities_WarnAboveKbps)
            {
                m_debugDisplay->SetColor(net_DebugEntities_WarningColor);
            }
            else
            {
                m_debugDisplay->SetColor(net_DebugEntities_BelowWarningColor);
            }

            if (networkEntity.second.m_down > net_DebugEntities_ShowAboveKbps && networkEntity.second.m_up > net_DebugEntities_ShowAboveKbps)
            {
                azsprintf(m_statusBuffer, "[%s] %.0f down / %0.f up (kbps)", networkEntity.second.m_name,
                    networkEntity.second.m_down, networkEntity.second.m_up);
            }
            else if (networkEntity.second.m_down > net_DebugEntities_ShowAboveKbps)
            {
                azsprintf(m_statusBuffer, "[%s] %.0f down (kbps)", networkEntity.second.m_name, networkEntity.second.m_down);
            }
            else
            {
                azsprintf(m_statusBuffer, "[%s] %.0f up (kbps)", networkEntity.second.m_name, networkEntity.second.m_up);
            }

            AZ::Vector3 entityPosition = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(entityPosition, networkEntity.first, &AZ::TransformBus::Events::GetWorldTranslation);
            if (entityPosition.IsZero() == false)
            {
                constexpr bool centerText = true;
                m_debugDisplay->DrawTextLabel(entityPosition, 1.0f, m_statusBuffer, centerText, 0, 0);
            }
        }

        m_debugDisplay->SetState(stateBefore);
    }
}

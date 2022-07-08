/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerDebugHierarchyReporter.h"

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#endif

namespace Multiplayer
{
    MultiplayerDebugHierarchyReporter::MultiplayerDebugHierarchyReporter()
        : m_updateDebugOverlay([this]() { UpdateDebugOverlay(); }, AZ::Name("UpdateHierarchyDebug"))
    {
        CollectHierarchyRoots();

        AZ::EntitySystemBus::Handler::BusConnect();
        m_updateDebugOverlay.Enqueue(AZ::Time::ZeroTimeMs, true);
    }

    MultiplayerDebugHierarchyReporter::~MultiplayerDebugHierarchyReporter()
    {
        AZ::EntitySystemBus::Handler::BusDisconnect();
    }

    // --------------------------------------------------------------------------------------------
    void MultiplayerDebugHierarchyReporter::OnImGuiUpdate()
    {
#if defined(IMGUI_ENABLED)
        ImGui::Text("Hierarchies");
        ImGui::Separator();

        for (const auto& root : m_hierarchyRoots)
        {
            if (const auto* rootComponent = root.second.m_rootComponent)
            {
                if (rootComponent->IsHierarchicalRoot())
                {
                    const AZStd::vector<AZ::Entity*>& hierarchicalChildren = rootComponent->GetHierarchicalEntities();

                    if (ImGui::TreeNode(rootComponent->GetEntity()->GetName().c_str(),
                        "[%s] %4zu members",
                        rootComponent->GetEntity()->GetName().c_str(),
                        hierarchicalChildren.size()))
                    {
                        ImGui::Separator();
                        ImGui::Columns(4, "hierarchy_columns");
                        ImGui::Text("EntityId");
                        ImGui::NextColumn();
                        ImGui::Text("NetEntityId");
                        ImGui::NextColumn();
                        ImGui::Text("Entity Name");
                        ImGui::NextColumn();
                        ImGui::Text("Role");
                        ImGui::NextColumn();

                        ImGui::Separator();
                        ImGui::Columns(4, "hierarchy child info");

                        bool firstEntity = true;
                        for (const AZ::Entity* entity : hierarchicalChildren)
                        {
                            ImGui::Text("%s", entity->GetId().ToString().c_str());
                            ImGui::NextColumn();
                            ImGui::Text("%llu", static_cast<AZ::u64>(GetMultiplayer()->GetNetworkEntityManager()->GetNetEntityIdById(entity->GetId())));
                            ImGui::NextColumn();
                            ImGui::Text("%s", entity->GetName().c_str());
                            ImGui::NextColumn();

                            if (firstEntity)
                            {
                                ImGui::Text("Root node");
                            }
                            else if (entity->FindComponent<NetworkHierarchyRootComponent>())
                            {
                                ImGui::Text("Inner root node");
                            }
                            else if (entity->FindComponent<NetworkHierarchyChildComponent>())
                            {
                                ImGui::Text("Child node");
                            }
                            ImGui::NextColumn();

                            firstEntity = false;
                        }

                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }
                }
            }
        }

        ImGui::Separator();
        if (ImGui::InputFloat("Awareness Radius", &m_awarenessRadius))
        {
            CollectHierarchyRoots();
        }
        if (ImGui::Button("Refresh"))
        {
            CollectHierarchyRoots();
        }
#endif
    }


    void MultiplayerDebugHierarchyReporter::UpdateDebugOverlay()
    {
        if (!m_hierarchyRoots.empty())
        {
            if (m_debugDisplay == nullptr)
            {
                AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
                AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
                m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
            }

            const AZ::u32 stateBefore = m_debugDisplay->GetState();
            m_debugDisplay->SetColor(AZ::Colors::White);

            for (const auto& root : m_hierarchyRoots)
            {
                if (const auto* rootComponent = root.second.m_rootComponent)
                {
                    if (rootComponent->IsHierarchicalRoot())
                    {
                        const AZStd::vector<AZ::Entity*>& hierarchicalChildren = rootComponent->GetHierarchicalEntities();

                        azsprintf(m_statusBuffer, "Hierarchy [%s] %u members", rootComponent->GetEntity()->GetName().c_str(),
                            aznumeric_cast<AZ::u32>(hierarchicalChildren.size()));

                        AZ::Vector3 entityPosition = rootComponent->GetEntity()->GetTransform()->GetWorldTranslation();
                        constexpr bool centerText = true;
                        m_debugDisplay->DrawTextLabel(entityPosition, 1.0f, m_statusBuffer, centerText, 0, 0);
                    }
                }
            }

            m_debugDisplay->SetState(stateBefore);
        }
    }

    void MultiplayerDebugHierarchyReporter::OnEntityActivated(const AZ::EntityId& entityId)
    {
        if (const AZ::Entity* childEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId))
        {
            if (auto* rootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>())
            {
                HierarchyRootInfo info;
                info.m_rootComponent = rootComponent;
                rootComponent->BindNetworkHierarchyChangedEventHandler(info.m_changedEvent);
                rootComponent->BindNetworkHierarchyLeaveEventHandler(info.m_leaveEvent);

                m_hierarchyRoots.insert(AZStd::make_pair(rootComponent, info));
            }
        }
    }

    void MultiplayerDebugHierarchyReporter::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        if (const AZ::Entity* childEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId))
        {
            if (auto* rootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>())
            {
                m_hierarchyRoots.erase(rootComponent);
            }
        }
    }

    void MultiplayerDebugHierarchyReporter::CollectHierarchyRoots()
    {
        m_hierarchyRoots.clear();
        AZ::Sphere awarenessSphere(AZ::Vector3::CreateZero(), m_awarenessRadius);

        const auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (const auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            awarenessSphere.SetCenter(viewportContext->GetCameraTransform().GetTranslation());
        }

        AZStd::vector<AzFramework::VisibilityEntry*> gatheredEntries;
        AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(awarenessSphere,
            [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                gatheredEntries.reserve(gatheredEntries.size() + nodeData.m_entries.size());
                for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
                {
                    if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                    {
                        gatheredEntries.push_back(visEntry);
                    }
                }
            }
        );

        for (const AzFramework::VisibilityEntry* entry : gatheredEntries)
        {
            const AZ::Entity* entity = static_cast<AZ::Entity*>(entry->m_userData);
            if (auto* rootComponent = entity->FindComponent<NetworkHierarchyRootComponent>())
            {
                if (awarenessSphere.GetCenter().GetDistanceEstimate(entity->GetTransform()->GetWorldTranslation()) < m_awarenessRadius)
                {
                    HierarchyRootInfo info;
                    info.m_rootComponent = rootComponent;
                    rootComponent->BindNetworkHierarchyChangedEventHandler(info.m_changedEvent);
                    rootComponent->BindNetworkHierarchyLeaveEventHandler(info.m_leaveEvent);

                    m_hierarchyRoots.insert(AZStd::make_pair(rootComponent, info));
                }
            }
        }
    }
}

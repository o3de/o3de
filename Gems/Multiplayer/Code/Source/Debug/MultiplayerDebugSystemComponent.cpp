/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Debug/MultiplayerDebugSystemComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Atom/Feature/ImGui/SystemBus.h>
#include <ImGuiContextScope.h>
#include <ImGui/ImGuiPass.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

void OnDebugEntities_ShowBandwidth_Changed(const bool& showBandwidth);

AZ_CVAR(bool, net_DebugEntities_ShowBandwidth, false, &OnDebugEntities_ShowBandwidth_Changed, AZ::ConsoleFunctorFlags::Null,
    "If true, prints bandwidth values over entities that use a considerable amount of network traffic");
AZ_CVAR(uint16_t, net_DebutAuditTrail_HistorySize, 20, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Length of networking debug Audit Trail");

namespace Multiplayer
{
    void MultiplayerDebugSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerDebugSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void MultiplayerDebugSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerDebugSystemComponent"));
    }

    void MultiplayerDebugSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        ;
    }

    void MultiplayerDebugSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile)
    {
        incompatbile.push_back(AZ_CRC_CE("MultiplayerDebugSystemComponent"));
    }

    void MultiplayerDebugSystemComponent::Activate()
    {
#ifdef IMGUI_ENABLED
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::QueryApplicationType, m_applicationType);
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
        m_networkMetrics = AZStd::make_unique<MultiplayerDebugNetworkMetrics>();
        m_multiplayerMetrics = AZStd::make_unique<MultiplayerDebugMultiplayerMetrics>();
#endif
    }

    void MultiplayerDebugSystemComponent::Deactivate()
    {
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif
    }

    void MultiplayerDebugSystemComponent::ShowEntityBandwidthDebugOverlay()
    {
#ifdef IMGUI_ENABLED
        m_reporter = AZStd::make_unique<MultiplayerDebugPerEntityReporter>();
#endif
    }

    void MultiplayerDebugSystemComponent::HideEntityBandwidthDebugOverlay()
    {
#ifdef IMGUI_ENABLED
        m_reporter.reset();
#endif
    }

    void MultiplayerDebugSystemComponent::AddAuditEntry(
        [[maybe_unused]] const AuditCategory category,
        [[maybe_unused]] const ClientInputId inputId,
        [[maybe_unused]] const HostFrameId frameId,
        [[maybe_unused]] const AZStd::string& name,
        [[maybe_unused]] AZStd::vector<MultiplayerAuditingElement>&& entryDetails)
    {
#ifdef IMGUI_ENABLED
        while (m_auditTrailElems.size() >= net_DebutAuditTrail_HistorySize)
        {
            m_auditTrailElems.pop_back();
        }

        m_auditTrailElems.emplace_front(category, inputId, frameId, name, AZStd::move(entryDetails));

        if (category == AuditCategory::Desync)
        {
            while (m_auditTrailElems.size() > 0)
            {
                m_pendingAuditTrail.push_front(AZStd::move(m_auditTrailElems.back()));
                m_auditTrailElems.pop_back();
            }

            while (m_pendingAuditTrail.size() >= net_DebutAuditTrail_HistorySize)
            {
                m_pendingAuditTrail.pop_back();
            }
        }
#endif
    }

#ifdef IMGUI_ENABLED
    void MultiplayerDebugSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Multiplayer"))
        {
            ImGui::Checkbox("Networking Stats", &m_displayNetworkingStats);
            ImGui::Checkbox("Multiplayer Stats", &m_displayMultiplayerStats);
            ImGui::Checkbox("Multiplayer Entity Stats", &m_displayPerEntityStats);
            ImGui::Checkbox("Multiplayer Hierarchy Debugger", &m_displayHierarchyDebugger);
            ImGui::Checkbox("Multiplayer Audit Trail", &m_displayNetAuditTrail);

            if (auto multiplayerInterface = AZ::Interface<IMultiplayer>::Get(); multiplayerInterface && !m_applicationType.IsEditor())
            {
                if (auto console = AZ::Interface<AZ::IConsole>::Get())
                {
                    const MultiplayerAgentType multiplayerAgentType = multiplayerInterface->GetAgentType();

                    // Enable the host level selection menu if we're neither a host nor client, or if we are hosting, but haven't loaded a level yet.
                    const bool isLevelLoaded = AzFramework::LevelSystemLifecycleInterface::Get()->IsLevelLoaded();
                    const bool isHosting = (multiplayerAgentType == MultiplayerAgentType::ClientServer) || (multiplayerAgentType == MultiplayerAgentType::DedicatedServer);
                    const bool enableHostLevelSelection = multiplayerAgentType == MultiplayerAgentType::Uninitialized || (isHosting && !isLevelLoaded);

                    if (ImGui::BeginMenu(HostLevelMenuTitle, enableHostLevelSelection))
                    {
                        // Run through all the assets in the asset catalog and gather up the list of level assets
                        AZ::Data::AssetType levelAssetType = azrtti_typeid<AzFramework::Spawnable>();
                        AZStd::set<AZStd::string> multiplayerLevelFilePaths;
                        auto enumerateCB =
                            [levelAssetType, &multiplayerLevelFilePaths]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& assetInfo)
                        {
                            // Skip everything that isn't a spawnable
                            if (assetInfo.m_assetType != levelAssetType)
                            {
                                return;
                            }

                            // Skip non-network spawnables
                            // A network spawnable is serialized to file as a ".network.spawnable". (See Multiplayer Gem's MultiplayerConstants.h)
                            if (!assetInfo.m_relativePath.ends_with(Multiplayer::NetworkSpawnableFileExtension))
                            {
                                return;   
                            }

                            // Skip spawnables not inside the levels folder
                            if (!assetInfo.m_relativePath.starts_with("levels"))
                            {
                                return;
                            }

                            // Skip spawnables that live inside level folders, but isn't the level itself
                            AZ::IO::PathView spawnableFilePath(assetInfo.m_relativePath);
                            AZ::IO::PathView filenameSansExtension = spawnableFilePath.Stem().Stem(); // Just the filename without the .network.spawnable extension
                            
                            AZ::IO::PathView::const_iterator parentFolderName = spawnableFilePath.end();
                            AZStd::advance(parentFolderName, -2);
                            if (parentFolderName->Native() != filenameSansExtension.Native())
                            {
                                return;
                            }

                            AZStd::string multiplayerLevelFilePath = assetInfo.m_relativePath;
                            AZ::StringFunc::Replace(multiplayerLevelFilePath, Multiplayer::NetworkFileExtension.data(), "");
                            multiplayerLevelFilePaths.emplace(multiplayerLevelFilePath);
                        };

                        AZ::Data::AssetCatalogRequestBus::Broadcast(
                            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

                        if (!multiplayerLevelFilePaths.empty())
                        {
                            int levelIndex = 0;
                            for (const auto& multiplayerLevelFilePath : multiplayerLevelFilePaths)
                            {
                                auto levelMenuItem = AZStd::string::format("%d- %s", levelIndex, multiplayerLevelFilePath.c_str());
                                if (ImGui::MenuItem(levelMenuItem.c_str()))
                                {
                                    AZ::TickBus::QueueFunction(
                                        [console, multiplayerLevelFilePath, isHosting]()
                                        {
                                            auto loadLevelString = AZStd::string::format("LoadLevel %s", multiplayerLevelFilePath.c_str());

                                            if (!isHosting)
                                            {
                                                console->PerformCommand("host");
                                            }

                                            console->PerformCommand(loadLevelString.c_str());
                                        });
                                }
                                ++levelIndex;
                            }
                        }
                        else
                        {
                            ImGui::MenuItem(NoMultiplayerLevelsFound);
                        }
                        
                        ImGui::EndMenu();
                    }
                    
                    // Disable the launch local client button if we're not hosting, or if even if we are hosting, but haven't loaded a level yet.
                    if (!isHosting || !isLevelLoaded)
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    }

                    if (ImGui::Button(LaunchLocalClientButtonTitle))
                    {
                        console->PerformCommand("sv_launch_local_client");
                    }

                    if (!isHosting || !isLevelLoaded)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }
                }
            }
            ImGui::EndMenu();
        }
    }

    void MultiplayerDebugSystemComponent::OnImGuiUpdate()
    {
        bool displaying = m_displayNetworkingStats || m_displayMultiplayerStats || m_displayPerEntityStats || m_displayHierarchyDebugger ||
            m_displayNetAuditTrail;

        // Get the default ImGui pass.
        AZ::Render::ImGuiPass* defaultImGuiPass = nullptr;
        AZ::Render::ImGuiSystemRequestBus::BroadcastResult(
            defaultImGuiPass, &AZ::Render::ImGuiSystemRequestBus::Events::GetDefaultImGuiPass);
        if (displaying && defaultImGuiPass)
        {
            if (m_previousSystemCursorState == AzFramework::SystemCursorState::Unknown)
            {
                AzFramework::InputSystemCursorRequestBus::EventResult(
                    m_previousSystemCursorState, AzFramework::InputDeviceMouse::Id,
                    &AzFramework::InputSystemCursorRequests::GetSystemCursorState);
                AzFramework::InputSystemCursorRequestBus::Event(
                    AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                    AzFramework::SystemCursorState::UnconstrainedAndVisible);
            }

            // Create an ImGui context scope using the default ImGui pass context.
            ImGui::ImGuiContextScope contextScope(defaultImGuiPass->GetContext());

            if (m_displayNetworkingStats)
            {
                if (ImGui::Begin("Networking Stats", &m_displayNetworkingStats, ImGuiWindowFlags_None))
                {
                    m_networkMetrics->OnImGuiUpdate();
                }
                ImGui::End();
            }

            if (m_displayMultiplayerStats)
            {
                if (ImGui::Begin("Multiplayer Stats", &m_displayMultiplayerStats, ImGuiWindowFlags_None))
                {
                    m_multiplayerMetrics->OnImGuiUpdate();
                }
                ImGui::End();
            }

            if (m_displayPerEntityStats)
            {
                if (ImGui::Begin("Multiplayer Per Entity Stats", &m_displayPerEntityStats, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    // This overrides @net_DebugNetworkEntity_ShowBandwidth value
                    if (m_reporter == nullptr)
                    {
                        ShowEntityBandwidthDebugOverlay();
                    }

                    if (m_reporter)
                    {
                        m_reporter->OnImGuiUpdate();
                    }
                }
                ImGui::End();
            }

            if (m_displayHierarchyDebugger)
            {
                if (ImGui::Begin("Multiplayer Hierarchy Debugger", &m_displayHierarchyDebugger))
                {
                    if (m_hierarchyDebugger == nullptr)
                    {
                        m_hierarchyDebugger = AZStd::make_unique<MultiplayerDebugHierarchyReporter>();
                    }

                    if (m_hierarchyDebugger)
                    {
                        m_hierarchyDebugger->OnImGuiUpdate();
                    }
                }
                ImGui::End();
            }
            else
            {
                if (m_hierarchyDebugger)
                {
                    m_hierarchyDebugger.reset();
                }
            }

            if (m_displayNetAuditTrail)
            {
                if (ImGui::Begin("Multiplayer Audit Trail", &m_displayNetAuditTrail))
                {
                    if (m_auditTrail == nullptr)
                    {
                        m_lastFilter = "";
                        m_auditTrail = AZStd::make_unique<MultiplayerDebugAuditTrail>();
                        m_committedAuditTrail = m_pendingAuditTrail;
                    }

                    if (m_auditTrail->TryPumpAuditTrail())
                    {
                        m_committedAuditTrail = m_pendingAuditTrail;
                    }

                    FilterAuditTrail();

                    if (m_auditTrail)
                    {
                        if (m_filteredAuditTrail.size() > 0)
                        {
                            m_auditTrail->OnImGuiUpdate(m_filteredAuditTrail);
                        }
                        else
                        {
                            m_auditTrail->OnImGuiUpdate(m_committedAuditTrail);
                        }
                    }
                }
                ImGui::End();
            }
            else
            {
                if (m_auditTrail)
                {
                    m_auditTrail.reset();
                }
            }
        }
        else if (m_previousSystemCursorState != AzFramework::SystemCursorState::Unknown)
        {
            AzFramework::InputSystemCursorRequestBus::Event(
                AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                m_previousSystemCursorState);
            m_previousSystemCursorState = AzFramework::SystemCursorState::Unknown;
        }
    }

    void MultiplayerDebugSystemComponent::FilterAuditTrail()
    {
        if (!m_auditTrail)
        {
            return;
        }

        AZStd::string filter = m_auditTrail->GetAuditTrialFilter();
        if (m_filteredAuditTrail.size() > 0 && filter == m_lastFilter)
        {
            return;
        }
        m_lastFilter = filter;
        m_filteredAuditTrail.clear();

        if (filter.size() == 0)
        {
            return;
        }

        for (auto elem = m_committedAuditTrail.begin(); elem != m_committedAuditTrail.end(); ++elem)
        {
            const char* nodeTitle = "";
            switch (elem->m_category)
            {
            case AuditCategory::Desync:
                nodeTitle = MultiplayerDebugAuditTrail::DESYNC_TITLE;
                break;
            case AuditCategory::Input:
                nodeTitle = MultiplayerDebugAuditTrail::INPUT_TITLE;
                break;
            case AuditCategory::Event:
                nodeTitle = MultiplayerDebugAuditTrail::EVENT_TITLE;
                break;
            }

            // Events only have one item
            if (elem->m_category == AuditCategory::Event)
            {
                if (elem->m_children.size() > 0 && elem->m_children.front().m_elements.size() > 0)
                {
                    if (AZStd::string::format(nodeTitle, elem->m_name.c_str()).contains(filter))
                    {
                        m_filteredAuditTrail.push_back(*elem);
                    }
                    else
                    {
                        AZStd::pair<AZStd::string, AZStd::string> cliServValues =
                            elem->m_children.front().m_elements.front()->GetClientServerValues();
                        if (AZStd::string::format(
                            "%d %d %s %s", static_cast<uint16_t>(elem->m_inputId), static_cast<uint32_t>(elem->m_hostFrameId),
                            cliServValues.first.c_str(), cliServValues.second.c_str())
                            .contains(filter))
                        {
                            m_filteredAuditTrail.push_back(*elem);
                        }
                    }
                }
            }
            // Desyncs and inputs can contain multiple line items
            else
            {
                if (AZStd::string::format(nodeTitle, elem->m_name.c_str()).contains(filter))
                {
                    m_filteredAuditTrail.push_back(*elem);
                }
                else if (AZStd::string::format("%hu %d", static_cast<uint16_t>(elem->m_inputId), static_cast<uint32_t>(elem->m_hostFrameId))
                    .contains(filter))
                {
                    m_filteredAuditTrail.push_back(*elem);
                }
                else
                {
                    // Attempt to construct a filtered input
                    Multiplayer::AuditTrailInput filteredInput(
                        elem->m_category, elem->m_inputId, elem->m_hostFrameId, elem->m_name, {});

                    for (const auto& child : elem->m_children)
                    {
                        if (child.m_name.contains(filter))
                        {
                            filteredInput.m_children.push_back(child);
                        }
                        else if (child.m_elements.size() > 0)
                        {
                            MultiplayerAuditingElement filteredChild;
                            filteredChild.m_name = child.m_name;

                            for (const auto& childElem : child.m_elements)
                            {
                                AZStd::pair<AZStd::string, AZStd::string> cliServValues = childElem->GetClientServerValues();
                                if (AZStd::string::format(
                                    "%s %s %s", childElem->GetName().c_str(), cliServValues.first.c_str(),
                                    cliServValues.second.c_str())
                                    .contains(filter))
                                {
                                    filteredChild.m_elements.push_back(childElem.get()->Clone());
                                }
                            }

                            if (filteredChild.m_elements.size() > 0)
                            {
                                filteredInput.m_children.push_back(filteredChild);
                            }
                        }
                    }

                    if (filteredInput.m_children.size() > 0 || elem->m_category == AuditCategory::Desync)
                    {
                        m_filteredAuditTrail.push_back(filteredInput);
                    }
                }
            }
        }
    }
#endif
}

void OnDebugEntities_ShowBandwidth_Changed(const bool& showBandwidth)
{
    if (showBandwidth)
    {
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->ShowEntityBandwidthDebugOverlay();
    }
    else
    {
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->HideEntityBandwidthDebugOverlay();
    }
}

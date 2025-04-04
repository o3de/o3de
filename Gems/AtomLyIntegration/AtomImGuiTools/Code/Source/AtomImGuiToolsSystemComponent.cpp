/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomImGuiToolsSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI.Profiler/GraphicsProfilerBus.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <AzFramework/Components/ConsoleBus.h>
#include <ImGuiBus.h>

namespace AtomImGuiTools
{
    void AtomImGuiToolsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomImGuiToolsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomImGuiToolsSystemComponent>("AtomImGuiTools", "[Manager of various Atom ImGui tools.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomImGuiToolsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomImGuiToolsService"));
    }

    void AtomImGuiToolsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomImGuiToolsService"));
    }

    void AtomImGuiToolsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AtomImGuiToolsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AtomImGuiToolsSystemComponent::Activate()
    {
#if defined(IMGUI_ENABLED)
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
        AtomImGuiToolsRequestBus::Handler::BusConnect();

        // load switchable render pipeline paths from setting registry
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        const char* settingName = "/O3DE/Viewport/SwitchableRenderPipelines";
        if (settingsRegistry)
        {
            settingsRegistry->GetObject<AZStd::set<AZStd::string>>(m_switchableRenderPipelines, settingName);
        }           
#endif
        CrySystemEventBus::Handler::BusConnect();
    }

    void AtomImGuiToolsSystemComponent::Deactivate()
    {
#if defined(IMGUI_ENABLED)
        m_imguiPassTree.Reset();
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
        AtomImGuiToolsRequestBus::Handler::BusDisconnect();
#endif

        CrySystemEventBus::Handler::BusDisconnect();
    }

#if defined(IMGUI_ENABLED)
    void AtomImGuiToolsSystemComponent::OnImGuiUpdate()
    {
        if (m_showPassTree)
        {
            m_imguiPassTree.Draw(m_showPassTree, AZ::RPI::PassSystemInterface::Get()->GetRootPass().get());
        }
        if (m_showGpuProfiler)
        {
            m_imguiGpuProfiler.Draw(m_showGpuProfiler, AZ::RPI::PassSystemInterface::Get()->GetRootPass().get());
        }
        if (m_showTransientAttachmentProfiler)
        {
            auto transientStats = AZ::RHI::RHIMemoryStatisticsInterface::Get()->GetTransientAttachmentStatistics();
            if (!transientStats.empty())
            {
                m_showTransientAttachmentProfiler = m_imguiTransientAttachmentProfiler.Draw(transientStats);
            }
        }
        m_showMaterialDetails = m_imguiMaterialDetails.Tick(m_materialDetailsController.GetMeshDrawPackets(), m_materialDetailsController.GetSelectionName().c_str());
    }

    void AtomImGuiToolsSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Atom Tools"))
        {
            if (ImGui::MenuItem("Dump loaded Asset info", ""))
            {
                AZ::Data::AssetManager::Instance().DumpLoadedAssetsInfo();
            }

            ImGui::MenuItem("Pass Viewer", "", &m_showPassTree);
            ImGui::MenuItem("Gpu Profiler", "", &m_showGpuProfiler);
            if (ImGui::MenuItem("Transient Attachment Profiler", "", &m_showTransientAttachmentProfiler))
            {
                AZ::RHI::RHISystemInterface::Get()->ModifyFrameSchedulerStatisticsFlags(
                    AZ::RHI::FrameSchedulerStatisticsFlags::GatherTransientAttachmentStatistics, m_showTransientAttachmentProfiler);
            }
            if (ImGui::MenuItem("Material Shader Details", "", &m_showMaterialDetails))
            {
                if (m_showMaterialDetails)
                {
                    m_imguiMaterialDetails.OpenDialog();
                }
                else
                {
                    m_imguiMaterialDetails.CloseDialog();
                }
            }
            if (ImGui::MenuItem("Trigger GPU Capture", "", false, AZ::RHI::GraphicsProfilerBus::HasHandlers()))
            {
                AZ::RHI::GraphicsProfilerBus::Broadcast(&AZ::RHI::GraphicsProfilerBus::Events::TriggerCapture);
            }
            ImGui::EndMenu();
        }

        if (m_switchableRenderPipelines.size() > 0)
        {
            if (ImGui::BeginMenu("Render Pipelines"))
            {
                for (const auto& renderPipelinePath : m_switchableRenderPipelines)
                {
                    if (ImGui::MenuItem(renderPipelinePath.c_str()))
                    {
                        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("r_renderPipelinePath", { renderPipelinePath });
                    }
                }
                ImGui::EndMenu();
            }
        }
    }
    
    void AtomImGuiToolsSystemComponent::ShowMaterialShaderDetailsForEntity(AZ::EntityId entity, bool autoOpenDialog)
    {
        m_materialDetailsController.SetSelectedEntityId(entity);

        if (autoOpenDialog)
        {
            m_imguiMaterialDetails.OpenDialog();
            m_showMaterialDetails = true;
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::ToggleToImGuiVisibleState, ImGui::DisplayState::Visible);
        }
    }

#endif

    void AtomImGuiToolsSystemComponent::OnCryEditorInitialized()
    {
        AzFramework::ConsoleRequestBus::Broadcast(&AzFramework::ConsoleRequestBus::Events::ExecuteConsoleCommand, "imgui_DiscreteInputMode 1");
    }
} // namespace AtomImGuiTools

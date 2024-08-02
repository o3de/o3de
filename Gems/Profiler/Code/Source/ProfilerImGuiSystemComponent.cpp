/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProfilerImGuiSystemComponent.h>

#include "ImGuiTreemapImpl.h"

#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzFramework/Components/ConsoleBus.h>
#if defined (CARBONATED)
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/Device.h>
#endif

namespace Profiler
{
    static constexpr AZ::Crc32 profilerImGuiServiceCrc = AZ_CRC_CE("ProfilerImGuiService");

    void ProfilerImGuiSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ProfilerImGuiSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ProfilerImGuiSystemComponent>("ProfilerImGui", "Provides in-game visualization of the performance data gathered by the ProfilerSystemComponent")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void ProfilerImGuiSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(profilerImGuiServiceCrc);
    }

    void ProfilerImGuiSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(profilerImGuiServiceCrc);
    }

    void ProfilerImGuiSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ProfilerImGuiSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ProfilerImGuiSystemComponent::ProfilerImGuiSystemComponent()
    {
#if defined(IMGUI_ENABLED)
        if (ProfilerImGuiInterface::Get() == nullptr)
        {
            ProfilerImGuiInterface::Register(this);
        }

        if (!ImGuiTreemapFactory::Interface::Get())
        {
            ImGuiTreemapFactory::Interface::Register(&m_imguiTreemapFactory);
        }
#endif // defined(IMGUI_ENABLED)
    }

    ProfilerImGuiSystemComponent::~ProfilerImGuiSystemComponent()
    {
#if defined(IMGUI_ENABLED)
        if (ImGuiTreemapFactory::Interface::Get() == &m_imguiTreemapFactory)
        {
            ImGuiTreemapFactory::Interface::Unregister(&m_imguiTreemapFactory);
        }

        if (ProfilerImGuiInterface::Get() == this)
        {
            ProfilerImGuiInterface::Unregister(this);
        }
#endif // defined(IMGUI_ENABLED)
    }

    void ProfilerImGuiSystemComponent::Activate()
    {
#if defined(IMGUI_ENABLED)
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif // defined(IMGUI_ENABLED)
    }

    void ProfilerImGuiSystemComponent::Deactivate()
    {
#if defined(IMGUI_ENABLED)
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif // defined(IMGUI_ENABLED)
    }

#if defined(IMGUI_ENABLED)
    void ProfilerImGuiSystemComponent::ShowCpuProfilerWindow(bool& keepDrawing)
    {
        m_imguiCpuProfiler.Draw(keepDrawing);
#if defined (CARBONATED)
        if (!keepDrawing)
        {
            AZ::RHI::Device* pDevice = AZ::RHI::RHISystemInterface::Get()->GetDevice();
            pDevice->DisableGatheringStats();
        }
#endif
    }

    void ProfilerImGuiSystemComponent::AddExternalProfilerTimingData(const ProfilerExternalTimingData& externalTimingData)
    {
        m_imguiCpuProfiler.AddExternalTimingEntries(externalTimingData);
    }

    void ProfilerImGuiSystemComponent::OnImGuiUpdate()
    {
        if (m_showCpuProfiler)
        {
            ShowCpuProfilerWindow(m_showCpuProfiler);
        }
    }

    void ProfilerImGuiSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Profiler"))
        {
            if (ImGui::MenuItem("CPU", "", &m_showCpuProfiler))
            {
                AZ::Debug::ProfilerSystemInterface::Get()->SetActive(m_showCpuProfiler);
#if defined (CARBONATED)
                if (m_showCpuProfiler)
                {
                    AZ::RHI::Device* pDevice = AZ::RHI::RHISystemInterface::Get()->GetDevice();
                    pDevice->EnableGatheringStats();
                }
#endif
            }
            ImGui::EndMenu();
        }
    }
#endif // defined(IMGUI_ENABLED)
} // namespace Profiler

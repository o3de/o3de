/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ImguiAtomSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/Feature/ImGui/ImGuiUtils.h>
#include <Atom/Feature/ImGui/SystemBus.h>
#include <Atom/RPI.Public/ViewportContext.h>

#if defined(IMGUI_ENABLED)
#include <ImGuiBus.h>
#endif

namespace AZ
{
    namespace LYIntegration
    {
        void ImguiAtomSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImguiAtomSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        void ImguiAtomSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("CommonService"));
        }

        void ImguiAtomSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile)
        {
            incompatbile.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::Activate()
        {
            ImGui::OtherActiveImGuiRequestBus::Handler::BusConnect();

            auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
            AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(contextName);

            m_initialized = false;
            InitializeViewportSizeIfNeeded();
        }

        void ImguiAtomSystemComponent::Deactivate()
        {
            ImGui::OtherActiveImGuiRequestBus::Handler::BusDisconnect();
            AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
        }

        void ImguiAtomSystemComponent::InitializeViewportSizeIfNeeded()
        {
#if defined(IMGUI_ENABLED)
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManagerListener::SetResolutionMode, ImGui::ImGuiResolutionMode::LockToResolution);
            if (m_initialized)
            {
                return;
            }
            auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            auto defaultViewportContext = atomViewportRequests->GetDefaultViewportContext();
            if (defaultViewportContext)
            {
                // If this succeeds, m_initialized will be set to true.
                OnViewportSizeChanged(defaultViewportContext->GetViewportSize());
            }
#endif
        }

        void ImguiAtomSystemComponent::RenderImGuiBuffers(const ImDrawData& drawData)
        {
            Render::ImGuiSystemRequestBus::Broadcast(&Render::ImGuiSystemRequests::RenderImGuiBuffersToCurrentViewport, drawData);
        }

        void ImguiAtomSystemComponent::OnRenderTick()
        {
#if defined(IMGUI_ENABLED)
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManagerListener::Render);
            InitializeViewportSizeIfNeeded();
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManagerListener::Render);
#endif
        }

        void ImguiAtomSystemComponent::OnViewportSizeChanged(AzFramework::WindowSize size)
        {
#if defined(IMGUI_ENABLED)
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManagerListener::SetImGuiRenderResolution, ImVec2{aznumeric_cast<float>(size.m_width), aznumeric_cast<float>(size.m_height)});
            ImGui::ImGuiManagerBus::Broadcast([this, size](ImGui::ImGuiManagerBus::Events* imgui)
            {
                imgui->OverrideRenderWindowSize(size.m_width, size.m_height);
                // ImGuiManagerBus may not have been connected when this system component is activated
                // as ImGuiManager is not part of a system component we can  require and instead just listens for ESYSTEM_EVENT_GAME_POST_INIT.
                // Let our ImguiAtomSystemComponent know once we successfully connect and update the viewport size.
                if (!m_initialized)
                {
                    m_initialized = true;
                }
            });
#endif
        }
    }
}
/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ImguiAtomSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/Feature/ImGui/ImGuiUtils.h>
#include <Atom/Feature/ImGui/SystemBus.h>
#include <Atom/RPI.Public/ViewportContext.h>

#if defined(IMGUI_ENABLED)
#include <ImGuiBus.h>
#endif

namespace AZ
{
    namespace LYIntegration
    {
        void ImguiAtomSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImguiAtomSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        void ImguiAtomSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("CommonService"));
        }

        void ImguiAtomSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile)
        {
            incompatbile.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::Activate()
        {
            ImGui::OtherActiveImGuiRequestBus::Handler::BusConnect();

            auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
            AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(contextName);

#if defined(IMGUI_ENABLED)
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManagerListener::SetResolutionMode, ImGui::ImGuiResolutionMode::LockToResolution);
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::SetResolutionMode, ImGui::ImGuiResolutionMode::LockToResolution);
            auto defaultViewportContext = atomViewportRequests->GetDefaultViewportContext();
            if (defaultViewportContext)
            {
                OnViewportSizeChanged(defaultViewportContext->GetViewportSize());
            }
#endif
        }

        void ImguiAtomSystemComponent::Deactivate()
        {
            ImGui::OtherActiveImGuiRequestBus::Handler::BusDisconnect();
            AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
        }

        void ImguiAtomSystemComponent::RenderImGuiBuffers(const ImDrawData& drawData)
        {
            Render::ImGuiSystemRequestBus::Broadcast(&Render::ImGuiSystemRequests::RenderImGuiBuffersToCurrentViewport, drawData);
        }

        void ImguiAtomSystemComponent::OnRenderTick()
        {
#if defined(IMGUI_ENABLED)
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManagerListener::Render);
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::Render);
#endif
        }

        void ImguiAtomSystemComponent::OnViewportSizeChanged(AzFramework::WindowSize size)
        {
#if defined(IMGUI_ENABLED)
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManagerListener::SetImGuiRenderResolution, ImVec2{aznumeric_cast<float>(size.m_width), aznumeric_cast<float>(size.m_height)});
            ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::SetImGuiRenderResolution, ImVec2{aznumeric_cast<float>(size.m_width), aznumeric_cast<float>(size.m_height)});
#endif
        }
    }
}

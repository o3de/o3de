/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <imgui/imgui.h>
#include <Atom/Feature/ImGui/SystemBus.h>

namespace AZ
{
    namespace Render
    {
        namespace ImGuiUtils
        {
            // Inline to ensure this executes in the caller's dll, since SetCurrentContext must be called on the same dll.
            inline void PrepareActiveContext()
            {
                ImGuiContext* imguiContext;
                ImGuiSystemRequestBus::BroadcastResult(imguiContext, &ImGuiSystemRequests::GetActiveContext);
                ImGui::SetCurrentContext(imguiContext);
            }
        }

        //! This class handles pushing a new context onto the active context stack, enabling it, then popping it from the stack when it's
        //! deleted. It's useful to make sure a specific context is set in some scope, and that the previous context is restored once
        //! that scope ends.
        class ImGuiActiveContextScope
            : private ImGuiSystemNotificationBus::Handler
        {
        public:
            ImGuiActiveContextScope() = default;
            ~ImGuiActiveContextScope()
            {
                PopContextIfEnabled();
                ImGuiSystemNotificationBus::Handler::BusDisconnect();
            }

            //! Sets the active context based on the current default ImGui pass. If no default ImGui pass is set, then do nothing.
            static ImGuiActiveContextScope FromDefaultPass()
            {
                ImGuiActiveContextScope scope;
                scope.ConnectToImguiNotificationBus();
                ImGuiSystemRequestBus::BroadcastResult(scope.m_isEnabled, &ImGuiSystemRequests::PushActiveContextFromDefaultPass);
                return scope;
            }

            //! Sets the active context based on the provided pass hierarchy filter. If the filter doesn't match exactly one pass, then do nothing.
            static ImGuiActiveContextScope FromPass(const AZStd::vector<AZStd::string>& passHierarchy)
            {
                ImGuiActiveContextScope scope;
                scope.ConnectToImguiNotificationBus();
                ImGuiSystemRequestBus::BroadcastResult(scope.m_isEnabled, &ImGuiSystemRequests::PushActiveContextFromPass, passHierarchy);
                return scope;
            }

            ImGuiActiveContextScope(ImGuiActiveContextScope&& other)
            {
                ImGuiSystemNotificationBus::Handler::BusDisconnect();
                if (!m_isEnabled && other.m_isEnabled)
                {
                    ImGuiSystemNotificationBus::Handler::BusConnect();
                }
                m_isEnabled = other.m_isEnabled;
                other.m_isEnabled = false;
            }

            ImGuiActiveContextScope& operator=(ImGuiActiveContextScope&& other)
            {
                if (this != &other)
                {
                    PopContextIfEnabled();
                    m_isEnabled = other.m_isEnabled;
                    other.m_isEnabled = false;
                }
                return *this;
            }

            //! Returns true if enabled. If not enabled, then the active context won't be changed on destruction.
            bool IsEnabled() const
            {
                return m_isEnabled;
            }

        private:

            void ConnectToImguiNotificationBus()
            {
                ImGuiSystemNotificationBus::Handler::BusConnect();
            }

            // AZ::Render::ImGuiSystemNotificationBus::Handler overrides ...
            void ActiveImGuiContextChanged(ImGuiContext* context) override
            {
                // Assume that any active context changes that happen during the scope's lifetime are due to pass system changes (like reloads) which should be respected.
                ImGui::SetCurrentContext(context);
            }

            void PopContextIfEnabled()
            {
                if (this->m_isEnabled)
                {
                    ImGuiSystemRequestBus::Broadcast(&ImGuiSystemRequests::PopActiveContext);
                }
            }

            bool m_isEnabled = false;
        };

    } // namespace Render
} // namespace AZ

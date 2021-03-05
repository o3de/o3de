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

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <imgui/imgui.h>

namespace AZ
{
    namespace RPI
    {
        class PassHierarchyFilter;
    }

    namespace Render
    {
        class ImGuiPass;

        class ImGuiSystemRequests
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Sets the size scale on all existing ImGui passes
            virtual void SetGlobalSizeScale(float scale) = 0;
            //! Sets the font scale on all existing ImGui passes
            virtual void SetGlobalFontScale(float scale) = 0;

            //! Disables all ImGui passes
            virtual void HideAllImGuiPasses() = 0;
            //! Enables all ImGui passes
            virtual void ShowAllImGuiPasses() = 0;

            //! Sets the provided ImGuiPass as the default pass. This is usually called by an ImGuiPass to mark itself as default.
            virtual void PushDefaultImGuiPass(ImGuiPass* imguiPass) = 0;
            //! Removes a default imgui pass from the stack. If the removed pass context is currently active, is is deactivated and the next pass on the stack is activated.
            virtual void RemoveDefaultImGuiPass(ImGuiPass* imguiPass) = 0;
            //! Gets the ImGuiPass currently set as the default pass. Will return nullptr if no default pass is set.
            virtual ImGuiPass* GetDefaultImGuiPass() = 0;

            //! Pushes whichever ImGui pass is default on the top of the active context stack. Returns true/false for success/fail.
            virtual bool PushActiveContextFromDefaultPass() = 0;
            //! Pushes whichever ImGui pass is provided in passHierarchy on the top of the active context stack. Returns true/false for success/fail.
            virtual bool PushActiveContextFromPass(const RPI::PassHierarchyFilter& passHierarchy) = 0;
            //! Pops the active context off the top of the active context stack. Returns true if there's a context to pop.
            virtual bool PopActiveContext() = 0;
            //! Gets the context at the top of the active context stack. Returns nullptr if the stack is emtpy.
            virtual ImGuiContext* GetActiveContext() = 0;

            //! Allows draw data from a different context to be passed in and rendered to the default context. ImDrawData must be from the ImGui version.
            virtual bool RenderImGuiBuffersToDefaultPass(const ImDrawData& drawData) = 0;
        };

        using ImGuiSystemRequestBus = AZ::EBus<ImGuiSystemRequests>;

        class ImGuiSystemNotifications
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            //! Sent when the active context changes so thet listners can prepare the active context
            virtual void ActiveImGuiContextChanged(ImGuiContext* context) = 0;
        };

        using ImGuiSystemNotificationBus = AZ::EBus<ImGuiSystemNotifications>;

    } // namespace Render
} // namespace AZ

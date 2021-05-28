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

#include <AzCore/Component/Component.h>

#ifdef IMGUI_ENABLED
#   include <imgui/imgui.h>
#   include <ImGuiBus.h>
#endif

namespace Multiplayer
{
    class MultiplayerDebugSystemComponent final
        : public AZ::Component
#ifdef IMGUI_ENABLED
        , public ImGui::ImGuiUpdateListenerBus::Handler
#endif
    {
    public:
        AZ_COMPONENT(MultiplayerDebugSystemComponent, "{060BF3F1-0BFE-4FCE-9C3C-EE991F0DA581}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile);

        ~MultiplayerDebugSystemComponent() override = default;

        //! AZ::Component overrides
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

#ifdef IMGUI_ENABLED
        //! ImGui::ImGuiUpdateListenerBus overrides
        //! @{
        void OnImGuiMainMenuUpdate() override;
        void OnImGuiUpdate() override;
        //! @}
#endif
    private:
        bool m_displayNetworkingStats = false;
        bool m_displayMultiplayerStats = false;
    };
}

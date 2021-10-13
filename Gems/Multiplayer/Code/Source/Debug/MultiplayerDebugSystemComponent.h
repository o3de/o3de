/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "MultiplayerDebugHierarchyReporter.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Interface/Interface.h>
#include <Debug/MultiplayerDebugPerEntityReporter.h>
#include <Multiplayer/IMultiplayerDebug.h>

#ifdef IMGUI_ENABLED
#   include <imgui/imgui.h>
#   include <ImGuiBus.h>
#endif

namespace Multiplayer
{
    class MultiplayerDebugSystemComponent final
        : public AZ::Component
        , public AZ::Interface<IMultiplayerDebug>::Registrar
#ifdef IMGUI_ENABLED
        , public ImGui::ImGuiUpdateListenerBus::Handler
#endif
    {
    public:
        AZ_COMPONENT(MultiplayerDebugSystemComponent, "{060BF3F1-0BFE-4FCE-9C3C-EE991F0DA581}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ~MultiplayerDebugSystemComponent() override = default;

        //! AZ::Component overrides
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! IMultiplayerDebug overrides
        //! @{
        void ShowEntityBandwidthDebugOverlay() override;
        void HideEntityBandwidthDebugOverlay() override;
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

        bool m_displayPerEntityStats = false;
        AZStd::unique_ptr<MultiplayerDebugPerEntityReporter> m_reporter;
        
        bool m_displayHierarchyDebugger = false;
        AZStd::unique_ptr<MultiplayerDebugHierarchyReporter> m_hierarchyDebugger;
    };
}

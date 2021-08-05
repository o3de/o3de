/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <ScriptCanvas/PerformanceStatistician.h>

#ifdef IMGUI_ENABLED
#include <imgui/imgui.h>
#include <ImGuiBus.h>
#endif // IMGUI_ENABLED

namespace ScriptCanvasDeveloper
{
    class SystemComponent
        : public AZ::Component
#ifdef IMGUI_ENABLED
        , public ImGui::ImGuiUpdateListenerBus::Handler
#endif // IMGUI_ENABLED
    {
    public:
        AZ_COMPONENT(SystemComponent, "{46BDD372-8E86-4C0F-B12C-DC271C5DCED1}");

        SystemComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

#ifdef IMGUI_ENABLED

        void OnImGuiMainMenuUpdate() override;

        void GraphHistoryListBox();

        void FullPerformanceWindow();

#endif // IMGUI_ENABLED
    private:
        ScriptCanvas::Execution::PerformanceStatistician  m_perfStatistician;
    };
}

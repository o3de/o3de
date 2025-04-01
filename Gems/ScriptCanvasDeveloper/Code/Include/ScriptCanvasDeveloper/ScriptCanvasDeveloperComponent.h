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
#include <ImGuiBus.h>


namespace ScriptCanvas::Developer
{
    class SystemComponent
        : public AZ::Component
        , public ImGui::ImGuiUpdateListenerBus::Handler
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

        // Avoid using IMGUI_ENABLED in any situation that could alter the final vtable or size of this
        // object, as this header may be compiled in different compile units with different defines
        void OnImGuiMainMenuUpdate() override;

#if defined(IMGUI_ENABLED)  
        // Non-overrides / non-virtuals are ok
        void GraphHistoryListBox();
        void FullPerformanceWindow();
#endif // IMGUI_ENABLED

    private:
        ScriptCanvas::Execution::PerformanceStatistician  m_perfStatistician;
    };
}

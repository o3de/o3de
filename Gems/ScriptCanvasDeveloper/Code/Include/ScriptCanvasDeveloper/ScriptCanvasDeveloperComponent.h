/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

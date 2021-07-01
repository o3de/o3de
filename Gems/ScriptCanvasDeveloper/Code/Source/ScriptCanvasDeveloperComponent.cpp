/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvasDeveloper/ScriptCanvasDeveloperComponent.h>

namespace ScriptCanvasDeveloperComponentCpp
{
    bool GetListEntryFromAZStdStringVector(void* data, int idx, const char** out_text)
    {
        AZStd::vector<AZStd::string>* vector = reinterpret_cast<AZStd::vector<AZStd::string>*>(data);

        if (idx < vector->size())
        {
            *out_text = (*vector)[idx].c_str();
            return true;
        }
        else
        {
            return false;
        }
    }
}

namespace ScriptCanvasDeveloper
{
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        ScriptCanvas::Execution::PerformanceStatistician::Reflect(context);
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasDeveloperService", 0xd2edba67));
    }


    void SystemComponent::Init()
    {
    }

    void SystemComponent::Activate()
    {
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif // IMGUI_ENABLED

    }

    void SystemComponent::Deactivate()
    {
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif // IMGUI_ENABLED
    }

#ifdef IMGUI_ENABLED

    void SystemComponent::FullPerformanceWindow()
    {
        GraphHistoryListBox();
    }

    void SystemComponent::GraphHistoryListBox()
    {
        AZStd::vector<AZStd::string> scriptHistory = m_perfStatistician.GetExecutedScriptsSinceLastSnapshot();
        int index = 0;
        const int k_HeightInItemCount = 30;
        ImGui::ListBox(":Graph", &index, &ScriptCanvasDeveloperComponentCpp::GetListEntryFromAZStdStringVector, &scriptHistory, aznumeric_cast<int>(scriptHistory.size()), k_HeightInItemCount);
    }

    void SystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Script Canvas"))
        {
            FullPerformanceWindow();
            ImGui::EndMenu();
        }
    }

#endif // IMGUI_ENABLED

}

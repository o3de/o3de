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

#include <ImGuiBus.h>
#include "ImGuiServerManager.h"

namespace MultiplayerDiagnostics
{
    // GridMate Network Live Analyzer component
    class MultiplayerImGuiSystemComponent
        : public AZ::Component
        , protected ImGui::ImGuiUpdateListenerBus::Handler
    {
    public:
        AZ_COMPONENT(MultiplayerImGuiSystemComponent, "{2C4C2978-2DF7-492D-BBBB-E72D708A216F}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // ImGuiUpdateListenerBus
        void OnImGuiInitialize() override;
        void OnImGuiUpdate() override;

    private:
        AZStd::unique_ptr<ImGuiServerManager> m_reporter;
    };
}

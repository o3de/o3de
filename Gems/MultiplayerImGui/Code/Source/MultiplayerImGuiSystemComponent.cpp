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


#include <MultiplayerImGuiSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace MultiplayerDiagnostics
{
    void MultiplayerImGuiSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MultiplayerImGuiSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MultiplayerImGuiSystemComponent>("MultiplayerImGui", "[GridMate Network Live Analyzer component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MultiplayerImGuiSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MultiplayerImGuiService"));
    }

    void MultiplayerImGuiSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MultiplayerImGuiService"));
    }

    void MultiplayerImGuiSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void MultiplayerImGuiSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void MultiplayerImGuiSystemComponent::Activate()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
    }

    void MultiplayerImGuiSystemComponent::Deactivate()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
    }

    void MultiplayerImGuiSystemComponent::OnImGuiInitialize()
    {
        m_reporter = AZStd::make_unique<ImGuiServerManager>();
    }

    void MultiplayerImGuiSystemComponent::OnImGuiUpdate()
    {
        if (m_reporter)
        {
            m_reporter->OnImGuiUpdate();
        }
    }
}

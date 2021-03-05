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

#include "ImguiAtomSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/Feature/ImGui/ImGuiUtils.h>
#include <Atom/Feature/ImGui/SystemBus.h>

namespace AZ
{
    namespace LYIntegration
    {
        void ImguiAtomSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImguiAtomSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        void ImguiAtomSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("CommonService"));
        }

        void ImguiAtomSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile)
        {
            incompatbile.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::Activate()
        {
            ImGui::OtherActiveImGuiRequestBus::Handler::BusConnect();
        }

        void ImguiAtomSystemComponent::Deactivate()
        {
            ImGui::OtherActiveImGuiRequestBus::Handler::BusDisconnect();
        }

        void ImguiAtomSystemComponent::RenderImGuiBuffers(const ImDrawData& drawData)
        {
            Render::ImGuiSystemRequestBus::Broadcast(&Render::ImGuiSystemRequests::RenderImGuiBuffersToDefaultPass, drawData);
        }
    }
}

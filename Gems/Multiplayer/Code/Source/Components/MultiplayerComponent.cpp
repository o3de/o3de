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

#include <Source/Components/MultiplayerComponent.h>
#include <Source/Components/NetBindComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Multiplayer
{
    void MultiplayerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MultiplayerComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void MultiplayerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetBindService"));
    }

    const NetBindComponent* MultiplayerComponent::GetNetBindComponent() const
    {
        return m_netBindComponent;
    }

    NetBindComponent* MultiplayerComponent::GetNetBindComponent()
    {
        return m_netBindComponent;
    }

    NetEntityId MultiplayerComponent::GetNetEntityId() const
    {
        return m_netBindComponent ? m_netBindComponent->GetNetEntityId() : InvalidNetEntityId;
    }

    NetEntityRole MultiplayerComponent::GetNetEntityRole() const
    {
        return m_netBindComponent ? m_netBindComponent->GetNetEntityRole() : NetEntityRole::InvalidRole;
    }

    ConstNetworkEntityHandle MultiplayerComponent::GetEntityHandle() const
    {
        const NetBindComponent* netBindComponent = GetNetBindComponent();
        return netBindComponent ? netBindComponent->GetEntityHandle() : ConstNetworkEntityHandle();
    }

    NetworkEntityHandle MultiplayerComponent::GetEntityHandle()
    {
        NetBindComponent* netBindComponent = GetNetBindComponent();
        return netBindComponent ? netBindComponent->GetEntityHandle() : NetworkEntityHandle();
    }

    void MultiplayerComponent::MarkDirty()
    {
        NetBindComponent* netBindComponent = GetNetBindComponent();
        if (netBindComponent != nullptr)
        {
            netBindComponent->MarkDirty();
        }
    }
}

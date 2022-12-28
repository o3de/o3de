/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/MultiplayerComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Multiplayer
{
    MultiplayerComponent::MultiplayerComponent()
        : m_networkActivatedHandler([this]() { OnNetworkActivated(); })
    {
        ;
    }

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

    bool MultiplayerComponent::IsNetEntityRoleAuthority() const
    {
        return m_netBindComponent ? m_netBindComponent->IsNetEntityRoleAuthority() : false;
    }

    bool MultiplayerComponent::IsNetEntityRoleAutonomous() const
    {
        return m_netBindComponent ? m_netBindComponent->IsNetEntityRoleAutonomous() : false;
    }

    bool MultiplayerComponent::IsNetEntityRoleServer() const
    {
        return m_netBindComponent ? m_netBindComponent->IsNetEntityRoleServer() : false;
    }

    bool MultiplayerComponent::IsNetEntityRoleClient() const
    {
        return m_netBindComponent ? m_netBindComponent->IsNetEntityRoleClient() : false;
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

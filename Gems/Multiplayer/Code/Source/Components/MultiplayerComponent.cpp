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

#include <Multiplayer/Components/MultiplayerComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
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

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<MultiplayerComponent>("MultiplayerComponent")
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")

                ->Method("Is Authority", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "MultiplayerComponent", false, "MultiplayerComponent IsAuthority failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    MultiplayerComponent* multiplayerComponent = entity->FindComponent<MultiplayerComponent>();
                    if (!multiplayerComponent)
                    {
                        AZ_Warning( "MultiplayerComponent", false, "MultiplayerComponent IsAuthority failed. Entity '%s' (id: %s) is missing a MultiplayerComponent, make sure this entity contains a component which derives from MultiplayerComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return multiplayerComponent->IsAuthority();
                })
                ->Method("Is Autonomous", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "MultiplayerComponent", false, "MultiplayerComponent IsAutonomous failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    MultiplayerComponent* multiplayerComponent = entity->FindComponent<MultiplayerComponent>();
                    if (!multiplayerComponent)
                    {
                        AZ_Warning("MultiplayerComponent", false, "MultiplayerComponent IsAutonomous failed. Entity '%s' (id: %s) is missing a MultiplayerComponent, make sure this entity contains a component which derives from MultiplayerComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return multiplayerComponent->IsAutonomous();
                })
                ->Method("Is Client", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "MultiplayerComponent", false, "MultiplayerComponent IsClient failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    MultiplayerComponent* multiplayerComponent = entity->FindComponent<MultiplayerComponent>();
                    if (!multiplayerComponent)
                    {
                        AZ_Warning("MultiplayerComponent", false, "MultiplayerComponent IsClient failed. Entity '%s' (id: %s) is missing a MultiplayerComponent, make sure this entity contains a component which derives from MultiplayerComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return multiplayerComponent->IsClient();
                })
                ->Method("Is Server", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "MultiplayerComponent", false, "MultiplayerComponent IsServer failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    MultiplayerComponent* multiplayerComponent = entity->FindComponent<MultiplayerComponent>();
                    if (!multiplayerComponent)
                    {
                        AZ_Warning("MultiplayerComponent", false, "MultiplayerComponent IsServer failed. Entity '%s' (id: %s) is missing a MultiplayerComponent, make sure this entity contains a component which derives from MultiplayerComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return multiplayerComponent->IsServer();
                })
            ;
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

    bool MultiplayerComponent::IsAuthority() const
    {
        return m_netBindComponent ? m_netBindComponent->IsAuthority() : false;
    }

    bool MultiplayerComponent::IsAutonomous() const
    {
        return m_netBindComponent ? m_netBindComponent->IsAutonomous() : false;
    }

    bool MultiplayerComponent::IsServer() const
    {
        return m_netBindComponent ? m_netBindComponent->IsServer() : false;
    }

    bool MultiplayerComponent::IsClient() const
    {
        return m_netBindComponent ? m_netBindComponent->IsClient() : false;
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

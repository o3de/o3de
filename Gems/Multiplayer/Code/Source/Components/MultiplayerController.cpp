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

#include <Multiplayer/Components/MultiplayerController.h>
#include <Multiplayer/Components/MultiplayerComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>

namespace Multiplayer
{
    MultiplayerController::MultiplayerController(MultiplayerComponent& owner)
        : m_owner(owner)
    {
        ;
    }

    NetEntityId MultiplayerController::GetNetEntityId() const
    {
        return m_owner.GetNetEntityId();
    }

    bool MultiplayerController::IsAuthority() const
    {
        return GetNetBindComponent() ? GetNetBindComponent()->IsAuthority() : false;
    }

    bool MultiplayerController::IsAutonomous() const
    {
        return GetNetBindComponent() ? GetNetBindComponent()->IsAutonomous() : false;
    }

    AZ::Entity* MultiplayerController::GetEntity() const
    {
        return m_owner.GetEntity();
    }

    ConstNetworkEntityHandle MultiplayerController::GetEntityHandle() const
    {
        return m_owner.GetEntityHandle();
    }

    NetworkEntityHandle MultiplayerController::GetEntityHandle()
    {
        return m_owner.GetEntityHandle();
    }

    const NetBindComponent* MultiplayerController::GetNetBindComponent() const
    {
        return m_owner.GetNetBindComponent();
    }

    NetBindComponent* MultiplayerController::GetNetBindComponent()
    {
        return m_owner.GetNetBindComponent();
    }

    const MultiplayerComponent& MultiplayerController::GetOwner() const
    {
        return m_owner;
    }

    MultiplayerComponent& MultiplayerController::GetOwner()
    {
        return m_owner;
    }

    bool MultiplayerController::IsProcessingInput() const
    {
        return GetNetBindComponent()->IsProcessingInput();
    }

    MultiplayerController* MultiplayerController::FindController(const AZ::Uuid& typeId, const NetworkEntityHandle& entityHandle) const
    {
        if (const AZ::Entity* entity = entityHandle.GetEntity())
        {
            MultiplayerComponent* component = azrtti_cast<MultiplayerComponent*>(entity->FindComponent(typeId));
            if (component != nullptr)
            {
                return component->GetController();
            }
        }
        return nullptr;
    }
}

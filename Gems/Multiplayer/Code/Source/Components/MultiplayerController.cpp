/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    bool MultiplayerController::IsNetEntityRoleAuthority() const
    {
        return GetNetBindComponent() ? GetNetBindComponent()->IsNetEntityRoleAuthority() : false;
    }

    bool MultiplayerController::IsNetEntityRoleAutonomous() const
    {
        return GetNetBindComponent() ? GetNetBindComponent()->IsNetEntityRoleAutonomous() : false;
    }

    AZ::Entity* MultiplayerController::GetEntity() const
    {
        return m_owner.GetEntity();
    }

    AZ::EntityId MultiplayerController::GetEntityId() const
    {
        return m_owner.GetEntityId();
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

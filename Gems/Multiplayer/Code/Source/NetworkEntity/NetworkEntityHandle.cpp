/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <Multiplayer/Components/MultiplayerController.h>
#include <Multiplayer/Components/MultiplayerComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    ConstNetworkEntityHandle::ConstNetworkEntityHandle(AZ::Entity* entity, const NetworkEntityTracker* networkEntityTracker)
        : m_entity(entity)
        , m_networkEntityTracker((networkEntityTracker != nullptr) ? networkEntityTracker : GetNetworkEntityTracker())
    {
        AZ_Assert(m_networkEntityTracker, "NetworkEntityTracker is not valid");
        m_changeDirty = m_networkEntityTracker->GetChangeDirty(m_entity);

        if (entity)
        {
            m_netBindComponent = m_networkEntityTracker->GetNetBindComponent(entity);
            if (m_netBindComponent != nullptr)
            {
                m_netEntityId = m_netBindComponent->GetNetEntityId();
            }
            else
            {
                *this = ConstNetworkEntityHandle();
            }
        }
    }

    bool ConstNetworkEntityHandle::Exists() const
    {
        if (!m_networkEntityTracker)
        {
            return false;
        }

        const uint32_t changeDirty = m_networkEntityTracker->GetChangeDirty(m_entity);
        if (m_changeDirty != changeDirty)
        {
            // Make sure to get change dirty with updated m_entity
            m_changeDirty = changeDirty;
            AZ::Entity* newEntity = m_networkEntityTracker->GetRaw(m_netEntityId);
            if (newEntity != m_entity)
            {
                // If the entity pointer has changed, update our entity pointer and reset our netBindComponent pointer
                m_entity = newEntity;
                m_netBindComponent = nullptr;
            }
        }
        return m_entity != nullptr;
    }

    AZ::Entity* ConstNetworkEntityHandle::GetEntity()
    {
        if (!Exists())
        {
            return nullptr;
        }
        return m_entity;
    }

    const AZ::Entity* ConstNetworkEntityHandle::GetEntity() const
    {
        if (!Exists())
        {
            return nullptr;
        }
        return m_entity;
    }

    ConstNetworkEntityHandle::operator bool() const
    {
        return Exists();
    }

    bool ConstNetworkEntityHandle::operator==(const ConstNetworkEntityHandle &b) const
    {
        return m_netEntityId == b.m_netEntityId;
    }

    bool ConstNetworkEntityHandle::operator!=(const ConstNetworkEntityHandle &b) const
    {
        return m_netEntityId != b.m_netEntityId;
    }

    bool ConstNetworkEntityHandle::operator<(const ConstNetworkEntityHandle &b) const
    {
        return m_netEntityId < b.m_netEntityId;
    }

    void ConstNetworkEntityHandle::Reset()
    {
        m_entity = nullptr;
        m_netBindComponent = nullptr;
        m_netEntityId = InvalidNetEntityId;
    }

    void ConstNetworkEntityHandle::Reset(const ConstNetworkEntityHandle& handle)
    {
        m_changeDirty = handle.m_changeDirty;
        m_entity = handle.m_entity;
        m_netBindComponent = handle.m_netBindComponent;
        m_networkEntityTracker = handle.m_networkEntityTracker;
        m_netEntityId = handle.m_netEntityId;
    }

    NetBindComponent* ConstNetworkEntityHandle::GetNetBindComponent() const
    {
        if (!Exists())
        {
            return nullptr;
        }
        if (m_netBindComponent == nullptr)
        {
            m_netBindComponent = m_networkEntityTracker->GetNetBindComponent(m_entity);
        }
        return m_netBindComponent;
    }

    const AZ::Component* ConstNetworkEntityHandle::FindComponent(const AZ::TypeId& typeId) const
    {
        if (const AZ::Entity* entity{ GetEntity() })
        {
            return entity->FindComponent(typeId);
        }
        return nullptr;
    }

    MultiplayerController* NetworkEntityHandle::FindController(const AZ::TypeId& typeId)
    {
        if (AZ::Entity* entity{ GetEntity() })
        {
            MultiplayerComponent* component = azrtti_cast<MultiplayerComponent*>(entity->FindComponent(typeId));
            if (component != nullptr)
            {
                return component->GetController();
            }
        }
        return nullptr;
    }

    AZ::Component* NetworkEntityHandle::FindComponent(const AZ::TypeId& typeId)
    {
        return const_cast<AZ::Component*>(const_cast<const ConstNetworkEntityHandle*>(static_cast<ConstNetworkEntityHandle*>(this))->FindComponent(typeId));
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkInput/NetworkInputChild.h>
#include <Multiplayer/IMultiplayer.h>
#include <AzNetworking/Serialization/ISerializer.h>

namespace Multiplayer
{
    NetworkInputChild::NetworkInputChild(const ConstNetworkEntityHandle& entityHandle)
        : m_owner(entityHandle)
    {
        Attach(m_owner);
    }

    NetworkInputChild& NetworkInputChild::operator= (const NetworkInputChild& rhs)
    {
        m_owner = rhs.m_owner;
        m_networkInput = rhs.m_networkInput;
        return *this;
    }

    void NetworkInputChild::Attach(const ConstNetworkEntityHandle& entityHandle)
    {
        m_owner = entityHandle;
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        if (netBindComponent)
        {
            m_networkInput.AttachNetBindComponent(netBindComponent);
        }
    }

    const ConstNetworkEntityHandle& NetworkInputChild::GetOwner() const
    {
        return m_owner;
    }

    const NetworkInput& NetworkInputChild::GetNetworkInput() const
    {
        return m_networkInput;
    }

    NetworkInput& NetworkInputChild::GetNetworkInput()
    {
        return m_networkInput;
    }

    bool NetworkInputChild::Serialize(AzNetworking::ISerializer& serializer)
    {
        NetEntityId ownerId = m_owner.GetNetEntityId();
        serializer.Serialize(ownerId, "OwnerId");
        if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
        {
            m_owner = GetNetworkEntityManager()->GetEntity(ownerId);
        }
        serializer.Serialize(m_networkInput, "NetworkInput");
        return serializer.IsValid();
    }
}

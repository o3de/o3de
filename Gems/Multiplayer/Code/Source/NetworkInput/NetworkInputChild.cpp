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

#include <Source/NetworkInput/NetworkInputChild.h>
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

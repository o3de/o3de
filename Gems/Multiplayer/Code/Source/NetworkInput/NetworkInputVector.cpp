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

#include <Source/NetworkInput/NetworkInputVector.h>
#include <Include/INetworkEntityManager.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Serialization/DeltaSerializer.h>

namespace Multiplayer
{
    NetworkInputVector::NetworkInputVector()
        : m_owner()
        , m_inputs()
    {
        ;
    }

    NetworkInputVector::NetworkInputVector(const ConstNetworkEntityHandle& entityHandle)
        : m_owner(entityHandle)
        , m_inputs()
    {
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        if (netBindComponent)
        {
            for (AZStd::size_t i = 0; i < m_inputs.size(); ++i)
            {
                m_inputs[i].m_networkInput.AttachNetBindComponent(netBindComponent);
            }
        }
    }

    NetworkInput& NetworkInputVector::operator[](uint32_t index)
    {
        return m_inputs[index].m_networkInput;
    }

    const NetworkInput& NetworkInputVector::operator[](uint32_t index) const
    {
        return m_inputs[index].m_networkInput;
    }

    void NetworkInputVector::SetPreviousInputId(ClientInputId previousInputId)
    {
        m_previousInputId = previousInputId;
    }

    ClientInputId NetworkInputVector::GetPreviousInputId() const
    {
        return m_previousInputId;
    }

    bool NetworkInputVector::Serialize(AzNetworking::ISerializer& serializer)
    {
        // Always serialize the full first element
        if (!m_inputs[0].m_networkInput.Serialize(serializer))
        {
            return false;
        }

        // For each subsequent element
        for (uint32_t i = 1; i < m_inputs.size(); ++i)
        {
            if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
            {
                AzNetworking::SerializerDelta deltaSerializer;
                // Read out the delta
                if (!deltaSerializer.Serialize(serializer))
                {
                    return false;
                }
                // Start with previous value
                m_inputs[i].m_networkInput = m_inputs[i - 1].m_networkInput;
                // Then apply delta
                AzNetworking::DeltaSerializerApply applySerializer(deltaSerializer);
                if (!applySerializer.ApplyDelta(m_inputs[i].m_networkInput))
                {
                    return false;
                }
            }
            else
            {
                AzNetworking::SerializerDelta deltaSerializer;
                // Create the delta
                AzNetworking::DeltaSerializerCreate createSerializer(deltaSerializer);
                if (!createSerializer.CreateDelta(m_inputs[i - 1].m_networkInput, m_inputs[i].m_networkInput))
                {
                    return false;
                }
                // Then write out the delta
                if (!deltaSerializer.Serialize(serializer))
                {
                    return false;
                }
            }
        }
        serializer.Serialize(m_previousInputId, "PreviousInputId");
        return true;
    }


    MigrateNetworkInputVector::MigrateNetworkInputVector()
        : m_owner()
    {
        ;
    }

    MigrateNetworkInputVector::MigrateNetworkInputVector(const ConstNetworkEntityHandle& entityHandle)
        : m_owner(entityHandle)
    {
        ;
    }

    uint32_t MigrateNetworkInputVector::GetSize() const
    {
        return aznumeric_cast<uint32_t>(m_inputs.size());
    }

    NetworkInput& MigrateNetworkInputVector::operator[](uint32_t index)
    {
        return m_inputs[index].m_networkInput;
    }

    const NetworkInput& MigrateNetworkInputVector::operator[](uint32_t index) const
    {
        return m_inputs[index].m_networkInput;
    }

    bool MigrateNetworkInputVector::PushBack(const NetworkInput& networkInput)
    {
        if (m_inputs.size() < m_inputs.capacity())
        {
            m_inputs.push_back(networkInput);
            return true;
        }
        return false;
    }

    bool MigrateNetworkInputVector::Serialize(AzNetworking::ISerializer& serializer)
    {
        NetEntityId ownerId = m_owner.GetNetEntityId();
        serializer.Serialize(ownerId, "OwnerId");

        uint32_t inputCount = aznumeric_cast<uint32_t>(m_inputs.size());
        serializer.Serialize(inputCount, "InputCount");

        if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
        {
            // make sure all the possible NetworkInputs get attached prior to serialization, this double sends the size, but this message is only sent on server migration
            m_inputs.resize(inputCount);
            m_owner = GetNetworkEntityManager()->GetEntity(ownerId);
            NetBindComponent* netBindComponent = m_owner.GetNetBindComponent();
            if (netBindComponent)
            {
                for (uint32_t i = 0; i < m_inputs.size(); ++i)
                {
                    m_inputs[i].m_networkInput.AttachNetBindComponent(netBindComponent);
                }
            }
        }
        return serializer.Serialize(m_inputs, "Inputs");
    }
}

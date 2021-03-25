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

#include <Source/NetworkInput/NetworkInput.h>
#include <Source/Components/NetBindComponent.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    NetworkInput::NetworkInput(const NetworkInput& rhs)
        : m_inputId(rhs.m_inputId)
        , m_owner(rhs.m_owner)
    {
        CopyInternal(rhs);
    }

    NetworkInput& NetworkInput::operator= (const NetworkInput& rhs)
    {
        if (&rhs != this)
        {
            CopyInternal(rhs);
        }
        return *this;
    }

    void NetworkInput::SetNetworkInputId(NetworkInputId inputId)
    {
        m_inputId = inputId;
    }

    NetworkInputId NetworkInput::GetNetworkInputId() const
    {
        return m_inputId;
    }


    NetworkInputId& NetworkInput::ModifyNetworkInputId()
    {
        return m_inputId;
    }

    void NetworkInput::AttachNetBindComponent(NetBindComponent* netBindComponent)
    {
        m_wasAttached = true;
        m_componentInputs.clear();
        if (netBindComponent)
        {
            m_owner = netBindComponent->GetEntityHandle();
            m_componentInputs = netBindComponent->AllocateComponentInputs();
        }
    }

    bool NetworkInput::Serialize(AzNetworking::ISerializer& serializer)
    {
        //static_assert(UINT8_MAX >= Multiplayer::ComponentTypes::c_Count, "Expected fewer than 255 components, this code needs to be updated");
        if (!serializer.Serialize(m_inputId, "InputId"))
        {
            return false;
        }

        uint8_t componentInputCount = static_cast<uint8_t>(m_componentInputs.size());
        serializer.Serialize(componentInputCount, "ComponentInputCount");
        m_componentInputs.resize(componentInputCount);
        if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
        {
            for (uint8_t i = 0; i < componentInputCount; ++i)
            {
                // We need to do a little extra work here, the delta serializer won't actually write out values if they were the same as the parent.
                // We need to make sure we don't lose state that is intrinsic to the underlying type
                // The default InvalidNetComponentId is a placeholder, we expect it to be overwritten by the serializer
                // This happens when deserializing a non-delta'd input command
                // However in the delta serializer case, we use the previous input as our initial value
                // which will have the NetworkInputs setup and therefore won't write out the componentId
                NetComponentId componentId = m_componentInputs[i] ? m_componentInputs[i]->GetComponentId() : InvalidNetComponentId;
                serializer.Serialize(componentId, "ComponentType");
                // Create a new input if we don't have one or the types do not match
                if ((m_componentInputs[i] == nullptr) || (componentId != m_componentInputs[i]->GetComponentId()))
                {
                    // TODO: ComponentInput factory, needs multiplayer component architecture and autogen
                    m_componentInputs[i] = nullptr; // ComponentInputFactory(componentId);
                }
                if (!m_componentInputs[i])
                {
                    // If the client tells us a component type that does not have a NetworkInput, they are likely hacking
                    AZLOG_ERROR("Unexpected MultiplayerComponent type, unable to deserialize, dropping message");
                    return false;
                }
                serializer.Serialize(*m_componentInputs[i], "ComponentInput");
            }
            m_wasAttached = true;
        }
        else
        {
            AZ_Assert(m_wasAttached, "AttachNetSystemComponent was never called for NetworkInput");
            // We assume that the order of the network inputs is fixed between the server and client
            for (auto& componentInput : m_componentInputs)
            {
                NetComponentId componentId = componentInput->GetComponentId();
                serializer.Serialize(componentId, "ComponentId");
                serializer.Serialize(*componentInput, "ComponentInput");
            }
        }
        return true;
    }

    const IMultiplayerComponentInput* NetworkInput::FindComponentInput(NetComponentId componentId) const
    {
        // linear search since we expect to have very few components
        for (auto& componentInput : m_componentInputs)
        {
            if (componentInput->GetComponentId() == componentId)
            {
                return componentInput.get();
            }
        }
        return nullptr;
    }

    IMultiplayerComponentInput* NetworkInput::FindComponentInput(NetComponentId componentId)
    {
        const NetworkInput* tmp = this;
        return const_cast<IMultiplayerComponentInput*>(tmp->FindComponentInput(componentId));
    }

    void NetworkInput::CopyInternal(const NetworkInput& rhs)
    {
        m_inputId = rhs.m_inputId;
        m_componentInputs.resize(rhs.m_componentInputs.size());
        for (int i = 0; i < rhs.m_componentInputs.size(); ++i)
        {
            if (m_componentInputs[i] == nullptr || m_componentInputs[i]->GetComponentId() != rhs.m_componentInputs[i]->GetComponentId())
            {
                // TODO: ComponentInput factory, needs multiplayer component architecture and autogen
                m_componentInputs[i] = nullptr; // ComponentInputFactory(rhs.m_componentInputs[i]->GetComponentId());
            }
            *m_componentInputs[i] = *rhs.m_componentInputs[i];
        }
        m_wasAttached = rhs.m_wasAttached;
    }
}

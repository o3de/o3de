/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkInput/NetworkInput.h>
#include <Multiplayer/Components/MultiplayerComponentRegistry.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/IMultiplayer.h>
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

    void NetworkInput::SetClientInputId(ClientInputId inputId)
    {
        m_inputId = inputId;
    }

    ClientInputId NetworkInput::GetClientInputId() const
    {
        return m_inputId;
    }

    ClientInputId& NetworkInput::ModifyClientInputId()
    {
        return m_inputId;
    }

    void NetworkInput::SetHostFrameId(HostFrameId hostFrameId)
    {
        m_hostFrameId = hostFrameId;
    }

    HostFrameId NetworkInput::GetHostFrameId() const
    {
        return m_hostFrameId;
    }

    HostFrameId& NetworkInput::ModifyHostFrameId()
    {
        return m_hostFrameId;
    }

    void NetworkInput::SetHostTimeMs(AZ::TimeMs hostTimeMs)
    {
        m_hostTimeMs = hostTimeMs;
    }

    AZ::TimeMs NetworkInput::GetHostTimeMs() const
    {
        return m_hostTimeMs;
    }

    AZ::TimeMs& NetworkInput::ModifyHostTimeMs()
    {
        return m_hostTimeMs;
    }

    void NetworkInput::SetHostBlendFactor(float hostBlendFactor)
    {
        m_hostBlendFactor = hostBlendFactor;
    }

    float NetworkInput::GetHostBlendFactor() const
    {
        return m_hostBlendFactor;
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
        if (!serializer.Serialize(m_inputId, "InputId")
         || !serializer.Serialize(m_hostTimeMs, "HostTimeMs")
         || !serializer.Serialize(m_hostFrameId, "HostFrameId")
         || !serializer.Serialize(m_hostBlendFactor, "HostBlendFactor"))
        {
            return false;
        }

        uint16_t componentInputCount = static_cast<uint16_t>(m_componentInputs.size());
        serializer.Serialize(componentInputCount, "ComponentInputCount");
        m_componentInputs.resize(componentInputCount);
        if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
        {
            for (uint16_t i = 0; i < componentInputCount; ++i)
            {
                // We need to do a little extra work here, the delta serializer won't actually write out values if they were the same as the parent.
                // We need to make sure we don't lose state that is intrinsic to the underlying type
                // The default InvalidNetComponentId is a placeholder, we expect it to be overwritten by the serializer
                // This happens when deserializing a non-delta'd input command
                // However in the delta serializer case, we use the previous input as our initial value
                // which will have the NetworkInputs setup and therefore won't write out the componentId
                NetComponentId componentId = m_componentInputs[i] ? m_componentInputs[i]->GetNetComponentId() : InvalidNetComponentId;
                serializer.Serialize(componentId, "ComponentType");
                // Create a new input if we don't have one or the types do not match
                if ((m_componentInputs[i] == nullptr) || (componentId != m_componentInputs[i]->GetNetComponentId()))
                {
                    m_componentInputs[i] = AZStd::move(GetMultiplayerComponentRegistry()->AllocateComponentInput(componentId));
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
                NetComponentId componentId = componentInput->GetNetComponentId();
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
            if (componentInput->GetNetComponentId() == componentId)
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
        m_hostFrameId = rhs.m_hostFrameId;
        m_hostTimeMs = rhs.m_hostTimeMs;
        m_hostBlendFactor = rhs.m_hostBlendFactor;
        m_componentInputs.resize(rhs.m_componentInputs.size());
        for (int32_t i = 0; i < rhs.m_componentInputs.size(); ++i)
        {
            const NetComponentId rhsComponentId = rhs.m_componentInputs[i]->GetNetComponentId();
            if (m_componentInputs[i] == nullptr || m_componentInputs[i]->GetNetComponentId() != rhsComponentId)
            {
                m_componentInputs[i] = AZStd::move(GetMultiplayerComponentRegistry()->AllocateComponentInput(rhsComponentId));
            }
            *(m_componentInputs[i]) = *(rhs.m_componentInputs[i]);
        }
        m_wasAttached = rhs.m_wasAttached;
    }
}

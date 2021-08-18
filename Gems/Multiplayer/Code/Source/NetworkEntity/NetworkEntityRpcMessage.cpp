/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    NetworkEntityRpcMessage::NetworkEntityRpcMessage(NetworkEntityRpcMessage&& rhs)
        : m_rpcDeliveryType(rhs.m_rpcDeliveryType)
        , m_entityId(rhs.m_entityId)
        , m_componentId(rhs.m_componentId)
        , m_rpcIndex(rhs.m_rpcIndex)
        , m_data(AZStd::move(rhs.m_data))
        , m_isReliable(rhs.m_isReliable)
    {
        ;
    }

    NetworkEntityRpcMessage::NetworkEntityRpcMessage(const NetworkEntityRpcMessage& rhs)
        : m_rpcDeliveryType(rhs.m_rpcDeliveryType)
        , m_entityId(rhs.m_entityId)
        , m_componentId(rhs.m_componentId)
        , m_rpcIndex(rhs.m_rpcIndex)
        , m_isReliable(rhs.m_isReliable)
    {
        if (rhs.m_data != nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
            (*m_data) = (*rhs.m_data); // Deep-copy
        }
    }

    NetworkEntityRpcMessage::NetworkEntityRpcMessage(RpcDeliveryType rpcDeliveryType, NetEntityId entityId, NetComponentId componentId, RpcIndex rpcIndex, ReliabilityType isReliable)
        : m_rpcDeliveryType(rpcDeliveryType)
        , m_entityId(entityId)
        , m_componentId(componentId)
        , m_rpcIndex(rpcIndex)
        , m_isReliable(isReliable)
    {
        ;
    }

    NetworkEntityRpcMessage& NetworkEntityRpcMessage::operator =(NetworkEntityRpcMessage&& rhs)
    {
        m_rpcDeliveryType = rhs.m_rpcDeliveryType;
        m_entityId = rhs.m_entityId;
        m_componentId = rhs.m_componentId;
        m_rpcIndex = rhs.m_rpcIndex;
        m_isReliable = rhs.m_isReliable;
        m_data = AZStd::move(rhs.m_data);
        return *this;
    }

    NetworkEntityRpcMessage& NetworkEntityRpcMessage::operator =(const NetworkEntityRpcMessage& rhs)
    {
        m_rpcDeliveryType = rhs.m_rpcDeliveryType;
        m_entityId = rhs.m_entityId;
        m_componentId = rhs.m_componentId;
        m_rpcIndex = rhs.m_rpcIndex;
        m_isReliable = rhs.m_isReliable;
        if (rhs.m_data != nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
            *m_data = (*rhs.m_data);
        }

        return *this;
    }

    bool NetworkEntityRpcMessage::operator ==(const NetworkEntityRpcMessage& rhs) const
    {
        // Note that we intentionally don't compare the blob buffers themselves
        return ((m_rpcDeliveryType == rhs.m_rpcDeliveryType)
             && (m_entityId == rhs.m_entityId)
             && (m_componentId == rhs.m_componentId)
             && (m_rpcIndex == rhs.m_rpcIndex));
    }

    bool NetworkEntityRpcMessage::operator !=(const NetworkEntityRpcMessage& rhs) const
    {
        return !(*this == rhs);
    }

    uint32_t NetworkEntityRpcMessage::GetEstimatedSerializeSize() const
    {
        static constexpr uint32_t sizeOfFields = sizeof(RpcDeliveryType)
            + sizeof(NetEntityId)
            + sizeof(NetComponentId)
            + sizeof(RpcIndex);

        // 2-byte size header + the actual blob payload itself
        const uint32_t sizeOfBlob = static_cast<uint32_t>((m_data != nullptr) ? sizeof(uint16_t) + m_data->GetSize() : 0);

        // No sliceId, remote replicator already exists so we don't need to know what type of entity this is
        return sizeOfFields + sizeOfBlob;
    }

    RpcDeliveryType NetworkEntityRpcMessage::GetRpcDeliveryType() const
    {
        return m_rpcDeliveryType;
    }

    void NetworkEntityRpcMessage::SetRpcDeliveryType(RpcDeliveryType value)
    {
        m_rpcDeliveryType = value;
    }

    NetEntityId NetworkEntityRpcMessage::GetEntityId() const
    {
        return m_entityId;
    }

    NetComponentId NetworkEntityRpcMessage::GetComponentId() const
    {
        return m_componentId;
    }

    RpcIndex NetworkEntityRpcMessage::GetRpcIndex() const
    {
        return m_rpcIndex;
    }

    bool NetworkEntityRpcMessage::SetRpcParams(IRpcParamStruct& params)
    {
        if (m_data == nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
        }

        AzNetworking::NetworkInputSerializer serializer(m_data->GetBuffer(), static_cast<uint32_t>(m_data->GetCapacity()));
        if (params.Serialize(serializer))
        {
            m_data->Resize(serializer.GetSize());
            return true;
        }

        // Serialization failed, just leave the blob at zero size
        return false;
    }

    bool NetworkEntityRpcMessage::GetRpcParams(IRpcParamStruct& outParams)
    {
        if (m_data == nullptr)
        {
            AZLOG_ERROR("Trying to retrieve RpcParams from an NetworkEntityRpcMessage with no blob buffer, this NetworkEntityRpcMessage has not been constructed or serialized");
            return false;
        }

        AzNetworking::NetworkOutputSerializer serializer(m_data->GetBuffer(), static_cast<uint32_t>(m_data->GetSize()));
        return outParams.Serialize(serializer);
    }

    bool NetworkEntityRpcMessage::Serialize(AzNetworking::ISerializer& serializer)
    {
        serializer.Serialize(m_rpcDeliveryType, "RpcDeliveryType");
        serializer.Serialize(m_entityId, "EntityId");
        serializer.Serialize(m_componentId, "ComponentId");
        serializer.Serialize(m_rpcIndex, "RpcIndex");

        // m_data should never be nullptr, it contains serialized data for our Rpc params struct
        if (m_data == nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
        }
        serializer.Serialize(*m_data, "data");

        // We intentionally do not serialize the reliability flag, or any other RPC metadata
        return serializer.IsValid();
    }

    void NetworkEntityRpcMessage::SetReliability(ReliabilityType reliabilityType)
    {
        m_isReliable = reliabilityType;
    }

    ReliabilityType NetworkEntityRpcMessage::GetReliability() const
    {
        return m_isReliable;
    }
}

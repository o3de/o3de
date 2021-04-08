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

#pragma once

#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <Source/MultiplayerTypes.h>

namespace Multiplayer
{
    struct IRpcParamStruct;

    // The maximum number of RPC's we can aggregate into a single packet
    static constexpr uint32_t MaxAggregateRpcMessages = 1024;

    //! @class NetworkEntityRpcMessage
    //! @brief Remote procedure call data.
    class NetworkEntityRpcMessage
    {
    public:
        AZ_TYPE_INFO(NetworkEntityRpcMessage, "{3AA5E1A5-6383-46C1-9817-F1B8C2325178}");

        NetworkEntityRpcMessage() = default;
        NetworkEntityRpcMessage(NetworkEntityRpcMessage&& rhs);
        NetworkEntityRpcMessage(const NetworkEntityRpcMessage& rhs);

        //! Fill explicit constructor.
        //! @param rpcDeliveryType the delivery type (origin and target) for this RPC
        //! @param entityId        the networked entityId of the entity handling this RPC
        //! @param componentType   the networked componentId of the component handling this RPC
        //! @param rpcMessageType  the component defined RPC type, so the component knows which RPC this message corresponds to
        //! @param isReliable      whether or not this RPC should be sent reliably
        explicit NetworkEntityRpcMessage(RpcDeliveryType rpcDeliveryType, NetEntityId entityId, NetComponentId componentId, uint8_t rpcMessageType, ReliabilityType isReliable);

        NetworkEntityRpcMessage& operator =(NetworkEntityRpcMessage&& rhs);
        NetworkEntityRpcMessage& operator =(const NetworkEntityRpcMessage& rhs);
        bool operator ==(const NetworkEntityRpcMessage& rhs) const;
        bool operator !=(const NetworkEntityRpcMessage& rhs) const;

        //! Returns an estimated serialization footprint for this NetworkEntityRpcMessage.
        //! @return an estimated serialization footprint for this NetworkEntityRpcMessage
        uint32_t GetEstimatedSerializeSize() const;

        //! Gets the current value of RpcDeliveryType.
        //! @return the current value of RpcDeliveryType
        RpcDeliveryType GetRpcDeliveryType() const;

        //! Sets the current value for RpcDeliveryType.
        //! @param value the value to set RpcDeliveryType to
        void SetRpcDeliveryType(RpcDeliveryType value);

        //! Gets the current value of EntityId.
        //! @return the current value of EntityId
        NetEntityId GetEntityId() const;

        //! Gets the current value of EntityComponentType.
        //! @return the current value of EntityComponentType
        NetComponentId GetComponentId() const;

        //! Gets the current value of RpcMessageType.
        //! @return the current value of RpcMessageType
        uint8_t GetRpcMessageType() const;

        //! Writes the data contained inside a_Params to this NetworkEntityRpcMessage's blob buffer.
        //! @param params the parameters to save inside this NetworkEntityRpcMessage instance
        bool SetRpcParams(IRpcParamStruct& params);

        //! Reads the data contained inside this NetworkEntityRpcMessage's blob buffer and stores them in outParams.
        //! @param outParams the parameters instance to store to the resulting data inside
        bool GetRpcParams(IRpcParamStruct& outParams);

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer);

        //! Sets this RPC's reliable delivery flag.
        //! @param reliabilityType the reliability type for this RPC
        void SetReliability(ReliabilityType reliabilityType);

        //! Returns whether or not this RPC has been flagged for reliable delivery.
        //! @return the reliability type of this RPC
        ReliabilityType GetReliability() const;

    private:

        // Serialized payload data
        RpcDeliveryType m_rpcDeliveryType = RpcDeliveryType::None;
        NetEntityId     m_entityId        = InvalidNetEntityId;
        NetComponentId  m_componentId     = InvalidNetComponentId;
        uint8_t         m_rpcMessageType  = 0;

        // Only allocated if we actually have data
        // This is to prevent blowing out stack memory if we declare an array of these EntityUpdateMessages
        AZStd::unique_ptr<AzNetworking::PacketEncodingBuffer> m_data;

        // Non-serialized RPC metadata
        ReliabilityType m_isReliable = ReliabilityType::Reliable;
    };

    struct IRpcParamStruct
    {
        virtual ~IRpcParamStruct() {}
        virtual bool Serialize(AzNetworking::ISerializer& serializer) = 0;
    };

    struct ComponentRpcEmptyStruct
        : public IRpcParamStruct
    {
        bool Serialize(AzNetworking::ISerializer&) override { return true; }
    };
}

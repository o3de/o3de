/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <AzCore/Name/Name.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace Multiplayer
{
    //! @class NetworkEntityUpdateMessage
    //! @brief Property replication packet.
    class NetworkEntityUpdateMessage
    {
    public:
        AZ_TYPE_INFO(NetworkEntityUpdateMessage, "{CFCA08F7-547B-4B89-9794-37A8679608DF}");

        NetworkEntityUpdateMessage() = default;
        NetworkEntityUpdateMessage(NetworkEntityUpdateMessage&& rhs);
        NetworkEntityUpdateMessage(const NetworkEntityUpdateMessage& rhs);

        //! Constructor for update without a slice name (remote replicator established).
        //! @param entityRole the role of the entity being replicated
        //! @param entityId   the networkId of the entity being replicated
        explicit NetworkEntityUpdateMessage(NetEntityRole entityRole, NetEntityId entityId);

        //! Constructor for update with a slice name (no remote replicator established).
        //! @param entityRole     the role of the entity being replicated
        //! @param entityId       the networkId of the entity being replicated
        //! @param prefabEntityId the prefab entityId to clone this replicated entity from
        explicit NetworkEntityUpdateMessage(NetEntityRole entityRole, NetEntityId entityId, const PrefabEntityId& prefabEntityId);

        //! Constructor for an entity delete message.
        //! @param entityId      the networkId of the entity being deleted
        //! @param isMigrated    whether or not the entity is being migrated or deleted
        explicit NetworkEntityUpdateMessage(NetEntityId entityId, bool isMigrated);

        NetworkEntityUpdateMessage& operator =(NetworkEntityUpdateMessage&& rhs);
        NetworkEntityUpdateMessage& operator =(const NetworkEntityUpdateMessage& rhs);
        bool operator ==(const NetworkEntityUpdateMessage& rhs) const;
        bool operator !=(const NetworkEntityUpdateMessage& rhs) const;

        //! Returns an estimated serialization footprint for this NetworkEntityUpdateMessage.
        //! @return an estimated serialization footprint for this NetworkEntityUpdateMessage
        uint32_t GetEstimatedSerializeSize() const;

        //! Gets the current value of NetworkRole.
        //! @return the current value of NetworkRole
        NetEntityRole GetNetworkRole() const;

        //! Gets the entities networkId.
        //! @return the entities networkId
        NetEntityId GetEntityId() const;

        //! Gets the current value of IsDelete (true if this represents a DeleteProxy message).
        //! @return the current value of IsDelete
        bool GetIsDelete() const;

        //! Returns whether or not the entity was migrated.
        //! @return whether or not the entity was migrated
        bool GetWasMigrated() const;

        //! Gets the current value of HasValidPrefabId.
        //! @return the current value of HasValidPrefabId
        bool GetHasValidPrefabId() const;

        //! Sets the current value for PrefabEntityId.
        //! @param value the value to set PrefabEntityId to
        void SetPrefabEntityId(const PrefabEntityId& value);

        //! Gets the current value of PrefabEntityId.
        //! @return the current value of PrefabEntityId
        const PrefabEntityId& GetPrefabEntityId() const;

        //! Sets the current value for Data
        //! @param value the value to set Data to
        void SetData(const AzNetworking::PacketEncodingBuffer& value);

        //! Gets the current value of Data.
        //! @return the current value of Data
        const AzNetworking::PacketEncodingBuffer* GetData() const;

        //! Retrieves a non-const reference to the value of Data.
        //! @return a non-const reference to the value of Data
        AzNetworking::PacketEncodingBuffer& ModifyData();

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer);

    private:

        NetEntityRole  m_networkRole = NetEntityRole::InvalidRole;
        NetEntityId    m_entityId = InvalidNetEntityId;
        bool           m_isDelete = false;
        bool           m_wasMigrated = false;
        bool           m_hasValidPrefabId = false;
        PrefabEntityId m_prefabEntityId;

        // Only allocated if we actually have data
        // This is to prevent blowing out stack memory if we declare an array of these EntityUpdateMessages
        AZStd::unique_ptr<AzNetworking::PacketEncodingBuffer> m_data;
    };
    using NetworkEntityUpdateVector = AZStd::fixed_vector<NetworkEntityUpdateMessage, MaxAggregateEntityMessages>;
}

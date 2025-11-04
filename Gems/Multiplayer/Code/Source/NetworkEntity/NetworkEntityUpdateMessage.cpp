/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkEntity/NetworkEntityUpdateMessage.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    NetworkEntityUpdateMessage::NetworkEntityUpdateMessage(NetworkEntityUpdateMessage&& rhs)
        : m_networkRole(rhs.m_networkRole)
        , m_entityId(rhs.m_entityId)
        , m_isDelete(rhs.m_isDelete)
        , m_wasMigrated(rhs.m_wasMigrated)
        , m_hasValidPrefabId(rhs.m_hasValidPrefabId)
        , m_prefabEntityId(rhs.m_prefabEntityId)
        , m_data(AZStd::move(rhs.m_data))
    {
        ;
    }

    NetworkEntityUpdateMessage::NetworkEntityUpdateMessage(const NetworkEntityUpdateMessage& rhs)
        : m_networkRole(rhs.m_networkRole)
        , m_entityId(rhs.m_entityId)
        , m_isDelete(rhs.m_isDelete)
        , m_wasMigrated(rhs.m_wasMigrated)
        , m_hasValidPrefabId(rhs.m_hasValidPrefabId)
        , m_prefabEntityId(rhs.m_prefabEntityId)
    {
        if (rhs.m_data != nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
            (*m_data) = (*rhs.m_data); // Deep-copy
        }
    }

    NetworkEntityUpdateMessage::NetworkEntityUpdateMessage(NetEntityRole networkRole, NetEntityId entityId, bool isDeleted, bool wasMigrated)
        : m_networkRole(networkRole)
        , m_entityId(entityId)
        , m_isDelete(isDeleted)
        , m_wasMigrated(wasMigrated)
    {
        ;
    }

    NetworkEntityUpdateMessage& NetworkEntityUpdateMessage::operator =(NetworkEntityUpdateMessage&& rhs)
    {
        m_networkRole = rhs.m_networkRole;
        m_entityId = rhs.m_entityId;
        m_isDelete = rhs.m_isDelete;
        m_wasMigrated = rhs.m_wasMigrated;
        m_hasValidPrefabId = rhs.m_hasValidPrefabId;
        m_prefabEntityId = rhs.m_prefabEntityId;
        m_data = AZStd::move(rhs.m_data);
        return *this;
    }

    NetworkEntityUpdateMessage& NetworkEntityUpdateMessage::operator =(const NetworkEntityUpdateMessage& rhs)
    {
        m_networkRole = rhs.m_networkRole;
        m_entityId = rhs.m_entityId;
        m_isDelete = rhs.m_isDelete;
        m_wasMigrated = rhs.m_wasMigrated;
        m_hasValidPrefabId = rhs.m_hasValidPrefabId;
        m_prefabEntityId = rhs.m_prefabEntityId;
        if (rhs.m_data != nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
            *m_data = (*rhs.m_data);
        }
        return *this;
    }

    bool NetworkEntityUpdateMessage::operator ==(const NetworkEntityUpdateMessage& rhs) const
    {
        // Note that we intentionally don't compare the blob buffers themselves
        return ((m_networkRole == rhs.m_networkRole)
             && (m_entityId == rhs.m_entityId)
             && (m_isDelete == rhs.m_isDelete)
             && (m_wasMigrated == rhs.m_wasMigrated)
             && (m_hasValidPrefabId == rhs.m_hasValidPrefabId)
             && (m_prefabEntityId == rhs.m_prefabEntityId));
    }

    bool NetworkEntityUpdateMessage::operator !=(const NetworkEntityUpdateMessage& rhs) const
    {
        return !(*this == rhs);
    }

    uint32_t NetworkEntityUpdateMessage::GetEstimatedSerializeSize() const
    {
        // * NOTE * Keep this in sync with the actual serialize method for this class
        // If we return an underestimate, the replicator could start generating update packets that fragment, which would be terrible for gameplay latency
        static const uint32_t sizeOfFlags = 1;
        static const uint32_t sizeOfEntityId = sizeof(NetEntityId);
        static const uint32_t sizeOfSliceId = 6;

        // 2-byte size header + the actual blob payload itself
        const uint32_t sizeOfBlob = static_cast<uint32_t>((m_data != nullptr) ? sizeof(PropertyIndex) + m_data->GetSize() : 0);

        if (m_hasValidPrefabId)
        {
            // sliceId is transmitted
            return sizeOfFlags + sizeOfEntityId + sizeOfSliceId + sizeOfBlob;
        }

        // No sliceId, remote replicator already exists so we don't need to know what type of entity this is
        return sizeOfFlags + sizeOfEntityId + sizeOfBlob;
    }

    NetEntityRole NetworkEntityUpdateMessage::GetNetworkRole() const
    {
        return m_networkRole;
    }

    NetEntityId NetworkEntityUpdateMessage::GetEntityId() const
    {
        return m_entityId;
    }

    bool NetworkEntityUpdateMessage::GetIsDelete() const
    {
        return m_isDelete;
    }

    bool NetworkEntityUpdateMessage::GetWasMigrated() const
    {
        return m_wasMigrated;
    }

    bool NetworkEntityUpdateMessage::GetHasValidPrefabId() const
    {
        return m_hasValidPrefabId;
    }

    void NetworkEntityUpdateMessage::SetPrefabEntityId(const PrefabEntityId& value)
    {
        m_hasValidPrefabId = true;
        m_prefabEntityId = value;
    }

    const PrefabEntityId& NetworkEntityUpdateMessage::GetPrefabEntityId() const
    {
        return m_prefabEntityId;
    }

    void NetworkEntityUpdateMessage::SetData(const AzNetworking::PacketEncodingBuffer& value)
    {
        if (m_data == nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
        }
        (*m_data) = value;
    }

    const AzNetworking::PacketEncodingBuffer* NetworkEntityUpdateMessage::GetData() const
    {
        return m_data.get();
    }

    AzNetworking::PacketEncodingBuffer& NetworkEntityUpdateMessage::ModifyData()
    {
        if (m_data == nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
        }
        return *m_data;
    }

    bool NetworkEntityUpdateMessage::Serialize(AzNetworking::ISerializer& serializer)
    {
        // Always serialize the entityId
        serializer.Serialize(m_entityId, "EntityId");

        // Use the upper 4 bits for boolean flags, and the lower 4 bits for the network role
        uint8_t networkTypeAndFlags = (m_isDelete ? 0x40 : 0x00)
                                    | (m_wasMigrated ? 0x20 : 0x00)
                                    | (m_hasValidPrefabId ? 0x10 : 0x00)
                                    | static_cast<uint8_t>(m_networkRole);

        if (serializer.Serialize(networkTypeAndFlags, "TypeAndFlags"))
        {
            m_isDelete = (networkTypeAndFlags & 0x40) == 0x40;
            m_wasMigrated = (networkTypeAndFlags & 0x20) == 0x20;
            m_hasValidPrefabId = (networkTypeAndFlags & 0x10) == 0x10;
            m_networkRole = static_cast<NetEntityRole>(networkTypeAndFlags & 0x0F);
        }

        // We always serialize the data, whether it's an update or a delete.
        // This ensures that we have consistent entity data at the point of deletion in case any
        // network properties are used during deactivation / deletion logic execution.

        if (m_hasValidPrefabId)
        {
            // Only serialize the prefabEntityId if one was provided.
            // Otherwise, a remote replicator is expected to be set up and the prefabEntityId would be redundant
            serializer.Serialize(m_prefabEntityId, "PrefabEntityId");
        }

        // m_data should never be nullptr
        if (m_data == nullptr)
        {
            m_data = AZStd::make_unique<AzNetworking::PacketEncodingBuffer>();
        }

        serializer.Serialize(*m_data, "Data");;

        return serializer.IsValid();
    }
}

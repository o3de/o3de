/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkEntity/NetworkEntityUpdateMessage.h>

#pragma optimize("", off)

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
        if (rhs.m_data.get() != nullptr)
        {
            m_data.reset(GetNewBuffer());
            (*m_data.get()) = (*rhs.m_data.get()); // Deep-copy
        }
    }

    NetworkEntityUpdateMessage::NetworkEntityUpdateMessage(NetEntityRole networkRole, NetEntityId entityId)
        : m_networkRole(networkRole)
        , m_entityId(entityId)
    {
        ;
    }

    NetworkEntityUpdateMessage::NetworkEntityUpdateMessage(NetEntityRole networkRole, NetEntityId entityId, const PrefabEntityId& prefabEntityId)
        : m_networkRole(networkRole)
        , m_entityId(entityId)
        , m_hasValidPrefabId(true)
        , m_prefabEntityId(prefabEntityId)
    {
        ;
    }

    NetworkEntityUpdateMessage::NetworkEntityUpdateMessage(NetEntityId entityId, bool wasMigrated)
        : m_entityId(entityId)
        , m_isDelete(true)
        , m_wasMigrated(wasMigrated)
    {
        // this is a delete entity message c-tor
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
        if (rhs.m_data.get() != nullptr)
        {
            m_data.reset(GetNewBuffer());
            (*m_data.get()) = (*rhs.m_data.get()); // Deep-copy
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

        if (m_isDelete)
        {
            return sizeOfFlags + sizeOfEntityId;
        }

        // 2-byte size header + the actual blob payload itself
        const uint32_t sizeOfBlob = static_cast<uint32_t>((m_data.get() != nullptr) ? sizeof(PropertyIndex) + m_data.get()->GetSize() : 0);

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
        if (m_data.get() == nullptr)
        {
            m_data.reset(GetNewBuffer());
        }
        (*m_data.get()) = value;
    }

    const AzNetworking::PacketEncodingBuffer* NetworkEntityUpdateMessage::GetData() const
    {
        return m_data.get();
    }

    AzNetworking::PacketEncodingBuffer& NetworkEntityUpdateMessage::ModifyData()
    {
        if (m_data.get() == nullptr)
        {
            m_data.reset(GetNewBuffer());
        }
        return *m_data.get();
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

        if (!m_isDelete)
        {
            // We only transmit sliceEntryId's and property data globs if we're not deleting the entity
            if (m_hasValidPrefabId)
            {
                // Only serialize the sliceEntryId if one was provided to the update message constructor
                // otherwise a remote replicator should be set up and the sliceEntryId would be redundant
                serializer.Serialize(m_prefabEntityId, "PrefabEntityId");
            }

            // m_data should never be nullptr unless this is a delete packet
            if (m_data.get() == nullptr)
            {
                m_data.reset(GetNewBuffer());
            }

            serializer.Serialize(*m_data.get(), "Data");;
        }

        return serializer.IsValid();
    }

    class GlobalBufferPool
    {
    public:
        AZ_CLASS_ALLOCATOR(GlobalBufferPool, AZ::SystemAllocator, 0);

        AzNetworking::PacketEncodingBuffer* GetNewBuffer();
        void ReturnBuffer(AzNetworking::PacketEncodingBuffer* buffer);
        void ReleaseAllBuffers();

        using BufferPtr = AZStd::unique_ptr<AzNetworking::PacketEncodingBuffer>;

        AZStd::vector<BufferPtr> m_owningPool;

        AZStd::deque<AzNetworking::PacketEncodingBuffer*> m_freePool;
    };

    static GlobalBufferPool* GlobalBufferPoolInstance = nullptr;

    void NetworkEntityUpdateMessage::InitializeBufferPool()
    {
        if (!GlobalBufferPoolInstance)
        {
            GlobalBufferPoolInstance = aznew GlobalBufferPool;
        }
    }

    void NetworkEntityUpdateMessage::ReleaseBufferPool()
    {
        if (!GlobalBufferPoolInstance)
        {
            const auto size = GlobalBufferPoolInstance->m_owningPool.size();
            AZ_Printf(__FUNCTION__, "pool size was %d", size);

            delete GlobalBufferPoolInstance;
            GlobalBufferPoolInstance = nullptr;
        }
    }

    AzNetworking::PacketEncodingBuffer* GlobalBufferPool::GetNewBuffer()
    {
        if (m_freePool.empty())
        {
            m_owningPool.emplace_back(AZStd::make_unique<AzNetworking::PacketEncodingBuffer>());
            return m_owningPool.back().get();
        }

        AzNetworking::PacketEncodingBuffer* buffer = m_freePool.back();
        m_freePool.pop_back();

        return buffer;
    }

    void GlobalBufferPool::ReturnBuffer(AzNetworking::PacketEncodingBuffer* buffer)
    {
        m_freePool.push_back(buffer);
    }

    void GlobalBufferPool::ReleaseAllBuffers()
    {
        m_freePool.clear();
        m_owningPool.clear();
    }

    NonOwningBuffer::~NonOwningBuffer()
    {
        ReleaseBuffer();
    }

    void NonOwningBuffer::reset(AzNetworking::PacketEncodingBuffer* buffer)
    {
        if (m_buffer)
        {
            ReleaseBuffer();
        }

        m_buffer = buffer;
    }

    AzNetworking::PacketEncodingBuffer* NonOwningBuffer::get() const
    {
        return m_buffer;
    }

    void NonOwningBuffer::ReleaseBuffer()
    {
        if (m_buffer)
        {
            if (GlobalBufferPoolInstance)
            {
                GlobalBufferPoolInstance->ReturnBuffer(m_buffer);
            }
            else
            {
                delete m_buffer;
            }
            m_buffer = nullptr;
        }
    }

    AzNetworking::PacketEncodingBuffer* NetworkEntityUpdateMessage::GetNewBuffer()
    {
        if (GlobalBufferPoolInstance)
        {
            return GlobalBufferPoolInstance->GetNewBuffer();
        }

        return aznew AzNetworking::PacketEncodingBuffer;
    }
}

#pragma optimize("", on)

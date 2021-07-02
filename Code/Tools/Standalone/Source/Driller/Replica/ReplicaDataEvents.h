/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_DATAEVENTS_H
#define DRILLER_REPLICA_DATAEVENTS_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>

#include "Source/Driller/DrillerEvent.h"

namespace Driller
{
    namespace Replica
    {
        enum ReplicaEventType
        {
            RET_CHUNK_DATASET_SENT = 0,
            RET_CHUNK_DATASET_RECEIVED,
            RET_CHUNK_RPC_SENT,
            RET_CHUNK_RPC_RECEIVED
        };
    }

    class ReplicaChunkEvent
        : public DrillerEvent
    {
    protected:
        ReplicaChunkEvent(Replica::ReplicaEventType eventType)
            : DrillerEvent(static_cast<int>(eventType))
            , m_replicaId(0)
            , m_replicaChunkId(0)
            , m_replicaChunkIndex(std::numeric_limits<AZ::u32>::max())
            , m_timeProcessed(0)
            , m_usageBytes(0)
        {
        }

    public:

        AZ_CLASS_ALLOCATOR(ReplicaChunkEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaChunkEvent, "{76B2DCFB-2D63-4B11-AD18-48843209FF26}", DrillerEvent);

        void SetReplicaName(const char* replicaName)
        {
            m_replicaName = replicaName;
        }

        const char* GetReplicaName() const
        {
            return m_replicaName.c_str();
        }

        void SetReplicaChunkIndex(AZ::u32 index)
        {
            m_replicaChunkIndex = index;
        }

        AZ::u32 GetReplicaChunkIndex() const
        {
            return m_replicaChunkIndex;
        }

        void SetChunkTypeName(const char* chunkTypeName)
        {
            m_chunkTypeName = chunkTypeName;

            // Temporary measure to maintain data integrity
            if (m_replicaChunkIndex == std::numeric_limits<AZ::u32>::max())
            {
                m_replicaChunkIndex = static_cast<AZ::u32>(AZ::Crc32(chunkTypeName));
            }
        }

        const char* GetChunkTypeName() const
        {
            return m_chunkTypeName.c_str();
        }

        void SetUsageBytes(size_t usageBytes)
        {
            m_usageBytes = usageBytes;
        }

        size_t GetUsageBytes() const
        {
            return m_usageBytes;
        }

        void SetReplicaId(AZ::u64 replicaId)
        {
            m_replicaId = replicaId;
        }

        AZ::u64 GetReplicaId() const
        {
            return m_replicaId;
        }

        void SetReplicaChunkId(AZ::u64 replicaChunkId)
        {
            m_replicaChunkId = replicaChunkId;
        }

        AZ::u64 GetReplicaChunkId() const
        {
            return m_replicaChunkId;
        }

        void SetTimeProcssed(const AZStd::chrono::milliseconds& timeProcessed)
        {
            m_timeProcessed = timeProcessed;
        }

        AZStd::chrono::milliseconds GetTimeProcessed() const
        {
            return m_timeProcessed;
        }

        void StepForward(Aggregator* data) override { (void)data; };
        void StepBackward(Aggregator* data) override { (void)data; };

    private:
        AZStd::string m_replicaName;
        AZStd::string m_chunkTypeName;

        AZ::u64 m_replicaId;
        AZ::u64 m_replicaChunkId;

        AZ::u32 m_replicaChunkIndex;

        AZStd::chrono::milliseconds m_timeProcessed;

        size_t m_usageBytes;
    };

    class ReplicaChunkDataSetEvent
        : public ReplicaChunkEvent
    {
    protected:
        ReplicaChunkDataSetEvent(Replica::ReplicaEventType eventType)
            : ReplicaChunkEvent(eventType)
            , m_hasIndex(false)
            , m_index(0)
        {
        }

    public:
        AZ_RTTI(ReplicaChunkDataSetEvent, "{39D9C3E7-B119-4C9C-BC70-DB4890A131FD}", ReplicaChunkEvent);

        void SetDataSetName(const char* dataSetName)
        {
            m_dataSetName = dataSetName;
        }

        const char* GetDataSetName() const
        {
            return m_dataSetName.c_str();
        }

        void SetIndex(size_t dataSetIndex)
        {
            m_hasIndex = true;
            m_index = dataSetIndex;
        }

        size_t GetIndex() const
        {
            return m_index;
        }

        bool HasIndex() const
        {
            return m_hasIndex;
        }

    private:
        AZStd::string m_dataSetName;
        bool m_hasIndex;
        size_t m_index;
    };

    class ReplicaChunkRPCEvent
        : public ReplicaChunkEvent
    {
    protected:
        ReplicaChunkRPCEvent(Replica::ReplicaEventType eventType)
            : ReplicaChunkEvent(eventType)
            , m_hasIndex(false)
            , m_index(0)
        {
        }

    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkDataSetEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaChunkRPCEvent, "{27213952-E66A-4DE7-A60D-683895A5A973}", ReplicaChunkEvent);

        void SetRPCName(const char* rpcName)
        {
            m_RPCName = rpcName;
        }

        const char* GetRPCName() const
        {
            return m_RPCName.c_str();
        }

        void SetIndex(size_t rpcIndex)
        {
            m_hasIndex = true;
            m_index = rpcIndex;
        }

        size_t GetIndex() const
        {
            return m_index;
        }

        bool HasIndex() const
        {
            return m_hasIndex;
        }

    private:
        AZStd::string m_RPCName;
        bool m_hasIndex;
        size_t m_index;
    };

    class ReplicaChunkSentDataSetEvent
        : public ReplicaChunkDataSetEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkSentDataSetEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaChunkSentDataSetEvent, "{2B6BDB9C-4465-4BC6-BB80-73CD85A0B818}", ReplicaChunkDataSetEvent);

        ReplicaChunkSentDataSetEvent()
            : ReplicaChunkDataSetEvent(Replica::RET_CHUNK_DATASET_SENT)
        {
        }
    };

    class ReplicaChunkReceivedDataSetEvent
        : public ReplicaChunkDataSetEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkReceivedDataSetEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaChunkReceivedDataSetEvent, "{138F7C4A-3727-4565-9395-673E43BC325C}", ReplicaChunkDataSetEvent);

        ReplicaChunkReceivedDataSetEvent()
            : ReplicaChunkDataSetEvent(Replica::RET_CHUNK_DATASET_RECEIVED)
        {
        }
    };

    class ReplicaChunkSentRPCEvent
        : public ReplicaChunkRPCEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkSentRPCEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaChunkSentRPCEvent, "{04E9EE7E-5F41-4566-B584-0C671B2E09DE}", ReplicaChunkRPCEvent);

        ReplicaChunkSentRPCEvent()
            : ReplicaChunkRPCEvent(Replica::RET_CHUNK_RPC_SENT)
        {
        }
    };

    class ReplicaChunkReceivedRPCEvent
        : public ReplicaChunkRPCEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkReceivedRPCEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaChunkReceivedRPCEvent, "{68482B1F-8A70-4152-9014-714B46641A12}", ReplicaChunkRPCEvent);

        ReplicaChunkReceivedRPCEvent()
            : ReplicaChunkRPCEvent(Replica::RET_CHUNK_RPC_RECEIVED)
        {
        }
    };
}

#endif

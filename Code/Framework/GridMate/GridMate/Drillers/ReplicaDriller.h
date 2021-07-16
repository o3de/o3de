/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_DRILLER_H
#define GM_REPLICA_DRILLER_H

#include <GridMate/Types.h>
#include <AzCore/Driller/Driller.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>

namespace GridMate
{
    namespace Debug
    {
        class ReplicaDriller
            : public AZ::Debug::Driller
            , public GridMate::Debug::ReplicaDrillerBus::Handler
        {
        public:
            struct Tags
            {
                // Driller
                static const AZ::Crc32 REPLICA_DRILLER;

                // Event Types
                static const AZ::Crc32 CHUNK_SEND_DATASET;
                static const AZ::Crc32 CHUNK_RECEIVE_DATASET;
                static const AZ::Crc32 CHUNK_SEND_RPC;
                static const AZ::Crc32 CHUNK_RECEIVE_RPC;

                // Data Fields
                static const AZ::Crc32 REPLICA_NAME;
                static const AZ::Crc32 REPLICA_ID;
                static const AZ::Crc32 CHUNK_TYPE;
                static const AZ::Crc32 CHUNK_INDEX;
                static const AZ::Crc32 DATA_SET_NAME;
                static const AZ::Crc32 DATA_SET_INDEX;
                static const AZ::Crc32 RPC_NAME;
                static const AZ::Crc32 RPC_INDEX;
                static const AZ::Crc32 SIZE;
                static const AZ::Crc32 TIME_PROCESSED_MILLISEC;
            };

            AZ_CLASS_ALLOCATOR(ReplicaDriller, AZ::OSAllocator, 0);
            ReplicaDriller();

            //////////////////////////////////////////////////////////////////////////
            // Driller
            const char*  GroupName() const override      { return "GridMate"; }
            const char*  GetName() const override        { return "ReplicaDriller"; }
            const char*  GetDescription() const override { return "Drills replicas."; }
            void         Start(const Param* params = NULL, int numParams = 0) override;
            void         Stop() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ReplicaDrillerEvents
            void OnSendDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len) override;
            void OnReceiveDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len) override;

            void OnSendRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len) override;
            void OnReceiveRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len) override;
            //////////////////////////////////////////////////////////////////////////

        private:

            void OutputBaseReplicaChunkTags(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, size_t len);
        };
    }
}

#endif

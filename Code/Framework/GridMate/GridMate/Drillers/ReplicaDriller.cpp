/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GridMate/Drillers/ReplicaDriller.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/String/string.h>

using namespace AZ::Debug;

namespace GridMate
{
    namespace Debug
    {
        const AZ::Crc32 ReplicaDriller::Tags::REPLICA_DRILLER = AZ_CRC("ReplicaDriller", 0xd832f49a);

        // Event Types
        const AZ::Crc32 ReplicaDriller::Tags::CHUNK_SEND_DATASET = AZ_CRC("ChunkSendDataSet", 0x085ea99b);
        const AZ::Crc32 ReplicaDriller::Tags::CHUNK_RECEIVE_DATASET = AZ_CRC("ChunkReceiveDataSet", 0x8d4536db);
        const AZ::Crc32 ReplicaDriller::Tags::CHUNK_SEND_RPC = AZ_CRC("ChunkSendRPC", 0x7c40afe0);
        const AZ::Crc32 ReplicaDriller::Tags::CHUNK_RECEIVE_RPC = AZ_CRC("ChunkReceiveRPC", 0xb49b302d);

        // Data Fields
        const AZ::Crc32 ReplicaDriller::Tags::REPLICA_NAME = AZ_CRC("ReplicaName", 0xc69b68ee);
        const AZ::Crc32 ReplicaDriller::Tags::REPLICA_ID = AZ_CRC("ReplicaID", 0x394dd741);
        const AZ::Crc32 ReplicaDriller::Tags::CHUNK_TYPE = AZ_CRC("TypeName", 0x115f811d);
        const AZ::Crc32 ReplicaDriller::Tags::CHUNK_INDEX = AZ_CRC("ChunkIndex", 0x25ba3370);
        const AZ::Crc32 ReplicaDriller::Tags::DATA_SET_NAME = AZ_CRC("DataSetName", 0xf22dbaae);
        const AZ::Crc32 ReplicaDriller::Tags::DATA_SET_INDEX = AZ_CRC("DataSetIndex", 0x58d2421f);
        const AZ::Crc32 ReplicaDriller::Tags::RPC_NAME = AZ_CRC("RPCName", 0x4c4cbf3a);
        const AZ::Crc32 ReplicaDriller::Tags::RPC_INDEX = AZ_CRC("RPCIndex", 0xaf0e7447);
        const AZ::Crc32 ReplicaDriller::Tags::SIZE = AZ_CRC("Size", 0xf7c0246a);
        const AZ::Crc32 ReplicaDriller::Tags::TIME_PROCESSED_MILLISEC = AZ_CRC("Time", 0x6f949845);

        ReplicaDriller::ReplicaDriller()
        {
        }

        void ReplicaDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;
            ReplicaDrillerBus::Handler::BusConnect();
        }

        void ReplicaDriller::Stop()
        {
            ReplicaDrillerBus::Handler::BusDisconnect();
        }

        void ReplicaDriller::OnSendDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len)
        {
            (void)from;
            (void)to;
            (void)data;

            const char* dataSetName = chunk->GetDescriptor()->GetDataSetName(chunk, dataSet);
            size_t dataSetIndex = chunk->GetDescriptor()->GetDataSetIndex(chunk, dataSet);

            m_output->BeginTag(Tags::REPLICA_DRILLER);
            m_output->BeginTag(Tags::CHUNK_SEND_DATASET);
            OutputBaseReplicaChunkTags(chunk, chunkIndex, len);
            m_output->Write(Tags::DATA_SET_NAME, dataSetName);
            m_output->Write(Tags::DATA_SET_INDEX, dataSetIndex);
            m_output->EndTag(Tags::CHUNK_SEND_DATASET);
            m_output->EndTag(Tags::REPLICA_DRILLER);
        }

        void ReplicaDriller::OnReceiveDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len)
        {
            (void)from;
            (void)to;
            (void)data;

            const char* dataSetName = chunk->GetDescriptor()->GetDataSetName(chunk, dataSet);
            size_t dataSetIndex = chunk->GetDescriptor()->GetDataSetIndex(chunk, dataSet);

            m_output->BeginTag(Tags::REPLICA_DRILLER);
            m_output->BeginTag(Tags::CHUNK_RECEIVE_DATASET);
            OutputBaseReplicaChunkTags(chunk, chunkIndex, len);
            m_output->Write(Tags::DATA_SET_NAME, dataSetName);
            m_output->Write(Tags::DATA_SET_INDEX, dataSetIndex);
            m_output->EndTag(Tags::CHUNK_RECEIVE_DATASET);
            m_output->EndTag(Tags::REPLICA_DRILLER);
        }

        void ReplicaDriller::OnSendRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len)
        {
            (void)from;
            (void)to;
            (void)data;

            const char* rpcName = chunk->GetDescriptor()->GetRpcName(chunk, rpc->m_rpc);
            size_t rpcIndex = chunk->GetDescriptor()->GetRpcIndex(chunk, rpc->m_rpc);


            m_output->BeginTag(Tags::REPLICA_DRILLER);
            m_output->BeginTag(Tags::CHUNK_SEND_RPC);
            OutputBaseReplicaChunkTags(chunk, chunkIndex, len);
            m_output->Write(Tags::RPC_NAME, rpcName);
            m_output->Write(Tags::RPC_INDEX, rpcIndex);
            m_output->EndTag(Tags::CHUNK_SEND_RPC);
            m_output->EndTag(Tags::REPLICA_DRILLER);
        }
        void ReplicaDriller::OnReceiveRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len)
        {
            (void)from;
            (void)to;
            (void)data;

            const char* rpcName = chunk->GetDescriptor()->GetRpcName(chunk, rpc->m_rpc);
            size_t rpcIndex = chunk->GetDescriptor()->GetRpcIndex(chunk, rpc->m_rpc);

            m_output->BeginTag(Tags::REPLICA_DRILLER);
            m_output->BeginTag(Tags::CHUNK_RECEIVE_RPC);
            OutputBaseReplicaChunkTags(chunk, chunkIndex, len);
            m_output->Write(Tags::RPC_NAME, rpcName);
            m_output->Write(Tags::RPC_INDEX, rpcIndex);
            m_output->EndTag(Tags::CHUNK_RECEIVE_RPC);
            m_output->EndTag(Tags::REPLICA_DRILLER);
        }

        void ReplicaDriller::OutputBaseReplicaChunkTags(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, size_t len)
        {
            const char* chunkTypeName = chunk->GetDescriptor()->GetChunkName();            
            const char* replicaName = chunk->GetReplica()->GetDebugName();            

            m_output->Write(Tags::REPLICA_NAME, replicaName);
            m_output->Write(Tags::REPLICA_ID, chunk->GetReplicaId());
            m_output->Write(Tags::CHUNK_TYPE, chunkTypeName);
            m_output->Write(Tags::CHUNK_INDEX, chunkIndex);
            m_output->Write(Tags::SIZE, len);
            m_output->Write(Tags::TIME_PROCESSED_MILLISEC, AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now().time_since_epoch()).count());
        }
    }
}

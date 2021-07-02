/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ReplicaDataParser.h"

#include "ReplicaDataAggregator.hxx"
#include "ReplicaDataEvents.h"

#include "GridMate/Drillers/ReplicaDriller.h"

namespace Driller
{
    //////////////////////
    // ReplicaDataParser
    //////////////////////

    ReplicaDataParser::ReplicaDataParser(ReplicaDataAggregator* aggregator)
        : DrillerHandlerParser(false)
        , m_currentType(Replica::DataType::NONE)
        , m_aggregator(aggregator)
    {
    }

    AZ::Debug::DrillerHandlerParser* ReplicaDataParser::OnEnterTag(AZ::u32 tagName)
    {
        if (tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_SEND_DATASET)
        {
            AZ_Assert(m_currentType == Replica::DataType::NONE, "ERROR: Bad flow received.");

            ReplicaChunkSentDataSetEvent* newEvent = aznew ReplicaChunkSentDataSetEvent;

            if (newEvent)
            {
                m_currentType = Replica::DataType::SENT_REPLICA_CHUNK;
                m_aggregator->AddEvent(newEvent);
                return this;
            }
        }
        else if (tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_RECEIVE_DATASET)
        {
            AZ_Assert(m_currentType == Replica::DataType::NONE, "ERROR: Bad flow received.");

            ReplicaChunkReceivedDataSetEvent* newEvent = aznew ReplicaChunkReceivedDataSetEvent;

            if (newEvent)
            {
                m_currentType = Replica::DataType::RECEIVED_REPLICA_CHUNK;
                m_aggregator->AddEvent(newEvent);
                return this;
            }
        }
        else if (tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_SEND_RPC)
        {
            AZ_Assert(m_currentType == Replica::DataType::NONE, "ERROR: Bad flow received.");

            ReplicaChunkSentRPCEvent* newEvent = aznew ReplicaChunkSentRPCEvent;

            if (newEvent)
            {
                m_currentType = Replica::DataType::SENT_REPLICA_CHUNK;
                m_aggregator->AddEvent(newEvent);
                return this;
            }
        }
        else if (tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_RECEIVE_RPC)
        {
            AZ_Assert(m_currentType == Replica::DataType::NONE, "ERROR: Bad flow received.");

            ReplicaChunkReceivedRPCEvent* newEvent = aznew ReplicaChunkReceivedRPCEvent;

            if (newEvent)
            {
                m_currentType = Replica::DataType::RECEIVED_REPLICA_CHUNK;
                m_aggregator->AddEvent(newEvent);
                return this;
            }
        }

        return nullptr;
    }

    void ReplicaDataParser::OnExitTag(AZ::Debug::DrillerHandlerParser* handler, AZ::u32 tagName)
    {
        (void)handler;

        if (tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_SEND_DATASET
            || tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_RECEIVE_DATASET
            || tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_SEND_RPC
            || tagName == GridMate::Debug::ReplicaDriller::Tags::CHUNK_RECEIVE_RPC)
        {
            m_currentType = Replica::DataType::NONE;
            m_aggregator->FinalizeEvent();
        }
    }

    void ReplicaDataParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        if (m_currentType == Replica::DataType::NONE
            || m_aggregator->GetEvents().empty())
        {
            return;
        }

        ProcessReplicaChunk(dataNode);

        switch (m_currentType)
        {
        case Replica::DataType::SENT_REPLICA_CHUNK:
            ProcessSentReplicaChunk(dataNode);
            break;
        case Replica::DataType::RECEIVED_REPLICA_CHUNK:
            ProcessReceivedReplicaChunk(dataNode);
            break;
        default:
            break;
        }
    }

    void ReplicaDataParser::ProcessReplicaChunk(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        ReplicaChunkEvent* receivedEvent = static_cast<ReplicaChunkEvent*>(m_aggregator->GetEvents().back());

        if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::CHUNK_TYPE)
        {
            AZStd::string chunkType;
            dataNode.Read(chunkType);

            receivedEvent->SetChunkTypeName(chunkType.c_str());
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::CHUNK_INDEX)
        {
            AZ::u32 chunkIndex;
            dataNode.Read(chunkIndex);

            receivedEvent->SetReplicaChunkIndex(chunkIndex);
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::SIZE)
        {
            size_t usageBytes;
            dataNode.Read(usageBytes);

            receivedEvent->SetUsageBytes(usageBytes);
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::REPLICA_ID)
        {
            AZ::u32 replicaId;
            dataNode.Read(replicaId);

            receivedEvent->SetReplicaId(replicaId);
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::REPLICA_NAME)
        {
            AZStd::string replicaName;
            dataNode.Read(replicaName);

            receivedEvent->SetReplicaName(replicaName.c_str());
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::TIME_PROCESSED_MILLISEC)
        {
            AZStd::sys_time_t time;
            dataNode.Read(time);

            AZStd::chrono::milliseconds timeMS(time);
            receivedEvent->SetTimeProcssed(timeMS);
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::DATA_SET_NAME)
        {
            AZStd::string dataSetName;
            dataNode.Read(dataSetName);

            ReplicaChunkDataSetEvent* dataSetEvent = static_cast<ReplicaChunkDataSetEvent*>(receivedEvent);

            dataSetEvent->SetDataSetName(dataSetName.c_str());
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::DATA_SET_INDEX)
        {
            size_t dataSetIndex;
            dataNode.Read(dataSetIndex);

            ReplicaChunkDataSetEvent* dataSetEvent = static_cast<ReplicaChunkDataSetEvent*>(receivedEvent);

            dataSetEvent->SetIndex(dataSetIndex);
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::RPC_NAME)
        {
            AZStd::string rpcName;
            dataNode.Read(rpcName);

            ReplicaChunkRPCEvent* rpcEvent = static_cast<ReplicaChunkRPCEvent*>(receivedEvent);

            rpcEvent->SetRPCName(rpcName.c_str());
        }
        else if (dataNode.m_name == GridMate::Debug::ReplicaDriller::Tags::RPC_INDEX)
        {
            size_t rpcIndex;
            dataNode.Read(rpcIndex);

            ReplicaChunkRPCEvent* rpcEvent = static_cast<ReplicaChunkRPCEvent*>(receivedEvent);
            rpcEvent->SetIndex(rpcIndex);
        }
    }

    void ReplicaDataParser::ProcessSentReplicaChunk(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        (void)dataNode;
    }

    void ReplicaDataParser::ProcessReceivedReplicaChunk(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        (void)dataNode;
    }
}

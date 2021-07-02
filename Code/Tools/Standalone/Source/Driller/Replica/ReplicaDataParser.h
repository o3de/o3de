/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_DATAPARSER_H
#define DRILLER_REPLICA_DATAPARSER_H

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    class ReplicaDataAggregator;

    namespace Replica
    {
        enum class DataType
        {
            NONE,
            SENT_REPLICA_CHUNK,
            RECEIVED_REPLICA_CHUNK
        };
    }

    class ReplicaDataParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        ReplicaDataParser(ReplicaDataAggregator* aggregator);

        AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName) override;
        void OnExitTag(AZ::Debug::DrillerHandlerParser* handler, AZ::u32 tagName) override;
        void OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode) override;

    private:

        void ProcessReplicaChunk(const AZ::Debug::DrillerSAXParser::Data& dataNode);
        void ProcessSentReplicaChunk(const AZ::Debug::DrillerSAXParser::Data& dataNode);
        void ProcessReceivedReplicaChunk(const AZ::Debug::DrillerSAXParser::Data& dataNode);

        Replica::DataType m_currentType;

        ReplicaDataAggregator* m_aggregator;
    };
}

#endif

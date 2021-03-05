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
#ifndef INCLUDE_NETWORKGRIDMATEPROFILING_HEADER
#define INCLUDE_NETWORKGRIDMATEPROFILING_HEADER

#pragma once

#include "NetworkGridMateCommon.h"

namespace GridMate
{
    struct CarrierStatistics
    {
        CarrierStatistics()
            : m_rtt(0.f)
            , m_packetLossRate(0.f)
            , m_totalReceivedBytes(0)
            , m_totalSentBytes(0)
            , m_packetsLost(0)
            , m_packetsReceived(0)
            , m_packetsSent(0)
        {
        }

        float   m_rtt;
        float   m_packetLossRate;

        uint32  m_totalReceivedBytes;
        uint32  m_totalSentBytes;
        uint32  m_packetsLost;
        uint32  m_packetsReceived;
        uint32  m_packetsSent;
    };

    struct GameStatistics
    {
        struct RMIStatistics
        {
            uint32  m_sendCount;
            uint32  m_receiveCount;
            uint32  m_totalSentBytes;
            uint32  m_totalReceivedBytes;

            RMIStatistics()
                : m_sendCount(0)
                , m_receiveCount(0)
                , m_totalSentBytes(0)
                , m_totalReceivedBytes(0)
            {
            }
        };

        struct AspectStatistics
        {
            uint32  m_sendCount;
            uint32  m_receiveCount;
            uint32  m_totalSentBytes;
            uint32  m_totalReceivedBytes;

            AspectStatistics()
                : m_sendCount(0)
                , m_receiveCount(0)
                , m_totalSentBytes(0)
                , m_totalReceivedBytes(0)
            {
            }
        };

        struct EntityStatistics
        {
            typedef std__hash_map<uint32, RMIStatistics> RMIInstanceMap;

            RMIInstanceMap m_rmiActor;
            RMIInstanceMap m_rmiLegacy;
            AspectStatistics m_aspects[ NUM_ASPECTS ];
            uint32 m_totalCostEstimate;

            EntityStatistics()
                : m_totalCostEstimate(0)
            {
            }
        };

        GameStatistics()
            : m_aspectsSent(0)
            , m_aspectsReceived(0)
            , m_aspectSentBytes(0)
            , m_aspectReceivedBytes(0)
        {
        }

        uint32 m_aspectsSent;
        uint32 m_aspectsReceived;
        uint32 m_aspectSentBytes;
        uint32 m_aspectReceivedBytes;

        RMIStatistics m_rmiGlobalActor;
        RMIStatistics m_rmiGlobalLegacy;
        RMIStatistics m_rmiGlobalScript;

        typedef std__hash_map<EntityId, EntityStatistics> EntityStatisticsMap;
        EntityStatisticsMap m_entities;
    };
} // namespace GridMate

#endif // INCLUDE_NETWORKGRIDMATEPROFILING_HEADER

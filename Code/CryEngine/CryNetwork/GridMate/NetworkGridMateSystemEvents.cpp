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
#include "CryNetwork_precompiled.h"

#include <I3DEngine.h>

#include "NetworkGridMate.h"
#include "NetworkGridMateSystemEvents.h"

namespace GridMate
{
    //-----------------------------------------------------------------------------
    NetworkSystemEvents::NetworkSystemEvents()
        : m_connected(false)
    {
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::Connect()
    {
        GridMate::NetworkSystemEventBus::Handler::BusConnect();

        m_connected = true;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::Disconnect()
    {
        GridMate::NetworkSystemEventBus::Handler::BusDisconnect();

        m_connected = false;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::ActorRMISent(EntityId entityId, const IActorRMIRep& rep, uint32 paramsSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        auto& global = stats.m_rmiGlobalActor;
        global.m_sendCount++;
        global.m_totalSentBytes += paramsSize;

        auto& entity = stats.m_entities[ entityId ];
        auto& rmiEntry = entity.m_rmiActor[ rep.GetUniqueId() ];
        rmiEntry.m_sendCount++;
        rmiEntry.m_totalSentBytes += paramsSize;
        entity.m_totalCostEstimate += paramsSize;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::ActorRMIReceived(EntityId entityId, const IActorRMIRep& rep, uint32 paramsSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        auto& global = stats.m_rmiGlobalActor;
        global.m_receiveCount++;
        global.m_totalReceivedBytes += paramsSize;

        auto& entity = stats.m_entities[ entityId ];
        auto& rmiEntry = entity.m_rmiActor[ rep.GetUniqueId() ];
        rmiEntry.m_receiveCount++;
        rmiEntry.m_totalReceivedBytes += paramsSize;
        entity.m_totalCostEstimate += paramsSize;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::LegacyRMISent(EntityId entityId, const IRMIRep& rep, uint32 paramsSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        auto& global = stats.m_rmiGlobalLegacy;
        global.m_sendCount++;
        global.m_totalSentBytes += paramsSize;

        auto& entity = stats.m_entities[ entityId ];
        auto& rmiEntry = entity.m_rmiLegacy[ rep.GetUniqueId() ];
        rmiEntry.m_sendCount++;
        rmiEntry.m_totalSentBytes += paramsSize;
        entity.m_totalCostEstimate += paramsSize;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::LegacyRMIReceived(EntityId entityId, const IRMIRep& rep, uint32 paramsSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        auto& global = stats.m_rmiGlobalLegacy;
        global.m_receiveCount++;
        global.m_totalReceivedBytes += paramsSize;

        auto& entity = stats.m_entities[ entityId ];
        auto& rmiEntry = entity.m_rmiLegacy[ rep.GetUniqueId() ];
        rmiEntry.m_receiveCount++;
        rmiEntry.m_totalReceivedBytes += paramsSize;
        entity.m_totalCostEstimate += paramsSize;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::ScriptRMISent(uint32 paramsSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        auto& global = stats.m_rmiGlobalScript;
        global.m_sendCount++;
        global.m_totalSentBytes += paramsSize;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::ScriptRMIReceived(uint32 paramsSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        auto& global = stats.m_rmiGlobalScript;
        global.m_receiveCount++;
        global.m_totalReceivedBytes += paramsSize;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::AspectSent(EntityId entityId, ASPECT_TYPE aspectBit, uint32 payloadSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        stats.m_aspectsSent++;
        stats.m_aspectSentBytes += payloadSize;

        auto& entity = stats.m_entities[ entityId ];
        auto& aspectEntry = entity.m_aspects[ BitIndex(aspectBit) ];
        aspectEntry.m_sendCount++;
        aspectEntry.m_totalSentBytes += payloadSize;
        entity.m_totalCostEstimate += payloadSize;
    }

    //-----------------------------------------------------------------------------
    void NetworkSystemEvents::AspectReceived(EntityId entityId, ASPECT_TYPE aspectBit, uint32 payloadSize)
    {
        auto& stats = Network::Get().GetGameStatistics();
        stats.m_aspectsReceived++;
        stats.m_aspectReceivedBytes += payloadSize;

        auto& entity = stats.m_entities[ entityId ];
        auto& aspectEntry = entity.m_aspects[ BitIndex(aspectBit) ];
        aspectEntry.m_receiveCount++;
        aspectEntry.m_totalReceivedBytes += payloadSize;
        entity.m_totalCostEstimate += payloadSize;
    }
} // namespace GridMate

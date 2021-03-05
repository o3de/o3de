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
#include "NetworkGridmateDebug.h"
#include "NetworkGridMateSessionEvents.h"

#include "Replicas/EntityReplica.h"

#include <GridMate/Replica/ReplicaFunctions.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    void SessionEvents::Connect(IGridMate* gridMate)
    {
        GridMate::SessionEventBus::Handler::BusConnect(gridMate);
        AzFramework::NetBindingSystemEventsBus::Handler::BusConnect();
    }

    //-----------------------------------------------------------------------------
    void SessionEvents::Disconnect()
    {
        AzFramework::NetBindingSystemEventsBus::Handler::BusDisconnect();
        GridMate::SessionEventBus::Handler::BusDisconnect();
    }

    //-----------------------------------------------------------------------------
    void SessionEvents::OnNetworkSessionCreated(GridMate::GridSession* session)
    {
        GM_DEBUG_TRACE("Session %s has been created.", session->GetId().c_str());

        auto& net = Network::Get();
        net.ClearNetworkStatistics();
        net.m_localChannelId = net.GetChannelIdForSessionMember(session->GetMyMember());
        net.m_session = session;

        if (session->IsHost())
        {
            net.MarkAsConnectedServer();
        }
        else
        {
            net.MarkAsConnectedClient();
        }
    }

    //-----------------------------------------------------------------------------
    void SessionEvents::OnNetworkSessionDeactivated([[maybe_unused]] GridMate::GridSession* session)
    {
        GM_DEBUG_TRACE("Session %s has been deleted.", session->GetId().c_str());

        auto& net = Network::Get();
        net.MarkAsLocalOnly();
        net.ClearNetworkStatistics();
        net.m_localChannelId = kOfflineChannelId;
        net.m_activeEntityReplicaMap.clear();
        net.m_newProxyEntities.clear();
        net.m_session = nullptr;
    }

    //-----------------------------------------------------------------------------
    void SessionEvents::OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member)
    {
        auto& net = Network::Get();
        if (session == net.m_session)
        {
            GM_ASSERT_TRACE(member, "NetworkGridMate::OnMemberLeaving(), departing member is null!");

            const ChannelId departedChannelId = net.GetChannelIdForSessionMember(member);
            GM_DEBUG_TRACE("Member for channel id %u has left the session.", departedChannelId);

            if (gEnv->bServer && departedChannelId != kInvalidChannelId)
            {
                net.m_statisticsPerChannel.erase(departedChannelId);
            }
        }
    }
} // namespace GridMate

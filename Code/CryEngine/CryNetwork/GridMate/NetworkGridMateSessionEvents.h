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
#ifndef INCLUDE_NETWORKGRIDMATESESSIONEVENTS_HEADER
#define INCLUDE_NETWORKGRIDMATESESSIONEVENTS_HEADER

#pragma once

#include <GridMate/Session/Session.h> // Base class
#include <AzFramework/Network/NetBindingSystemBus.h>

namespace GridMate
{
    /*!
     * Acts as a sink for the session EBus.
     */
    class SessionEvents
        : public GridMate::SessionEventBus::Handler
        , public AzFramework::NetBindingSystemEventsBus::Handler
    {
    public:
        void Connect(IGridMate* gridMate);
        void Disconnect();

        bool IsConnected() const { return GridMate::SessionEventBus::Handler::BusIsConnected() && AzFramework::NetBindingSystemEventsBus::Handler::BusIsConnected(); }

        void OnNetworkSessionCreated(GridMate::GridSession* session) override;
        void OnNetworkSessionDeactivated(GridMate::GridSession* session) override;

        ///////////////////////////////////////////////////
        // SessionEventBus
        void OnMemberLeaving(GridSession* session, GridMember* member) override;
        ///////////////////////////////////////////////////
    };
} // namespace GridMate

#endif // INCLUDE_NETWORKGRIDMATESESSIONEVENTS_HEADER

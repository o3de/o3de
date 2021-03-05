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

#pragma once

#include <AzCore/Preprocessor/Enum.h>

namespace AzNetworking
{
    AZ_ENUM_CLASS(ReliabilityType
        , Reliable
        , Unreliable
    );

    AZ_ENUM_CLASS(TerminationEndpoint
        , Local
        , Remote
    );

    AZ_ENUM_CLASS(ConnectionRole
        , Connector
        , Acceptor
    );

    AZ_ENUM_CLASS(ConnectionState
        , Disconnected
        , Disconnecting
        , Connected
        , Connecting
    );

    AZ_ENUM_CLASS(DisconnectReason
        , None
        , Unknown
        , StreamError
        , NetworkError
        , Timeout
        , ConnectTimeout
        , ConnectionRetry
        , HeartbeatTimeout
        , TransportError
        , TerminatedByClient
        , TerminatedByServer
        , TerminatedByUser
        , TerminatedByMultipleLogin
        , RemoteHostClosedConnection
        , ReliableTransportFailure
        , ReliableQueueFull
        , ConnectionRejected
        , ConnectionDeleted
        , ServerNotReady
        , ServerError
        , ClientMigrated
        , SslFailure
        , VersionMismatch
        , NonceRejected
        , DtlsHandshakeError
        , MAX
    );

    AZ_ENUM_CLASS(ConnectResult
        , Rejected     // Connection attempt was rejected
        , Accepted     // Connection was accepted
    );
}

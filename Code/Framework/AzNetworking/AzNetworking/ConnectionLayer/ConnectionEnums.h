/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        , ServerNoLevelLoaded
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

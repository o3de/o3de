/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_STATUS_INTERFACE_H
#define GM_REPLICA_STATUS_INTERFACE_H

#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/ReplicaChunkInterface.h>

namespace GridMate
{
    //-------------------------------------------------------------------------
    // Replica RPC interface
    //-------------------------------------------------------------------------
    struct ReplicaStatusInterface
        : public ReplicaChunkInterface
    {
        virtual bool RequestOwnershipFn(PeerId requestor, const RpcContext& rpcContext) = 0;
        virtual bool MigrationSuspendUpstreamFn(PeerId ownerId, AZ::u32 requestTime, const RpcContext& rpcContext) = 0;
        virtual bool MigrationRequestDownstreamAckFn(PeerId ownerId, AZ::u32 requestTime, const RpcContext& rpcContext) = 0;
    };
}   // namespace GridMate
#endif  // GM_REPLICA_STATUS_INTERFACE_H

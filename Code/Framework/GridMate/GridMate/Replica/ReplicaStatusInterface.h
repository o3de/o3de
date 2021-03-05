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

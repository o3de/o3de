/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICADEFS_H
#define GM_REPLICADEFS_H

/// \file ReplicaChunk.h

#include <AzCore/Math/Crc.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    // ReplicaPeer Flags
    //-----------------------------------------------------------------------------
    struct PeerFlags
    {
        enum
        {
            Peer_New = 1 << 0,
            Peer_SyncHost = 1 << 1,
            Peer_ReadyForRemoval = 1 << 2,
            Peer_Accepted = 1 << 4
        };
    };
    //-----------------------------------------------------------------------------
    typedef AZ::u32 ReplicaId;
    typedef ReplicaId RepIdSeed;
    typedef ReplicaId CmdId;
    typedef AZ::Crc32 ReplicaChunkClassId;
    typedef AZ::u32 PeerId; /// Crc32
    //-----------------------------------------------------------------------------
    enum ReservedIds : ReplicaId
    {
        Invalid_Cmd_Or_Id,  // Invalid
        Cmd_Greetings,      // First message sent by newly connected peers
        Cmd_NewProxy,       // Notify that a new proxy should be created
        Cmd_DestroyProxy,   // Notify that a proxy should be deleted
        Cmd_NewOwner,       // Notify that this replica has changed owner
        Cmd_Heartbeat,      // DEBUG: heartbeat
        Cmd_Count,          // Total number of Ids

        RepId_SessionInfo,  // SessionInfo will always use this id;

        Max_Reserved_Cmd_Or_Id,

        // Replica Ids start here. The CmdId for 'UpdateReplica' is implied by a
        // CmdId higher than Max_Reserved_Cmd_Or_Id (the Replica's Id). This is to save
        // sending an unnecessary byte per update.
    };
    //-----------------------------------------------------------------------------
    struct ReplicaMarshalFlags
    {
        enum
        {
            IncludeDatasets =   1 << 0,
            ForceDirty =        1 << 1,
            Authoritative =     1 << 2,
            Reliable =          1 << 3,
            IncludeCtorData =   1 << 4,
            OmitUnmodified =    1 << 5,
            ForceReliable =     1 << 6,

            None = 0,
            NewProxy = IncludeDatasets | OmitUnmodified | Authoritative | Reliable | ForceReliable,
            FullSync = IncludeDatasets | ForceDirty | Authoritative | Reliable | ForceReliable,
        };
    };
    //-----------------------------------------------------------------------------

    /**
        A user customisable set of flags that are used to logically separate
        the different node types within the network topology.
    **/
    typedef AZ::u32 ZoneMask;
    static const ZoneMask ZoneMask_All = (ZoneMask) - 1;
}   // namespace Gridmate

#endif // GM_REPLICADEFS_H

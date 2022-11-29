/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Multiplayer
{
    //! Declares multiplayer metric group ids.
    enum MultiplayerGroupIds
    {
        MultiplayerGroup_Networking = 101           // A group of multiplayer metrics
    };

    //! Declares multiplayer metric stat ids.
    enum MultiplayerStatIds
    {
        MultiplayerStat_EntityCount = 1001,         // Number of multiplayer entities active
        MultiplayerStat_FrameTimeUs,                // Multiplayer tick cost within a single application frame
        MultiplayerStat_ClientConnectionCount,      // Current connection (applicable to a server)
        MultiplayerStat_ApplicationFrameTimeUs,     // The entire application frame time
        MultiplayerStat_DesyncCorrections,          // Number of corrections (desyncs)

        // Network stats
        MultiplayerStat_TotalTimeSpentUpdatingMs,
        MultiplayerStat_TotalSendTimeMs,
        MultiplayerStat_TotalSentPackets,
        MultiplayerStat_TotalSentBytesAfterCompression,
        MultiplayerStat_TotalSentBytesBeforeCompression,
        MultiplayerStat_TotalResentPacketsDueToPacketLoss,
        MultiplayerStat_TotalReceiveTimeInMs,
        MultiplayerStat_TotalReceivedPackets,
        MultiplayerStat_TotalReceivedBytesAfterCompression,
        MultiplayerStat_TotalReceivedBytesBeforeCompression,
        MultiplayerStat_TotalPacketsDiscardedDueToLoad,

        // Other systems
        MultiplayerStat_PhysicsFrameTimeUs,
    };
}

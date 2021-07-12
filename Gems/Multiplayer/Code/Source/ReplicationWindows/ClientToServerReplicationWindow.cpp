/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ReplicationWindows/ClientToServerReplicationWindow.h>

namespace Multiplayer
{
    bool ClientToServerReplicationWindow::ReplicationSetUpdateReady()
    {
        return true;
    }

    const ReplicationSet& ClientToServerReplicationWindow::GetReplicationSet() const
    {
        return m_emptySet;
    }

    uint32_t ClientToServerReplicationWindow::GetMaxEntityReplicatorSendCount() const
    {
        return 128;
    }

    bool ClientToServerReplicationWindow::IsInWindow([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle, NetEntityRole& outNetworkRole) const
    {
        outNetworkRole = NetEntityRole::InvalidRole;
        return false;
    }

    void ClientToServerReplicationWindow::UpdateWindow()
    {
        ;
    }

    void ClientToServerReplicationWindow::DebugDraw() const
    {
        // Nothing to draw
    }
}

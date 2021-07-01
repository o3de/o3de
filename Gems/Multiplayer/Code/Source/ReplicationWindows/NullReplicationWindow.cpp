/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ReplicationWindows/NullReplicationWindow.h>

namespace Multiplayer
{
    bool NullReplicationWindow::ReplicationSetUpdateReady()
    {
        return true;
    }

    const ReplicationSet& NullReplicationWindow::GetReplicationSet() const
    {
        return m_emptySet;
    }

    uint32_t NullReplicationWindow::GetMaxEntityReplicatorSendCount() const
    {
        return 0;
    }

    bool NullReplicationWindow::IsInWindow([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle, NetEntityRole& outNetworkRole) const
    {
        outNetworkRole = NetEntityRole::InvalidRole;
        return false;
    }

    void NullReplicationWindow::UpdateWindow()
    {
        ;
    }

    void NullReplicationWindow::DebugDraw() const
    {
        // Nothing to draw
    }
}

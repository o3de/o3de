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

#include "NullReplicationWindow.h"

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

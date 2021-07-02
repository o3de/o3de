/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/ReplicationWindows/IReplicationWindow.h>

namespace Multiplayer
{
    class NullReplicationWindow
        : public IReplicationWindow
    {
    public:
        NullReplicationWindow() = default;

        //! IReplicationWindow interface
        //! @{
        bool ReplicationSetUpdateReady() override;
        const ReplicationSet& GetReplicationSet() const override;
        uint32_t GetMaxEntityReplicatorSendCount() const override;
        bool IsInWindow(const ConstNetworkEntityHandle& entityPtr, NetEntityRole& outNetworkRole) const override;
        void UpdateWindow() override;
        void DebugDraw() const override;
        //! @}

    private:
        ReplicationSet m_emptySet;
    };
}

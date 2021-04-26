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

#include <Source/ReplicationWindows/IReplicationWindow.h>

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

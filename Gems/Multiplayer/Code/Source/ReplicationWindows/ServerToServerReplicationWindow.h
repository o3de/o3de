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
#include <Source/NetworkEntity/NetworkEntityHandle.h>
#include <Source/NetworkEntity/INetworkEntityManager.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace Multiplayer
{
    class ServerToServerReplicationWindow
        : public IReplicationWindow
    {
    public:
        ServerToServerReplicationWindow(const AZ::Aabb& aabb);

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
        void OnControllersActivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating);
        void OnControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating);

        ServerToServerReplicationWindow& operator=(const ServerToServerReplicationWindow&) = delete;

        ReplicationSet m_replicationSet;
        AZ::ScheduledEvent m_updateWindowEvent;
        ControllersActivatedEvent::Handler m_controllersActivatedHandler;
        ControllersDeactivatedEvent::Handler m_controllersDeactivatedHandler;
        AZ::Aabb m_aabb;
        bool m_initialGatherComplete = false; // Replication window need to run an initial gather on connection
    };
}

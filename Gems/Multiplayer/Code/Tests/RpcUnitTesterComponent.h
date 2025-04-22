/*
* Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#pragma once

#include <Tests/AutoGen/RpcUnitTesterComponent.AutoComponent.h>

namespace MultiplayerTest
{
    class RpcUnitTesterComponent
        : public RpcUnitTesterComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(MultiplayerTest::RpcUnitTesterComponent, s_rpcUnitTesterComponentConcreteUuid, MultiplayerTest::RpcUnitTesterComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        RpcUnitTesterComponentController* GetTestController();

        void HandleRPC_AuthorityToClient([[maybe_unused]] AzNetworking::IConnection* invokingConnection) override
        {
            m_authorityToClientCalls++;
        }
        int m_authorityToClientCalls = 0;
    };

    class RpcUnitTesterComponentController
        : public RpcUnitTesterComponentControllerBase
    {
    public:
        explicit RpcUnitTesterComponentController(RpcUnitTesterComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        void HandleRPC_ServerToAuthority([[maybe_unused]] AzNetworking::IConnection* invokingConnection) override
        {
            m_serverToAuthorityCalls++;
        }
        int m_serverToAuthorityCalls = 0;

        void HandleRPC_AuthorityToAutonomous([[maybe_unused]] AzNetworking::IConnection* invokingConnection) override
        {
            m_authorityToAutonomousCalls++;
        }
        int m_authorityToAutonomousCalls = 0;

        void HandleRPC_AutonomousToAuthority([[maybe_unused]] AzNetworking::IConnection* invokingConnection) override
        {
            m_autonomousToAuthorityCalls++;
        }
        int m_autonomousToAuthorityCalls = 0;
    };
}

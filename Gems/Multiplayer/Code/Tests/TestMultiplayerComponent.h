/*
* Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#pragma once

#include <Tests/AutoGen/TestMultiplayerComponent.AutoComponent.h>

namespace MultiplayerTest
{
    // Dummy class for satisfying "MultiplayerInputDriver" component dependency
    class TestInputDriverComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestInputDriverComponent, "{C3877905-3B61-45AE-A636-9845C3AAA39D}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.emplace_back(AZ_CRC_CE("MultiplayerInputDriver"));
        }

        void Activate() override {}
        void Deactivate() override {}
    };

    // Test multiplayer component with ability to create and process network input
    class TestMultiplayerComponent
        : public TestMultiplayerComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(MultiplayerTest::TestMultiplayerComponent, s_testMultiplayerComponentConcreteUuid, MultiplayerTest::TestMultiplayerComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnNetworkActivated() override;

        AZStd::function<void(Multiplayer::NetEntityId, Multiplayer::NetworkInput& input, float deltaTime)> m_createInputCallback;
        AZStd::function<void(Multiplayer::NetEntityId, Multiplayer::NetworkInput& input, float deltaTime)> m_processInputCallback;
    };

    // Multiplayer controller for the test component
    class TestMultiplayerComponentController
        : public TestMultiplayerComponentControllerBase
    {
    public:
        TestMultiplayerComponentController(TestMultiplayerComponent& parent);

        //! TestMultiplayerComponentControllerBase
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        //! MultiplayerController interface
        void CreateInput(Multiplayer::NetworkInput& input, float deltaTime) override;
        void ProcessInput(Multiplayer::NetworkInput& input, float deltaTime) override;
    };
}

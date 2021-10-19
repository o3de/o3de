/*
* Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#include <Tests/TestMultiplayerComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace MultiplayerTest
{
    void TestInputDriverComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TestInputDriverComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void TestMultiplayerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TestMultiplayerComponent, TestMultiplayerComponentBase>()
                ->Version(1);
        }
        TestMultiplayerComponentBase::Reflect(context);
    }

    void TestMultiplayerComponent::OnInit()
    {
    }

    void TestMultiplayerComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void TestMultiplayerComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    TestMultiplayerComponentController::TestMultiplayerComponentController(TestMultiplayerComponent& parent)
        : TestMultiplayerComponentControllerBase(parent)
    {
    }

    void TestMultiplayerComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void TestMultiplayerComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void TestMultiplayerComponentController::CreateInput(Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        auto* networkInput = input.FindComponentInput<TestMultiplayerComponentNetworkInput>();
        networkInput->m_ownerId = GetParent().GetId();
    }

    void TestMultiplayerComponentController::ProcessInput(Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        auto& component = GetParent();
        [[maybe_unused]] auto* networkInput = input.FindComponentInput<TestMultiplayerComponentNetworkInput>();
        AZ_Assert(networkInput->m_ownerId == component.GetId(), "Input Id doesn't match the owner component Id");

        if (component.m_processInputCallback)
        {
            component.m_processInputCallback(GetNetEntityId());
        }
    }
}

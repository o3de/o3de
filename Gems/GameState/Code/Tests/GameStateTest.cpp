/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameStateSystemComponent.h>

#include <AzTest/AzTest.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>

class GameStateTest
    : public UnitTest::LeakDetectionFixture
{
protected:
    void SetUp() override
    {
        LeakDetectionFixture::SetUp();
        m_gameStateSystemComponent = AZStd::make_unique<GameState::GameStateSystemComponent>();
        m_gameStateSystemComponent->GameState::GameStateRequestBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        m_gameStateSystemComponent->GameState::GameStateRequestBus::Handler::BusDisconnect();
        m_gameStateSystemComponent.reset();
        LeakDetectionFixture::TearDown();
    }

private:
    AZStd::unique_ptr<GameState::GameStateSystemComponent> m_gameStateSystemComponent;
};

class TestGameStateA : public GameState::IGameState
{
    public:
        AZ_CLASS_ALLOCATOR(TestGameStateA, AZ::SystemAllocator);
        AZ_RTTI(TestGameStateA, "{81345EC1-3F5F-4F6E-AEC0-49143BE8D133}", IGameState);

        void OnPushed() override
        {
            ASSERT_TRUE(!m_isPushed);
            m_isPushed = true;
        }

        void OnPopped() override
        {
            ASSERT_TRUE(m_isPushed);
            m_isPushed = false;
        }

        void OnEnter() override
        {
            ASSERT_TRUE(m_isPushed);
            ASSERT_TRUE(!m_isActive);
            m_isActive = true;
        }

        void OnExit() override
        {
            ASSERT_TRUE(m_isPushed);
            ASSERT_TRUE(m_isActive);
            m_isActive = false;
        }

    private:
        bool m_isPushed = false;
        bool m_isActive = false;
};

class TestGameStateB : public TestGameStateA
{
    public:
        AZ_CLASS_ALLOCATOR(TestGameStateB, AZ::SystemAllocator);
        AZ_RTTI(TestGameStateB, "{DBA86F9F-DEAF-426D-8496-AC9A20256E5D}", TestGameStateA);
};

class TestGameStateC : public TestGameStateA
{
    public:
        AZ_CLASS_ALLOCATOR(TestGameStateC, AZ::SystemAllocator);
        AZ_RTTI(TestGameStateC, "{F6C6C512-9F19-4B2B-A8B2-A0F0552E27EB}", TestGameStateA);
};

class TestGameStateX : public GameState::IGameState
{
    public:
        AZ_CLASS_ALLOCATOR(TestGameStateX, AZ::SystemAllocator);
        AZ_RTTI(TestGameStateX, "{FCF63A12-ED21-4432-AB71-F268CC49126E}", IGameState);
};

TEST_F(GameStateTest, PushThenPopOneGameState)
{
    // Push A
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());

    // Pop A
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
}

TEST_F(GameStateTest, PushThenPopTwoGameStates)
{
    // Push A
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    
    // Push B
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateB>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    
    // Pop B
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());

    // Pop A
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
}

TEST_F(GameStateTest, PopAllGameStates)
{
    // Push A
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    
    // Push B
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateB>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());

    // Push C
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateC>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Pop all game states
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopAllGameStates);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Check the active game state is null
    AZStd::shared_ptr<GameState::IGameState> activeGameState;
    GameState::GameStateRequestBus::BroadcastResult(activeGameState, &GameState::GameStateRequests::GetActiveGameState);
    ASSERT_TRUE(activeGameState == nullptr);
}

TEST_F(GameStateTest, PopActiveGameStateUntilOfType)
{
    // Push A
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    
    // Push B
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateB>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());

    // Push C
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateC>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Pop until C is active (ie. do nothing)
    ASSERT_TRUE(GameState::GameStateRequests::PopActiveGameStateUntilOfType<TestGameStateC>());
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Pop until something inheriting from A is active (which C does, so do nothing again)
    ASSERT_TRUE(GameState::GameStateRequests::PopActiveGameStateUntilOfType<TestGameStateA>());
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Pop until something inheriting from B is active (which C doesn't)
    ASSERT_TRUE(GameState::GameStateRequests::PopActiveGameStateUntilOfType<TestGameStateB>());
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());

    // Pop until something inheriting from C is active (C is no longer in the stack)
    ASSERT_FALSE(GameState::GameStateRequests::PopActiveGameStateUntilOfType<TestGameStateC>());
}

TEST_F(GameStateTest, ReplaceActiveGameState)
{
    // Push A
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());

    // Replace A with B
    AZStd::shared_ptr<GameState::IGameState> gameStateB = AZStd::make_shared<TestGameStateB>();
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::ReplaceActiveGameState, gameStateB);
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());

    // Pop B
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
}

TEST_F(GameStateTest, DoesStackContainGameStateOfType)
{
    // Push A
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateC>());
    
    // Push B
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateB>();
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateA>());
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateC>());

    // Push C
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateC>();
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateA>());
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateB>());
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateC>());
    
    // Pop C
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateA>());
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateC>());
    
    // Pop B
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_TRUE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateC>());

    // Pop A
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::DoesStackContainGameStateOfType<TestGameStateC>());
}

TEST_F(GameStateTest, PushSameGameStateTwice)
{
    // Push A
    AZStd::shared_ptr<GameState::IGameState> gameStateA = AZStd::make_shared<TestGameStateA>();
    bool result = false;
    GameState::GameStateRequestBus::BroadcastResult(result, &GameState::GameStateRequests::PushGameState, gameStateA);
    ASSERT_TRUE(result);
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    
    // Push same instance of A again
    GameState::GameStateRequestBus::BroadcastResult(result, &GameState::GameStateRequests::PushGameState, gameStateA);
    ASSERT_FALSE(result);
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());

    // Pop A
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
}

TEST_F(GameStateTest, AddGameStateFactoryOverrideWithDerived)
{
    // Override A with B
    ASSERT_TRUE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateB>();
    }));

    // Push A (overriden by B)
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());

    // Pop A (overriden by B)
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
}

TEST_F(GameStateTest, AddGameStateFactoryOverrideWithNotDerived)
{
    // Override A with X
    ASSERT_FALSE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateX>();
    }));

    // Push A (not overriden by X)
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateX>());

    // Pop A (not overriden by X)
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateX>());
}

TEST_F(GameStateTest, AddGameStateFactoryOverrideButDontCheck)
{
    // Override A with B
    ASSERT_TRUE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateB>();
    }));

    // Push A (overriden by B, but don't check for overrides)
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>(false);
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
}

TEST_F(GameStateTest, AddGameStateFactoryOverrideTwice)
{
    // Override A with B
    ASSERT_TRUE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateB>();
    }));

    // Try override A with C
    ASSERT_FALSE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateC>();
    }));

    // Try override A with B again
    ASSERT_FALSE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateB>();
    }));
}

TEST_F(GameStateTest, RemoveGameStateFactoryOverride)
{
    // Override A with B
    ASSERT_TRUE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateB>();
    }));

    // Push A (overriden by B)
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Remove Override A
    ASSERT_TRUE(GameState::GameStateRequests::RemoveGameStateFactoryOverrideForType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::RemoveGameStateFactoryOverrideForType<TestGameStateA>());

    // Push A
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Override A with C
    ASSERT_TRUE(GameState::GameStateRequests::AddGameStateFactoryOverrideForType<TestGameStateA>([]()
    {
        return AZStd::make_shared<TestGameStateC>();
    }));

    // Push A (overriden by C)
    GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<TestGameStateA>();
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Remove Override A
    ASSERT_TRUE(GameState::GameStateRequests::RemoveGameStateFactoryOverrideForType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::RemoveGameStateFactoryOverrideForType<TestGameStateA>());

    // Pop A (overriden by C)
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Pop A
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_TRUE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());

    // Pop A (overriden by B)
    GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateA>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateB>());
    ASSERT_FALSE(GameState::GameStateRequests::IsActiveGameStateOfType<TestGameStateC>());
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

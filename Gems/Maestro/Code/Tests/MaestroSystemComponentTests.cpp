/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>

#include <Maestro/MaestroSystemComponent.h>
#include <Maestro/Types/SequenceType.h>

#include <ISystem.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

namespace Maestro
{
    class CompetingSequenceSystemLifecycle
        : public AzFramework::ISequenceSystemLifecycle
    {
    public:
        void OnSystemCVarRegistry() override {}
        void OnSystemInitialized(bool) override {}
        void OnSystemShutdown() override {}
        void OnPreUpdate(bool) override {}
        void OnPostUpdate(bool) override {}
        void OnLevelEntitiesReady() override {}
        void OnLevelUnload() override {}
    };

    class TestMaestroSystemComponent
        : public MaestroSystemComponent
    {
    public:
        using MaestroSystemComponent::Activate;
        using MaestroSystemComponent::Deactivate;
        using MaestroSystemComponent::OnSystemCVarRegistry;
        using MaestroSystemComponent::OnSystemInitialized;
        using MaestroSystemComponent::OnSystemShutdown;
        using MaestroSystemComponent::OnPreUpdate;
        using MaestroSystemComponent::OnPostUpdate;
        using MaestroSystemComponent::OnLevelEntitiesReady;
        using MaestroSystemComponent::OnLevelUnload;
    };

    TEST(MaestroSystemComponentTests, ExistingComponentTypeIdsRemainStable)
    {
        EXPECT_EQ(
            azrtti_typeid<MaestroAllocatorComponent>(),
            AZ::TypeId("{3636E0F4-5208-450F-83F4-BE09F6EE7FBC}"));
        EXPECT_EQ(
            azrtti_typeid<MaestroSystemComponent>(),
            AZ::TypeId("{47991994-4417-4CD7-AE0B-FEF1C8720766}"));
    }

    TEST(MaestroSystemComponentTests, RegistersAsTheSingleRuntimeSequenceLifecycleProvider)
    {
        ASSERT_EQ(AzFramework::ISequenceSystemLifecycle::Get(), nullptr);

        TestMaestroSystemComponent component;
        component.Activate();
        EXPECT_NE(AzFramework::ISequenceSystemLifecycle::Get(), nullptr);

        CompetingSequenceSystemLifecycle competingProvider;
        EXPECT_FALSE(AzFramework::SequenceSystemLifecycleInterface::Register(&competingProvider).IsSuccess());

        component.Deactivate();
        EXPECT_EQ(AzFramework::ISequenceSystemLifecycle::Get(), nullptr);
    }

    TEST(MaestroSystemComponentTests, OwnsLegacyMovieCVarsAndShutdownIsIdempotent)
    {
        ::testing::StrictMock<ConsoleMock> console;
        SSystemGlobalEnvironment environment{};
        environment.pConsole = &console;
        SSystemGlobalEnvironment* previousEnvironment = gEnv;
        gEnv = &environment;

        EXPECT_CALL(
            console,
            Register(
                ::testing::StrEq("sys_maxTimeStepForMovieSystem"),
                ::testing::A<float*>(),
                0.1f,
                VF_NULL,
                ::testing::_,
                ConsoleVarFunc{},
                true));
        EXPECT_CALL(
            console,
            Register(
                ::testing::StrEq("sys_trackview"),
                ::testing::A<int*>(),
                1,
                VF_NULL,
                ::testing::_,
                ConsoleVarFunc{},
                true));

        TestMaestroSystemComponent component;
        component.OnSystemCVarRegistry();
        component.OnSystemInitialized(true);

        {
            ::testing::InSequence unregisterOrder;
            EXPECT_CALL(console, UnregisterVariable(::testing::StrEq("sys_trackview"), false));
            EXPECT_CALL(console, UnregisterVariable(::testing::StrEq("sys_maxTimeStepForMovieSystem"), false));
        }
        component.OnSystemShutdown();

        // Repeated cleanup notifications must not touch an already-released console or movie system.
        component.OnSystemShutdown();
        gEnv = previousEnvironment;
    }

    class MaestroRuntimeSchedulingTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_previousEnvironment = gEnv;
            m_environment.pConsole = &m_console;
            m_environment.pSystem = &m_system;
            gEnv = &m_environment;
            ON_CALL(m_time, GetLastSimulationTickTime()).WillByDefault(::testing::Return(AZ::TimeUs{1}));
            ON_CALL(m_time, GetSimulationTickDeltaOverride()).WillByDefault(::testing::Return(AZ::Time::ZeroTimeUs));
            ON_CALL(m_time, GetRealTickDeltaTimeUs()).WillByDefault([this]() { return AZ::TimeUs{m_realDelta}; });
            EXPECT_CALL(m_console, Register(::testing::StrEq("sys_trackview"), ::testing::A<int*>(),
                1, VF_NULL, ::testing::_, ConsoleVarFunc{}, true))
                .WillOnce(::testing::DoAll(::testing::SaveArg<1>(&m_enabled), ::testing::Return(nullptr)));
            EXPECT_CALL(m_console, Register(::testing::StrEq("sys_maxTimeStepForMovieSystem"), ::testing::A<float*>(),
                0.1f, VF_NULL, ::testing::_, ConsoleVarFunc{}, true))
                .WillOnce(::testing::DoAll(::testing::SaveArg<1>(&m_maxDelta), ::testing::Return(nullptr)));
            m_component.Activate();
            m_component.OnSystemCVarRegistry();
            ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_console));
            m_component.OnSystemInitialized(false);
            m_movie = AZ::Interface<IMovieSystem>::Get();
            ASSERT_NE(m_movie, nullptr);
            m_sequence = m_movie->CreateSequence("Scheduling", false, 0, SequenceType::SequenceComponent, AZ::EntityId(42));
            m_movie->PlaySequence(m_sequence, nullptr, true, false);
        }

        void TearDown() override
        {
            m_component.OnLevelUnload();
            m_component.Deactivate();
            EXPECT_EQ(AZ::Interface<IMovieSystem>::Get(), nullptr);
            EXPECT_EQ(AzFramework::ISequenceSystemLifecycle::Get(), nullptr);
            gEnv = m_previousEnvironment;
        }

        ::testing::NiceMock<ConsoleMock> m_console;
        ::testing::NiceMock<SystemMock> m_system;
        ::testing::NiceMock<AZ::MockTimeSystem> m_time;
        SSystemGlobalEnvironment m_environment{};
        SSystemGlobalEnvironment* m_previousEnvironment = nullptr;
        TestMaestroSystemComponent m_component;
        IMovieSystem* m_movie = nullptr;
        IAnimSequence* m_sequence = nullptr;
        int* m_enabled = nullptr;
        float* m_maxDelta = nullptr;
        int64_t m_realDelta = 50000;
    };

    TEST_F(MaestroRuntimeSchedulingTests, UsesRealTimeRatherThanSimulationDeltaAndDoesNotAdvanceTwice)
    {
        ON_CALL(m_time, GetSimulationTickDeltaTimeUs()).WillByDefault(::testing::Return(AZ::TimeUs{900000}));
        m_component.OnPreUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.05f);
        m_component.OnPostUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.05f);
    }

    TEST_F(MaestroRuntimeSchedulingTests, LongFrameClampsToExistingDefaultAndHonorsCVarChanges)
    {
        m_realDelta = 5000000;
        m_component.OnPreUpdate(false);
        m_component.OnPostUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.1f);
        *m_maxDelta = 0.2f;
        m_component.OnPreUpdate(false);
        m_component.OnPostUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.3f);
    }

    TEST_F(MaestroRuntimeSchedulingTests, EditorUpdatesAndDisabledTrackViewDoNotAdvance)
    {
        m_component.OnPreUpdate(true);
        m_component.OnPostUpdate(true);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.0f);
        *m_enabled = 0;
        m_component.OnPreUpdate(false);
        m_component.OnPostUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.0f);
        *m_enabled = 1;
        m_component.OnPreUpdate(false);
        m_component.OnPostUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.05f);
    }

    TEST_F(MaestroRuntimeSchedulingTests, MoviePauseStillAppliesThroughLifecycleCallbacks)
    {
        m_movie->Pause();
        m_component.OnPreUpdate(false);
        m_component.OnPostUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.0f);
        m_movie->Resume();
        m_component.OnPreUpdate(false);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.05f);
    }

    TEST_F(MaestroRuntimeSchedulingTests, ReadyRestartsAutoplayAndRepeatedUnloadIsSafe)
    {
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_PlayOnReset);
        m_component.OnPreUpdate(false);
        m_component.OnLevelEntitiesReady();
        EXPECT_TRUE(m_movie->IsPlaying(m_sequence));
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.0f);
        m_component.OnLevelUnload();
        m_component.OnLevelUnload();
        EXPECT_EQ(m_movie->GetNumSequences(), 0);
        EXPECT_EQ(m_movie->GetNumPlayingSequences(), 0);
    }

    TEST_F(MaestroRuntimeSchedulingTests, ShutdownRemovesMovieInterfaceAndLaterNotificationsAreHarmless)
    {
        m_component.OnLevelUnload();
        m_component.OnSystemShutdown();
        EXPECT_EQ(AZ::Interface<IMovieSystem>::Get(), nullptr);
        m_component.OnPreUpdate(false);
        m_component.OnPostUpdate(false);
        m_component.OnLevelEntitiesReady();
        m_component.OnLevelUnload();
        m_component.OnSystemShutdown();
    }
}

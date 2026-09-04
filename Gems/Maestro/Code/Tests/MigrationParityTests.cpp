/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzTest/AzTest.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>
#include <Maestro/Types/SequenceType.h>

// This file is deliberately shared, unchanged, with the pre-extraction worktree.
// Only the physical include location differs. Assertions must not vary by revision.
#if __has_include(<Maestro/Cinematics/Movie.h>)
#include <Maestro/Cinematics/Movie.h>
#else
#include <Cinematics/Movie.h>
#endif

namespace Maestro::MigrationParity
{
    class MoviePlaybackTests
        : public ::testing::Test
        , public IMovieListener
    {
    protected:
        void SetUp() override
        {
            ASSERT_EQ(AZ::Interface<IMovieSystem>::Get(), nullptr);
            m_previousEnvironment = gEnv;
            m_environment.pConsole = &m_console;
            m_environment.pSystem = &m_system;
            gEnv = &m_environment;
            ON_CALL(m_time, GetLastSimulationTickTime()).WillByDefault([this]() { return AZ::TimeUs{m_frame}; });
            ON_CALL(m_time, GetSimulationTickDeltaOverride()).WillByDefault(::testing::Return(AZ::Time::ZeroTimeUs));
            m_movie = aznew CMovieSystem(&m_system);
            m_sequence = m_movie->CreateSequence("MigrationParity", false, 0, SequenceType::SequenceComponent, AZ::EntityId(42));
            m_sequence->SetTimeRange(Range(0.0f, 4.0f));
            m_movie->AddMovieListener(m_sequence, this);
        }

        void TearDown() override
        {
            m_movie->RemoveMovieListener(m_sequence, this);
            m_movie->RemoveAllSequences();
            delete m_movie;
            gEnv = m_previousEnvironment;
        }

        void OnMovieEvent(EMovieEvent event, IAnimSequence*) override
        {
            m_events.push_back(event);
        }

        void Frame(float deltaTime)
        {
            ++m_frame;
            m_movie->PreUpdate(deltaTime);
            m_movie->PostUpdate(deltaTime);
        }

        ::testing::NiceMock<ConsoleMock> m_console;
        ::testing::NiceMock<SystemMock> m_system;
        ::testing::NiceMock<AZ::MockTimeSystem> m_time;
        SSystemGlobalEnvironment m_environment{};
        SSystemGlobalEnvironment* m_previousEnvironment = nullptr;
        CMovieSystem* m_movie = nullptr;
        IAnimSequence* m_sequence = nullptr;
        AZStd::vector<EMovieEvent> m_events;
        int64_t m_frame = 1;
    };

    TEST_F(MoviePlaybackTests, SequenceIdentityAndLookupSurvivePlayback)
    {
        EXPECT_EQ(m_movie->FindSequence(AZ::EntityId(42)), m_sequence);
        EXPECT_EQ(m_movie->FindSequenceById(m_sequence->GetId()), m_sequence);
        EXPECT_EQ(m_movie->FindLegacySequenceByName("migrationparity"), m_sequence);
        EXPECT_EQ(m_movie->GetNumSequences(), 1);
        m_movie->PlaySequence(m_sequence);
        m_movie->PlaySequence(m_sequence);
        EXPECT_EQ(m_movie->GetNumPlayingSequences(), 1);
        EXPECT_EQ(m_movie->GetPlayingSequence(0), m_sequence);
        EXPECT_EQ(m_events, (AZStd::vector<EMovieEvent>{eMovieEvent_Started}));
    }

    TEST_F(MoviePlaybackTests, NormalSequenceAdvancesBeforeTickButAnimatesAfterTick)
    {
        m_movie->PlaySequence(m_sequence);
        m_events.clear();
        m_movie->PreUpdate(0.25f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.25f);
        EXPECT_TRUE(m_events.empty());
        m_movie->PostUpdate(0.25f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.25f);
        EXPECT_EQ(m_events, (AZStd::vector<EMovieEvent>{eMovieEvent_Updated}));
    }

    TEST_F(MoviePlaybackTests, EarlySequenceAnimatesBeforeTickOnly)
    {
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_EarlyMovieUpdate);
        m_movie->PlaySequence(m_sequence);
        m_events.clear();
        m_movie->PreUpdate(0.25f);
        EXPECT_EQ(m_events, (AZStd::vector<EMovieEvent>{eMovieEvent_Updated}));
        m_events.clear();
        m_movie->PostUpdate(0.25f);
        EXPECT_TRUE(m_events.empty());
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.25f);
    }

    TEST_F(MoviePlaybackTests, MoviePauseAndSequencePauseDoNotAdvanceTime)
    {
        m_movie->PlaySequence(m_sequence);
        m_movie->Pause();
        Frame(0.5f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.0f);
        m_movie->Resume();
        Frame(0.5f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.5f);
        m_sequence->Pause();
        Frame(0.5f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.5f);
        m_sequence->Resume();
        Frame(0.5f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 1.0f);
    }

    TEST_F(MoviePlaybackTests, SpeedAndSeekFlagsPreserveExistingSemantics)
    {
        EXPECT_FALSE(m_movie->SetPlayingTime(m_sequence, 1.0f));
        m_movie->PlaySequence(m_sequence);
        EXPECT_TRUE(m_movie->SetPlayingSpeed(m_sequence, 2.0f));
        Frame(0.25f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.5f);
        EXPECT_TRUE(m_movie->SetPlayingTime(m_sequence, 2.0f));
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 2.0f);
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_NoSeek | IAnimSequence::eSeqFlags_NoSpeed);
        EXPECT_FALSE(m_movie->SetPlayingTime(m_sequence, 3.0f));
        EXPECT_FALSE(m_movie->SetPlayingSpeed(m_sequence, 0.0f));
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 2.0f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingSpeed(m_sequence), 2.0f);
    }

    TEST_F(MoviePlaybackTests, FixedDeltaOverrideAppliesExactlyOncePerFrame)
    {
        ON_CALL(m_time, GetSimulationTickDeltaOverride()).WillByDefault(::testing::Return(AZ::TimeUs{100000}));
        m_movie->PlaySequence(m_sequence);
        Frame(0.7f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.1f);
    }

    TEST_F(MoviePlaybackTests, LastFrameIsEvaluatedBeforeStopping)
    {
        m_movie->PlaySequence(m_sequence);
        Frame(4.0f);
        EXPECT_TRUE(m_movie->IsPlaying(m_sequence));
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 4.0f);
        Frame(0.01f);
        EXPECT_FALSE(m_movie->IsPlaying(m_sequence));
        EXPECT_EQ(m_events.back(), eMovieEvent_Stopped);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), -1.0f);
    }

    TEST_F(MoviePlaybackTests, LoopRestartsAtStartWithoutCarryingOvershoot)
    {
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_OutOfRangeLoop);
        m_movie->PlaySequence(m_sequence);
        Frame(4.5f);
        EXPECT_TRUE(m_movie->IsPlaying(m_sequence));
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.0f);
        Frame(0.25f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.25f);
    }

    TEST_F(MoviePlaybackTests, ConstantRangeContinuesAndBatchCaptureOverridesLoop)
    {
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_OutOfRangeConstant);
        m_movie->PlaySequence(m_sequence);
        Frame(4.5f);
        EXPECT_TRUE(m_movie->IsPlaying(m_sequence));
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 4.5f);
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_OutOfRangeLoop);
        m_movie->EnableBatchRenderMode(true);
        Frame(0.1f);
        EXPECT_FALSE(m_movie->IsPlaying(m_sequence));
    }

    TEST_F(MoviePlaybackTests, ResetAbortsThenResetsAndRestartsAutoplayInOrder)
    {
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_PlayOnReset);
        m_movie->PlaySequence(m_sequence);
        Frame(0.5f);
        m_movie->Pause();
        m_events.clear();
        m_movie->Reset(true, false);
        EXPECT_EQ(m_events, (AZStd::vector<EMovieEvent>{
            eMovieEvent_Aborted, eMovieEvent_Started, eMovieEvent_Stopped, eMovieEvent_Started}));
        EXPECT_TRUE(m_movie->IsPlaying(m_sequence));
        Frame(0.25f);
        EXPECT_FLOAT_EQ(m_movie->GetPlayingTime(m_sequence), 0.25f);
    }

    TEST_F(MoviePlaybackTests, ResetWithoutAutoplayAndRepeatedRemovalLeaveNoSequences)
    {
        m_sequence->SetFlags(IAnimSequence::eSeqFlags_PlayOnReset);
        m_movie->PlaySequence(m_sequence);
        m_movie->Reset(false, false);
        EXPECT_FALSE(m_movie->IsPlaying(m_sequence));
        m_movie->RemoveMovieListener(m_sequence, this);
        m_movie->RemoveAllSequences();
        m_movie->RemoveAllSequences();
        EXPECT_EQ(m_movie->GetNumSequences(), 0);
        EXPECT_EQ(m_movie->GetNumPlayingSequences(), 0);
        EXPECT_EQ(m_movie->FindSequence(AZ::EntityId(42)), nullptr);
        m_sequence = nullptr;
    }

    TEST_F(MoviePlaybackTests, StopAndAbortHaveDistinctNotifications)
    {
        m_movie->PlaySequence(m_sequence);
        m_events.clear();
        EXPECT_TRUE(m_movie->StopSequence(m_sequence));
        EXPECT_FALSE(m_movie->StopSequence(m_sequence));
        EXPECT_EQ(m_events, (AZStd::vector<EMovieEvent>{eMovieEvent_Stopped}));
        m_movie->PlaySequence(m_sequence);
        m_events.clear();
        EXPECT_TRUE(m_movie->AbortSequence(m_sequence, true));
        EXPECT_EQ(m_events, (AZStd::vector<EMovieEvent>{eMovieEvent_Aborted}));
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>

#include <Integration/System/SystemCommon.h>

#include <Tests/SystemComponentFixture.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>

#include <AzCore/Math/MathUtils.h>

#include <vector>


namespace EMotionFX
{

    // The start state of the motion instance.
    struct MotionInstanceInputState
    {
        float m_currentTime;
        float m_playSpeed;
        bool m_freezeAtLLastFrame;
        AZ::u32 m_numCurrentLoops;
        AZ::u32 m_maxNumLoops;
        EPlayMode m_playMode;
    };

    // The expected output state of the motion instance.
    struct MotionInstanceOutputState
    {
        float m_currentTime;
        float m_lastCurrentTime;
        AZ::u32 m_numLoops;
        bool m_hasEnded;
        bool m_hasLooped;
        bool m_isFrozen;
    };

    // Test parameters (input and expected output).
    struct MotionInstanceTestParams
    {
        MotionInstanceInputState m_inputState;
        MotionInstanceOutputState m_outputState;
        float m_deltaTime;
    };

    class MotionInstanceFixture
        : public SystemComponentFixture
        , public ::testing::WithParamInterface<MotionInstanceTestParams>
    {
    public:
        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_motion = aznew Motion("MotionInstanceTest");
            m_motion->SetMotionData(aznew NonUniformMotionData());
            m_motion->GetMotionData()->SetDuration(1.0f);
            m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(5);
            ASSERT_NE(m_actor.get(), nullptr) << "Expected actor to not be a nullptr.";
            m_actorInstance = ActorInstance::Create(m_actor.get());
            ASSERT_NE(m_actorInstance, nullptr) << "Expected actor instance to not be a nullptr.";
            m_motionInstance = MotionInstance::Create(m_motion, m_actorInstance);
            ASSERT_NE(m_motionInstance, nullptr) << "Expected motion instance to not be a nullptr.";
        }

        void TearDown() override
        {
            if (m_motionInstance)
            {
                m_motionInstance->Destroy();
            }

            if (m_motion)
            {
                m_motion->Destroy();
            }

            if (m_actorInstance)
            {
                m_actorInstance->Destroy();
            }
            m_actor.reset();

            SystemComponentFixture::TearDown();
        }

        void InitMotionInstance(const MotionInstanceInputState& state)
        {
            m_motionInstance->SetCurrentTime(state.m_currentTime, true);
            m_motionInstance->SetPlaySpeed(state.m_playSpeed);
            m_motionInstance->SetFreezeAtLastFrame(state.m_freezeAtLLastFrame);
            m_motionInstance->SetNumCurrentLoops(state.m_numCurrentLoops);
            m_motionInstance->SetMaxLoops(state.m_maxNumLoops);
            m_motionInstance->SetPlayMode(state.m_playMode);
        }

        void VerifyOutputState(const MotionInstanceOutputState& state)
        {
            EXPECT_TRUE(AZ::IsClose(m_motionInstance->GetCurrentTime(), state.m_currentTime, AZ::Constants::FloatEpsilon)) << "Expected the current play time to be different.";
            EXPECT_TRUE(AZ::IsClose(m_motionInstance->GetLastCurrentTime(), state.m_lastCurrentTime, AZ::Constants::FloatEpsilon)) << "Expected the last current play time to be different.";
            EXPECT_EQ(m_motionInstance->GetNumCurrentLoops(), state.m_numLoops) << "Expected the current number of loops to be different.";
            EXPECT_EQ(m_motionInstance->GetHasEnded(), state.m_hasEnded) << "Expected the has ended state to be different.";
            EXPECT_EQ(m_motionInstance->GetHasLooped(), state.m_hasLooped) << "Expected the looped state to be different.";
            EXPECT_EQ(m_motionInstance->GetIsFrozen(), state.m_isFrozen) << "Expected the frozen state to be different.";
        }

    protected:
        AZStd::unique_ptr<Actor> m_actor;
        ActorInstance* m_actorInstance = nullptr;
        Motion* m_motion = nullptr;
        MotionInstance* m_motionInstance = nullptr;
    };

    TEST_P(MotionInstanceFixture, Update)
    {
        // Initialize the motion instance in our input state.
        const MotionInstanceTestParams& params = GetParam();
        InitMotionInstance(params.m_inputState);

        // Perform an update.
        m_motionInstance->Update(params.m_deltaTime);

        // Verify the expected output.
        VerifyOutputState(params.m_outputState);
    }

    std::vector<MotionInstanceTestParams> motionInstanceTestParams
    {
        //////////////////////////////// FORWARD PLAYBACK ////////////////////////////////////

        { // [0] Forward just a little bit in time.
            {   // Input state.
                0.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.1f,   // Current play time.
                0.0f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [1] Forward the exact full amount of the motion's duration, triggering a loop.
            {   // Input state.
                0.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.0f,   // Current play time.
                0.0f,   // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            1.0f // Delta update time, in seconds.
        },
        { // [2] Start near the end, trigger a loop by wrapping around.
            {   // Input state.
                0.9f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.1f,   // Current play time.
                0.9f,   // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            0.2f // Delta update time, in seconds.
        },
        { // [3] Update with a time value that is 3x as large as the motion duration.
            {   // Input state.
                0.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.5f,   // Current play time.
                0.5f,   // Last current play time.
                1,      // Current loops. This currently isn't 3, as we currently do not support handling multiple loops in one update.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            3.0f // Delta update time, in seconds.
        },
        { // [4] Start out of range, in negative time.
            {   // Input state.
                -3.5f,  // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.2f,   // Current play time.
                -3.5f,  // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.2f // Delta update time, in seconds.
        },
        { // [5] Start out of range, past the duration.
            {   // Input state.
                3.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.1f,   // Current play time.
                3.5f,   // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },

        //////////////////////////////// BACKWARD PLAYBACK ////////////////////////////////////

        { // [6] Progress just a little bit in time.
            {   // Input state.
                0.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD   // The play mode.
            },
            {   // Expected output state.
                0.4f,   // Current play time.
                0.5f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [7] Progress the exact full amount of the motion's duration, triggering a loop.
            {   // Input state.
                1.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD   // The play mode.
            },
            {   // Expected output state.
                1.0f,   // Current play time.
                1.0f,   // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            1.0f // Delta update time, in seconds.
        },
        { // [8] Start near the beginning, trigger a loop by wrapping around.
            {   // Input state.
                0.1f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD   // The play mode.
            },
            {   // Expected output state.
                0.9f,   // Current play time.
                0.1f,   // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            0.2f // Delta update time, in seconds.
        },
        { // [9] Update with a time value that is 3x as large as the motion duration.
            {   // Input state.
                0.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD   // The play mode.
            },
            {   // Expected output state.
                0.5f,   // Current play time.
                0.5f,   // Last current play time.
                1,      // Current loops. This currently isn't 3, as we currently do not support handling multiple loops in one update.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            3.0f // Delta update time, in seconds.
        },
        { // [10] Start out of range, in negative time.
            {   // Input state.
                -3.5f,  // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.9f,   // Current play time.
                -3.5f,  // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [11] Start out of range, past the duration.
            {   // Input state.
                3.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.9f,   // Current play time.
                3.5f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },

        //////////////////////////////// FORWARD PLAYBACK WHILE FREEZING AT LAST FRAME, ONE LOOP MAX ////////////////////////////////////

        { // [12] Forward just a little bit in time.
            {   // Input state.
                0.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.1f,   // Current play time.
                0.0f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [13] Forward the exact full amount of the motion's duration, triggering a loop.
            {   // Input state.
                0.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                1.0f,   // Current play time.
                0.0f,   // Last current play time.
                1,      // Current loops.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            1.0f // Delta update time, in seconds.
        },
        { // [14] Start near the end, trigger a loop by wrapping around.
            {   // Input state.
                0.9f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                1.0f,   // Current play time.
                0.9f,   // Last current play time.
                1,      // Current loops.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            0.2f // Delta update time, in seconds.
        },
        { // [15] Update with a time value that is 3x as large as the motion duration.
            {   // Input state.
                0.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                1.0f,   // Current play time.
                0.5f,   // Last current play time.
                1,      // Current loops. This currently isn't 3, as we currently do not support handling multiple loops in one update.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            3.0f // Delta update time, in seconds.
        },
        { // [16] Start out of range, in negative time.
            {   // Input state.
                -3.5f,  // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.1f,   // Current play time.
                -3.5f,  // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [17] Start out of range, past the duration.
            {   // Input state.
                3.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                1.0f,   // Current play time.
                3.5f,   // Last current play time.
                1,      // Current loops.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },

        //////////////////////////////// BACKWARD PLAYBACK WHILE FREEZING AT LAST FRAME, ONE LOOP MAX ////////////////////////////////////

        { // [18] Forward just a little bit in time.
            {   // Input state.
                1.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.9f,   // Current play time.
                1.0f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [19] Forward the exact full amount of the motion's duration, triggering a loop.
            {   // Input state.
                1.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.0f,   // Current play time.
                1.0f,   // Last current play time.
                1,      // Current loops.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            1.0f // Delta update time, in seconds.
        },
        { // [20] Start near the end, trigger a loop by wrapping around.
            {   // Input state.
                0.1f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.0f,   // Current play time.
                0.1f,   // Last current play time.
                1,      // Current loops.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            0.2f // Delta update time, in seconds.
        },
        { // [21] Update with a time value that is 3x as large as the motion duration.
            {   // Input state.
                0.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.0f,   // Current play time.
                0.5f,   // Last current play time.
                1,      // Current loops. This currently isn't 3, as we currently do not support handling multiple loops in one update.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            3.0f // Delta update time, in seconds.
        },
        { // [22] Start out of range, in negative time.
            {   // Input state.
                -3.5f,  // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.0f,   // Current play time.
                -3.5f,  // Last current play time.
                1,      // Current loops.
                true,   // Has this motion ended?
                true,   // Has looped?
                true,   // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [23] Start out of range, past the duration.
            {   // Input state.
                3.5f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                true,   // Freeze at the last frame?
                0,      // Current number of loops.
                1,      // Maximum loops allowed.
                PLAYMODE_BACKWARD    // The play mode.
            },
            {   // Expected output state.
                0.9f,   // Current play time.
                3.5f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },

        //////////////////////////////// PLAYSPEED TESTS ////////////////////////////////////

        { // [24] Forward just a little bit in time, with increased play speed.
            {   // Input state.
                0.0f,   // Current play time, in seconds.
                3.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.3f,   // Current play time.
                0.0f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [25] Forward in time but wrap around, with higher play speed.
            {   // Input state.
                0.9f,   // Current play time, in seconds.
                3.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.2f,   // Current play time.
                0.9f,   // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [26] Backward with increased play speed.
            {   // Input state.
                1.0f,   // Current play time, in seconds.
                3.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD   // The play mode.
            },
            {   // Expected output state.
                0.7f,   // Current play time.
                1.0f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },
        { // [27] Backward in time but wrap around, with higher play speed.
            {   // Input state.
                0.1f,   // Current play time, in seconds.
                3.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_BACKWARD   // The play mode.
            },
            {   // Expected output state.
                0.8f,   // Current play time.
                0.1f,   // Last current play time.
                1,      // Current loops.
                false,  // Has this motion ended?
                true,   // Has looped?
                false,  // Are we in a frozen state?
            },
            0.1f // Delta update time, in seconds.
        },

        //////////////////////////////// MISC TESTS ////////////////////////////////////

        { // [28] Zero time delta.
            {   // Input state.
                0.3f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.3f,   // Current play time.
                0.3f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.0f // Delta update time, in seconds.
        },
        { // [29] Zero time delta while on the motion duration edge.
            {   // Input state.
                1.0f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                1.0f,   // Current play time.
                1.0f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            0.0f // Delta update time, in seconds.
        },
        { // [30] Negative delta time.
            {   // Input state.
                0.2f,   // Current play time, in seconds.
                1.0f,   // Play speed.
                false,  // Freeze at the last frame?
                0,      // Current number of loops.
                EMFX_LOOPFOREVER,   // Maximum loops allowed.
                PLAYMODE_FORWARD    // The play mode.
            },
            {   // Expected output state.
                0.2f,   // Current play time.
                0.2f,   // Last current play time.
                0,      // Current loops.
                false,  // Has this motion ended?
                false,  // Has looped?
                false,  // Are we in a frozen state?
            },
            -0.5f // Delta update time, in seconds.
        }
    };

    INSTANTIATE_TEST_CASE_P(MotionInstanceTests, MotionInstanceFixture, ::testing::ValuesIn(motionInstanceTestParams));
} // namespace EMotionFX

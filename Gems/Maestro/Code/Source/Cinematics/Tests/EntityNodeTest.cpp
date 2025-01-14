/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(_RELEASE)

#include "Cinematics/CharacterTrack.h"
#include "Cinematics/CharacterTrackAnimator.h"

#include <AzTest/AzTest.h>

namespace Maestro
{
    namespace EntityNodeTest
    {
        // constants to set up test key frame, at 1.0 seconds, lasting for 1.0 seconds
        constexpr int KEY_IDX = 0;
        constexpr float KEY_TIME = 1.0f;
        constexpr float KEY_DURATION = 1.0f;

        // Testing sub-class
        class CryMovie_CharacterTrackAnimator_Test
            : public CCharacterTrackAnimator
            , public ::testing::Test
        {
        public:
            CryMovie_CharacterTrackAnimator_Test();

            void CreateTestKey();

            // a track with a single anim key frame with time=1.0sec and duration 1.0 sec
            CCharacterTrack m_dummyTrack;
        };

        // Testing setup methods
        CryMovie_CharacterTrackAnimator_Test::CryMovie_CharacterTrackAnimator_Test()
            : CCharacterTrackAnimator()
        {
            CreateTestKey();
        }

        void CryMovie_CharacterTrackAnimator_Test::CreateTestKey()
        {
            // create a key at times 1.0 sec, duration 1.0 sec
            m_dummyTrack.CreateKey(EntityNodeTest::KEY_TIME);

            ICharacterKey key;
            key.time = EntityNodeTest::KEY_TIME;
            key.m_duration = EntityNodeTest::KEY_DURATION;

            m_dummyTrack.SetKey(EntityNodeTest::KEY_IDX, &key);
        }

        // Tests
        static constexpr int NUM_ANIMATION_USER_DATA_SLOTS = 8;

        // Test CAnimEntityNode::ComputeAnimKeyNormalizedTime with a clip set not to loop
        TEST_F(CryMovie_CharacterTrackAnimator_Test, CryMovieUnitTest_CharacterTrackAnimator_ComputeAnimKeyNormalizedTime_NoLoop)
        {
            const float NORMALIZED_CLIP_START = .0f;
            const float NORMALIZED_CLIP_END = 1.0f;
            ICharacterKey key;
            m_dummyTrack.GetKey(EntityNodeTest::KEY_IDX, &key);
            key.m_bLoop = false;

            // check that the key is in what we expect for the test
            ASSERT_EQ(EntityNodeTest::KEY_TIME, key.time)
                << "Test Key frame should start at 1 second; something's wrong with the test setup.";
            ASSERT_EQ(EntityNodeTest::KEY_DURATION, key.m_duration)
                << "Test Key frame should last for 1 second; something's wrong with the test setup.";

            float test_sampleTime;
            f32 normalizedTime;

            // sample before clip starts
            test_sampleTime = KEY_TIME - 0.5f;
            normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);
            EXPECT_FLOAT_EQ(NORMALIZED_CLIP_START, normalizedTime);

            // sample after clip ends
            test_sampleTime = KEY_TIME + KEY_DURATION + 0.5f;
            normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);
            EXPECT_FLOAT_EQ(NORMALIZED_CLIP_END, normalizedTime);

            // sample at clip start/end - should return normalized time of .0f and 1.0f
            test_sampleTime = KEY_TIME;
            normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);
            EXPECT_FLOAT_EQ(NORMALIZED_CLIP_START, normalizedTime);

            test_sampleTime = KEY_TIME + KEY_DURATION;
            normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);
            EXPECT_FLOAT_EQ(NORMALIZED_CLIP_END, normalizedTime);

            // sample in different locations in the clip, before and after the clip to test looping
            const int NUM_TEST_SAMPLES = 10;
            for (int i = 0; i <= NUM_ANIMATION_USER_DATA_SLOTS; i++)
            {
                const float clipFraction = static_cast<float>(i) / static_cast<float>(NUM_TEST_SAMPLES);

                test_sampleTime = KEY_TIME + (clipFraction * KEY_DURATION);
                normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);

                EXPECT_FLOAT_EQ(clipFraction, normalizedTime);
            }
        };

        // Test CAnimEntityNode::ComputeAnimKeyNormalizedTime with a clip set  to loop
        TEST_F(CryMovie_CharacterTrackAnimator_Test, CryMovieUnitTest_CharacterTrackAnimator_ComputeAnimKeyNormalizedTime_Loop)
        {
            const float NORMALIZED_CLIP_START = .0f;
            const float ERROR_TOLERANCE = 0.0001f;
            ICharacterKey key;
            m_dummyTrack.GetKey(EntityNodeTest::KEY_IDX, &key);
            key.m_bLoop = true;

            // check that the key is in what we expect for the test
            ASSERT_EQ(EntityNodeTest::KEY_TIME, key.time)
                << "Test Key frame should start at 1 second; something's wrong with the test setup.";
            ASSERT_EQ(EntityNodeTest::KEY_DURATION, key.m_duration)
                << "Test Key frame should last for 1 second; something's wrong with the test setup.";

            float test_sampleTime;
            f32 normalizedTime;

            // sample in different locations in the clip, before and after the clip to test looping
            const int NUM_TEST_SAMPLES = 10;
            for (int i = 0; i <= NUM_ANIMATION_USER_DATA_SLOTS; i++)
            {
                const float clipFraction = static_cast<float>(i) / static_cast<float>(NUM_TEST_SAMPLES);

                // sample before clip
                test_sampleTime = KEY_TIME - (clipFraction * KEY_DURATION);
                normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);
                EXPECT_FLOAT_EQ(NORMALIZED_CLIP_START, normalizedTime);

                // sample within clip
                test_sampleTime = KEY_TIME + (clipFraction * KEY_DURATION);
                normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);
                EXPECT_NEAR(clipFraction, normalizedTime, ERROR_TOLERANCE);

                if (i > 0)
                {
                    // sample after clip
                    test_sampleTime = KEY_TIME + KEY_DURATION + (clipFraction * KEY_DURATION);
                    normalizedTime = ComputeAnimKeyNormalizedTime(key, test_sampleTime);
                    EXPECT_NEAR(clipFraction, normalizedTime, ERROR_TOLERANCE);
                }
            }
        };

    }; // namespace EntityNodeTest

} // namespace Maestro

#endif // !defined(_RELEASE)

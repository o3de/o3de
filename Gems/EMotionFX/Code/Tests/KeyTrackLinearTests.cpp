/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SystemComponentFixture.h"

#include <AzTest/AzTest.h>
#include <AzCore/Math/MathUtils.h>

#include <EMotionFX/Source/KeyTrackLinearDynamic.h>

#include <Integration/System/SystemCommon.h>


namespace EMotionFX
{
    //--------------------------------------------------------------------------
    // The test fixture setup and helpers.
    //--------------------------------------------------------------------------
    class KeyTrackLinearDynamicFixture : public SystemComponentFixture
    {
    public:
        KeyTrackLinearDynamicFixture()
            : SystemComponentFixture()
        {
        }

        void LogFloatTrack(KeyTrackLinearDynamic<float, float>& track)
        {
            AZ_Printf("EMotionFX", "----------\n");
            for (size_t i=0; i < track.GetNumKeys(); ++i)
            {
                AZ_Printf("EMotionFX", "#%zu = time:%f  value:%f\n", i, track.GetKey(i)->GetTime(), track.GetKey(i)->GetValue());
            }
        }

        void FillFloatTrackZeroToThree(KeyTrackLinearDynamic<float, float>& track)
        {
            track.ClearKeys();
            track.AddKey(0.0f, 0.0f);
            track.AddKey(1.0f, 1.0f);
            track.AddKey(2.0f, 2.0f);
            track.AddKey(3.0f, 3.0f);
            track.Init();
        }
    };

    //--------------------------------------------------------------------------
    // The actual tests.
    //--------------------------------------------------------------------------
    TEST_F(KeyTrackLinearDynamicFixture, KeyTrackAdd)
    {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);
        ASSERT_EQ(track.GetNumKeys(), 4);
    }

    TEST_F(KeyTrackLinearDynamicFixture, KeyTrackAddSorted)
    {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        track.AddKeySorted(1.0f, 1.0f);
        track.AddKeySorted(0.0f, 0.0f);
        track.AddKeySorted(3.0f, 3.0f);
        track.AddKeySorted(2.0f, 2.0f);
        track.Init();
        ASSERT_EQ(track.GetNumKeys(), 4);
        ASSERT_TRUE(AZ::IsClose(track.GetKey(0)->GetTime(), 0.0f, 0.00001f));
        ASSERT_TRUE(AZ::IsClose(track.GetKey(1)->GetTime(), 1.0f, 0.00001f));
        ASSERT_TRUE(AZ::IsClose(track.GetKey(2)->GetTime(), 2.0f, 0.00001f));
        ASSERT_TRUE(AZ::IsClose(track.GetKey(3)->GetTime(), 3.0f, 0.00001f));
    }

    TEST_F(KeyTrackLinearDynamicFixture, KeyTrackRemoveKey)
    {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);
        ASSERT_EQ(track.GetNumKeys(), 4);

        track.RemoveKey(1);
        track.Init();
        ASSERT_FLOAT_EQ(track.GetKey(0)->GetTime(), 0.0f);
        ASSERT_FLOAT_EQ(track.GetKey(1)->GetTime(), 2.0f);
        ASSERT_FLOAT_EQ(track.GetKey(2)->GetTime(), 3.0f);

        track.RemoveKey(2);
        track.Init();
        ASSERT_FLOAT_EQ(track.GetKey(0)->GetTime(), 0.0f);
        ASSERT_FLOAT_EQ(track.GetKey(1)->GetTime(), 2.0f);
        
        track.RemoveKey(0);
        track.Init();

        // The time should be 0, because Init makes sure the first keyframe starts at time 0
        ASSERT_FLOAT_EQ(track.GetKey(0)->GetTime(), 0.0f);
 
        track.RemoveKey(0);
        track.Init();
        ASSERT_EQ(track.GetNumKeys(), 0);
    }

    TEST_F(KeyTrackLinearDynamicFixture, KeyTrackClearKeys)
    {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);
        ASSERT_EQ(track.GetNumKeys(), 4);
        track.ClearKeys();
        ASSERT_EQ(track.GetNumKeys(), 0);
    }

    TEST_F(KeyTrackLinearDynamicFixture, KeyTrackCheckIfIsAnimated)
    {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);
        ASSERT_TRUE(track.CheckIfIsAnimated(0.0f, 0.00001f));

        track.ClearKeys();
        track.AddKey(0.0f, 1.0f);
        track.AddKey(1.0f, 1.0f);
        track.AddKey(2.0f, 1.0f);
        track.AddKey(3.0f, 1.0f);
        ASSERT_TRUE(!track.CheckIfIsAnimated(1.0f, 0.00001f));

        track.ClearKeys();
        track.AddKey(0.0f, 1.0f);
        track.AddKey(1.0f, 1.0f);
        track.AddKey(2.0f, 1.01f);
        track.AddKey(3.0f, 1.0f);
        ASSERT_TRUE(track.CheckIfIsAnimated(1.0f, 0.00001f));
    }

    TEST_F(KeyTrackLinearDynamicFixture, KeyTrackGetFirstKey)
    {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);
        ASSERT_FLOAT_EQ(track.GetFirstKey()->GetTime(), 0.0f);
        ASSERT_FLOAT_EQ(track.GetFirstKey()->GetValue(), 0.0f);

        track.RemoveKey(2);
        track.Init();
        ASSERT_FLOAT_EQ(track.GetFirstKey()->GetTime(), 0.0f);
        ASSERT_FLOAT_EQ(track.GetFirstKey()->GetValue(), 0.0f);
 
        track.RemoveKey(0);
        track.Init();
 
        // Time value is expected to be 0 for the first key, after calling Init.
        // It is remapped internally to 0, if the first key's time isn't.
        // The value remains the same though.
        ASSERT_FLOAT_EQ(track.GetFirstKey()->GetTime(), 0.0f);
        ASSERT_FLOAT_EQ(track.GetFirstKey()->GetValue(), 1.0f);

        track.ClearKeys();
        track.Init();
        ASSERT_TRUE(!track.GetFirstKey());
    }

    TEST_F(KeyTrackLinearDynamicFixture, KeyTrackGetLastKey)
    {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);
        ASSERT_FLOAT_EQ(track.GetLastKey()->GetTime(), 3.0f);
        ASSERT_FLOAT_EQ(track.GetLastKey()->GetValue(), 3.0f);

        track.RemoveKey(2);
        track.Init();
        ASSERT_FLOAT_EQ(track.GetLastKey()->GetTime(), 3.0f);
        ASSERT_FLOAT_EQ(track.GetLastKey()->GetValue(), 3.0f);
 
        track.RemoveKey(track.GetNumKeys() - 1);
        track.Init();
        ASSERT_FLOAT_EQ(track.GetLastKey()->GetTime(), 1.0f);
        ASSERT_FLOAT_EQ(track.GetLastKey()->GetValue(), 1.0f);

        track.ClearKeys();
        track.Init();
        ASSERT_TRUE(!track.GetLastKey());
    }

   TEST_F(KeyTrackLinearDynamicFixture, KeyTrackFindKeyNumber)
   {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);

        ASSERT_EQ(track.FindKeyNumber(-1.0f), InvalidIndex);
        ASSERT_EQ(track.FindKeyNumber(0.0f), 0);
        ASSERT_EQ(track.FindKeyNumber(1.0f), 1);
        ASSERT_EQ(track.FindKeyNumber(2.0f), 2);
        ASSERT_EQ(track.FindKeyNumber(2.4f), 2);
        ASSERT_EQ(track.FindKeyNumber(2.8f), 2);
        ASSERT_EQ(track.FindKeyNumber(2.999f), 2);
        ASSERT_EQ(track.FindKeyNumber(3.0f), InvalidIndex);
        ASSERT_EQ(track.FindKeyNumber(3.001f), InvalidIndex);
        ASSERT_EQ(track.FindKeyNumber(4.0f), InvalidIndex);
   }

   TEST_F(KeyTrackLinearDynamicFixture, KeyTrackSetNumKeys)
   {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);
        track.SetNumKeys(10);
        ASSERT_EQ(track.GetNumKeys(), 10);
        track.SetNumKeys(15);
        ASSERT_EQ(track.GetNumKeys(), 15);
        track.SetNumKeys(5);
        ASSERT_EQ(track.GetNumKeys(), 5);
        track.SetNumKeys(0);
        ASSERT_EQ(track.GetNumKeys(), 0);
   }

   TEST_F(KeyTrackLinearDynamicFixture, KeyTrackOptimize)
   {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        track.AddKey(0.0f, 0.0f);
        track.AddKey(1.0f, 1.0f);
        track.AddKey(2.0f, 2.0f);
        track.AddKey(3.0f, 3.0f);
        track.Init();
        track.Optimize(0.00001f);
        ASSERT_EQ(track.GetNumKeys(), 2);
        ASSERT_FLOAT_EQ(track.GetKey(0)->GetTime(), 0.0f);
        ASSERT_FLOAT_EQ(track.GetKey(1)->GetTime(), 3.0f);

        track.ClearKeys();
        track.AddKey(0.0f, 0.0f);
        track.AddKey(1.0f, 1.0f);
        track.AddKey(2.0f, 1.0f);
        track.AddKey(2.01f, 1.0001f);
        track.AddKey(3.0f, 3.0f);
        track.Init();
        const size_t numKeysRemoved = track.Optimize(0.001f);
        ASSERT_EQ(numKeysRemoved, 1);
        ASSERT_EQ(track.GetNumKeys(), 4);
        ASSERT_FLOAT_EQ(track.GetKey(0)->GetTime(), 0.0f);
        ASSERT_FLOAT_EQ(track.GetKey(1)->GetTime(), 1.0f);
        ASSERT_FLOAT_EQ(track.GetKey(2)->GetTime(), 2.01f);
        ASSERT_FLOAT_EQ(track.GetKey(3)->GetTime(), 3.0f);
   }

   TEST_F(KeyTrackLinearDynamicFixture, KeyTrackGetValueAtTime)
   {
        EMotionFX::KeyTrackLinearDynamic<float, float> track;
        FillFloatTrackZeroToThree(track);

        ASSERT_FLOAT_EQ(track.GetValueAtTime(0.0f), 0.0f);
        ASSERT_FLOAT_EQ(track.GetValueAtTime(0.5f), 0.5f);
        ASSERT_FLOAT_EQ(track.GetValueAtTime(1.0f), 1.0f);
        ASSERT_FLOAT_EQ(track.GetValueAtTime(3.0f), 3.0f);
        ASSERT_FLOAT_EQ(track.GetValueAtTime(4.0f), 3.0f);

        uint8 cacheHit = 0;
        size_t cached = 0;
        ASSERT_FLOAT_EQ(track.GetValueAtTime(0.0f, &cached, &cacheHit), 0.0f);
        ASSERT_EQ(cached, 0);
        ASSERT_EQ(cacheHit, 1);

        ASSERT_FLOAT_EQ(track.GetValueAtTime(0.5f, &cached, &cacheHit), 0.5f);
        ASSERT_EQ(cached, 0);
        ASSERT_EQ(cacheHit, 1);

        ASSERT_FLOAT_EQ(track.GetValueAtTime(1.0f, &cached, &cacheHit), 1.0f);
        ASSERT_EQ(cached, 0);
        ASSERT_EQ(cacheHit, 1);

        ASSERT_FLOAT_EQ(track.GetValueAtTime(2.999f, &cached, &cacheHit), 2.999f);
        ASSERT_EQ(cached, 2);
        ASSERT_EQ(cacheHit, 0);

        ASSERT_FLOAT_EQ(track.GetValueAtTime(0.0f, &cached, &cacheHit), 0.0f);
        ASSERT_EQ(cached, 0);
        ASSERT_EQ(cacheHit, 0);
   }

} // namespace EMotionFX

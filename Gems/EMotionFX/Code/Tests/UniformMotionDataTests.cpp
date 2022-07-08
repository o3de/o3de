/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>
#include <MCore/Source/AzCoreConversions.h>
#include <Tests/ActorFixture.h>
#include <Tests/Matchers.h>

namespace EMotionFX
{
    class UniformMotionDataTests
        : public ActorFixture
        , public UnitTest::TraceBusRedirector
    {
    public:
        void SetUp()
        {
            UnitTest::TraceBusRedirector::BusConnect();
            ActorFixture::SetUp();
        }

        void TearDown()
        {
            ActorFixture::TearDown();
            UnitTest::TraceBusRedirector::BusDisconnect();
        }
    };

    TEST_F(UniformMotionDataTests, ZeroInit)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_sampleSpacing = 1.0f / 30.0f;
        settings.m_numSamples = 0;
        settings.m_numJoints = 0;
        settings.m_numMorphs = 0;
        settings.m_numFloats = 0;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 0.0f);
        EXPECT_FLOAT_EQ(motionData.GetSampleSpacing(), 1.0f / 30.0f);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 30.0f);
        EXPECT_EQ(motionData.GetNumSamples(), 0);
    }

    TEST_F(UniformMotionDataTests, Init)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_sampleSpacing = 1.0f / 30.0f;
        settings.m_numSamples = 301;
        settings.m_numJoints = 3;
        settings.m_numMorphs = 4;
        settings.m_numFloats = 5;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 10.0f);
        EXPECT_FLOAT_EQ(motionData.GetSampleSpacing(), 1.0f / 30.0f);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 30.0f);
        EXPECT_EQ(motionData.GetNumSamples(), 301);
        EXPECT_EQ(motionData.GetNumJoints(), 3);
        EXPECT_EQ(motionData.GetNumMorphs(), 4);
        EXPECT_EQ(motionData.GetNumFloats(), 5);
    }

    TEST_F(UniformMotionDataTests, Clear)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_sampleSpacing = 1.0f / 30.0f;
        settings.m_numSamples = 31;
        settings.m_numJoints = 3;
        settings.m_numMorphs = 4;
        settings.m_numFloats = 5;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 1.0f);
        EXPECT_EQ(motionData.GetNumSamples(), 31);
        EXPECT_EQ(motionData.GetNumJoints(), 3);
        EXPECT_EQ(motionData.GetNumMorphs(), 4);
        EXPECT_EQ(motionData.GetNumFloats(), 5);
        motionData.Clear();
        EXPECT_EQ(motionData.GetNumSamples(), 0);
        EXPECT_FLOAT_EQ(motionData.GetSampleSpacing(), 1.0f / 30.0f);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 0.0f);
        EXPECT_EQ(motionData.GetNumSamples(), 0);
        EXPECT_EQ(motionData.GetNumJoints(), 0);
        EXPECT_EQ(motionData.GetNumMorphs(), 0);
        EXPECT_EQ(motionData.GetNumFloats(), 0);
    }

    TEST_F(UniformMotionDataTests, CalculateSampleRate)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_sampleSpacing = 1.0f / 30.0f;
        settings.m_numSamples = 31;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 30.0f);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 1.0f);

        settings.m_sampleSpacing = 1.0f / 60.0f;
        settings.m_numSamples = 121;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 60.0f);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 2.0f);

        settings.m_sampleSpacing = 1.0f / 30.0f;
        settings.m_numSamples = 0;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 30.0f);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 0.0f);

/*
        settings.m_sampleSpacing = 0.0f;
        settings.m_numSamples = 31;
        AZ_TEST_START_TRACE_SUPPRESSION;
        motionData.Init(settings);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 0.0f);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 0.0f);

        settings.m_sampleSpacing = -1.0f;
        settings.m_numSamples = 31;
        AZ_TEST_START_TRACE_SUPPRESSION;
        motionData.Init(settings);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 0.0f);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 0.0f);
*/
    }

    TEST_F(UniformMotionDataTests, FindMotionLinkData)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_sampleSpacing = 1.0f / 10.0f;
        settings.m_numSamples = 11;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 1.0f);
        EXPECT_FLOAT_EQ(motionData.GetSampleSpacing(), 1.0f / 10.0f);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 10.0f);
        EXPECT_EQ(motionData.GetNumSamples(), 11);
        EXPECT_EQ(motionData.GetNumMotionLinkCacheEntries(), 0);

        AZStd::unique_ptr<Actor> clonedActor(m_actor->Clone());
        const MotionLinkData* linkDataA = motionData.FindMotionLinkData(m_actor.get());
        EXPECT_EQ(motionData.GetNumMotionLinkCacheEntries(), 1);
        const MotionLinkData* linkDataB = motionData.FindMotionLinkData(m_actor.get());
        EXPECT_EQ(motionData.GetNumMotionLinkCacheEntries(), 1);
        const MotionLinkData* linkDataC = motionData.FindMotionLinkData(const_cast<Actor*>(clonedActor.get()));
        EXPECT_EQ(motionData.GetNumMotionLinkCacheEntries(), 2);
        const MotionLinkData* linkDataD = motionData.FindMotionLinkData(const_cast<Actor*>(clonedActor.get()));
        EXPECT_EQ(motionData.GetNumMotionLinkCacheEntries(), 2);

        EXPECT_NE(linkDataA, nullptr);
        EXPECT_NE(linkDataB, nullptr);
        EXPECT_NE(linkDataC, nullptr);
        EXPECT_NE(linkDataD, nullptr);
        EXPECT_EQ(linkDataA, linkDataB);
        EXPECT_NE(linkDataA, linkDataC);
        EXPECT_EQ(linkDataC, linkDataD);

        clonedActor.reset(nullptr);
        EXPECT_EQ(motionData.GetNumMotionLinkCacheEntries(), 1);
    }

    TEST_F(UniformMotionDataTests, RemoveItems)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_sampleSpacing = 1.0f / 30.0f;
        settings.m_numSamples = 300;
        settings.m_numJoints = 4;
        settings.m_numMorphs = 4;
        settings.m_numFloats = 4;
        motionData.Init(settings);
        ASSERT_EQ(motionData.GetNumJoints(), 4);
        ASSERT_EQ(motionData.GetNumMorphs(), 4);
        ASSERT_EQ(motionData.GetNumFloats(), 4);
        motionData.SetJointName(0, "Joint1");
        motionData.SetJointName(1, "Joint2");
        motionData.SetJointName(2, "Joint3");
        motionData.SetJointName(3, "Joint4");
        motionData.SetMorphName(0, "Morph1");
        motionData.SetMorphName(1, "Morph2");
        motionData.SetMorphName(2, "Morph3");
        motionData.SetMorphName(3, "Morph4");
        motionData.SetFloatName(0, "Float1");
        motionData.SetFloatName(1, "Float2");
        motionData.SetFloatName(2, "Float3");
        motionData.SetFloatName(3, "Float4");
        motionData.RemoveJoint(0);
        motionData.RemoveMorph(1);
        motionData.RemoveFloat(2);
        ASSERT_EQ(motionData.GetNumJoints(), 3);
        ASSERT_EQ(motionData.GetNumMorphs(), 3);
        ASSERT_EQ(motionData.GetNumFloats(), 3);
        EXPECT_STREQ(motionData.GetJointName(0).c_str(), "Joint2");
        EXPECT_STREQ(motionData.GetJointName(1).c_str(), "Joint3");
        EXPECT_STREQ(motionData.GetJointName(2).c_str(), "Joint4");
        EXPECT_STREQ(motionData.GetMorphName(0).c_str(), "Morph1");
        EXPECT_STREQ(motionData.GetMorphName(1).c_str(), "Morph3");
        EXPECT_STREQ(motionData.GetMorphName(2).c_str(), "Morph4");
        EXPECT_STREQ(motionData.GetFloatName(0).c_str(), "Float1");
        EXPECT_STREQ(motionData.GetFloatName(1).c_str(), "Float2");
        EXPECT_STREQ(motionData.GetFloatName(2).c_str(), "Float4");
    }

    TEST_F(UniformMotionDataTests, FindData)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_numJoints = 3;
        settings.m_numMorphs = 3;
        settings.m_numFloats = 3;
        motionData.Init(settings);
        ASSERT_EQ(motionData.GetNumJoints(), 3);
        ASSERT_EQ(motionData.GetNumMorphs(), 3);
        ASSERT_EQ(motionData.GetNumFloats(), 3);
        motionData.SetJointName(0, "Joint1");
        motionData.SetJointName(1, "Joint2");
        motionData.SetJointName(2, "Joint3");
        motionData.SetMorphName(0, "Morph1");
        motionData.SetMorphName(1, "Morph2");
        motionData.SetMorphName(2, "Morph3");
        motionData.SetFloatName(0, "Float1");
        motionData.SetFloatName(1, "Float2");
        motionData.SetFloatName(2, "Float3");
        EXPECT_FALSE(motionData.FindJointIndexByName("Blah").IsSuccess());
        EXPECT_FALSE(motionData.FindMorphIndexByName("Blah").IsSuccess());
        EXPECT_FALSE(motionData.FindFloatIndexByName("Blah").IsSuccess());
        ASSERT_TRUE(motionData.FindJointIndexByName("Joint1").IsSuccess());
        ASSERT_TRUE(motionData.FindJointIndexByName("Joint2").IsSuccess());
        ASSERT_TRUE(motionData.FindJointIndexByName("Joint3").IsSuccess());
        ASSERT_TRUE(motionData.FindMorphIndexByName("Morph1").IsSuccess());
        ASSERT_TRUE(motionData.FindMorphIndexByName("Morph2").IsSuccess());
        ASSERT_TRUE(motionData.FindMorphIndexByName("Morph3").IsSuccess());
        ASSERT_TRUE(motionData.FindFloatIndexByName("Float1").IsSuccess());
        ASSERT_TRUE(motionData.FindFloatIndexByName("Float2").IsSuccess());
        ASSERT_TRUE(motionData.FindFloatIndexByName("Float3").IsSuccess());
        EXPECT_EQ(motionData.FindJointIndexByName("Joint1").GetValue(), 0);
        EXPECT_EQ(motionData.FindJointIndexByName("Joint2").GetValue(), 1);
        EXPECT_EQ(motionData.FindJointIndexByName("Joint3").GetValue(), 2);
        EXPECT_EQ(motionData.FindMorphIndexByName("Morph1").GetValue(), 0);
        EXPECT_EQ(motionData.FindMorphIndexByName("Morph2").GetValue(), 1);
        EXPECT_EQ(motionData.FindMorphIndexByName("Morph3").GetValue(), 2);
        EXPECT_EQ(motionData.FindFloatIndexByName("Float1").GetValue(), 0);
        EXPECT_EQ(motionData.FindFloatIndexByName("Float2").GetValue(), 1);
        EXPECT_EQ(motionData.FindFloatIndexByName("Float3").GetValue(), 2);
    }

    TEST_F(UniformMotionDataTests, MainTest)
    {
        UniformMotionData motionData;
        UniformMotionData::InitSettings settings;
        settings.m_sampleSpacing = 1.0f / 30.0f;
        settings.m_numSamples = 301;
        settings.m_numJoints = 3;
        settings.m_numMorphs = 3;
        settings.m_numFloats = 3;
        motionData.Init(settings);
        EXPECT_FLOAT_EQ(motionData.GetDuration(), 10.0f);
        EXPECT_FLOAT_EQ(motionData.GetSampleSpacing(), 1.0f / 30.0f);
        EXPECT_FLOAT_EQ(motionData.CalculateSampleRate(), 30.0f);
        EXPECT_EQ(motionData.GetNumSamples(), 301);
        EXPECT_EQ(motionData.GetNumJoints(), 3);
        EXPECT_EQ(motionData.GetNumMorphs(), 3);
        EXPECT_EQ(motionData.GetNumFloats(), 3);
        motionData.SetJointName(0, "Joint1");
        motionData.SetJointName(1, "Joint2");
        motionData.SetJointName(2, "Joint3");
        motionData.SetMorphName(0, "Morph1");
        motionData.SetMorphName(1, "Morph2");
        motionData.SetMorphName(2, "Morph3");
        motionData.SetFloatName(0, "Float1");
        motionData.SetFloatName(1, "Float2");
        motionData.SetFloatName(2, "Float3");

        for (size_t i = 0; i < 3; ++i)
        {
            const float iFloat = static_cast<float>(i);
            motionData.SetMorphPoseValue(i, iFloat);
            motionData.SetFloatPoseValue(i, iFloat);
            EXPECT_FLOAT_EQ(motionData.GetMorphPoseValue(i), iFloat);
            EXPECT_FLOAT_EQ(motionData.GetFloatPoseValue(i), iFloat);
        }

        motionData.AllocateMorphSamples(0);
        motionData.AllocateFloatSamples(0);
        EXPECT_TRUE(motionData.IsMorphAnimated(0));
        EXPECT_TRUE(motionData.IsFloatAnimated(0));
        EXPECT_FALSE(motionData.IsMorphAnimated(1));
        EXPECT_FALSE(motionData.IsFloatAnimated(1));
        EXPECT_FALSE(motionData.IsMorphAnimated(2));
        EXPECT_FALSE(motionData.IsFloatAnimated(2));
        for (size_t i = 0; i < motionData.GetNumSamples(); ++i)
        {
            const float iFloat = static_cast<float>(i);
            motionData.SetMorphSample(0, i, iFloat);
            motionData.SetFloatSample(0, i, iFloat * 10.0f);
            EXPECT_FLOAT_EQ(motionData.GetMorphSample(0, i).m_value, iFloat);
            EXPECT_FLOAT_EQ(motionData.GetMorphSample(0, i).m_time, static_cast<float>(i * motionData.GetSampleSpacing()));
            EXPECT_FLOAT_EQ(motionData.GetFloatSample(0, i).m_value, iFloat * 10.0f);
            EXPECT_FLOAT_EQ(motionData.GetFloatSample(0, i).m_time, static_cast<float>(i * motionData.GetSampleSpacing()));
        }

        // Test morph sampling.
        AZ::Outcome<size_t> index = motionData.FindMorphIndexByName("Morph1");
        ASSERT_TRUE(index.IsSuccess());
        if (index.IsSuccess())
        {
            float result = -1.0f;
            const AZ::u32 id = motionData.GetMorphNameId(index.GetValue());

            using ValuePair = AZStd::pair<float, float>; // First = time, second = expectedValue.
            const AZStd::vector<ValuePair> values{
                { -1.0f, 0.0f },
                { 0.0f, 0.0f },
                { motionData.GetSampleSpacing() * 0.25f, 0.25f },
                { motionData.GetSampleSpacing() * 0.5f, 0.5f },
                { motionData.GetSampleSpacing() * 0.75f, 0.75f },
                { motionData.GetSampleSpacing(), 1.0f },
                { motionData.GetDuration() * 0.5f, static_cast<float>(motionData.GetNumSamples() / 2) },
                { motionData.GetDuration(), static_cast<float>(motionData.GetNumSamples() - 1) },
                { motionData.GetDuration() + 1.0f, static_cast<float>(motionData.GetNumSamples() - 1) }
            };

            for (const ValuePair& valuePair : values)
            {
                MotionData::SampleSettings sampleSettings;
                sampleSettings.m_sampleTime = valuePair.first;
                sampleSettings.m_actorInstance = m_actorInstance;
                const bool success = motionData.SampleMorph(sampleSettings, id, result);
                EXPECT_TRUE(success);
                EXPECT_FLOAT_EQ(result, valuePair.second);
            }
        }

        // Test float sampling.
        index = motionData.FindFloatIndexByName("Float1");
        ASSERT_TRUE(index.IsSuccess());
        if (index.IsSuccess())
        {
            float result = -1.0f;
            const AZ::u32 id = motionData.GetFloatNameId(index.GetValue());

            using ValuePair = AZStd::pair<float, float>; // First = time, second = expectedValue.
            const AZStd::vector<ValuePair> values{
                { -1.0f, 0.0f },
                { 0.0f, 0.0f },
                { motionData.GetSampleSpacing() * 0.25f, 2.5f },
                { motionData.GetSampleSpacing() * 0.5f, 5.0f },
                { motionData.GetSampleSpacing() * 0.75f, 7.5f },
                { motionData.GetSampleSpacing(), 10.0f },
                { motionData.GetDuration() * 0.5f, static_cast<float>(motionData.GetNumSamples() / 2) * 10.0f },
                { motionData.GetDuration(), static_cast<float>(motionData.GetNumSamples() - 1) * 10.0f },
                { motionData.GetDuration() + 1.0f, static_cast<float>(motionData.GetNumSamples() - 1) * 10.0f }
            };

            for (const ValuePair& valuePair : values)
            {
                MotionData::SampleSettings sampleSettings;
                sampleSettings.m_sampleTime = valuePair.first;
                sampleSettings.m_actorInstance = m_actorInstance;
                const bool success = motionData.SampleFloat(sampleSettings, id, result);
                EXPECT_TRUE(success);
                EXPECT_FLOAT_EQ(result, valuePair.second);
            }
        }

        // Test sampling morphs and floats without any animation data.
        MotionData::SampleSettings sampleSettings;
        sampleSettings.m_actorInstance = m_actorInstance;
        sampleSettings.m_sampleTime = motionData.GetDuration() / 2.0f;
        float sampleResult = motionData.SampleFloat(sampleSettings, 1);
        EXPECT_FLOAT_EQ(sampleResult, 1.0f);
        sampleResult = motionData.SampleFloat(sampleSettings, 2);
        EXPECT_FLOAT_EQ(sampleResult, 2.0f);
        sampleResult = motionData.SampleMorph(sampleSettings, 1);
        EXPECT_FLOAT_EQ(sampleResult, 1.0f);
        sampleResult = motionData.SampleMorph(sampleSettings, 2);
        EXPECT_FLOAT_EQ(sampleResult, 2.0f);

        // Test adding a joint.
        AZ::Quaternion q1, q2;
        q1.SetFromEulerDegrees(AZ::Vector3(0.1f, 0.2f, 0.3f));
        q2.SetFromEulerDegrees(AZ::Vector3(0.4f, 0.5f, 0.6f));
        const Transform poseTransform(AZ::Vector3(1.0f, 2.0f, 3.0f), q1, AZ::Vector3(1.0f, 2.0f, 3.0f));
        const Transform bindTransform(AZ::Vector3(4.0f, 5.0f, 6.0f), q2, AZ::Vector3(4.0f, 5.0f, 6.0f));
        const size_t jointIndex = motionData.AddJoint("Joint4", poseTransform, bindTransform);
        EXPECT_EQ(jointIndex, 3);
        EXPECT_FALSE(motionData.IsJointAnimated(3));
        EXPECT_STREQ(motionData.GetJointName(3).c_str(), "Joint4");

        EXPECT_THAT(motionData.GetJointPoseTransform(3).m_position, IsClose(poseTransform.m_position));
        EXPECT_THAT(motionData.GetJointPoseTransform(3).m_rotation, IsClose(poseTransform.m_rotation));
#ifndef EMFX_SCALE_DISABLED
        EXPECT_THAT(motionData.GetJointPoseTransform(3).m_scale, IsClose(poseTransform.m_scale));
#endif
        EXPECT_THAT(motionData.GetJointBindPoseTransform(3).m_position, IsClose(bindTransform.m_position));
        EXPECT_THAT(motionData.GetJointBindPoseTransform(3).m_rotation, IsClose(bindTransform.m_rotation));
#ifndef EMFX_SCALE_DISABLED
        EXPECT_THAT(motionData.GetJointBindPoseTransform(3).m_scale, IsClose(bindTransform.m_scale));
#endif

        // Test adding a morph.
        const size_t morphIndex = motionData.AddMorph("Morph4", 1.0f);
        EXPECT_EQ(morphIndex, 3);
        EXPECT_FALSE(motionData.IsMorphAnimated(3));
        EXPECT_STREQ(motionData.GetMorphName(3).c_str(), "Morph4");
        EXPECT_FLOAT_EQ(motionData.GetMorphPoseValue(3), 1.0f);

        // Test adding a float.
        const size_t floatIndex = motionData.AddFloat("Float4", 1.0f);
        EXPECT_EQ(floatIndex, 3);
        EXPECT_FALSE(motionData.IsFloatAnimated(3));
        EXPECT_STREQ(motionData.GetFloatName(3).c_str(), "Float4");
        EXPECT_FLOAT_EQ(motionData.GetFloatPoseValue(3), 1.0f);

        // Construct some transform tracks.
        EXPECT_FALSE(motionData.IsJointAnimated(0));
        EXPECT_FALSE(motionData.IsJointPositionAnimated(0));
        EXPECT_FALSE(motionData.IsJointRotationAnimated(0));
#ifndef EMFX_SCALE_DISABLED
        EXPECT_FALSE(motionData.IsJointScaleAnimated(0));
#endif
        motionData.AllocateJointPositionSamples(0);
        motionData.AllocateJointRotationSamples(0);
        EXPECT_TRUE(motionData.IsJointPositionAnimated(0));
        EXPECT_TRUE(motionData.IsJointRotationAnimated(0));
#ifndef EMFX_SCALE_DISABLED
        EXPECT_FALSE(motionData.IsJointScaleAnimated(0));
        motionData.AllocateJointScaleSamples(0);
        EXPECT_TRUE(motionData.IsJointScaleAnimated(0));
#endif

        // Set the values for the transform samples.
        for (size_t i = 0; i < motionData.GetNumSamples(); ++i)
        {
            const float iFloat = static_cast<float>(i);
            const float expectedKeyTime = static_cast<float>(motionData.GetSampleSpacing() * i);

            const AZ::Vector3 position(iFloat, 1.0f, 2.0f);
            motionData.SetJointPositionSample(0, i, position);
            EXPECT_THAT(motionData.GetJointPositionSample(0, i).m_value, IsClose(position));
            EXPECT_NEAR(motionData.GetJointPositionSample(0, i).m_time, expectedKeyTime, 0.00001f);

            const AZ::Quaternion rotation = AZ::Quaternion::CreateRotationZ((iFloat / static_cast<float>(motionData.GetNumSamples())) * 180.0f).GetNormalized();
            motionData.SetJointRotationSample(0, i, rotation);
            EXPECT_THAT(motionData.GetJointRotationSample(0, i).m_value, IsClose(rotation));
            EXPECT_NEAR(motionData.GetJointRotationSample(0, i).m_time, expectedKeyTime, 0.00001f);

#ifndef EMFX_SCALE_DISABLED
            const AZ::Vector3 scale(iFloat + 1.0f, iFloat * 2.0f + 1.0f, iFloat * 3.0f + 1.0f);
            motionData.SetJointScaleSample(0, i, scale);
            EXPECT_THAT(motionData.GetJointScaleSample(0, i).m_value, IsClose(scale));
            EXPECT_NEAR(motionData.GetJointScaleSample(0, i).m_time, expectedKeyTime, 0.00001f);
#endif
        }

        // Rename our sub motion data to match our actor.
        const Skeleton* skeleton = m_actor->GetSkeleton();
        for (size_t i = 0; i < 3; ++i)
        {
            motionData.SetJointName(i, skeleton->GetNode(static_cast<AZ::u32>(i))->GetNameString());
            EXPECT_STREQ(motionData.GetJointName(i).c_str(), skeleton->GetNode(static_cast<AZ::u32>(i))->GetName());
        }

        // Adjust bind pose of our fourth joint.
        Pose* bindPose = m_actorInstance->GetTransformData()->GetBindPose();
#ifndef EMFX_SCALE_DISABLED
        const Transform expectedBindTransform(AZ::Vector3(0.0f, 1.0f, 2.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(10.0f, 20.0f, 30.0f));
#else
        const Transform expectedBindTransform(AZ::Vector3(0.0f, 1.0f, 2.0f), AZ::Quaternion::CreateIdentity());
#endif
        bindPose->SetLocalSpaceTransform(3, expectedBindTransform);

        // Now sample the joint transforms.
        const size_t lastSampleIndex = motionData.GetNumSamples() - 1;
#ifndef EMFX_SCALE_DISABLED
        const AZ::Vector3 firstScaleSample = motionData.GetJointScaleSample(0, 0).m_value;
        const AZ::Vector3 lastScaleSample = motionData.GetJointScaleSample(0, lastSampleIndex).m_value;
#else
        const AZ::Vector3 firstScaleSample(1.0f, 1.0f, 1.0f);
        const AZ::Vector3 lastScaleSample(1.0f, 1.0f, 1.0f);
#endif

        using TransformPair = AZStd::pair<float, Transform>; // First = time, second = expectedValue.
        const AZStd::vector<TransformPair> values{
            { -1.0f,
                Transform(
                    motionData.GetJointPositionSample(0, 0).m_value,
                    motionData.GetJointRotationSample(0, 0).m_value,
                    firstScaleSample) },
            { 0.0f,
                Transform(
                    motionData.GetJointPositionSample(0, 0).m_value,
                    motionData.GetJointRotationSample(0, 0).m_value,
                    firstScaleSample) },
            { motionData.GetSampleSpacing() * 0.25f,
                Transform(
                    AZ::Vector3(0.25f, 1.0f, 2.0f),
                    AZ::Quaternion::CreateRotationZ((0.25f / static_cast<float>(motionData.GetNumSamples())) * 180.0f).GetNormalized(),
                    AZ::Vector3(1.25f, 1.5f, 1.75f)) },
            { motionData.GetSampleSpacing() * 0.5f,
                Transform(
                    AZ::Vector3(0.5f, 1.0f, 2.0f),
                    AZ::Quaternion::CreateRotationZ((0.5f / static_cast<float>(motionData.GetNumSamples())) * 180.0f).GetNormalized(),
                    AZ::Vector3(1.5f, 2.0f, 2.5f)) },
            { motionData.GetSampleSpacing() * 0.75f,
                Transform(
                    AZ::Vector3(0.75f, 1.0f, 2.0f),
                    AZ::Quaternion::CreateRotationZ((0.75f / static_cast<float>(motionData.GetNumSamples())) * 180.0f).GetNormalized(),
                    AZ::Vector3(1.75f, 2.5f, 3.25f)) },
            { motionData.GetSampleSpacing(),
                Transform(
                    AZ::Vector3(1.0f, 1.0f, 2.0f),
                    AZ::Quaternion::CreateRotationZ((1.0f / static_cast<float>(motionData.GetNumSamples())) * 180.0f).GetNormalized(),
                    AZ::Vector3(2.0f, 3.0f, 4.0f)) },
            { motionData.GetSampleSpacing() * 5.5f,
                Transform(
                    AZ::Vector3(5.5f, 1.0f, 2.0f),
                    AZ::Quaternion::CreateRotationZ((5.5f / static_cast<float>(motionData.GetNumSamples())) * 180.0f).GetNormalized(),
                    AZ::Vector3(6.5f, 12.0f, 17.5f)) },
            { motionData.GetDuration() + 1.0f,
                Transform(
                    motionData.GetJointPositionSample(0, lastSampleIndex).m_value,
                    motionData.GetJointRotationSample(0, lastSampleIndex).m_value,
                    lastScaleSample) }
        };

        for (const TransformPair& expectation : values)
        {
            MotionData::SampleSettings sampleSettings;
            sampleSettings.m_actorInstance = m_actorInstance;
            sampleSettings.m_sampleTime = expectation.first;
            const Transform sampledResult = motionData.SampleJointTransform(sampleSettings, 0);
            EXPECT_THAT(sampledResult.m_position, IsClose(expectation.second.m_position));
            EXPECT_THAT(sampledResult.m_rotation, IsClose(expectation.second.m_rotation));
#ifndef EMFX_SCALE_DISABLED
            EXPECT_THAT(sampledResult.m_scale, IsClose(expectation.second.m_scale));
#endif

            // Fourth joint has no motion data to apply to our actor, so expect a bind pose.
            // It has motion data, but there is no joint in the skeleton that matches its name, so it is like motion data for a joint that doesn't exist in our actor.
            const Transform fourthJointTransform = motionData.SampleJointTransform(sampleSettings, 3);
            EXPECT_THAT(fourthJointTransform.m_position, IsClose(expectedBindTransform.m_position));
            EXPECT_THAT(fourthJointTransform.m_rotation, IsClose(expectedBindTransform.m_rotation));
#ifndef EMFX_SCALE_DISABLED
            EXPECT_THAT(fourthJointTransform.m_scale, IsClose(expectedBindTransform.m_scale));
#endif
        }

        // Sample the entire pose.
        for (const TransformPair& expectation : values)
        {
            Pose pose;
            pose.LinkToActorInstance(m_actorInstance);

            MotionData::SampleSettings sampleSettings;
            sampleSettings.m_actorInstance = m_actorInstance;
            sampleSettings.m_sampleTime = expectation.first;
            motionData.SamplePose(sampleSettings, &pose);

            // We only verify the first joint, to see if it interpolated fine.
            const Transform sampledResult = pose.GetLocalSpaceTransform(0);
            EXPECT_THAT(sampledResult.m_position, IsClose(expectation.second.m_position));
            EXPECT_THAT(sampledResult.m_rotation, IsClose(expectation.second.m_rotation));
#ifndef EMFX_SCALE_DISABLED
            EXPECT_THAT(sampledResult.m_scale, IsClose(expectation.second.m_scale));
#endif

            // Fourth joint has no motion data to apply to our actor, so expect a bind pose.
            // It has motion data, but there is no joint in the skeleton that matches its name, so it is like motion data for a joint that doesn't exist in our actor.
            const Transform fourthJointTransform = pose.GetLocalSpaceTransform(3);
            EXPECT_THAT(fourthJointTransform.m_position, IsClose(expectedBindTransform.m_position));
            EXPECT_THAT(fourthJointTransform.m_rotation, IsClose(expectedBindTransform.m_rotation));
#ifndef EMFX_SCALE_DISABLED
            EXPECT_THAT(fourthJointTransform.m_scale, IsClose(expectedBindTransform.m_scale));
#endif
        }
    }
} // namespace EMotionFX

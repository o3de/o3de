/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <Tests/Integration/PoseComparisonFixture.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/KeyTrackLinearDynamic.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <Tests/Printers.h>


namespace EMotionFX
{
    void PrintTo(const Recorder::ActorInstanceData& actorInstanceData, ::std::ostream* os)
    {
        *os << actorInstanceData.m_actorInstance->GetActor()->GetName();
    }

    template<class ReturnType, class StorageType = ReturnType>
    void PrintTo(const KeyFrame<ReturnType, StorageType>* keyFrame, ::std::ostream* os)
    {
        *os << "(Time: " << keyFrame->GetTime() << ", Value: ";
        PrintTo(keyFrame->GetValue(), os);
        *os << ")";
    }

    template<class T>
    void PrintTo(const KeyTrackLinearDynamic<T>& keyTrack, ::std::ostream* os)
    {
        *os << "KeyTrackLinearDynamic<" << AZ::AzTypeInfo<T>::Name() << "> with " << keyTrack.GetNumKeys() << " keyframes";
    }

    void PrintTo(const Recorder::TransformTracks& tracks, ::std::ostream* os)
    {
        PrintTo(tracks.m_positions, os);
        PrintTo(tracks.m_rotations, os);
    }

    AZ_PUSH_DISABLE_WARNING(4100, "-Wmissing-declarations") // 'result_listener': unreferenced formal parameter
    MATCHER(FloatEq, "Test if two floats are close to each other")
    {
        return ::testing::ExplainMatchResult(::testing::FloatEq(::testing::get<1>(arg)), ::testing::get<0>(arg), result_listener);
    }

    MATCHER_P2(AZIsClose, expected, tolerance, "")
    {
        return expected.IsClose(arg, tolerance);
    }

    MATCHER_P(KeyIsClose, tolerance, "")
    {
        using LhsType = typename ::testing::tuple_element<0, arg_type>::type;
        using RhsType = typename ::testing::tuple_element<1, arg_type>::type;
        using ::testing::get;
        LhsType got = get<0>(arg);
        RhsType expected = get<1>(arg);

        return ::testing::ExplainMatchResult(::testing::FloatEq(expected->GetTime()), got->GetTime(), result_listener)
            && ::testing::ExplainMatchResult(AZIsClose(expected->GetValue(), tolerance), got->GetValue(), result_listener);
    }
    AZ_POP_DISABLE_WARNING

    // This class is modeled after the built-in testing::Pointwise fixture. It
    // doesn't work for our use case because the KeyTrack class does not have
    // an STL-like interface, and it is overly verbose for large containers.
    // This matcher will ensure that the key tracks have the same number of
    // keys, and that each key is "close".
    template<class T>
    class KeyTrackMatcher
        : public ::testing::MatcherInterface<const KeyTrackLinearDynamic<T>&>
    {
    public:
        using InnerMatcherArg = ::testing::tuple<const KeyFrame<T>*, const KeyFrame<T>*>;

        KeyTrackMatcher(const KeyTrackLinearDynamic<T>& expected, const char* nodeName)
            : m_expected(expected)
            , m_nodeName(nodeName)
        {
        }

        bool MatchAndExplain(const KeyTrackLinearDynamic<T>& got, ::testing::MatchResultListener* result_listener) const override
        {
            const size_t gotSize = got.GetNumKeys();
            const size_t expectedSize = m_expected.GetNumKeys();
            const size_t commonSize = AZStd::min(gotSize, expectedSize);

            for (size_t i = 0; i != commonSize; ++i)
            {
                const KeyFrame<T>* gotKey = got.GetKey(i);
                const KeyFrame<T>* expectedKey = m_expected.GetKey(i);
                const auto innerMatcher = ::testing::SafeMatcherCast<InnerMatcherArg>(KeyIsClose(0.01f));
                if (!innerMatcher.MatchAndExplain(::testing::make_tuple(gotKey, expectedKey), result_listener))
                {
                    *result_listener << "where the value pair at index #" << i << " don't match\n";

                    const uint32 numContextLines = 2;
                    const size_t beginContextLines = i > numContextLines ? i - numContextLines : 0;
                    const size_t endContextLines = i > commonSize - numContextLines - 1 ? commonSize : i + numContextLines + 1;
                    for (size_t contextIndex = beginContextLines; contextIndex < endContextLines; ++contextIndex)
                    {
                        const bool contextLineMatches = ::testing::Matches(innerMatcher)(::testing::make_tuple(got.GetKey(contextIndex), m_expected.GetKey(contextIndex)));
                        if (!contextLineMatches)
                        {
                            *result_listener << "\033[0;31m"; // red
                        }
                        *result_listener << contextIndex << ": Expected: ";
                        PrintTo(m_expected.GetKey(contextIndex), result_listener->stream());
                        *result_listener << "\n" << contextIndex << ":   Actual: ";
                        PrintTo(got.GetKey(contextIndex), result_listener->stream());
                        if (!contextLineMatches)
                        {
                            *result_listener << "\033[0;m";
                        }
                        if (contextIndex != endContextLines-1)
                        {
                            *result_listener << "\n";
                        }
                    }
                    return false;
                }
            }

            return gotSize == expectedSize;
        }

        void DescribeTo(::std::ostream* os) const override
        {
            PrintTo(m_expected, os);
            *os << " for node " << m_nodeName;
        }
        void DescribeNegationTo(::std::ostream* os) const override
        {
            PrintTo(m_expected, os);
            *os << " for node " << m_nodeName << " shouldn't match";
        }

    private:
        const KeyTrackLinearDynamic<T>& m_expected;
        const char* m_nodeName;
    };

    template<class T>
    inline ::testing::Matcher<const KeyTrackLinearDynamic<T>&> MatchesKeyTrack(const KeyTrackLinearDynamic<T>& expected, const char* nodeName) {
        return MakeMatcher(new KeyTrackMatcher<T>(expected, nodeName));
    }

    void PoseComparisonFixture::SetUp()
    {
        SystemComponentFixture::SetUp();

        LoadAssets();
    }

    void PoseComparisonFixture::TearDown()
    {
        m_actorInstance->Destroy();

        m_actor.reset();

        delete m_motionSet;
        m_motionSet = nullptr;

        delete m_animGraph;
        m_animGraph = nullptr;

        SystemComponentFixture::TearDown();
    }

    void PoseComparisonFixture::LoadAssets()
    {
        const AZStd::string actorPath = ResolvePath(GetParam().m_actorFile);
        m_actor = EMotionFX::GetImporter().LoadActor(actorPath);
        ASSERT_TRUE(m_actor) << "Failed to load actor";

        const AZStd::string animGraphPath = ResolvePath(GetParam().m_animGraphFile);
        m_animGraph = EMotionFX::GetImporter().LoadAnimGraph(animGraphPath);
        ASSERT_TRUE(m_animGraph) << "Failed to load anim graph";

        const AZStd::string motionSetPath = ResolvePath(GetParam().m_motionSetFile);
        m_motionSet = EMotionFX::GetImporter().LoadMotionSet(motionSetPath);
        ASSERT_TRUE(m_motionSet) << "Failed to load motion set";
        m_motionSet->Preload();

        m_actorInstance = ActorInstance::Create(m_actor.get());
        m_actorInstance->SetAnimGraphInstance(AnimGraphInstance::Create(m_animGraph, m_actorInstance, m_motionSet));
    }

    TEST_P(PoseComparisonFixture, TestPoses)
    {
        const AZStd::string recordingPath = ResolvePath(GetParam().m_recordingFile);
        Recorder* recording = EMotionFX::Recorder::LoadFromFile(recordingPath.c_str());
        const EMotionFX::Recorder::ActorInstanceData& expectedActorInstanceData = recording->GetActorInstanceData(0);

        EMotionFX::GetRecorder().StartRecording(recording->GetRecordSettings());
        for (const float timeDelta : recording->GetTimeDeltas())
        {
            EXPECT_GE(timeDelta, 0) << "Expected a positive time delta";
            EMotionFX::GetEMotionFX().Update(timeDelta);
        }

        EMotionFX::Recorder::ActorInstanceData& gotActorInstanceData = EMotionFX::GetRecorder().GetActorInstanceData(0);

        // Make sure that the captured times match the expected times
        EXPECT_THAT(GetRecorder().GetTimeDeltas(), ::testing::Pointwise(FloatEq(), recording->GetTimeDeltas()));

        const AZStd::vector<Recorder::TransformTracks>& gotTracks = gotActorInstanceData.m_transformTracks;
        const AZStd::vector<Recorder::TransformTracks>& expectedTracks = expectedActorInstanceData.m_transformTracks;
        EXPECT_EQ(gotTracks.size(), expectedTracks.size()) << "recording has a different number of transform tracks";

        const size_t numberOfItemsInCommon = AZStd::min(gotTracks.size(), expectedTracks.size());
        for (size_t trackNum = 0; trackNum < numberOfItemsInCommon; ++trackNum)
        {
            const Recorder::TransformTracks& gotTrack = gotTracks[trackNum];
            const Recorder::TransformTracks& expectedTrack = expectedTracks[trackNum];
            const char* nodeName = gotActorInstanceData.m_actorInstance->GetActor()->GetSkeleton()->GetNode(trackNum)->GetName();

            EXPECT_THAT(gotTrack.m_positions, MatchesKeyTrack(expectedTrack.m_positions, nodeName));
            EXPECT_THAT(gotTrack.m_rotations, MatchesKeyTrack(expectedTrack.m_rotations, nodeName));
        }

        recording->Destroy();
    }

    TEST_P(TestPoseComparisonFixture, TestRecording)
    {
        // Make one recording, 10 seconds at 60 fps
        Recorder::RecordSettings settings;
        settings.m_fps                       = 1000000;
        settings.m_recordTransforms          = true;
        settings.m_recordAnimGraphStates     = false;
        settings.m_recordNodeHistory         = false;
        settings.m_recordScale               = false;
        settings.m_initialAnimGraphAnimBytes = 4 * 1024 * 1024; // 4 mb
        settings.m_historyStatesOnly         = false;
        settings.m_recordEvents              = false;

        EMotionFX::GetRecorder().StartRecording(settings);

        const float fps = 60.0f;
        const float fixedTimeDelta = 1.0f / fps;
        for (uint32 keyID = 0; keyID < fps * 10.0f; ++keyID)
        {
            EMotionFX::GetEMotionFX().Update(fixedTimeDelta);
        }

        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&buffer);
        const bool serializeSuccess = AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, &EMotionFX::GetRecorder());
        ASSERT_TRUE(serializeSuccess);
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        Recorder* recording = AZ::Utils::LoadObjectFromStream<Recorder>(stream);

        m_actorInstance->Destroy();
        m_actorInstance = ActorInstance::Create(m_actor.get());
        m_actorInstance->SetAnimGraphInstance(AnimGraphInstance::Create(m_animGraph, m_actorInstance, m_motionSet));

        EMotionFX::GetRecorder().StartRecording(settings);
        for (const float timeDelta : recording->GetTimeDeltas())
        {
            EMotionFX::GetEMotionFX().Update(timeDelta);
        }

        const EMotionFX::Recorder::ActorInstanceData& expectedActorInstanceData = recording->GetActorInstanceData(0);
        const EMotionFX::Recorder::ActorInstanceData& gotActorInstanceData = EMotionFX::GetRecorder().GetActorInstanceData(0);

        // Make sure that the captured times match the expected times
        EXPECT_THAT(GetRecorder().GetTimeDeltas(), ::testing::Pointwise(FloatEq(), recording->GetTimeDeltas()));

        const AZStd::vector<Recorder::TransformTracks>& gotTracks = gotActorInstanceData.m_transformTracks;
        const AZStd::vector<Recorder::TransformTracks>& expectedTracks = expectedActorInstanceData.m_transformTracks;
        EXPECT_EQ(gotTracks.size(), expectedTracks.size()) << "recording has a different number of transform tracks";

        const size_t numberOfItemsInCommon = AZStd::min(gotTracks.size(), expectedTracks.size());
        for (size_t trackNum = 0; trackNum < numberOfItemsInCommon; ++trackNum)
        {
            const Recorder::TransformTracks& gotTrack = gotTracks[trackNum];
            const Recorder::TransformTracks& expectedTrack = expectedTracks[trackNum];
            const char* nodeName = gotActorInstanceData.m_actorInstance->GetActor()->GetSkeleton()->GetNode(trackNum)->GetName();

            EXPECT_THAT(gotTrack.m_positions, MatchesKeyTrack(expectedTrack.m_positions, nodeName));
            EXPECT_THAT(gotTrack.m_rotations, MatchesKeyTrack(expectedTrack.m_rotations, nodeName));
        }

        recording->Destroy();
    }

    INSTANTIATE_TEST_CASE_P(DISABLED_TestPoses, PoseComparisonFixture,
        ::testing::Values(
            PoseComparisonFixtureParams (
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.actor",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.animgraph",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.motionset",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.emfxrecording"
            ),
            PoseComparisonFixtureParams (
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.actor",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.animgraph",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.motionset",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.emfxrecording"
            )
        )
    );

    INSTANTIATE_TEST_CASE_P(DISABLED_TestPoseComparison, TestPoseComparisonFixture,
        ::testing::Values(
            PoseComparisonFixtureParams (
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.actor",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.animgraph",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.motionset",
                "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.emfxrecording"
            )
        )
    );
}; // namespace EMotionFX

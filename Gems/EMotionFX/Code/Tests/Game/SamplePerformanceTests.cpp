
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Game/SampleGameFixture.h>
#include <AzCore/Debug/Timer.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Asset/AssetManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MultiThreadScheduler.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionInstancePool.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/SingleThreadScheduler.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/IntParameter.h>
#include <EMotionFX/Source/Parameter/FloatParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>


namespace EMotionFX
{
    struct SampleParameterSet
    {
        std::vector<std::tuple<std::string, AZ::Vector2>> m_vec2Params;
        std::vector<std::pair<std::string, bool>> m_boolParams;
        std::vector<std::pair<std::string, float>> m_floatParams;
    };

    struct PerformanceTestParameters
    {
        const char* m_description;
        float m_fps;
        float m_motionSamplingRate;
        float m_totalTestTimeInSeconds;
        size_t m_numInstances;
        size_t m_numSkinAttachmentsPerInstance;
        AZ::u32 m_lodLevel;
        bool m_includeSoftwareSkinning;
        bool m_useMultiThreading;

        void Print() const
        {
            printf("-------------------------------\n");
            printf("- Performance Test Parameters\n");
            printf("- Description: %s\n", m_description);
            printf("- FPS:            %f\n", m_fps);
            printf("- Motion Sampling Rate: %f\n", m_motionSamplingRate);
            printf("- Total Time (s): %f\n", m_totalTestTimeInSeconds);
            printf("- Frames:         %d\n", static_cast<int>(m_totalTestTimeInSeconds * m_fps));
            printf("- Num Instances:  %zd\n", m_numInstances);
            printf("- Num Skin Attachents per Instance: %zd\n", m_numSkinAttachmentsPerInstance);
            printf("- LOD Level:      %d\n", m_lodLevel);
            printf("- Software Skin:  %s\n", m_includeSoftwareSkinning ? "True" : "False");
            printf("- Multi-threading:%s\n", m_useMultiThreading ? "True" : "False");
        }
    };

    class PerformanceTestFixture
        : public SampleGameFixture
        , public ::testing::WithParamInterface<PerformanceTestParameters>
    {
    public:
        void SetUp() override
        {
            SampleGameFixture::SetUp();

            m_random = new AZ::SimpleLcgRandom();
            m_random->SetSeed(875960);

            SampleParameterSet params;
            params.m_floatParams.push_back({ "movement_speed", 0.0f });
            params.m_vec2Params.push_back({ "movement_direction", AZ::Vector2(0.0f, 0.0f) });
            params.m_boolParams.push_back({ "jumping", false });
            params.m_boolParams.push_back({ "attacking", true });
            m_pyroParameterSet.emplace_back(params);

            params.m_floatParams.push_back({ "movement_speed", 1.0f });
            params.m_vec2Params.push_back({ "movement_direction", AZ::Vector2(1.0f, 0.0f) });
            params.m_boolParams.push_back({ "jumping", false });
            params.m_boolParams.push_back({ "attacking", false });
            m_pyroParameterSet.emplace_back(params);

            params.m_floatParams.push_back({ "movement_speed", 0.5f });
            params.m_vec2Params.push_back({ "movement_direction", AZ::Vector2(0.5f, 0.5f) });
            params.m_boolParams.push_back({ "jumping", false });
            params.m_boolParams.push_back({ "attacking", false });
            m_pyroParameterSet.emplace_back(params);

            params.m_floatParams.push_back({ "movement_speed", 0.0f });
            params.m_vec2Params.push_back({ "movement_direction", AZ::Vector2(0.0f, 0.0f) });
            params.m_boolParams.push_back({ "jumping", true });
            params.m_boolParams.push_back({ "attacking", false });
            m_pyroParameterSet.emplace_back(params);
        }

        void TearDown() override
        {
            delete m_random;
            SampleGameFixture::TearDown();
        }

        float RandomRange(float min, float max) const
        {
            return min + m_random->GetRandomFloat() * (max - min);
        }

        AZ::Vector3 RandomRangeVec3(float min, float max) const
        {
            return AZ::Vector3(
                RandomRange(min, max),
                RandomRange(min, max),
                RandomRange(min, max));
        }

        void RandomizeParameters(const AZStd::vector<ActorInstance*>& actorInstances)
        {
            for (ActorInstance* actorInstance : actorInstances)
            {
                AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                AZ_Assert(animGraphInstance, "Actor instance should have an anim graph instance playing.");
                const AnimGraph* animGraph = animGraphInstance->GetAnimGraph();

                const size_t parameterSetIndex = static_cast<size_t>(RandomRange(0.0f, static_cast<float>(m_pyroParameterSet.size())-0.0001f));
                const SampleParameterSet& params = m_pyroParameterSet[parameterSetIndex];

                // Set parameters to default values.
                const size_t numParameters = animGraph->GetNumValueParameters();
                for (size_t i = 0; i < numParameters; ++i)
                {
                    const ValueParameter* parameter = animGraph->FindValueParameter(i);
                    MCore::Attribute* attribute = animGraphInstance->GetParameterValue(static_cast<AZ::u32>(i));

                    if (parameter->RTTI_GetType() == azrtti_typeid<BoolParameter>())
                    {
                        const BoolParameter* boolParameter = static_cast<const BoolParameter*>(parameter);
                        MCore::AttributeBool* boolAttribute = static_cast<MCore::AttributeBool*>(attribute);
                        boolAttribute->SetValue(boolParameter->GetDefaultValue());
                    }
                    if (parameter->RTTI_GetType() == azrtti_typeid<FloatParameter>())
                    {
                        const FloatParameter* floatParameter = static_cast<const FloatParameter*>(parameter);
                        MCore::AttributeFloat* floatAttribute = static_cast<MCore::AttributeFloat*>(attribute);
                        floatAttribute->SetValue(floatParameter->GetDefaultValue());
                    }
                    if (parameter->RTTI_GetType() == azrtti_typeid<Vector2Parameter>())
                    {
                        const Vector2Parameter* vec2Parameter = static_cast<const Vector2Parameter*>(parameter);
                        MCore::AttributeVector2* vec2Attribute = static_cast<MCore::AttributeVector2*>(attribute);
                        vec2Attribute->SetValue(vec2Parameter->GetDefaultValue());
                    }
                }

                // Bool parameters
                for (const auto& boolParamPair : params.m_boolParams)
                {
                    const AZ::Outcome<size_t> parameterIndex = animGraph->FindValueParameterIndexByName(boolParamPair.first.c_str());
                    ASSERT_TRUE(parameterIndex.IsSuccess());
                    const ValueParameter* parameter = animGraph->FindValueParameter(parameterIndex.GetValue());
                    ASSERT_NE(parameter, nullptr);
                    ASSERT_EQ(parameter->GetName(), boolParamPair.first.c_str());

                    MCore::Attribute* attribute = animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
                    MCore::AttributeBool* boolAttribute = static_cast<MCore::AttributeBool*>(attribute);
                    boolAttribute->SetValue(boolParamPair.second);
                }

                // Vec2 parameters
                for (const auto& vec2ParamTuple : params.m_vec2Params)
                {
                    const AZ::Outcome<size_t> parameterIndex = animGraph->FindValueParameterIndexByName(std::get<0>(vec2ParamTuple).c_str());
                    ASSERT_TRUE(parameterIndex.IsSuccess());
                    const ValueParameter* parameter = animGraph->FindValueParameter(parameterIndex.GetValue());
                    ASSERT_NE(parameter, nullptr);
                    ASSERT_EQ(parameter->GetName(), std::get<0>(vec2ParamTuple).c_str());

                    MCore::Attribute* attribute = animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
                    MCore::AttributeVector2* vec2Attribute = static_cast<MCore::AttributeVector2*>(attribute);
                    vec2Attribute->SetValue(std::get<1>(vec2ParamTuple));
                }

                // Float parameters
                for (const auto& floatParamPair : params.m_floatParams)
                {
                    const AZ::Outcome<size_t> parameterIndex = animGraph->FindValueParameterIndexByName(floatParamPair.first.c_str());
                    ASSERT_TRUE(parameterIndex.IsSuccess());
                    const ValueParameter* parameter = animGraph->FindValueParameter(parameterIndex.GetValue());
                    ASSERT_NE(parameter, nullptr);
                    ASSERT_EQ(parameter->GetName(), floatParamPair.first.c_str());

                    MCore::Attribute* attribute = animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
                    MCore::AttributeFloat* floatAttribute = static_cast<MCore::AttributeFloat*>(attribute);
                    floatAttribute->SetValue(floatParamPair.second);
                }
            }
        }

        static AZStd::tuple<float, float, float, float> CalculateStats(const AZStd::vector<float>& samples)
        {
            const size_t numSamples = samples.size();
            if (numSamples == 0)
            {
                return { 0.0f, 0.0f, 0.0f, 0.0f };
            }

            float best = samples[0];
            float worst = samples[0];
            float accumulated = 0.0f;
            for (const float sample : samples)
            {
                best = AZ::GetMin<float>(best, sample);
                worst = AZ::GetMax<float>(worst, sample);
                accumulated += sample;
            }
            const float mean = accumulated / static_cast<float>(numSamples);

            float variance = 0.0f;
            for (const float sample : samples)
            {
                variance += powf(sample - mean, 2.0f);
            }
            variance = variance / static_cast<float>(numSamples);
            const float stdDeviation = sqrtf(variance);

            return { best, mean, worst, stdDeviation };
        }

        void PrintReport(const AZStd::vector<float>& transformUpdateFrameTimes,
            const AZStd::vector<float>& meshDeformFrameTimes,
            float totalTransformUpdateTime,
            float totalMeshDeformTime)
        {
            const PerformanceTestParameters& param = GetParam();

            // Totals
            printf("----------------------------------------------------\n");
            printf("- Performance Test Report                          -\n");
            if (param.m_includeSoftwareSkinning)
            {
                printf("- Totals:\n");
                printf("    Total Time (s):                  %.4f s\n", totalTransformUpdateTime + totalMeshDeformTime);
                printf("    Total Transform Update Time (s): %.4f s\n", totalTransformUpdateTime);
                printf("    Total Mesh Deform Time (s):      %.4f s\n", totalMeshDeformTime);
                printf("    Transform Mesh Ratio:            %.4f %%\n", totalTransformUpdateTime / totalMeshDeformTime);
            }
            else
            {
                printf("- Total Time (s):                    %.4f s\n", totalTransformUpdateTime + totalMeshDeformTime);
            }

            // Transform update
            float transformBest;
            float transformMean;
            float transformWorst;
            float transformStdDeviation;
            AZStd::tie(transformBest, transformMean, transformWorst, transformStdDeviation) = CalculateStats(transformUpdateFrameTimes);

            printf("- Transform update:\n");
            printf("    Best Frame:                      %.4f ms (%.1f FPS)\n", transformBest * 1000.0f, transformBest > 0.0f ? 1.0f / transformBest : 0.0f);
            printf("    Mean Frame:                      %.4f ms (%.1f FPS)\n", transformMean * 1000.0f, transformMean > 0.0f ? 1.0f / transformMean : 0.0f);
            printf("    Worst Frame:                     %.4f ms (%.1f FPS)\n", transformWorst * 1000.0f, transformWorst > 0.0f ? 1.0f / transformWorst : 0.0f);
            printf("    Std Deviation:                   %.4f ms\n", transformStdDeviation * 1000.0f);

            // Mesh deforms
            if (param.m_includeSoftwareSkinning)
            {
                float meshDeformBest;
                float meshDeformMean;
                float meshDeformWorst;
                float meshDeformStdDeviation;
                AZStd::tie(meshDeformBest, meshDeformMean, meshDeformWorst, meshDeformStdDeviation) = CalculateStats(meshDeformFrameTimes);

                printf("- Mesh deforms:\n");
                printf("    Best Frame:                      %.4f ms\n", meshDeformBest * 1000.0f);
                printf("    Mean Frame:                      %.4f ms\n", meshDeformMean * 1000.0f);
                printf("    Worst Frame:                     %.4f ms\n", meshDeformWorst * 1000.0f);
                printf("    Std Deviation:                   %.4f ms\n", meshDeformStdDeviation * 1000.0f);
            }

            printf("----------------------------------------------------\n");
        }

        void TestMotionSamplingPerformance(const char* motionFilename)
        {
            const AZStd::string assetFolder = GetAssetFolder();
            GetEMotionFX().SetMediaRootFolder(assetFolder.c_str());
            GetEMotionFX().InitAssetFolderPaths();

            const char* actorFilename  = "@products@\\animationsamples\\advanced_rinlocomotion\\actor\\rinactor.actor";

            Importer* importer = GetEMotionFX().GetImporter();
            importer->SetLoggingEnabled(false);

            const AZStd::string resolvedActorFilename = ResolvePath(actorFilename);
            EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Exists(resolvedActorFilename.c_str()))
                << AZStd::string::format("Actor file '%s' does not exist on local hard drive.", resolvedActorFilename.c_str()).c_str();
            AZStd::unique_ptr<Actor> actor = importer->LoadActor(resolvedActorFilename);
            ASSERT_NE(actor, nullptr) << "Actor failed to load.";
            Motion* motion = importer->LoadMotion(ResolvePath(motionFilename));
            ASSERT_NE(motion, nullptr) << "Motion failed to load.";

            ActorInstance* actorInstance = ActorInstance::Create(actor.get());

            const Pose* bindPose = actor->GetBindPose();
            Pose outPose;
            outPose.InitFromBindPose(actorInstance);

            MotionInstance* motionInstance = GetMotionInstancePool().RequestNew(motion, actorInstance);

            const size_t numSamples = 100000;

            AZ::Debug::Timer timer;

            // Sample the same time value.
            timer.Stamp();
            for (size_t i = 0; i < numSamples; ++i)
            {
                motionInstance->SetCurrentTimeNormalized(0.33f);
                motion->Update(bindPose, &outPose, motionInstance);
            }
            float activationTime = timer.GetDeltaTimeInSeconds();
            printf("Sampling same frame = %.2f ms\n", activationTime * 1000.0f);

            // Sample random time values.
            timer.Stamp();
            for (size_t i = 0; i < numSamples; ++i)
            {
                motionInstance->SetCurrentTimeNormalized(rand() / static_cast<float>(RAND_MAX));
                motion->Update(bindPose, &outPose, motionInstance);
            }
            activationTime = timer.GetDeltaTimeInSeconds();
            printf("Sampling random frame = %.2f ms\n", activationTime * 1000.0f);

            // Sample forward.
            timer.Stamp();
            for (size_t i = 0; i < numSamples; ++i)
            {
                motionInstance->SetCurrentTimeNormalized(i / static_cast<float>(numSamples));
                motion->Update(bindPose, &outPose, motionInstance);
            }
            activationTime = timer.GetDeltaTimeInSeconds();
            printf("Sampling forward sequential = %.2f ms\n", activationTime * 1000.0f);

            // Sample backward.
            timer.Stamp();
            for (size_t i = 0; i < numSamples; ++i)
            {
                motionInstance->SetCurrentTimeNormalized(1.0f - (i / static_cast<float>(numSamples)));
                motion->Update(bindPose, &outPose, motionInstance);
            }
            activationTime = timer.GetDeltaTimeInSeconds();
            printf("Sampling backward sequential = %.2f ms\n", activationTime * 1000.0f);

            GetMotionInstancePool().Free(motionInstance);
            actorInstance->Destroy();
            motion->Destroy();
        }

    private:
        AZ::SimpleLcgRandom* m_random;
        std::vector<SampleParameterSet> m_pyroParameterSet;
    };

    TEST_P(PerformanceTestFixture, DISABLED_PerformanceTest)
    {
        const PerformanceTestParameters& param = GetParam();
        const size_t numIterations = static_cast<size_t>(param.m_totalTestTimeInSeconds * param.m_fps);
        const float frameTimeDelta = 1.0f / param.m_fps;
        param.Print();

        ActorManager* actorManager = GetEMotionFX().GetActorManager();
        ActorUpdateScheduler* scheduler = nullptr;
        if (param.m_useMultiThreading)
        {
            scheduler = MultiThreadScheduler::Create();
        }
        else
        {
            scheduler = SingleThreadScheduler::Create();
        }
        actorManager->SetScheduler(scheduler);

        const AZStd::string assetFolder = GetAssetFolder();
        GetEMotionFX().SetMediaRootFolder(assetFolder.c_str());
        GetEMotionFX().InitAssetFolderPaths();

        // This path points to assets in the advance rin demo.
        // To test different assets, change the path here.
        const char* actorFilename     = "@products@\\AnimationSamples\\Advanced_RinLocomotion\\Actor\\rinActor.actor";
        const char* motionSetFilename = "@products@\\AnimationSamples\\Advanced_RinLocomotion\\AnimationEditorFiles\\Advanced_RinLocomotion.motionset";
        const char* animGraphFilename = "@products@\\AnimationSamples\\Advanced_RinLocomotion\\AnimationEditorFiles\\Advanced_RinLocomotion.animgraph";

        Importer* importer = GetEMotionFX().GetImporter();
        importer->SetLoggingEnabled(false);

        AZStd::unique_ptr<Actor> actor = importer->LoadActor(ResolvePath(actorFilename));
        ASSERT_NE(actor, nullptr) << "Actor failed to load.";
        MotionSet* motionSet = importer->LoadMotionSet(ResolvePath(motionSetFilename));
        ASSERT_NE(motionSet, nullptr) << "Motion set failed to load.";

        AnimGraph* animGraph = importer->LoadAnimGraph(ResolvePath(animGraphFilename));
        ASSERT_NE(animGraph, nullptr) << "Anim graph failed to load.";

        // Create instances and start running the anim graphs.
        AZStd::vector<ActorInstance*> actorInstances;
        AZStd::vector<ActorInstance*> actorInstancesIncludingAttachments;
        actorInstances.reserve(param.m_numInstances);
        for (size_t i = 0; i < param.m_numInstances; ++i)
        {
            ActorInstance* actorInstance = ActorInstance::Create(actor.get());
            if (!AZ::IsClose(param.m_motionSamplingRate, 0.0f, AZ::Constants::FloatEpsilon))
            {
                actorInstance->SetMotionSamplingRate(1.0f / param.m_motionSamplingRate);
            }
            actorInstance->SetLocalSpacePosition(RandomRangeVec3(-100.0f, 100.0f));
            actorInstances.emplace_back(actorInstance);
            actorInstancesIncludingAttachments.emplace_back(actorInstance);

            AnimGraphInstance* animGraphInstance = AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
            actorInstance->SetAnimGraphInstance(animGraphInstance);

            actorInstance->SetLODLevel(param.m_lodLevel);
            actorInstance->UpdateTransformations(0.0f);

            // Add skin attachments.
            for (size_t attachmentNr = 0; attachmentNr < param.m_numSkinAttachmentsPerInstance; ++attachmentNr)
            {
                ActorInstance* attachmentActorInstance = ActorInstance::Create(actor.get());
                EMotionFX::Attachment* attachment = EMotionFX::AttachmentSkin::Create(/*attachmentTarget*/actorInstance, attachmentActorInstance);
                actorInstance->AddAttachment(attachment);
                actorInstancesIncludingAttachments.emplace_back(attachmentActorInstance);
            }
        }

        // Preload motions and make sure they got loaded successfully.
        motionSet->Preload();
        const auto& motionEntries = motionSet->GetMotionEntries();
        for (const auto& motionEntryPair : motionEntries)
        {
            const MotionSet::MotionEntry* motionEntry = motionEntryPair.second;
            ASSERT_NE(motionEntry->GetMotion(), nullptr);
        }

        AZ::Debug::Timer timer;
        AZStd::vector<float> transformUpdateFrameTimes;
        AZStd::vector<float> meshDeformFrameTimes;
        transformUpdateFrameTimes.reserve(numIterations);
        meshDeformFrameTimes.reserve(numIterations);
        float totalTransformUpdateTime = 0.0f;
        float totalMeshDeformTime = 0.0f;

        const float randomizeParametersEvery = 1.0; // Change parameters every second
        float randomizeParameterTimer = 0.0f;

        for (size_t i = 0; i < numIterations; ++i)
        {
            randomizeParameterTimer += frameTimeDelta;
            if (randomizeParameterTimer >= randomizeParametersEvery)
            {
                RandomizeParameters(actorInstances);
                randomizeParameterTimer = 0.0;
            }

            // Output skeletal poses.
            timer.Stamp();
            GetEMotionFX().Update(frameTimeDelta);
            const float transformUpdateTime = timer.GetDeltaTimeInSeconds();
            totalTransformUpdateTime += transformUpdateTime;
            transformUpdateFrameTimes.emplace_back(transformUpdateTime);

            // Update mesh deformers (software skinning).
            if (param.m_includeSoftwareSkinning)
            {
                timer.Stamp();
                for (ActorInstance* actorInstance : actorInstancesIncludingAttachments)
                {
                    actorInstance->UpdateMeshDeformers(frameTimeDelta);
                }
                const float meshDeformTime = timer.GetDeltaTimeInSeconds();
                totalMeshDeformTime += meshDeformTime;
                meshDeformFrameTimes.emplace_back(meshDeformTime);
            }
        }

        PrintReport(transformUpdateFrameTimes, meshDeformFrameTimes, totalTransformUpdateTime, totalMeshDeformTime);

        for (ActorInstance* actorInstance : actorInstancesIncludingAttachments)
        {
            actorInstance->Destroy();
        }
        delete animGraph;
        delete motionSet;
    }

    const float g_updatesPerSec = 60.0f;
    const float g_totalTestTime = 60.0f; // 1 minute
    const size_t g_numInstances = 100;

    std::vector<PerformanceTestParameters> performanceTestData
    {

        // Baseline
        {
            "Baseline",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            false // multi-threading
        },
        // Multi-threading
        {
            "Multi-threading",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading with 1 skin attachments
        {
            "Multi-threading with 1 skin attachments",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            1, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading and restricted motion sampling rate
        {
            "Multi-threading and restricted motion sampling rate",
            g_updatesPerSec, // fps
            60.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading and lower motion sampling rate
        {
            "Multi-threading and lower motion sampling rate",
            g_updatesPerSec, // fps
            30.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading and lower motion sampling rate
        {
            "Multi-threading and lower motion sampling rate",
            g_updatesPerSec, // fps
            10.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading at LOD level = 1
        {
            "Multi-threading at LOD level = 1",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            1, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading at LOD level = 2
        {
            "Multi-threading at LOD level = 2",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            2, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading at LOD level = 3
        {
            "Multi-threading at LOD level = 3",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            3, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Multi-threading at LOD level = 4
        {
            "Multi-threading at LOD level = 4",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            4, // lod level
            false, // software skinning
            true // multi-threading
        },
        // Server test: LOD0
        {
            "Server test: LOD0",
            30.0f, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            false // multi-threading
        },
        // Server test: LOD4
        {
            "Server test: LOD4",
            30.0f, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            4, // lod level
            false, // software skinning
            false // multi-threading
        },
        // Server test: Server actor optimization (bone removal).
        {
            "Server test: Server actor optimization (bone removal)",
            30.0f, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            false // multi-threading
        }/*,
        // Software skinning (this is extremely slow and will take a long time to run)
        {
            "Software skinning",
            g_updatesPerSec, // fps
            0.0f, // motion sampling rate
            g_totalTestTime, // total test time
            g_numInstances, // instances
            0, // num skin attachments per instance
            0, // lod level
            true, // software skinning
            false // multi-threading
        }*/
    };

    std::vector<PerformanceTestParameters> debugTestData
    {
        {
            "debug",
            g_updatesPerSec, // fps
            60.0f, // motion sampling rate
            g_totalTestTime, // total test time
            1, // instances
            0, // num skin attachments per instance
            0, // lod level
            false, // software skinning
            true // multi-threading
        }
    };

    INSTANTIATE_TEST_CASE_P(PerformanceTests,
        PerformanceTestFixture,
        ::testing::ValuesIn(performanceTestData));

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    TEST_F(PerformanceTestFixture, DISABLED_DeferredInitPerformanceTest)
    {
        const AZStd::string assetFolder = GetAssetFolder();
        GetEMotionFX().SetMediaRootFolder(assetFolder.c_str());
        GetEMotionFX().InitAssetFolderPaths();

        // This path points to assets in the advance rin demo.
        // To test different assets, change the path here.
        const char* actorFilename     = "@products@\\AnimationSamples\\Advanced_RinLocomotion\\Actor\\rinActor.actor";
        const char* motionSetFilename = "@products@\\AnimationSamples\\Advanced_RinLocomotion\\AnimationEditorFiles\\Advanced_RinLocomotion.motionset";
        const char* animGraphFilename = "@products@\\AnimationSamples\\Advanced_RinLocomotion\\AnimationEditorFiles\\Advanced_RinLocomotion.animgraph";

        Importer* importer = GetEMotionFX().GetImporter();
        importer->SetLoggingEnabled(false);

        const AZStd::string resolvedActorFilename = ResolvePath(actorFilename);
        EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Exists(resolvedActorFilename.c_str()))
            << AZStd::string::format("Actor file '%s' does not exist on local hard drive.", resolvedActorFilename.c_str()).c_str();
        AZStd::unique_ptr<Actor> actor = importer->LoadActor(resolvedActorFilename);
        ASSERT_NE(actor, nullptr) << "Actor failed to load.";
        MotionSet* motionSet = importer->LoadMotionSet(ResolvePath(motionSetFilename));
        ASSERT_NE(motionSet, nullptr) << "Motion set failed to load.";

        AnimGraph* animGraph = importer->LoadAnimGraph(ResolvePath(animGraphFilename));
        ASSERT_NE(animGraph, nullptr) << "Anim graph failed to load.";

        // Create instances.
        const size_t numInstances = 1000;
        AZStd::vector<ActorInstance*> actorInstances;
        for (size_t i = 0; i < numInstances; ++i)
        {
            ActorInstance* actorInstance = ActorInstance::Create(actor.get());
            actorInstances.emplace_back(actorInstance);
        }

        // Preload motions and make sure they got loaded successfully.
        motionSet->Preload();
        const auto& motionEntries = motionSet->GetMotionEntries();
        for (const auto& motionEntryPair : motionEntries)
        {
            const MotionSet::MotionEntry* motionEntry = motionEntryPair.second;
            ASSERT_NE(motionEntry->GetMotion(), nullptr);
        }

        AZ::Debug::Timer timer;
        timer.Stamp();
        for (ActorInstance* actorInstance : actorInstances)
        {
            AnimGraphInstance* animGraphInstance = AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
            actorInstance->SetAnimGraphInstance(animGraphInstance);
        }
        const float activationTime = timer.GetDeltaTimeInSeconds();
        printf("Instantiating took = %.2f ms\n", activationTime * 1000.0f);
        printf("Activation Time = %.2f ms\n", activationTime * 1000.0f);

        for (ActorInstance* actorInstance : actorInstances)
        {
            actorInstance->Destroy();
        }
        delete animGraph;
        delete motionSet;
    }

    TEST_F(PerformanceTestFixture, DISABLED_MotionSamplingPerformanceNonUniform)
    {
        // Make sure that the motion is set to use NonUniform sampling! Change this in the scene settings! Otherwise you get wrong results.
        TestMotionSamplingPerformance("@products@\\animationsamples\\advanced_rinlocomotion\\motions\\rin_idle.motion");
    }

    TEST_F(PerformanceTestFixture, DISABLED_MotionSamplingPerformanceUniform)
    {
        // Make sure that the motion is set to use Uniform sampling! Change this in the scene settings! Otherwise you get wrong results.
        TestMotionSamplingPerformance("@products@\\animationsamples\\advanced_rinlocomotion\\motions\\rin_walk_kick_01.motion");
    }

} // namespace EMotionFX

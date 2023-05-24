/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Timer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Task/TaskGraph.h>

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/Motion.h>

#include <Allocators.h>
#include <Feature.h>
#include <FeatureMatrixMinMaxScaler.h>
#include <FeatureMatrixStandardScaler.h>
#include <FeatureSchemaDefault.h>
#include <FeatureTrajectory.h>
#include <FrameDatabase.h>
#include <KdTree.h>
#include <MotionMatchingData.h>

namespace EMotionFX::MotionMatching
{
    AZ_CVAR_EXTERNED(bool, mm_multiThreadedInitialization);

    AZ_CLASS_ALLOCATOR_IMPL(MotionMatchingData, MotionMatchAllocator)

    MotionMatchingData::MotionMatchingData(const FeatureSchema& featureSchema)
        : m_featureSchema(featureSchema)
    {
        m_kdTree = AZStd::make_unique<KdTree>();
    }

    MotionMatchingData::~MotionMatchingData()
    {
        Clear();
    }

    bool MotionMatchingData::ExtractFeatures(ActorInstance* actorInstance, FrameDatabase* frameDatabase)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingData::ExtractFeatures");
        AZ::Debug::Timer timer;
        timer.Stamp();

        const size_t numFrames = frameDatabase->GetNumFrames();
        if (numFrames == 0)
        {
            return true;
        }

        // Initialize all features before we process each frame.
        FeatureMatrix::Index featureComponentCount = 0;
        for (Feature* feature : m_featureSchema.GetFeatures())
        {
            Feature::InitSettings frameSettings;
            frameSettings.m_actorInstance = actorInstance;
            if (!feature->Init(frameSettings))
            {
                return false;
            }

            feature->SetColumnOffset(featureComponentCount);
            featureComponentCount += feature->GetNumDimensions();
        }

        // Allocate memory for the feature matrix
        m_featureMatrix.resize(/*rows=*/numFrames, /*columns=*/featureComponentCount);

        // Multi-threaded
        if (mm_multiThreadedInitialization)
        {
            const size_t numBatches = aznumeric_caster(ceilf(aznumeric_cast<float>(numFrames) / aznumeric_cast<float>(s_numFramesPerBatch)));

            AZ::TaskGraphActiveInterface* taskGraphActiveInterface = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();
            const bool useTaskGraph = taskGraphActiveInterface && taskGraphActiveInterface->IsTaskGraphActive();
            if (useTaskGraph)
            {
                AZ::TaskGraph m_taskGraph{ "MotionMatching FeatureExtraction" };

                // Split-up the motion database into batches of frames and extract the feature values for each batch simultaneously.
                for (size_t batchIndex = 0; batchIndex < numBatches; ++batchIndex)
                {
                    const size_t startFrame = batchIndex * s_numFramesPerBatch;
                    const size_t endFrame = AZStd::min(startFrame + s_numFramesPerBatch, numFrames);

                    // Create a task for every batch and extract the features simultaneously.
                    AZ::TaskDescriptor taskDescriptor{ "ExtractFeatures", "MotionMatching" };
                    m_taskGraph.AddTask(
                        taskDescriptor,
                        [this, actorInstance, startFrame, endFrame]()
                        {
                            ExtractFeatureValuesRange(actorInstance, m_frameDatabase, m_featureSchema, m_featureMatrix, startFrame, endFrame);
                        });
                }

                AZ::TaskGraphEvent finishedEvent{ "MotionMatching FeatureExtraction Wait" };
                m_taskGraph.Submit(&finishedEvent);
                finishedEvent.Wait();
            }
            else // job system
            {
                AZ::JobCompletion jobCompletion;

                // Split-up the motion database into batches of frames and extract the feature values for each batch simultaneously.
                for (size_t batchIndex = 0; batchIndex < numBatches; ++batchIndex)
                {
                    const size_t startFrame = batchIndex * s_numFramesPerBatch;
                    const size_t endFrame = AZStd::min(startFrame + s_numFramesPerBatch, numFrames);

                    // Create a job for every batch and extract the features simultaneously.
                    AZ::JobContext* jobContext = nullptr;
                    AZ::Job* job = AZ::CreateJobFunction([this, actorInstance, startFrame, endFrame]()
                        {
                            ExtractFeatureValuesRange(actorInstance, m_frameDatabase, m_featureSchema, m_featureMatrix, startFrame, endFrame);
                        }, /*isAutoDelete=*/true, jobContext);
                    job->SetDependent(&jobCompletion);
                    job->Start();
                }

                jobCompletion.StartAndWaitForCompletion();
            }
        }
        else // Single-threaded
        {
            ExtractFeatureValuesRange(actorInstance, m_frameDatabase, m_featureSchema, m_featureMatrix, /*startFrame=*/0, numFrames);
        }

        const float extractFeaturesTime = timer.GetDeltaTimeInSeconds();
        AZ_Printf("Motion Matching", "Extracting features for %zu frames took %.2f ms.", m_featureMatrix.rows(), extractFeaturesTime * 1000.0f);
        return true;
    }

    void MotionMatchingData::ExtractFeatureValuesRange(ActorInstance* actorInstance, FrameDatabase& frameDatabase, const FeatureSchema& featureSchema, FeatureMatrix& featureMatrix, size_t startFrame, size_t endFrame)
    {
        // Iterate over all frames and extract the data for this frame.
        AnimGraphPosePool posePool;
        AnimGraphPose* pose = posePool.RequestPose(actorInstance);

        Feature::ExtractFeatureContext context(featureMatrix, posePool);
        context.m_frameDatabase = &frameDatabase;
        context.m_framePose = &pose->GetPose();
        context.m_actorInstance = actorInstance;
        const auto& frames = frameDatabase.GetFrames();

        for (size_t frameIndex = startFrame; frameIndex < endFrame; ++frameIndex)
        {
            const Frame& frame = frames[frameIndex];
            context.m_frameIndex = frame.GetFrameIndex();

            // Pre-sample the frame pose as that will be needed by many of the feature extraction calculations.
            frame.SamplePose(const_cast<Pose*>(context.m_framePose));

            // Extract all features for the given frame.
            {
                for (Feature* feature : featureSchema.GetFeatures())
                {
                    feature->ExtractFeatureValues(context);
                }
            }
        }

        posePool.FreePose(pose);
    }

    bool MotionMatchingData::Init(const InitSettings& settings)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingData::Init");

        AZ::Debug::Timer initTimer;
        initTimer.Stamp();

        ///////////////////////////////////////////////////////////////////////
        // 1. Import motion data

        // Import all motion frames.
        size_t totalNumFramesImported = 0;
        size_t totalNumFramesDiscarded = 0;
        for (Motion* motion : settings.m_motionList)
        {
            size_t numFrames = 0;
            size_t numDiscarded = 0;
            std::tie(numFrames, numDiscarded) = m_frameDatabase.ImportFrames(motion, settings.m_frameImportSettings, false);
            totalNumFramesImported += numFrames;
            totalNumFramesDiscarded += numDiscarded;

            if (settings.m_importMirrored)
            {
                std::tie(numFrames, numDiscarded) = m_frameDatabase.ImportFrames(motion, settings.m_frameImportSettings, true);
                totalNumFramesImported += numFrames;
                totalNumFramesDiscarded += numDiscarded;
            }
        }

        if (totalNumFramesImported > 0 || totalNumFramesDiscarded > 0)
        {
            AZ_TracePrintf("Motion Matching", "Imported a total of %d frames (%d frames discarded) across %d motions. This is %.2f seconds (%.2f minutes) of motion data.",
                totalNumFramesImported,
                totalNumFramesDiscarded,
                settings.m_motionList.size(),
                totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate,
                (totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate) / 60.0f);
        }

        ///////////////////////////////////////////////////////////////////////
        // 2. Extract feature data and place the values into the feature matrix.

        if (!ExtractFeatures(settings.m_actorInstance, &m_frameDatabase))
        {
            AZ_Error("Motion Matching", false, "Failed to extract features from motion database.");
            return false;
        }

        ///////////////////////////////////////////////////////////////////////
        // 3. Transform feature data / -matrix
        // Note: Do this before initializing the KD-tree as the query vector will contain pre-transformed data as well.
        if (settings.m_normalizeData)
        {
            AZ_PROFILE_SCOPE(Animation, "MotionMatchingData::TransformFeatures");
            AZ::Debug::Timer transformFeatureTimer;
            transformFeatureTimer.Stamp();

            switch (settings.m_featureScalerType)
            {
            case FeatureScalerType::StandardScalerType:
                {
                    m_featureTransformer.reset(aznew StandardScaler());
                    break;
                }
            case FeatureScalerType::MinMaxScalerType:
                {
                    m_featureTransformer.reset(aznew MinMaxScaler());
                    break;
                }
            default:
                {
                    m_featureTransformer.reset();
                    AZ_Error("Motion Matching", false, "Unknown feature scaler type.")
                }
            }

            m_featureTransformer->Fit(m_featureMatrix, settings.m_featureTansformerSettings);
            m_featureMatrix = m_featureTransformer->Transform(m_featureMatrix);

            const float transformFeatureTime = transformFeatureTimer.GetDeltaTimeInSeconds();
            AZ_Printf("Motion Matching", "Transforming/normalizing features took %.2f ms.", transformFeatureTime * 1000.0f);
        }
        else
        {
            m_featureTransformer.reset();
        }

        ///////////////////////////////////////////////////////////////////////
        // 4. Initialize the kd-tree used to accelerate the searches
        {
            // Use all features other than the trajectory for the broad-phase search using the KD-Tree.
            for (Feature* feature : m_featureSchema.GetFeatures())
            {
                if (feature->RTTI_GetType() != azrtti_typeid<FeatureTrajectory>())
                {
                    m_featuresInKdTree.push_back(feature);
                }
            }

            if (!m_kdTree->Init(m_frameDatabase, m_featureMatrix, m_featuresInKdTree, settings.m_maxKdTreeDepth, settings.m_minFramesPerKdTreeNode)) // Internally automatically clears any existing contents.
            {
                AZ_Error("EMotionFX", false, "Failed to initialize KdTree acceleration structure.");
                return false;
            }
        }

        const float initTime = initTimer.GetDeltaTimeInSeconds();
        AZ_Printf("Motion Matching", "Feature matrix (%zu, %zu) uses %.2f MB and took %.2f ms to initialize (including initialization of acceleration structures).",
            m_featureMatrix.rows(),
            m_featureMatrix.cols(),
            static_cast<float>(m_featureMatrix.CalcMemoryUsageInBytes()) / 1024.0f / 1024.0f,
            initTime * 1000.0f);

        return true;
    }

    void MotionMatchingData::Clear()
    {
        m_frameDatabase.Clear();
        m_featureMatrix.Clear();
        m_kdTree->Clear();
        m_featuresInKdTree.clear();
    }
} // namespace EMotionFX::MotionMatching

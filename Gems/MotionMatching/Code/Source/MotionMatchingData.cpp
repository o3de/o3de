/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Timer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/Motion.h>

#include <Allocators.h>
#include <Feature.h>
#include <FeatureSchemaDefault.h>
#include <FeatureTrajectory.h>
#include <FrameDatabase.h>
#include <KdTree.h>
#include <MotionMatchingData.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionMatchingData, MotionMatchAllocator, 0)

    MotionMatchingData::MotionMatchingData(const FeatureSchema& featureSchema)
        : m_featureSchema(featureSchema)
    {
        m_kdTree = AZStd::make_unique<KdTree>();
    }

    MotionMatchingData::~MotionMatchingData()
    {
        Clear();
    }

    bool MotionMatchingData::ExtractFeatures(ActorInstance* actorInstance, FrameDatabase* frameDatabase, size_t maxKdTreeDepth, size_t minFramesPerKdTreeNode)
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

        const auto& frames = frameDatabase->GetFrames();

        // Allocate memory for the feature matrix
        m_featureMatrix.resize(/*rows=*/numFrames, /*columns=*/featureComponentCount);

        // Iterate over all frames and extract the data for this frame.
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool();
        AnimGraphPose* pose = posePool.RequestPose(actorInstance);

        Feature::ExtractFeatureContext context(m_featureMatrix);
        context.m_frameDatabase = frameDatabase;
        context.m_framePose = &pose->GetPose();
        context.m_actorInstance = actorInstance;

        for (const Frame& frame : frames)
        {
            context.m_frameIndex = frame.GetFrameIndex();

            // Pre-sample the frame pose as that will be needed by many of the feature extraction calculations.
            frame.SamplePose(const_cast<Pose*>(context.m_framePose));

            // Extract all features for the given frame.
            {
                for (Feature* feature : m_featureSchema.GetFeatures())
                {
                    feature->ExtractFeatureValues(context);
                }
            }
        }

        posePool.FreePose(pose);

        const float extractFeaturesTime = timer.GetDeltaTimeInSeconds();
        timer.Stamp();

        // Initialize the kd-tree used to accelerate the searches.
        if (!m_kdTree->Init(*frameDatabase, m_featureMatrix, m_featuresInKdTree, maxKdTreeDepth, minFramesPerKdTreeNode)) // Internally automatically clears any existing contents.
        {
            AZ_Error("EMotionFX", false, "Failed to initialize KdTree acceleration structure.");
            return false;
        }

        const float initKdTreeTimer = timer.GetDeltaTimeInSeconds();

        AZ_Printf("MotionMatching", "Feature matrix (%zu, %zu) uses %.2f MB and took %.2f ms to initialize (KD-Tree %.2f ms).",
            m_featureMatrix.rows(),
            m_featureMatrix.cols(),
            static_cast<float>(m_featureMatrix.CalcMemoryUsageInBytes()) / 1024.0f / 1024.0f,
            extractFeaturesTime * 1000.0f,
            initKdTreeTimer * 1000.0f);

        return true;
    }

    bool MotionMatchingData::Init(const InitSettings& settings)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingData::Init");

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

        // Use all features other than the trajectory for the broad-phase search using the KD-Tree.
        for (Feature* feature : m_featureSchema.GetFeatures())
        {
            if (feature->RTTI_GetType() != azrtti_typeid<FeatureTrajectory>())
            {
                m_featuresInKdTree.push_back(feature);
            }
        }

        // Extract feature data and place the values into the feature matrix.
        if (!ExtractFeatures(settings.m_actorInstance, &m_frameDatabase, settings.m_maxKdTreeDepth, settings.m_minFramesPerKdTreeNode))
        {
            AZ_Error("Motion Matching", false, "Failed to extract features from motion database.");
            return false;
        }

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

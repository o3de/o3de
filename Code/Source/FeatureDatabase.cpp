/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Debug/Timer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Allocators.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EMotionFXManager.h>

#include <FeatureDatabase.h>
#include <FrameDatabase.h>
#include <KdTree.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeatureDatabase, MotionMatchAllocator, 0)

    FeatureDatabase::FeatureDatabase()
    {
        m_kdTree = AZStd::make_unique<KdTree>();
    }

    FeatureDatabase::~FeatureDatabase()
    {
        Clear();
    }

    void FeatureDatabase::Clear()
    {
        m_featureSchema.Clear();
        m_featureMatrix.Clear();
        m_kdTree->Clear();
    }

    size_t FeatureDatabase::CalcMemoryUsageInBytes() const
    {
        size_t result = 0;
        result += m_kdTree->CalcMemoryUsageInBytes();
        result += m_featureMatrix.CalcMemoryUsageInBytes();
        return result;
    }

    bool FeatureDatabase::ExtractFeatures(ActorInstance* actorInstance, FrameDatabase* frameDatabase, size_t maxKdTreeDepth, size_t minFramesPerKdTreeNode)
    {
        AZ_PROFILE_SCOPE(Animation, "FeatureDatabase::ExtractFeatures");
        AZ::Debug::Timer timer;
        timer.Stamp();

        const size_t numFrames = frameDatabase->GetNumFrames();
        if (numFrames == 0)
        {
            return true;
        }

        // Initialize all frame datas before we process each frame.
        FeatureMatrix::Index featureComponentCount = 0;
        for (Feature* feature : m_featureSchema.GetFeatures())
        {
            if (feature && feature->GetId().IsNull())
            {
                return false;
            }

            Feature::InitSettings frameSettings;
            frameSettings.m_actorInstance = actorInstance;
            feature->SetFrameDatabase(frameDatabase);
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
        if (!m_kdTree->Init(*frameDatabase, *this, maxKdTreeDepth, minFramesPerKdTreeNode)) // Internally automatically clears any existing contents.
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

    void FeatureDatabase::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
        MotionMatchingInstance* instance,
        size_t frameIndex)
    {
        for (Feature* feature: m_featureSchema.GetFeatures())
        {
            if (feature && feature->GetId().IsNull())
            {
                continue;
            }

            if (feature->GetDebugDrawEnabled())
            {
                feature->DebugDraw(debugDisplay, instance, frameIndex);
            }
        }
    }

    void FeatureDatabase::SaveAsCsv(const AZStd::string& filename)
    {
        AZStd::vector<AZStd::string> columnNames;

        for (Feature* feature: m_featureSchema.GetFeatures())
        {
            const size_t numDimensions = feature->GetNumDimensions();
            for (size_t dimension = 0; dimension < numDimensions; ++dimension)
            {
                columnNames.push_back(feature->GetDimensionName(dimension));
            }
        }

        m_featureMatrix.SaveAsCsv(filename, columnNames);
    }

    size_t FeatureDatabase::CalcNumDataDimensionsForKdTree(const FeatureDatabase& featureDatabase) const
    {
        size_t result = 0;
        for (Feature* feature : featureDatabase.GetFeaturesInKdTree())
        {
            if (feature->GetId().IsNull())
            {
                continue;
            }

            result += feature->GetNumDimensions();
        }
        return result;
    }
} // namespace EMotionFX::MotionMatching

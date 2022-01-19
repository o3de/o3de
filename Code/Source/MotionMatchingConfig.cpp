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
#include <MotionMatchingConfig.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionMatchingConfig, MotionMatchAllocator, 0)

    MotionMatchingConfig::MotionMatchingConfig()
    {
        m_kdTree = AZStd::make_unique<KdTree>();
    }

    MotionMatchingConfig::~MotionMatchingConfig()
    {
        Clear();
    }

    bool MotionMatchingConfig::RegisterFeatures(const InitSettings& settings)
    {
        const Node* rootJoint = settings.m_actorInstance->GetActor()->GetMotionExtractionNode();
        if (!rootJoint)
        {
            AZ_Error("MotionMatching", false, "Cannot register features. Cannot find motion extraction joint.");
            return false;
        }

        DefaultFeatureSchemaInitSettings defaultSettings;
        defaultSettings.m_rootJointName = rootJoint->GetNameString();
        defaultSettings.m_leftFootJointName = "L_foot_JNT";
        defaultSettings.m_rightFootJointName = "R_foot_JNT";
        defaultSettings.m_pelvisJointName = "C_pelvis_JNT";
        DefaultFeatureSchema(m_featureSchema, defaultSettings);

        return true;
    }

    bool MotionMatchingConfig::ExtractFeatures(ActorInstance* actorInstance, FrameDatabase* frameDatabase, size_t maxKdTreeDepth, size_t minFramesPerKdTreeNode)
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

    bool MotionMatchingConfig::Init(const InitSettings& settings)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingConfig::Init");

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
            AZ_TracePrintf("EMotionFX", "Motion matching config '%s' has imported a total of %d frames (%d frames discarded) across %d motions. This is %.2f seconds (%.2f minutes) of motion data.", RTTI_GetTypeName(), totalNumFramesImported, totalNumFramesDiscarded, settings.m_motionList.size(), totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate, (totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate) / 60.0f);
        }

        if (!RegisterFeatures(settings))
        {
            AZ_Error("EMotionFX", false, "Failed to register features inside motion matching config.");
            return false;
        }

        // Use all features other than the trajectory for the broad-phase search using the KD-Tree.
        for (Feature* feature : m_featureSchema.GetFeatures())
        {
            if (feature->RTTI_GetType() != azrtti_typeid<FeatureTrajectory>())
            {
                m_featuresInKdTree.push_back(feature);
            }
        }

        // Now build the per frame data (slow).
        if (!ExtractFeatures(settings.m_actorInstance, &m_frameDatabase, settings.m_maxKdTreeDepth, settings.m_minFramesPerKdTreeNode))
        {
            AZ_Error("EMotionFX", false, "Failed to generate frame datas inside motion matching config.");
            return false;
        }

        return true;
    }

    void MotionMatchingConfig::Clear()
    {
        m_frameDatabase.Clear();
        m_featureSchema.Clear();
        m_featureMatrix.Clear();
        m_kdTree->Clear();
        m_featuresInKdTree.clear();
    }

    void MotionMatchingConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionMatchingConfig>()
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MotionMatchingConfig>("MotionMatchingConfig", "Base class for motion matching configs")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
    }
} // namespace EMotionFX::MotionMatching

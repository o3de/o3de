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

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <MotionMatchingConfig.h>
#include <MotionMatchingInstance.h>
#include <Feature.h>
#include <FeatureTrajectory.h>
#include <FrameDatabase.h>
#include <EMotionFX/Source/TransformData.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(MotionMatchingConfig, MotionMatchAllocator, 0)

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

            // Now build the per frame data (slow).
            if (!m_features.ExtractFeatures(settings.m_actorInstance, &m_frameDatabase, settings.m_maxKdTreeDepth, settings.m_minFramesPerKdTreeNode))
            {
                AZ_Error("EMotionFX", false, "Failed to generate frame datas inside motion matching config.");
                return false;
            }

            return true;
        }

        void MotionMatchingConfig::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, MotionMatchingInstance* instance)
        {
            AZ_PROFILE_SCOPE(Animation, "MotionMatchingConfig::DebugDraw");

            // Get the lowest cost frame index from the last search. As we're searching the feature database with a much lower
            // frequency and sample the animation onwards from this, the resulting frame index does not represent the current
            // feature values from the shown pose.
            const size_t curFrameIndex = instance->GetLowestCostFrameIndex();
            if (curFrameIndex == InvalidIndex)
            {
                return;
            }

            // Find the frame index in the frame database that belongs to the currently used pose.
            MotionInstance* motionInstance = instance->GetMotionInstance();
            const size_t currentFrame = m_frameDatabase.FindFrameIndex(motionInstance->GetMotion(), motionInstance->GetCurrentTime());
            if (currentFrame != InvalidIndex)
            {
                m_features.DebugDraw(debugDisplay, instance, currentFrame);
            }

            // Draw the desired future trajectory and the sampled version of the past trajectory.
            const TrajectoryQuery& trajectoryQuery = instance->GetTrajectoryQuery();
            const AZ::Color trajectoryQueryColor = AZ::Color::CreateFromRgba(90,219,64,255);
            trajectoryQuery.DebugDraw(debugDisplay, trajectoryQueryColor);

            // Draw the trajectory history starting after the sampled version of the past trajectory.
            const FeatureTrajectory* trajectoryFeature = instance->GetConfig()->GetTrajectoryFeature();
            const TrajectoryHistory& trajectoryHistory = instance->GetTrajectoryHistory();
            trajectoryHistory.DebugDraw(debugDisplay, trajectoryQueryColor, trajectoryFeature->GetPastTimeRange());
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
    } // namespace MotionMatching
} // namespace EMotionFX

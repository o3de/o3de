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

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>

#include <FeatureDatabase.h>
#include <FrameDatabase.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class ActorInstance;

    namespace MotionMatching
    {
        class MotionMatchingInstance;
        class FeatureTrajectory;

        class EMFX_API MotionMatchingConfig
        {
        public:
            AZ_RTTI(MotionMatchingConfig, "{7BC3DFF5-8864-4518-B6F0-0553ADFAB5C1}")
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API InitSettings
            {
                ActorInstance* m_actorInstance = nullptr;
                AZStd::vector<Motion*> m_motionList;
                FrameDatabase::FrameImportSettings m_frameImportSettings;
                size_t m_maxKdTreeDepth = 20;
                size_t m_minFramesPerKdTreeNode = 1000;
                bool m_importMirrored = false;

            };

            MotionMatchingConfig() = default;
            virtual ~MotionMatchingConfig() = default;

            virtual bool RegisterFeatures(const InitSettings& settings) = 0;
            virtual bool Init(const InitSettings& settings);
            virtual void DebugDraw([[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay,
                [[maybe_unused]] MotionMatchingInstance* instance) {}
            virtual FeatureTrajectory* GetTrajectoryFeature() const = 0;

            virtual size_t FindLowestCostFrameIndex(MotionMatchingInstance* instance, const Pose& currentPose, size_t currentFrameIndex) = 0;

            static void Reflect(AZ::ReflectContext* context);

            const FrameDatabase& GetFrameDatabase() const { return m_frameDatabase; }
            FrameDatabase& GetFrameDatabase() { return m_frameDatabase; }

            const FeatureDatabase& GetFeatures() const { return m_features; }
            FeatureDatabase& GetFeatures() { return m_features; }

        protected:
            FrameDatabase m_frameDatabase; /**< The frames and their data. */
            FeatureDatabase m_features;
            float m_newMotionTime = 0.0f; /**< New motion instance time before sync. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX

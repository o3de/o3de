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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>
#include <MotionMatchingConfig.h>
#include <TrajectoryHistory.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        class FeaturePosition;
        class FeatureVelocity;
        class FeatureTrajectory;

        class EMFX_API LocomotionConfig
            : public MotionMatchingConfig
        {
        public:
            AZ_RTTI(LocomotionConfig, "{ACCA8610-5F87-49D7-8064-17DA281F8CD0}", MotionMatchingConfig)
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API FactorWeights
            {
                float m_footPositionFactor = 1.0f;
                float m_footVelocityFactor = 1.0f;
                float m_rootFutureFactor = 1.0f;
                float m_rootPastFactor = 1.0f;
                float m_differentMotionFactor = 1.0f;
            };

            LocomotionConfig();
            ~LocomotionConfig() override = default;

            bool RegisterFeatures(const InitSettings& settings) override;

            size_t FindLowestCostFrameIndex(MotionMatchingInstance* instance, const Feature::FrameCostContext& context, size_t currentFrameIndex) override;

            AZ_INLINE FactorWeights& GetFactorWeights() { return m_factorWeights; }
            AZ_INLINE const FactorWeights& GetFactorWeights() const { return m_factorWeights; }
            AZ_INLINE void SetFactorWeights(const FactorWeights& factors) { m_factorWeights = factors; }

            FeatureTrajectory* GetTrajectoryFeature() const override { return m_rootTrajectoryData; }

        private:
            FeaturePosition* m_leftFootPositionData = nullptr;
            FeaturePosition* m_rightFootPositionData = nullptr;
            FeatureVelocity* m_leftFootVelocityData = nullptr;
            FeatureVelocity* m_rightFootVelocityData = nullptr;
            FeatureVelocity* m_pelvisVelocityData = nullptr;
            FeatureTrajectory* m_rootTrajectoryData = nullptr;
            size_t m_rootNodeIndex = InvalidIndex32;
            size_t m_leftFootNodeIndex = InvalidIndex32;
            size_t m_rightFootNodeIndex = InvalidIndex32;
            size_t m_pelvisNodeIndex = InvalidIndex32;
            FactorWeights m_factorWeights;
        };
    } // namespace MotionMatching
} // namespace EMotionFX

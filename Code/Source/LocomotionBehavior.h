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
#include <EMotionFX/Source/DebugDraw.h>
#include <Behavior.h>
#include <TrajectoryHistory.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        class FeaturePosition;
        class FeatureVelocity;
        class FeatureTrajectory;
        class FeatureDirection;

        enum EControlSplineMode : AZ::u8
        {
            MODE_TARGETDRIVEN = 0,
            MODE_ONE = 1,
            MODE_TWO = 2,
            MODE_THREE = 3,
            MODE_FOUR = 4
        };

        class EMFX_API LocomotionBehavior
            : public Behavior
        {
        public:
            AZ_RTTI(LocomotionBehavior, "{ACCA8610-5F87-49D7-8064-17DA281F8CD0}", Behavior)
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API FactorWeights
            {
                float m_footPositionFactor = 1.0f;
                float m_footVelocityFactor = 1.0f;
                float m_rootFutureFactor = 1.0f;
                float m_rootPastFactor = 1.0f;
                float m_differentMotionFactor = 1.0f;
                float m_rootDirectionFactor = 1.0f;
            };

            LocomotionBehavior();
            ~LocomotionBehavior() override = default;

            bool RegisterParameters(const InitSettings& settings) override;
            bool RegisterFrameDatas(const InitSettings& settings) override;

            void DebugDraw(AZ::RPI::AuxGeomDrawPtr& drawQueue,
                EMotionFX::DebugDraw::ActorInstanceData& draw,
                BehaviorInstance* behaviorInstance) override;

            void DebugDrawControlSpline(AZ::RPI::AuxGeomDrawPtr& drawQueue,
                EMotionFX::DebugDraw::ActorInstanceData& draw,
                BehaviorInstance* behaviorInstance);

            size_t FindLowestCostFrameIndex(BehaviorInstance* behaviorInstance, const Pose& inputPose, const Pose& previousPose, size_t currentFrameIndex, float timeDelta) override;
            void BuildControlSpline(BehaviorInstance* behaviorInstance, EControlSplineMode mode, const AZ::Vector3& targetPos, const TrajectoryHistory& trajectoryHistory, float timeDelta, float pathRadius, float pathSpeed);

            static void Reflect(AZ::ReflectContext* context);

            AZ_INLINE FactorWeights& GetFactorWeights() { return m_factorWeights; }
            AZ_INLINE const FactorWeights& GetFactorWeights() const { return m_factorWeights; }
            AZ_INLINE void SetFactorWeights(const FactorWeights& factors) { m_factorWeights = factors; }

        private:
            void OnSettingsChanged();

            FeaturePosition* m_leftFootPositionData = nullptr;
            FeaturePosition* m_rightFootPositionData = nullptr;
            FeatureVelocity* m_leftFootVelocityData = nullptr;
            FeatureVelocity* m_rightFootVelocityData = nullptr;
            //FeatureDirection* m_rootDirectionData = nullptr;
            FeatureTrajectory* m_rootTrajectoryData = nullptr;
            size_t m_rootNodeIndex = InvalidIndex32;
            size_t m_leftFootNodeIndex = InvalidIndex32;
            size_t m_rightFootNodeIndex = InvalidIndex32;
            FactorWeights m_factorWeights;
        };
    } // namespace MotionMatching
} // namespace EMotionFX

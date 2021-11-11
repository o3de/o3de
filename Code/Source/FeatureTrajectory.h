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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Transform.h>
#include <BehaviorInstance.h>
#include <FeatureTrajectory.h>
#include <Feature.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Pose;
    class Motion;
    class MotionInstance;

    namespace MotionMatching
    {
        class FrameDatabase;

        class EMFX_API FeatureTrajectory
            : public Feature
        {
        public:
            AZ_RTTI(FeatureTrajectory, "{0451E95B-A452-439A-81ED-3962A06A3992}", Feature)
            AZ_CLASS_ALLOCATOR_DECL

            enum class Axis
            {
                X = 0,
                Y = 1,
                Z = 2
            };

            struct EMFX_API Sample
            {
                AZ::Vector2 m_position;
                AZ::Vector2 m_facingDirection;

                static constexpr size_t s_componentsPerSample = 4;
            };

            FeatureTrajectory();
            ~FeatureTrajectory() override = default;

            bool Init(const InitSettings& settings) override;
            void ExtractFeatureValues(const ExtractFrameContext& context) override;
            void DebugDraw(AZ::RPI::AuxGeomDrawPtr& drawQueue,
                EMotionFX::DebugDraw::ActorInstanceData& draw,
                BehaviorInstance* behaviorInstance,
                size_t frameIndex) override;

            struct EMFX_API FrameCostContext
            {
                FrameCostContext(const FeatureMatrix& featureMatrix)
                    : m_featureMatrix(featureMatrix)
                {
                }

                const FeatureMatrix& m_featureMatrix;
                const Pose* m_pose;
                const TrajectoryQuery* m_trajectoryQuery;
                AZ::Vector3 m_facingDirectionRelative = AZ::Vector3(0.0f, 1.0f, 0.0f);
            };
            float CalculateFutureFrameCost(size_t frameIndex, const FrameCostContext& context) const;
            float CalculatePastFrameCost(size_t frameIndex, const FrameCostContext& context) const;
            //float CalculateDirectionCost(size_t frameIndex, const FrameCostContext& context) const;

            void SetNumPastSamplesPerFrame(size_t numHistorySamples);
            void SetNumFutureSamplesPerFrame(size_t numFutureSamples);
            void SetPastTimeRange(float timeInSeconds);
            void SetFutureTimeRange(float timeInSeconds);
            void SetFacingAxis(const Axis axis);

            size_t GetNumFutureSamples() const { return m_numFutureSamples; }
            size_t GetNumPastSamples() const { return m_numPastSamples; }

            float GetFutureTimeRange() const { return m_futureTimeRange; }
            float GetPastTimeRange() const { return m_pastTimeRange; }

            AZ::Vector3 CalculateFacingDirectionWorldSpace(const Pose& pose, Axis facingAxis, size_t jointIndex) const;
            //float CalculateFacingAngle(const Transform& invBaseTransform, const Pose& pose, const AZ::Vector3& velocityDirection) const;

            void SetNodeIndex(size_t nodeIndex);

            static void Reflect(AZ::ReflectContext* context);

            size_t GetNumDimensions() const override;
            AZStd::string GetDimensionName(size_t index, Skeleton* skeleton) const override;

        private:
            size_t CalcMidFrameDataIndex() const;
            size_t CalcPastFrameDataIndex(size_t historyFrameIndex) const;
            size_t CalcFutureFrameDataIndex(size_t futureFrameIndex) const;
            size_t CalcNumSamplesPerFrame() const;

            using SplineToFeatureMatrixIndex = AZStd::function<size_t(size_t)>;
            float CalculateCost(const FeatureMatrix& featureMatrix,
                size_t frameIndex,
                const Transform& invRootTransform,
                const AZStd::vector<TrajectoryQuery::ControlPoint>& controlPoints,
                const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex) const;

            Sample GetSampleFromPose(const Pose& pose, const Transform& invRootTransform) const;

            Sample GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex) const;
            void SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex, const Sample& sample);

            void DebugDrawTrajectory(AZ::RPI::AuxGeomDrawPtr& drawQueue,
                EMotionFX::DebugDraw::ActorInstanceData& draw,
                BehaviorInstance* behaviorInstance,
                size_t frameIndex,
                const Transform& transform,
                const AZ::Color& color,
                size_t numSamples,
                const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex) const;

            size_t m_nodeIndex = InvalidIndex32; /**< The node to grab the data from. */
            size_t m_numFutureSamples = 5; /**< How many samples do we store per frame, for the future trajectory of this frame? */
            size_t m_numPastSamples = 5; /**< How many samples do we store per frame, for the past (history) of the trajectory of this frame? */
            float m_futureTimeRange = 1.0f; /**< How many seconds do we look into the future? */
            float m_pastTimeRange = 1.0f; /**< How many seconds do we look back in the past? */
            Axis m_facingAxis = Axis::Y; /** Which of this node's axes points forward? */
        };
    } // namespace MotionMatching
} // namespace EMotionFX

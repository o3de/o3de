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
                X_NEGATIVE = 2,
                Y_NEGATIVE = 3,
            };

            struct EMFX_API Sample
            {
                AZ::Vector2 m_position; //! Position in the space relative to the extracted frame.
                AZ::Vector2 m_facingDirection; //! Facing direction in the space relative to the extracted frame.

                static constexpr size_t s_componentsPerSample = 4;
            };

            FeatureTrajectory();
            ~FeatureTrajectory() override = default;

            bool Init(const InitSettings& settings) override;
            void ExtractFeatureValues(const ExtractFrameContext& context) override;
            void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
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
            };
            float CalculateFutureFrameCost(size_t frameIndex, const FrameCostContext& context) const;
            float CalculatePastFrameCost(size_t frameIndex, const FrameCostContext& context) const;

            void SetNumPastSamplesPerFrame(size_t numHistorySamples);
            void SetNumFutureSamplesPerFrame(size_t numFutureSamples);
            void SetPastTimeRange(float timeInSeconds);
            void SetFutureTimeRange(float timeInSeconds);
            void SetFacingAxis(const Axis axis);

            size_t GetNumFutureSamples() const { return m_numFutureSamples; }
            size_t GetNumPastSamples() const { return m_numPastSamples; }

            float GetFutureTimeRange() const { return m_futureTimeRange; }
            float GetPastTimeRange() const { return m_pastTimeRange; }

            AZ::Vector2 CalculateFacingDirection(const Pose& pose, const Transform& invRootTransform) const;
            AZ::Vector3 GetFacingAxisDir() const { return m_facingAxisDir; }

            void SetNodeIndex(size_t nodeIndex);

            static void Reflect(AZ::ReflectContext* context);

            size_t GetNumDimensions() const override;
            AZStd::string GetDimensionName(size_t index, Skeleton* skeleton) const override;

            // Shared helper function to draw a facing direction.
            static void DebugDrawFacingDirection(AzFramework::DebugDisplayRequests& debugDisplay,
                const AZ::Vector3& positionWorldSpace,
                const AZ::Vector3& facingDirectionWorldSpace);

        private:
            size_t CalcMidFrameIndex() const;
            size_t CalcPastFrameIndex(size_t historyFrameIndex) const;
            size_t CalcFutureFrameIndex(size_t futureFrameIndex) const;
            size_t CalcNumSamplesPerFrame() const;

            using SplineToFeatureMatrixIndex = AZStd::function<size_t(size_t)>;
            float CalculateCost(const FeatureMatrix& featureMatrix,
                size_t frameIndex,
                const Transform& invRootTransform,
                const AZStd::vector<TrajectoryQuery::ControlPoint>& controlPoints,
                const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex) const;

            //! Called for every sample in the past or future range to extract its information.
            //! @param[in] pose The sampled pose within the trajectory range [m_pastTimeRange, m_futureTimeRange].
            //! @param[in] invRootTransform The inverse of the world space transform of the joint at frame time that the feature is extracted for.
            Sample GetSampleFromPose(const Pose& pose, const Transform& invRootTransform) const;

            Sample GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex) const;
            void SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, size_t sampleIndex, const Sample& sample);

            void DebugDrawTrajectory(AzFramework::DebugDisplayRequests& debugDisplay,
                BehaviorInstance* behaviorInstance,
                size_t frameIndex,
                const Transform& transform,
                const AZ::Color& color,
                size_t numSamples,
                const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex) const;

            void DebugDrawFacingDirection(AzFramework::DebugDisplayRequests& debugDisplay,
                const Transform& worldSpaceTransform,
                const Sample& sample,
                const AZ::Vector3& samplePosWorldSpace) const;

            size_t m_nodeIndex = InvalidIndex32; /**< The node to grab the data from. */
            size_t m_numFutureSamples = 5; /**< How many samples do we store per frame, for the future trajectory of this frame? */
            size_t m_numPastSamples = 5; /**< How many samples do we store per frame, for the past (history) of the trajectory of this frame? */
            float m_futureTimeRange = 1.0f; /**< How many seconds do we look into the future? */
            float m_pastTimeRange = 1.0f; /**< How many seconds do we look back in the past? */

            Axis m_facingAxis = Axis::Y; /** Which of this node's axes points forward? */
            AZ::Vector3 m_facingAxisDir = AZ::Vector3::CreateAxisY();
        };
    } // namespace MotionMatching
} // namespace EMotionFX

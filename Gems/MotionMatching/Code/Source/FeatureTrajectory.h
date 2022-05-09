/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <MotionMatchingInstance.h>
#include <FeatureTrajectory.h>
#include <Feature.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX::MotionMatching
{
    class FrameDatabase;

    //! Matches the root joint past and future trajectory.
    //! For each frame in the motion database, the position and facing direction relative to the current frame of the joint will be evaluated for a past and future time window.
    //! The past and future samples together form the trajectory of the current frame within the time window. This basically describes where the character came from to reach the
    //! current frame and where it will go when continuing to play the animation.
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

        FeatureTrajectory() = default;
        ~FeatureTrajectory() override = default;

        bool Init(const InitSettings& settings) override;
        void ExtractFeatureValues(const ExtractFeatureContext& context) override;
        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
            const Pose& currentPose,
            const FeatureMatrix& featureMatrix,
            size_t frameIndex) override;

        float CalculateFutureFrameCost(size_t frameIndex, const FrameCostContext& context) const;
        float CalculatePastFrameCost(size_t frameIndex, const FrameCostContext& context) const;

        void SetNumPastSamplesPerFrame(size_t numHistorySamples);
        void SetNumFutureSamplesPerFrame(size_t numFutureSamples);
        void SetPastTimeRange(float timeInSeconds);
        void SetFutureTimeRange(float timeInSeconds);
        void SetFacingAxis(const Axis axis);
        void UpdateFacingAxis();

        float GetPastTimeRange() const { return m_pastTimeRange; }
        size_t GetNumPastSamples() const { return m_numPastSamples; }
        float GetPastCostFactor() const { return m_pastCostFactor; }

        float GetFutureTimeRange() const { return m_futureTimeRange; }
        size_t GetNumFutureSamples() const { return m_numFutureSamples; }
        float GetFutureCostFactor() const { return m_futureCostFactor; }

        AZ::Vector2 CalculateFacingDirection(const Pose& pose, const Transform& invRootTransform) const;
        AZ::Vector3 GetFacingAxisDir() const { return m_facingAxisDir; }

        static void Reflect(AZ::ReflectContext* context);

        size_t GetNumDimensions() const override;
        AZStd::string GetDimensionName(size_t index) const override;

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
            const FeatureMatrix& featureMatrix,
            size_t frameIndex,
            const Transform& transform,
            const AZ::Color& color,
            size_t numSamples,
            const SplineToFeatureMatrixIndex& splineToFeatureMatrixIndex) const;

        void DebugDrawFacingDirection(AzFramework::DebugDisplayRequests& debugDisplay,
            const Transform& worldSpaceTransform,
            const Sample& sample,
            const AZ::Vector3& samplePosWorldSpace) const;

        AZ::Crc32 GetCostFactorVisibility() const override;

        float m_pastTimeRange = 0.7f; //< The time window the samples are distributed along for the past trajectory.
        size_t m_numPastSamples = 4; //< The number of samples stored per frame for the past (history) trajectory.
        float m_pastCostFactor = 0.5f; //< Normalized value to weight or scale the future trajectory cost.

        float m_futureTimeRange = 1.2f; //< The time window the samples are distributed along for the future trajectory.
        size_t m_numFutureSamples = 6; //< The number of samples stored per frame for the future trajectory.
        float m_futureCostFactor = 0.75f; //< Normalized value to weight or scale the future trajectory cost.

        Axis m_facingAxis = Axis::Y; //< Which axis of the joint transform is facing forward?
        AZ::Vector3 m_facingAxisDir = AZ::Vector3::CreateAxisY();
    };
} // namespace EMotionFX::MotionMatching

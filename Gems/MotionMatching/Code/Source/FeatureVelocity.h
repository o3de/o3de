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
#include <EMotionFX/Source/Velocity.h>
#include <Feature.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX::MotionMatching
{
    class FrameDatabase;

    class EMFX_API FeatureVelocity
        : public Feature
    {
    public:
        AZ_RTTI(FeatureVelocity, "{DEEA4F0F-CE70-4F16-9136-C2BFDDA29336}", Feature)
        AZ_CLASS_ALLOCATOR_DECL

        FeatureVelocity() = default;
        ~FeatureVelocity() override = default;

        void ExtractFeatureValues(const ExtractFeatureContext& context) override;

        static void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
            const Pose& pose,
            const AZ::Vector3& velocity, // in relative-to-joint space
            size_t jointIndex,
            size_t relativeToJointIndex,
            const AZ::Color& color);

        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
            const Pose& currentPose,
            const FeatureMatrix& featureMatrix,
            size_t frameIndex) override;

        float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const override;

        void FillQueryFeatureValues(size_t startIndex, AZStd::vector<float>& queryFeatureValues, const FrameCostContext& context) override;

        static void Reflect(AZ::ReflectContext* context);

        size_t GetNumDimensions() const override;
        AZStd::string GetDimensionName(size_t index) const override;
        AZ::Vector3 GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex) const;
        void SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, const AZ::Vector3& velocity);
    };
} // namespace EMotionFX::MotionMatching

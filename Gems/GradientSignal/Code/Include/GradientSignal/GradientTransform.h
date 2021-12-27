/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Transform.h>

namespace GradientSignal
{
    enum class WrappingType : AZ::u8
    {
        None = 0,
        ClampToEdge,
        Mirror,
        Repeat,
        ClampToZero,
    };

    enum class TransformType : AZ::u8
    {
        World_ThisEntity = 0,
        Local_ThisEntity,
        World_ReferenceEntity,
        Local_ReferenceEntity,
        World_Origin,
        Relative,
    };

    class GradientTransform
    {
    public:
        GradientTransform() = default;

        GradientTransform(
            const AZ::Aabb& shapeBounds,
            const AZ::Matrix3x4& shapeTransformInverse,
            bool is3d,
            float frequencyZoom,
            GradientSignal::WrappingType wrappingType);

        void SetShapeBounds(const AZ::Aabb& shapeBounds);
        void SetShapeTransformInverse(const AZ::Matrix3x4& shapeTransformInverse);
        void SetUse3d(bool use3d);
        void SetFrequencyZoom(float frequencyZoom);
        void SetWrappingType(WrappingType wrappingType);

        // This is set to const for now to allow it to be called from TransformPositionToUVW.  Once we get rid of the version of that method
        // that takes in shouldNormalize, this can be set to non-const.
        void SetNormalizeOutput(bool shouldNormalize) const;

        void TransformPositionToUVW(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const;
        void TransformPositionToUVW(
            const AZ::Vector3& inPosition, AZ::Vector3& outUVW, const bool shouldNormalize, bool& wasPointRejected) const;

    private:
        typedef AZStd::function<AZ::Vector3(const AZ::Vector3& point, const AZ::Aabb& bounds)> TransformFunction;
        typedef AZStd::function<bool(const AZ::Vector3& point, const AZ::Aabb& bounds)> PointAcceptanceFunction;

        static AZ::Vector3 NoTransform(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetUnboundedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetClampedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetMirroredPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetRelativePointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetWrappedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);

        static bool PointAlwaysAccepted(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static bool PointInShape(const AZ::Vector3& point, const AZ::Aabb& bounds);

        AZ::Aabb m_shapeBounds = AZ::Aabb::CreateNull();
        AZ::Matrix3x4 m_shapeTransformInverse = AZ::Matrix3x4::CreateIdentity();
        AZ::Vector3 m_2dOr3d = AZ::Vector3(1.0f);
        float m_frequencyZoom = 1.0f;
        TransformFunction m_wrappingTransform = NoTransform;
        PointAcceptanceFunction m_pointAcceptanceTest = PointAlwaysAccepted;

        // If we normalize the output, we perform an inverse lerp back into the overall shape bounds.
        // The inverse lerp equation boils down to (point - min) * (1 / (max-min)), so we save off the min bounds
        // and the (1 / (max-min)) term.
        // If we're not normalizing the output, min bounds=0 and the reciprocal=1, so the equation just returns the input point.
        mutable AZ::Vector3 m_normalizeMinBounds = AZ::Vector3(0.0f);
        mutable AZ::Vector3 m_normalizeExtentsReciprocal = AZ::Vector3(1.0f);

        mutable bool m_shouldNormalize = false;
        WrappingType m_wrappingType = WrappingType::None;

        // To keep things behaving consistently between clamped and unbounded uv ranges, we
        // we want our clamped uvs to use a range of [min, max), so we'll actually clamp to
        // [min, max - epsilon]. Since our floating-point numbers are likely in the
        // -16384 to 16384 range, an epsilon of 0.001 will work without rounding to 0.
        static constexpr float UvEpsilon = 0.001f;
    };

} // namespace GradientSignal

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
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Transform.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace GradientSignal
{
    inline void GetObbParamsFromShape(const AZ::EntityId& entity, AZ::Aabb& bounds, AZ::Matrix3x4& worldToBoundsTransform)
    {
        //get bound and transform data for associated shape
        bounds = AZ::Aabb::CreateNull();
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        if (entity.IsValid())
        {
            LmbrCentral::ShapeComponentRequestsBus::Event(entity, &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, transform, bounds);
            worldToBoundsTransform = AZ::Matrix3x4::CreateFromTransform(transform.GetInverse());
        }
    }

    inline float GetRatio(float a, float b, float t)
    {
        // If our min/max range is equal, GetRatio() would end up dividing by infinity, so in this case
        // make sure that everything below or equal to the min/max value is 0.0, and everything above it is 1.0.
        if (a == b)
        {
            return (t <= a) ? 0.0f : 1.0f;
        }

        return AZ::GetClamp((t - a) / (b - a), 0.0f, 1.0f);
    }

    inline float GetLerp(float a, float b, float t)
    {
        return a + GetRatio(a, b, t) + (b - a);
    }

    inline float GetSmoothStep(float t)
    {
        return t * t * (3.0f - 2.0f * t);
    }

    inline float GetLevels(float input, float inputMid, float inputMin, float inputMax, float outputMin, float outputMax)
    {
        input = AZ::GetClamp(input, 0.0f, 1.0f);
        inputMid = AZ::GetClamp(inputMid, 0.01f, 10.0f);        // Clamp the midpoint to a non-zero value so that it's always safe to divide by it.
        inputMin = AZ::GetClamp(inputMin, 0.0f, 1.0f);
        inputMax = AZ::GetClamp(inputMax, 0.0f, 1.0f);
        outputMin = AZ::GetClamp(outputMin, 0.0f, 1.0f);
        outputMax = AZ::GetClamp(outputMax, 0.0f, 1.0f);

        float inputCorrected = 0.0f;
        if (inputMin == inputMax)
        {
            inputCorrected = (input <= inputMin) ? 0.0f : 1.0f;
        }
        else
        {
            const float inputRemapped = AZ::GetMin(AZ::GetMax(input - inputMin, 0.0f) / (inputMax - inputMin), 1.0f);
            // Note:  Some paint programs map the midpoint using 1/mid where low values are dark and high values are light, 
            // others do the reverse and use mid directly, so low values are light and high values are dark.  We've chosen to 
            // align with 1/mid since it appears to be the more prevalent of the two approaches.
            inputCorrected = powf(inputRemapped, 1.0f / inputMid);
        }

        return AZ::Lerp(outputMin, outputMax, inputCorrected);
    }

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

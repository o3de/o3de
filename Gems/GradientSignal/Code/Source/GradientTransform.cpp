/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Math/MathUtils.h>
#include <GradientSignal/Util.h>


namespace GradientSignal
{
    GradientTransform::GradientTransform(
        const AZ::Aabb& shapeBounds,
        const AZ::Matrix3x4& shapeTransformInverse,
        bool is3d,
        float frequencyZoom,
        GradientSignal::WrappingType wrappingType)
    {
        SetShapeBounds(shapeBounds);
        SetShapeTransformInverse(shapeTransformInverse);
        SetUse3d(is3d);
        SetFrequencyZoom(frequencyZoom);
        SetNormalizeOutput(false);
        SetWrappingType(wrappingType);
    }

    void GradientTransform::SetShapeBounds(const AZ::Aabb& shapeBounds)
    {
        m_shapeBounds = shapeBounds;

        // Refresh the values for anything that depends on shapeBounds.
        SetWrappingType(m_wrappingType);
        SetNormalizeOutput(m_shouldNormalize);
    }

    void GradientTransform::SetShapeTransformInverse(const AZ::Matrix3x4& shapeTransformInverse)
    {
        m_shapeTransformInverse = shapeTransformInverse;
    }

    void GradientTransform::SetUse3d(bool use3d)
    {
        // This is used to either clear or preserve the Z component of the passed-in position.  For 2D gradients, we always treat Z as 0.
        // For 3D gradients, we use the Z as a part of the gradient lookup.
        m_2dOr3d = use3d ? AZ::Vector3(1.0f, 1.0f, 1.0f) : AZ::Vector3(1.0f, 1.0f, 0.0f);
    }

    void GradientTransform::SetFrequencyZoom(float frequencyZoom)
    {
        m_frequencyZoom = frequencyZoom;
    }

    void GradientTransform::SetWrappingType(WrappingType wrappingType)
    {
        m_wrappingType = wrappingType;
        m_wrappingTransform = NoTransform;
        m_pointAcceptanceTest = PointAlwaysAccepted;

        if (m_shapeBounds.IsValid())
        {
            // all wrap types and transformations are applied after the coordinate is transformed into shape relative space
            // this allows all calculations to be simplified and done using the shapes untransformed aabb
            // outputting a value that can be used to sample a gradient in its local space
            switch (wrappingType)
            {
            default:
            case WrappingType::None:
                m_wrappingTransform = GetUnboundedPointInAabb;
                break;
            case WrappingType::ClampToEdge:
                m_wrappingTransform = GetClampedPointInAabb;
                break;
            case WrappingType::ClampToZero:
                m_pointAcceptanceTest = PointInShape;
                m_wrappingTransform = GetClampedPointInAabb;
                break;
            case WrappingType::Mirror:
                m_wrappingTransform = GetMirroredPointInAabb;
                break;
            case WrappingType::Repeat:
                m_wrappingTransform = GetWrappedPointInAabb;
                break;
            }
        }
    }

    void GradientTransform::SetNormalizeOutput(bool shouldNormalize) const
    {
        m_shouldNormalize = shouldNormalize;

        if (shouldNormalize)
        {
            m_normalizeMinBounds = m_shapeBounds.GetMin();
            m_normalizeExtentsReciprocal = AZ::Vector3(
                AZ::IsClose(0.0f, m_shapeBounds.GetXExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetXExtent()),
                AZ::IsClose(0.0f, m_shapeBounds.GetYExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetYExtent()),
                AZ::IsClose(0.0f, m_shapeBounds.GetZExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetZExtent()));
        }
        else
        {
            m_normalizeMinBounds = AZ::Vector3(0.0f);
            m_normalizeExtentsReciprocal = AZ::Vector3(1.0f);
        }
    }

    void GradientTransform::TransformPositionToUVW(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const
    {
        // Transform coordinate into "local" relative space of shape bounds, and set Z to 0 if this is a 2D gradient.
        outUVW = m_shapeTransformInverse * inPosition * m_2dOr3d;
        wasPointRejected = !m_pointAcceptanceTest(outUVW, m_shapeBounds);
        outUVW = m_wrappingTransform(outUVW, m_shapeBounds);
        outUVW *= m_frequencyZoom;

        // This effectively does AZ::LerpInverse(bounds.GetMin(), bounds.GetMax(), point) if shouldNormalize is true,
        // and just returns outUVW if shouldNormalize is false.
        outUVW = m_normalizeExtentsReciprocal * (outUVW - m_normalizeMinBounds);
    }

    void GradientTransform::TransformPositionToUVW(
        const AZ::Vector3& inPosition, AZ::Vector3& outUVW, const bool shouldNormalize, bool& wasPointRejected) const
    {
        SetNormalizeOutput(shouldNormalize);
        TransformPositionToUVW(inPosition, outUVW, wasPointRejected);
    }

    AZ::Vector3 GradientTransform::NoTransform(const AZ::Vector3& point, const AZ::Aabb& /*bounds*/)
    {
        return point;
    }

    AZ::Vector3 GradientTransform::GetUnboundedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& /*bounds*/)
    {
        return point;
    }

    AZ::Vector3 GradientTransform::GetClampedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        // We want the clamped sampling states to clamp uvs to the [min, max) range.
        return point.GetClamp(bounds.GetMin(), bounds.GetMax() - AZ::Vector3(UvEpsilon));
    }

    AZ::Vector3 GradientTransform::GetWrappedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        return AZ::Vector3(
            AZ::Wrap(point.GetX(), bounds.GetMin().GetX(), bounds.GetMax().GetX()),
            AZ::Wrap(point.GetY(), bounds.GetMin().GetY(), bounds.GetMax().GetY()),
            AZ::Wrap(point.GetZ(), bounds.GetMin().GetZ(), bounds.GetMax().GetZ()));
    }

    AZ::Vector3 GradientTransform::GetMirroredPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        auto GetMirror = [](float value, float min, float max) -> float
        {
            float relativeValue = value - min;
            float range = max - min;
            float rangeX2 = range * 2.0f;
            if (relativeValue < 0.0)
            {
                relativeValue = rangeX2 - fmod(-relativeValue, rangeX2);
            }
            else
            {
                relativeValue = fmod(relativeValue, rangeX2);
            }
            if (relativeValue >= range)
            {
                // Since we want our uv range to stay in the [min, max) range,
                // it means that for mirroring, we want both the "forward" values
                // and the "mirrored" values to be in [0, range).  We don't want
                // relativeValue == range, so we shift relativeValue by a small epsilon
                // in the mirrored case.
                relativeValue = rangeX2 - (relativeValue + UvEpsilon);
            }

            return relativeValue + min;
        };

        return AZ::Vector3(
            GetMirror(point.GetX(), bounds.GetMin().GetX(), bounds.GetMax().GetX()),
            GetMirror(point.GetY(), bounds.GetMin().GetY(), bounds.GetMax().GetY()),
            GetMirror(point.GetZ(), bounds.GetMin().GetZ(), bounds.GetMax().GetZ()));
    }

    AZ::Vector3 GradientTransform::GetRelativePointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        return point - bounds.GetMin();
    }

    bool GradientTransform::PointAlwaysAccepted([[maybe_unused]] const AZ::Vector3& point, [[maybe_unused]] const AZ::Aabb& bounds)
    {
        return true;
    }

    bool GradientTransform::PointInShape(const AZ::Vector3& point, const AZ::Aabb& bounds)
    {
        // We don't want to use m_shapeBounds.Contains() here because Contains() is inclusive on all edges.
        // For uv consistency between clamped and unclamped states, we only want to accept uv ranges of [min, max),
        // so we specifically need to exclude the max edges here.
        return point.IsGreaterEqualThan(bounds.GetMin()) && point.IsLessThan(bounds.GetMax());
    }
}

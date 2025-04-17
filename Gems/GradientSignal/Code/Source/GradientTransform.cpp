/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Math/MathUtils.h>
#include <GradientSignal/GradientTransform.h>


namespace GradientSignal
{
    GradientTransform::GradientTransform(
        const AZ::Aabb& shapeBounds, const AZ::Matrix3x4& transform, bool use3d,
        float frequencyZoom, GradientSignal::WrappingType wrappingType)
        : m_shapeBounds(shapeBounds)
        , m_transform(transform)
        , m_inverseTransform(transform.GetInverseFull())
        , m_frequencyZoom(frequencyZoom)
        , m_wrappingType(wrappingType)
        , m_alwaysAcceptPoint(true)
    {
        // If we want this to be a 2D gradient lookup, we always want to set the W result in the output to 0.
        // The easiest / cheapest way to make this happen is just to clear out the third row in the inverseTransform.
        if (!use3d)
        {
            m_inverseTransform.SetRow(2, AZ::Vector4::CreateZero());
        }

        // If we have invalid shape bounds, reset the wrapping type back to None. Wrapping won't work without valid bounds.
        if (!m_shapeBounds.IsValid())
        {
            m_wrappingType = WrappingType::None;
        }

        // ClampToZero is the only wrapping type that allows us to return a "pointIsRejected" result for points that fall
        // outside the shape bounds.
        if (m_wrappingType == WrappingType::ClampToZero)
        {
            m_alwaysAcceptPoint = false;
        }

        m_normalizeExtentsReciprocal = AZ::Vector3(
            AZ::IsClose(0.0f, m_shapeBounds.GetXExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetXExtent()),
            AZ::IsClose(0.0f, m_shapeBounds.GetYExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetYExtent()),
            AZ::IsClose(0.0f, m_shapeBounds.GetZExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetZExtent()));
    }

    void GradientTransform::TransformLocalPositionToUVW(
        const AZ::Vector3& inLocalPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const
    {
        outUVW = inLocalPosition;

        // For most wrapping types, we always accept the point, but for ClampToZero we only accept it if it's within
        // the shape bounds. We don't use m_shapeBounds.Contains() here because Contains() is inclusive on all edges.
        // For uv consistency between clamped and unclamped states, we only want to accept uv ranges of [min, max),
        // so we specifically need to exclude the max edges here.
        bool wasPointAccepted = m_alwaysAcceptPoint ||
            (outUVW.IsGreaterEqualThan(m_shapeBounds.GetMin()) && outUVW.IsLessThan(m_shapeBounds.GetMax()));
        wasPointRejected = !wasPointAccepted;

        switch (m_wrappingType)
        {
        default:
        case WrappingType::None:
            outUVW = GetUnboundedPointInAabb(outUVW, m_shapeBounds);
            break;
        case WrappingType::ClampToEdge:
            outUVW = GetClampedPointInAabb(outUVW, m_shapeBounds);
            break;
        case WrappingType::ClampToZero:
            outUVW = GetClampedPointInAabb(outUVW, m_shapeBounds);
            break;
        case WrappingType::Mirror:
            outUVW = GetMirroredPointInAabb(outUVW, m_shapeBounds);
            break;
        case WrappingType::Repeat:
            outUVW = GetWrappedPointInAabb(outUVW, m_shapeBounds);
            break;
        }

        outUVW *= m_frequencyZoom;
    }

    void GradientTransform::TransformLocalPositionToUVWNormalized(
        const AZ::Vector3& inLocalPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const
    {
        TransformLocalPositionToUVW(inLocalPosition, outUVW, wasPointRejected);

        // This effectively does AZ::LerpInverse(bounds.GetMin(), bounds.GetMax(), point) if shouldNormalize is true,
        // and just returns outUVW if shouldNormalize is false.
        outUVW = m_normalizeExtentsReciprocal * (outUVW - m_shapeBounds.GetMin());
    }

    void GradientTransform::TransformPositionToUVW(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const
    {
        // Transform coordinate into "local" relative space of shape bounds, and set W to 0 if this is a 2D gradient.
        AZ::Vector3 inLocalPosition = m_inverseTransform * inPosition;

        TransformLocalPositionToUVW(inLocalPosition, outUVW, wasPointRejected);
    }

    void GradientTransform::TransformPositionToUVWNormalized(
        const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const
    {
        // Transform coordinate into "local" relative space of shape bounds, and set W to 0 if this is a 2D gradient.
        AZ::Vector3 inLocalPosition = m_inverseTransform * inPosition;

        TransformLocalPositionToUVWNormalized(inLocalPosition, outUVW, wasPointRejected);
    }

    WrappingType GradientTransform::GetWrappingType() const
    {
        return m_wrappingType;
    }

    AZ::Aabb GradientTransform::GetBounds() const
    {
        return m_shapeBounds;
    }

    AZ::Vector3 GradientTransform::GetScale() const
    {
        return m_transform.RetrieveScale();
    }

    float GradientTransform::GetFrequencyZoom() const
    {
        return m_frequencyZoom;
    }

    AZ::Matrix3x4 GradientTransform::GetTransformMatrix() const
    {
        return m_transform;
    }

    void GradientTransform::GetMinMaxUvwValues(AZ::Vector3& minUvw, AZ::Vector3& maxUvw) const
    {
        // Get the UVW values at the min & max corners.
        bool wasPointRejected;
        TransformLocalPositionToUVW(m_shapeBounds.GetMin(), minUvw, wasPointRejected);
        TransformLocalPositionToUVW(m_shapeBounds.GetMax(), maxUvw, wasPointRejected);
    }

    void GradientTransform::GetMinMaxUvwValuesNormalized(AZ::Vector3& minUvw, AZ::Vector3& maxUvw) const
    {
        // Get the normalized UVW values at the min & max corners.
        bool wasPointRejected;
        TransformLocalPositionToUVWNormalized(m_shapeBounds.GetMin(), minUvw, wasPointRejected);
        TransformLocalPositionToUVWNormalized(m_shapeBounds.GetMax(), maxUvw, wasPointRejected);
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
        /* For mirroring, we want to produce the following pattern:
        *   [min, max) : value
        *   [max, min) : max - value - epsilon
        *   [min, max) : value
        *   [max, min) : max - value - epsilon
        *   ...
        *   The epsilon is because we always want to keep our output values in the [min, max) range. We apply the epsilon to all
        *   the mirrored values so that we get consistent spacing between the values.
        */

        auto GetMirror = [](float value, float min, float max) -> float
        {
            // To calculate the mirror value, we move our value into relative space of [0, rangeX2), then use
            // the first half of the range for our "[min, max)" range, and the second half for our "[max, min)" mirrored range.

            float relativeValue = value - min;
            float range = max - min;
            float rangeX2 = range * 2.0f;

            // A positive relativeValue will produce a value of [0, rangeX2) from a single mod, but a negative relativeValue
            // will produce a value of (-rangeX2, 0]. Adding rangeX2 to the result and taking the mod again puts us back in
            // the range of [0, rangeX2) for both negative and positive values. This keeps our mirroring pattern consistent and
            // unbroken across both negative and positive coordinate space.
            relativeValue = AZ::Mod(AZ::Mod(relativeValue, rangeX2) + rangeX2, rangeX2);

            // [range, rangeX2) is our mirrored range, so flip the value when we're in this range and apply the epsilon so that
            // we never return the max value, and so that our mirrored values have consistent spacing in the results.
            if (relativeValue >= range)
            {
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
}

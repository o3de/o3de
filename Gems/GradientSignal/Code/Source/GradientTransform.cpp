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
        , m_inverseTransform(transform.GetInverseFull())
        , m_clearWFor2dGradients(1.0f, 1.0f, use3d ? 1.0f : 0.0f)
        , m_frequencyZoom(frequencyZoom)
        , m_wrappingTransform(NoTransform)
        , m_alwaysAcceptPoint(true)
    {
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
                m_alwaysAcceptPoint = false;
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

        m_normalizeExtentsReciprocal = AZ::Vector3(
            AZ::IsClose(0.0f, m_shapeBounds.GetXExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetXExtent()),
            AZ::IsClose(0.0f, m_shapeBounds.GetYExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetYExtent()),
            AZ::IsClose(0.0f, m_shapeBounds.GetZExtent()) ? 0.0f : (1.0f / m_shapeBounds.GetZExtent()));
    }

    void GradientTransform::TransformPositionToUVW(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const
    {
        // Transform coordinate into "local" relative space of shape bounds, and set W to 0 if this is a 2D gradient.
        outUVW = m_inverseTransform * inPosition * m_clearWFor2dGradients;

        // For most wrapping types, we always accept the point, but for ClampToZero we only accept it if it's within
        // the shape bounds. We don't use m_shapeBounds.Contains() here because Contains() is inclusive on all edges.
        // For uv consistency between clamped and unclamped states, we only want to accept uv ranges of [min, max),
        // so we specifically need to exclude the max edges here.
        bool wasPointAccepted = m_alwaysAcceptPoint ||
            (outUVW.IsGreaterEqualThan(m_shapeBounds.GetMin()) && outUVW.IsLessThan(m_shapeBounds.GetMax()));
        wasPointRejected = !wasPointAccepted;

        outUVW = m_wrappingTransform(outUVW, m_shapeBounds);
        outUVW *= m_frequencyZoom;
    }

    void GradientTransform::TransformPositionToUVWNormalized(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const
    {
        TransformPositionToUVW(inPosition, outUVW, wasPointRejected);

        // This effectively does AZ::LerpInverse(bounds.GetMin(), bounds.GetMax(), point) if shouldNormalize is true,
        // and just returns outUVW if shouldNormalize is false.
        outUVW = m_normalizeExtentsReciprocal * (outUVW - m_shapeBounds.GetMin());
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
}

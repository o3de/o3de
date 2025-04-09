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
#include <AzCore/std/functional.h>

namespace GradientSignal
{
    //! Controls how the gradient repeats itself when queried outside the bounds of the shape.
    enum class WrappingType : AZ::u8
    {
        None = 0,           //! Unbounded - the gradient ignores the shape bounds.
        ClampToEdge,        //! The values on the edge of the shape will be extended outward in each direction.
        Mirror,             //! The gradient signal will be repeated but mirrored on every repeat.
        Repeat,             //! The gradient signal will be repeated in every direction.
        ClampToZero,        //! The value will always be 0 outside of the shape.
    };

    class GradientTransform
    {
    public:
        GradientTransform() = default;

        /**
         * Create a GradientTransform with the given parameters.
         * GradientTransform is a utility class that converts world space positions to gradient space UVW values which can be used
         * to look up deterministic gradient values for the input spatial locations.
         * \param shapeBounds The bounds of the shape associated with the gradient, in local space.
         * \param transform The transform to use to convert from world space to gradient space.
         * \param use3d True for 3D gradient lookup outputs, false for 2D gradient lookup outputs. (i.e. output W will be nonzero or zero)
         * \param frequencyZoom Amount to scale the UVW results after wrapping is applied.
         * \param wrappingType The way in which the gradient repeats itself outside the shape bounds.
         */
        GradientTransform(
            const AZ::Aabb& shapeBounds,
            const AZ::Matrix3x4& transform,
            bool use3d,
            float frequencyZoom,
            GradientSignal::WrappingType wrappingType);

        /**
         * Checks to see if two GradientTransform instances are equivalent.  
         * Useful for being able to send out notifications when a GradientTransform has changed.
         * \param rhs The second GradientTranform to compare against.
         * \return True if they're equal, False if they aren't.
         */        
        bool operator==(const GradientTransform& rhs) const
        {
            return (
                (m_shapeBounds == rhs.m_shapeBounds) &&
                (m_inverseTransform == rhs.m_inverseTransform) &&
                (m_alwaysAcceptPoint == rhs.m_alwaysAcceptPoint) &&
                (m_frequencyZoom == rhs.m_frequencyZoom) &&
                (m_wrappingType == rhs.m_wrappingType) &&
                (m_normalizeExtentsReciprocal == rhs.m_normalizeExtentsReciprocal));
        }

        /**
         * Checks to see if two GradientTransform instances aren't equivalent.
         * Useful for being able to send out notifications when a GradientTransform has changed.
         * \param rhs The second GradientTranform to compare against.
         * \return True if they're not equal, False if they are.
         */
        bool operator!=(const GradientTransform& rhs) const
        {
            return !(*this == rhs);
        }
 

        /**
         * Transform the given world space position to a gradient space UVW lookup value.
         * \param inPosition The input world space position to transform.
         * \param outUVW [out] The UVW value that can be used to look up a deterministic gradient value.
         * \param wasPointRejected [out] True if the input position doesn't have a gradient value, false if it does.
         *     Most gradients have values mapped to infinite world space, so wasPointRejected will almost always be false.
         *     It will only be true when using ClampToZero and the world space position falls outside the shape bounds.
         */
        void TransformPositionToUVW(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const;

        /**
         * Transform the given world space position to a gradient space UVW lookup value and normalize to the shape bounds.
         * "Normalizing" in this context means that regardless of the world space coordinates, (0,0,0) represents the minimum
         * shape bounds corner, and (1,1,1) represents the maximum shape bounds corner. Depending on the wrapping type, it's possible
         * (and even likely) to get values outside the 0-1 range.
         * \param inPosition The input world space position to transform.
         * \param outUVW [out] The UVW value that can be used to look up a deterministic gradient value.
         * \param wasPointRejected [out] True if the input position doesn't have a gradient value, false if it does.
         *     Most gradients have values mapped to infinite world space, so wasPointRejected will almost always be false.
         *     It will only be true when using ClampToZero and the world space position falls outside the shape bounds.
         */
        void TransformPositionToUVWNormalized(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const;

        /**
         * Epsilon value to allow our UVW range to go to [min, max) by using the range [min, max - epsilon].
         * To keep things behaving consistently between clamped and unbounded uv ranges, we want our clamped uvs to use a
         * range of [min, max), so we'll actually clamp to [min, max - epsilon]. Since our floating-point numbers are likely in the
         * -16384 to 16384 range, an epsilon of 0.001 will work without rounding to 0.
         * (This constant is public so that it can be used from unit tests for validating transformation results)
        */
        static constexpr float UvEpsilon = 0.001f;

        //! Return the WrappingType for this GradientTransform
        WrappingType GetWrappingType() const;

        //! Return the AABB bounds for this GradientTransform
        //! The bounds that are returned are in the local space of the shape, not world space.
        AZ::Aabb GetBounds() const;

        //! Return the scale for this GradientTransform
        AZ::Vector3 GetScale() const;

        //! Return the frequency zoom for this GradientTransform
        float GetFrequencyZoom() const;

        //! Get the transform matrix used by this gradient transform.
        AZ::Matrix3x4 GetTransformMatrix() const;

        //! Get the UVW values at the min and max corners of the shape's local bounds.
        //! @param minUvw [output] The UVW values at the min corner of the local bounds.
        //! @param maxUvw [output] The UVW values at the max corner of the local bounds.
        void GetMinMaxUvwValues(AZ::Vector3& minUvw, AZ::Vector3& maxUvw) const;
        void GetMinMaxUvwValuesNormalized(AZ::Vector3& minUvw, AZ::Vector3& maxUvw) const;

    private:

        void TransformLocalPositionToUVW(const AZ::Vector3& inLocalPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const;
        void TransformLocalPositionToUVWNormalized(const AZ::Vector3& inLocalPosition, AZ::Vector3& outUVW, bool& wasPointRejected) const;

        //! These are the various transformations that will be performed, based on wrapping type.
        static AZ::Vector3 NoTransform(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetUnboundedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetClampedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetMirroredPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetRelativePointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);
        static AZ::Vector3 GetWrappedPointInAabb(const AZ::Vector3& point, const AZ::Aabb& bounds);

        //! The shape bounds are used for determining the wrapping bounds, and to normalize the UVW results into if requested.
        AZ::Aabb m_shapeBounds = AZ::Aabb::CreateNull();

        /**
         * The relative transform to use for converting from world space to gradient space, stored as an inverse transform.
         * We only ever need to use the inverse transform, so we compute it once and store it instead of using the original
         * transform. Note that the GradientTransformComponent has many options for choosing which relative space to use
         * for the transform, so the transform passed in to this class might already have many modifications applied to it.
         * The inverse transform will also get its 3rd row cleared out if "use3d" is false and we're only performing 2D gradient
         * transformations, so that the W component of the UVW output will always be 0.
         */
        AZ::Matrix3x4 m_inverseTransform = AZ::Matrix3x4::CreateIdentity();

        /**
         * The transform that goes from gradient space to world space. The GradientTransform class itself doesn't need this
         * for its computations, since it only needs to go from world space to gradient space with the inverse transform.
         * However, we'll keep it around to be able to provide the transform or parts of it (like scale) to other classes
         * that need deeper knowledge about how things are being transformed.
         */
        AZ::Matrix3x4 m_transform = AZ::Matrix3x4::CreateIdentity();

        /**
         * Whether or not to always accept the input point as a valid output point.
         * Most of the time, the gradient exists everywhere in world space, so we always accept the input point.
         * The one exception is ClampToZero, which will return that the point is rejected if it falls outside the shape bounds.
         */
        bool m_alwaysAcceptPoint = true;

        //! Apply a scale to the point *after* the wrapping is applied.
        float m_frequencyZoom = 1.0f;

        //! How the gradient should repeat itself outside of the shape bounds.
        WrappingType m_wrappingType = WrappingType::None;

        /**
         * Cached reciprocal for performing an inverse lerp back to shape bounds.
         * When normalizing the output UVW back into the shape bounds, we perform an inverse lerp. The inverse lerp
         * equation is (point - min) * (1 / (max-min)), so we save off the (1 / (max-min)) term to avoid recalculating it on every point.
         */
        AZ::Vector3 m_normalizeExtentsReciprocal = AZ::Vector3(1.0f);
    };

} // namespace GradientSignal

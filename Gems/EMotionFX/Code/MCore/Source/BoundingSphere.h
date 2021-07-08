/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "Algorithms.h"
#include "AzCore/Math/Vector3.h"

namespace MCore
{
    // forward declarations
    class AABB;


    /**
     * A 3D bounding sphere class.
     * This template represents a 3D bounding sphere, which has a center point and a radius.
     * This type of bounding volume can be used to speedup collision detection or (ray) intersection tests or visibility tests.
     */
    class MCORE_API BoundingSphere
    {
    public:
        /**
         * Default constructor.
         * Sets the sphere center to (0,0,0) and makes the radius 0.
         */
        MCORE_INLINE BoundingSphere()
            : mCenter(AZ::Vector3::CreateZero())
            , mRadius(0.0f)
            , mRadiusSq(0.0f) {}

        /**
         * Constructor which sets the center of the sphere and it's radius.
         * Automatically calculates the squared radius too.
         * @param pos The center position of the sphere.
         * @param rad The radius of the sphere.
         */
        MCORE_INLINE BoundingSphere(const AZ::Vector3& pos, float rad)
            : mCenter(pos)
            , mRadius(rad)
            , mRadiusSq(rad * rad) {}

        /**
         * Constructor which sets the center, radius and squared radius.
         * Use this constructor when the squared radius is already known, so an extra multiplication is eliminated.
         * @param pos The center position of the sphere.
         * @param rad The radius of the sphere.
         * @param radSq The squared radius of the sphere (rad*rad).
         */
        MCORE_INLINE BoundingSphere(const AZ::Vector3& pos, float rad, float radSq)
            : mCenter(pos)
            , mRadius(rad)
            , mRadiusSq(radSq)  {}

        /**
         * Initialize the spheres center, radius and square radius.
         * This will set the center position to (0,0,0) and both the radius and squared radius to 0.
         * Call this method when you want to reset the sphere. Note that this is already done by the default constructor.
         */
        MCORE_INLINE void Init()                                                                { mCenter = AZ::Vector3::CreateZero(); mRadius = mRadiusSq = 0.0f; }

        /**
         * Encapsulate a 3D point to the sphere.
         * Automatically adjusts the radius of the sphere. Only use this method when the center of the sphere is already known.
         * This is the faster way of encapsulating. But again, only use this method when the center of the sphere is known upfront and won't change.
         * If the center of the sphere should automatically adjust as well, use the Encapsulate method instead (of course that's slower too).
         * @param v The vector representing the 3D point to encapsulate (add) to the sphere.
         */
        MCORE_INLINE void EncapsulateFast(const AZ::Vector3& v)
        {
            AZ::Vector3 diff = (mCenter - v);
            const float dist = diff.Dot(diff);
            if (dist > mRadiusSq)
            {
                mRadiusSq = dist;
                mRadius = Math::Sqrt(dist);
            }
        }

        /**
         * Check if the sphere contains a given 3D point.
         * Note that the border of the sphere is also counted as inside.
         * @param v The vector representing the 3D point to perform the test with.
         * @result Returns true when 'v' is inside the spheres volume, otherwise false is returned.
         */
        MCORE_INLINE bool Contains(const AZ::Vector3& v)                                        { return ((mCenter - v).GetLengthSq() <= mRadiusSq); }

        /**
         * Check if the sphere COMPLETELY contains a given other sphere.
         * Note that the border of the sphere is also counted as inside.
         * @param s The sphere to perform the test with.
         * @result Returns true when 's' is completely inside this sphere. False is returned in any other case.
         */
        MCORE_INLINE bool Contains(const BoundingSphere& s) const                               { return ((mCenter - s.mCenter).GetLengthSq() <= (mRadiusSq - s.mRadiusSq)); }

        /**
         * Check if a given sphere intersects with this sphere.
         * Note that the border of this sphere is counted as 'inside'. So if only the borders of the spheres intersects, this is seen as intersection.
         * @param s The sphere to perform the intersection test with.
         * @result Returns true when 's' intersects this sphere. So if it's partially or completely inside this sphere or if the borders overlap.
         */
        MCORE_INLINE bool Intersects(const BoundingSphere& s) const                             { return ((mCenter - s.mCenter).GetLengthSq() <= (mRadiusSq + s.mRadiusSq)); }

        /**
         * Encapsulate a given 3D point with this sphere.
         * Automatically adjust the center and radius of the sphere after 'adding' the given point to the sphere.
         * Note that you should only use this method when the center of the bounding sphere isnt exactly known yet.
         * This encapsulation method will adjust the center of the sphere as well. If the center of the sphere is known upfront and it is
         * known upfront as well that this center point won't change, you should use the Encapsulate() method instead.
         * @param v The vector representing the 3D point to use in the encapsulation.
         */
        void Encapsulate(const AZ::Vector3& v);

        /**
         * Checks if this sphere completely contains a given Axis Aligned Bounding Box (AABB).
         * Note that the border of the sphere is counted as 'inside'.
         * @param 'b' The box to perform the test with.
         * @result Returns true when 'b' is COMPLETELY inside the spheres volume, otherwise false is returned.
         */
        bool Contains(const AABB& b) const;

        /**
         * Checks if a given Axis Aligned Bounding Box (AABB) intersects this sphere.
         * Note that the border of the sphere is counted as inside.
         * @param 'b' The box to perform the test with.
         * @result Returns true when 'b' is completely or partially inside the volume of this sphere.
         */
        bool Intersects(const AABB& b) const;

        /**
         * Get the radius of the sphere.
         * @result Returns the radius of the sphere.
         */
        MCORE_INLINE float GetRadius() const                                                    { return mRadius; }

        /**
         * Get the squared radius of the sphere.
         * @result Returns the squared radius of the sphere (no calculations done for this), since it's already known.
         */
        MCORE_INLINE float GetRadiusSquared() const                                             { return mRadiusSq; }

        /**
         * Get the center of the sphere. So the position of the sphere.
         * @result Returns the center position of the sphere.
         */
        MCORE_INLINE const AZ::Vector3& GetCenter() const                                       { return mCenter; }

        /**
         * Set the center of the sphere.
         * @param center The center position of the sphere.
         */
        MCORE_INLINE void SetCenter(const AZ::Vector3& center)                                  { mCenter = center; }

        /**
         * Set the radius of the sphere.
         * The squared radius will automatically be updated inside this method.
         * @param radius The radius of the sphere.
         */
        MCORE_INLINE void SetRadius(float radius)                                               { mRadius = radius; mRadiusSq = radius * radius; }

    private:
        AZ::Vector3     mCenter;    /**< The center of the sphere. */
        float           mRadius;    /**< The radius of the sphere. */
        float           mRadiusSq;  /**< The squared radius of the sphere (mRadius*mRadius).*/
    };
} // namespace MCore

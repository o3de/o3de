/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "Vector.h"
#include "Algorithms.h"


namespace MCore
{
    /**
     * An axis aligned bounding box (AABB).
     * This box is constructed out of two 3D points, a minimum and a maximum.
     * This means the box does not rotate, but always keeps aligned each axis.
     * Usage for this template could be for building a bounding volume around any given 3D object and
     * use it to accelerate ray tracing or visibility tests.
     */
    class AABB
    {
    public:
        /**
         * Default constructor. Initializes the min and max point to the extremes.
         * This means it's basically inside out at infinite size. So you are ready to start encapsulating points.
         */
        MCORE_INLINE AABB()                                                                     { Init(); }

        /**
         * Constructor which inits the minimum and maximum point of the box.
         * @param minPnt The minimum point.
         * @param maxPnt The maximum point.
         */
        MCORE_INLINE AABB(const AZ::Vector3& minPnt, const AZ::Vector3& maxPnt)
            : m_min(minPnt)
            , m_max(maxPnt) {}

        /**
         * Initialize the box minimum and maximum points.
         * Sets the points to floating point maximum and minimum. After calling this method you are ready to encapsulate points in it.
         * Note, that the default constructor already calls this method. So you should only call it when you want to 'reset' the minimum and
         * maximum points of the box.
         */
        MCORE_INLINE void Init()                                                                { m_min.Set(FLT_MAX, FLT_MAX, FLT_MAX); m_max.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX); }

        /**
         * Check if this is a valid AABB or not.
         * The box is only valid if the minimum values are all smaller or equal than the maximum values.
         * @result True when the AABB is valid, otherwise false is returned.
         */
        MCORE_INLINE bool CheckIfIsValid() const
        {
            if (m_min.GetX() > m_max.GetX())
            {
                return false;
            }
            if (m_min.GetY() > m_max.GetY())
            {
                return false;
            }
            if (m_min.GetZ() > m_max.GetZ())
            {
                return false;
            }
            return true;
        }

        /**
         * Encapsulate a point in the box.
         * This means that the box will automatically calculate the new minimum and maximum points when needed.
         * @param v The vector (3D point) to 'add' to the box.
         */
        MCORE_INLINE void Encapsulate(const AZ::Vector3& v);

        /**
         * Encapsulate another AABB with this box.
         * This method automatically adjusts the minimum and maximum point of the box after 'adding' the given AABB to this box.
         * @param box The AABB to 'add' to this box.
         */
        MCORE_INLINE void Encapsulate(const AABB& box)                                          { Encapsulate(box.m_min); Encapsulate(box.m_max); }

        /**
         * Widen the box in all dimensions with a given number of units.
         * @param delta The delta size in units to enlarge the box with. The delta value will be added to the maximum point as well as subtracted from the minimum point.
         */
        MCORE_INLINE void Widen(float delta)
        {
            m_min.SetX(m_min.GetX() - delta);
            m_min.SetY(m_min.GetY() - delta);
            m_min.SetZ(m_min.GetZ() - delta);
            m_max.SetX(m_max.GetX() + delta);
            m_max.SetY(m_max.GetY() + delta);
            m_max.SetZ(m_max.GetZ() + delta);
        }

        /**
         * Translates the box with a given offset vector.
         * This means the middle of the box will be moved by the given vector.
         * @param offset The offset vector to translate (move) the box with.
         */
        MCORE_INLINE void Translate(const AZ::Vector3& offset)                                   { m_min += offset; m_max += offset; }

        /**
         * Checks if a given point is inside this box or not.
         * Note that the edges/planes of the box are also counted as 'inside'.
         * @param v The vector (3D point) to perform the test with.
         * @result Returns true when the given point is inside the box, otherwise false is returned.
         */
        MCORE_INLINE bool Contains(const AZ::Vector3& v) const                                   { return (InRange(v.GetX(), m_min.GetX(), m_max.GetX()) && InRange(v.GetZ(), m_min.GetZ(), m_max.GetZ()) && InRange(v.GetY(), m_min.GetY(), m_max.GetY())); }

        /**
         * Checks if a given AABB is COMPLETELY inside this box or not.
         * Note that the edges/planes of this box are counted as 'inside'.
         * @param box The AABB to perform the test with.
         * @result Returns true when the AABB 'b' is COMPLETELY inside this box. If it's completely or partially outside, false will be returned.
         */
        MCORE_INLINE bool Contains(const AABB& box) const                                       { return (Contains(box.m_min) && Contains(box.m_max)); }

        /**
         * Checks if a given AABB partially or completely contains, so intersects, this box or not.
         * Please note that the edges of this box are counted as 'inside'.
         * @param box The AABB to perform the test with.
         * @result Returns true when the given AABB 'b' is completely or partially inside this box. Only false will be returned when the given AABB 'b' is COMPLETELY outside this box.
         */
        MCORE_INLINE bool Intersects(const AABB& box) const                                     { return !(m_min.GetX() > box.m_max.GetX() || m_max.GetX() < box.m_min.GetX() || m_min.GetY() > box.m_max.GetY() || m_max.GetY() < box.m_min.GetY() || m_min.GetZ() > box.m_max.GetZ() || m_max.GetZ() < box.m_min.GetZ()); }

        /**
         * Calculates and returns the width of the box.
         * The width is the distance between the minimum and maximum point, along the X-axis.
         * @result The width of the box.
         */
        MCORE_INLINE float CalcWidth() const                                                    { return m_max.GetX() - m_min.GetX(); }

        /**
         * Calculates and returns the height of the box.
         * The height is the distance between the minimum and maximum point, along the Z-axis.
         * @result The height of the box.
         */
        MCORE_INLINE float CalcHeight() const                                                   { return m_max.GetZ() - m_min.GetZ(); }

        /**
         * Calculates and returns the depth of the box.
         * The depth is the distance between the minimum and maximum point, along the Y-axis.
         * @result The depth of the box.
         */
        MCORE_INLINE float CalcDepth() const                                                    { return m_max.GetY() - m_min.GetY(); }

        /**
         * Calculate the volume of the box.
         * This equals width x height x depth.
         * @result The volume of the box.
         */
        MCORE_INLINE float CalcVolume() const                                                   { return (m_max.GetX() - m_min.GetX()) * (m_max.GetY() - m_min.GetY()) * (m_max.GetZ() - m_min.GetZ()); }

        /**
         * Calculate the surface area of the box.
         * @result The surface area.
         */
        MCORE_INLINE float CalcSurfaceArea() const
        {
            const float width = m_max.GetX() - m_min.GetX();
            const float height = m_max.GetY() - m_min.GetY();
            const float depth = m_max.GetZ() - m_min.GetZ();
            return (2.0f * height * width) + (2.0f * height * depth) + (2.0f * width * depth);
        }

        /**
         * Calculates the center/middle of the box.
         * This is simply done by taking the average of the minimum and maximum point along each axis.
         * @result The center (or middle) point of this box.
         */
        MCORE_INLINE AZ::Vector3 CalcMiddle() const                                             { return (m_min + m_max) * 0.5f; }

        /**
         * Calculates the extents of the box.
         * This is the vector from the center to a corner of the box.
         * @result The vector containing the extents.
         */
        MCORE_INLINE AZ::Vector3 CalcExtents() const                                            { return (m_max - m_min) * 0.5f; }

        /**
         * Calculates the radius of this box.
         * With radius we mean the length of the vector from the center of the box to the minimum or maximum point.
         * So if you would construct a sphere with as center the middle of this box and with a radius returned by this method, you will
         * get the minimum sphere which exactly contains this box.
         * @result The length of the center of the box to one of the extreme points. So the minimum radius of the bounding sphere containing this box.
         */
        MCORE_INLINE float CalcRadius() const                                                   { return SafeLength(m_max - m_min) * 0.5f; }

        /**
         * Get the minimum point of the box.
         * @result The minimum point of the box.
         */
        MCORE_INLINE const AZ::Vector3& GetMin() const                                              { return m_min; }

        /**
         * Get the maximum point of the box.
         * @result The maximum point of the box.
         */
        MCORE_INLINE const AZ::Vector3& GetMax() const                                              { return m_max; }

        /**
         * Set the minimum point of the box.
         * @param minVec The vector representing the minimum point of the box.
         */
        MCORE_INLINE void SetMin(const AZ::Vector3& minVec)                                         { m_min = minVec; }

        /**
         * Set the maximum point of the box.
         * @param maxVec The vector representing the maximum point of the box.
         */
        MCORE_INLINE void SetMax(const AZ::Vector3& maxVec)                                         { m_max = maxVec; }

        void CalcCornerPoints(AZ::Vector3* outPoints) const
        {
            outPoints[0].Set(m_min.GetX(), m_min.GetY(), m_max.GetZ());       //   4-------------5
            outPoints[1].Set(m_max.GetX(), m_min.GetY(), m_max.GetZ());       //  /|           / |
            outPoints[2].Set(m_max.GetX(), m_min.GetY(), m_min.GetZ());       // 0------------1  |
            outPoints[3].Set(m_min.GetX(), m_min.GetY(), m_min.GetZ());       // | |          |  |
            outPoints[4].Set(m_min.GetX(), m_max.GetY(), m_max.GetZ());       // | 7----------|--6
            outPoints[5].Set(m_max.GetX(), m_max.GetY(), m_max.GetZ());       // | /          | /
            outPoints[6].Set(m_max.GetX(), m_max.GetY(), m_min.GetZ());       // |/           |/
            outPoints[7].Set(m_min.GetX(), m_max.GetY(), m_min.GetZ());       // 3------------2
        }


    private:
        AZ::Vector3     m_min;   /**< The minimum point. */
        AZ::Vector3     m_max;   /**< The maximum point. */
    };


    // encapsulate a point in the box
    MCORE_INLINE void AABB::Encapsulate(const AZ::Vector3& v)
    {
        m_min.SetX(Min(m_min.GetX(), v.GetX()));
        m_min.SetY(Min(m_min.GetY(), v.GetY()));
        m_min.SetZ(Min(m_min.GetZ(), v.GetZ()));

        m_max.SetX(Max(m_max.GetX(), v.GetX()));
        m_max.SetY(Max(m_max.GetY(), v.GetY()));
        m_max.SetZ(Max(m_max.GetZ(), v.GetZ()));
    }
}   // namespace MCore

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "Vector.h"
#include <AzCore/std/containers/vector.h>


namespace MCore
{
    /**
     * The Plane Equation template.
     * This represents an infinite plane, which is mathematically represented by the following equation:
     * Ax + By + Cz + d = 0. Where ABC is the XYZ of the planes normal and where xyz is a point on the plane.
     * The value d is a constant value, which is precalculated. Now if we put a random point inside this equation, when we
     * already know the normal and the value for d, we can see if the result of Ax + By + Cz + d is 0 or not.
     * If it is 0, this means the point is on the plane. We can also use this to calculate on what side of the plane a point is.
     * This is for example used for constructing BSP trees.
     * So, from this we can conclude that the result of (normal.Dot(point) + d) is the distance from point to the plane, along the planes normal.
     */
    class MCORE_API PlaneEq
    {
    public:
        /**
         * Axis aligned planes.
         * Used to determine the most dominant axis, which can be used for planar mapping.
         * This is for example used to generate the texture coordinates for lightmaps.
         */
        enum EPlane
        {
            PLANE_XY = 0,   /**< The XY plane, so where Z is constant. */
            PLANE_XZ = 1,   /**< The XZ plane, so where Y is constant. */
            PLANE_YZ = 2    /**< The YZ plane, so where X is constant. */
        };

        /**
         * The default constructor. Does not initialize anything.
         * So this does not result in a usable/valid plane.
         */
        MCORE_INLINE PlaneEq()                                                                                          {}

        /**
         * Constructor when you know the normal of the plane and a point on the plane.
         * @param norm The normal of the plane.
         * @param pnt A point on the plane.
         */
        MCORE_INLINE PlaneEq(const AZ::Vector3& norm, const AZ::Vector3& pnt)
            : m_normal(norm)
            , m_dist(-((norm.GetX() * pnt.GetX()) + (norm.GetY() * pnt.GetY()) + (norm.GetZ() * pnt.GetZ()))) {}

        /**
         * Constructor when you know the normal and the value of d out of the plane equation (Ax + By + Cz + d = 0)
         * @param norm The normal of the plane
         * @param d The value of 'd' out of the plane equation.
         */
        MCORE_INLINE PlaneEq(const AZ::Vector3& norm, float d)
            : m_normal(norm)
            , m_dist(d) {}

        /**
         * Constructor when you know 3 points on the plane (the winding matters here (clockwise vs counter-clockwise)
         * The normal will be calculated as ((v2-v1).Cross(v3-v1)).Normalize().
         * @param v1 The first point on the plane.
         * @param v2 The second point on the plane.
         * @param v3 The third point on the plane.
         */
        MCORE_INLINE PlaneEq(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3)
            : m_normal((v2 - v1).Cross(v3 - v1).GetNormalized())
            , m_dist(-(m_normal.Dot(v1))) {}

        /**
         * Calculates and returns the dominant plane.
         * A dominant plane is an axis aligned plane, so it can be 3 types of planes, one for each axis.
         * The way this is calculated is by looking at the normal of the plane, and looking which axis of the normal is the most dominant.
         * Based on this, corresponding axis aligned plane, can be returned.
         * @result Returns the type of axis aligned plane. This can be aligned to the XY, XZ or YZ plane. See EPlane for the plane types.
         */
        MCORE_INLINE EPlane CalcDominantPlane() const
        {
            return (Math::Abs(m_normal.GetY()) > Math::Abs(m_normal.GetX())
                    ? (Math::Abs(m_normal.GetZ()) > Math::Abs(m_normal.GetY())
                       ? PLANE_XY : PLANE_XZ) : (Math::Abs(m_normal.GetZ()) > Math::Abs(m_normal.GetX())
                                                 ? PLANE_XY : PLANE_YZ));
        }

        /**
         * Calculates the distance of a given point to the plane, along the normal.
         * A mathematical explanation of how this is done can be read in the description of this template/class.
         * @param v The vector representing the 3D point to use for the calculation.
         * @result The distance from 'v' to this plane, along the normal of this plane.
         */
        MCORE_INLINE float CalcDistanceTo(const AZ::Vector3& v) const                                                       { return m_normal.Dot(v) + m_dist; }

        /**
         * Construct the plane when the normal of the plane and a point on the plane are known.
         * @param normal The normal of the plane.
         * @param pointOnPlane A point on the plane.
         */
        MCORE_INLINE void Construct(const AZ::Vector3& normal, const AZ::Vector3& pointOnPlane)
        {
            m_normal = normal;
            m_dist = -((normal.GetX() * pointOnPlane.GetX()) + (normal.GetY() * pointOnPlane.GetY()) + (normal.GetZ() * pointOnPlane.GetZ()));
        }

        /**
         * Construct the plane when the normal of the plane is known, as well as the value of 'd' in the plane equation (Ax + By + Cz + d = 0)
         * @param normal The normal of the plane.
         * @param d The value of 'd' in the above mentioned plane equation.
         */
        MCORE_INLINE void Construct(const AZ::Vector3& normal, float d)
        {
            m_normal = normal;
            m_dist = d;
        }

        /**
         * Construct the plane when you know three points on the plane. The winding of the vertices matters (clockwise vs counter-clockwise).
         * The normal is calculated as ((v2-v1).Cross(v3-v1)).Normalize()
         * @param v1 The first point on the plane.
         * @param v2 The second point on the plane.
         * @param v3 The third point on the plane.
         */
        MCORE_INLINE void Construct(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3)
        {
            m_normal = (v2 - v1).Cross(v3 - v1).GetNormalized();
            m_dist = -(m_normal.Dot(v1));
        }

        /**
         * Get the normal of the plane.
         * @result Returns the normal of the plane.
         */
        MCORE_INLINE const AZ::Vector3& GetNormal() const                                                                   { return m_normal; }

        /**
         * Get the 'd' out of the plane equation (Ax + By + Cz + d = 0).
         * @result Returns the 'd' from the plane equation.
         */
        MCORE_INLINE float GetDist() const                                                                              { return m_dist; }

        /**
         * Project a vector onto the plane.
         * @param vectorToProject The vector you wish to project onto the plane.
         * @result The projected vector.
         */
        MCORE_INLINE AZ::Vector3 Project(const AZ::Vector3& vectorToProject)            { return vectorToProject - vectorToProject.Dot(m_normal) * m_normal; }


    private:
        AZ::Vector3 m_normal;    /**< The normal of the plane. */
        float   m_dist;      /**< The D in the plane equation (Ax + By + Cz + D = 0). */
    };

}   // namespace MCore

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "StandardHeaders.h"
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>


namespace MCore
{

    /**
     * Given a set of points, this class computes the Delaunay triangulation for
     * those points.
     */
    class MCORE_API DelaunayTriangulator
    {
        MCORE_MEMORYOBJECTCATEGORY(DelaunayTriangulator, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_TRIANGULATOR);

    public:
        class Triangle
        {
        public:
            Triangle(const AZStd::vector<AZ::Vector2>& points, int vert0, int vert1, int vert2);

            // Returns true if the point is inside the circum circle of the triangle. 
            bool DoesCircumCircleContainPoint(const AZ::Vector2& point) const
            {
                return (point - m_circumCircleCenter).GetLengthSq() <= m_circumCircleRadiusSqr;
            }

            int VertIndex(int num) const
            {
                return m_vertIndices[num];
            }

        private:
            int         m_vertIndices[3]; // indices corresponding to the triangle's 3 vertices in an an array of vertices
            AZ::Vector2 m_circumCircleCenter;
            float       m_circumCircleRadiusSqr;
        };
        using Triangles = AZStd::vector<Triangle>;

    public:
        DelaunayTriangulator();
        ~DelaunayTriangulator();

        const Triangles& Triangulate(const AZStd::vector<AZ::Vector2>& points);

    private:
        void AddSuperTriangle();
        void DoTriangulation(int newPointsStart, int newPointsEnd);

    private:
        AZStd::vector<AZ::Vector2>  m_points;
        Triangles                   m_triangles;
    };

}   // namespace MCore

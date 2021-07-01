/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DelaunayTriangulator.h"
#include "FastMath.h"
#include <algorithm>

namespace
{
    struct TriangleEdge
    {
        int m_vert1;
        int m_vert2;

        TriangleEdge(int vert1, int vert2)
            : m_vert1(vert1)
            , m_vert2(vert2)
        {
        }
        bool operator==(const TriangleEdge& rhs)
        {
            return ((m_vert1 == rhs.m_vert1 && m_vert2 == rhs.m_vert2) ||
                (m_vert1 == rhs.m_vert2 && m_vert2 == rhs.m_vert1));
        }
    };

    struct TriangleEdgeInfo
    {
        TriangleEdge    m_edge;
        int             m_count;

        TriangleEdgeInfo(const TriangleEdge& edge, int count)
            : m_edge(edge)
            , m_count(count)
        {
        }
    };
    using TriangleEdgeInfos = AZStd::vector<TriangleEdgeInfo>;

    inline void AddTriangleEdge(TriangleEdgeInfos& edgeInfos, const TriangleEdge& edge)
    {
        for (TriangleEdgeInfo& edgeInfo : edgeInfos)
        {
            if (edgeInfo.m_edge == edge)
            {
                edgeInfo.m_count++;
                return;
            }
        }
        edgeInfos.push_back(TriangleEdgeInfo(edge, 1));
    }
}

namespace MCore
{

    DelaunayTriangulator::Triangle::Triangle(const AZStd::vector<AZ::Vector2>& points, int vert0, int vert1, int vert2)
    {
        AZ_Assert((vert0 < points.size()) && (vert1 < points.size()) && (vert2 < points.size()), "Invalid vert index");

        m_vertIndices[0] = vert0;
        m_vertIndices[1] = vert1;
        m_vertIndices[2] = vert2;

        const float x0 = points[vert0].GetX();
        const float y0 = points[vert0].GetY();
        const float x1 = points[vert1].GetX();
        const float y1 = points[vert1].GetY();
        const float x2 = points[vert2].GetX();
        const float y2 = points[vert2].GetY();

        const float a = x1 - x0;
        const float b = y1 - y0;
        const float c = x2 - x0;
        const float d = y2 - y0;
        const float e = a * (x0 + x1) + b * (y0 + y1);
        const float f = c * (x0 + x2) + d * (y0 + y2);
        const float g = 2 * (a * (y2 - y1) - b * (x2 - x1));

        if (::fabsf(g) < MCore::Math::epsilon) 
        {
            // Points are colinear. This should not really be happening.
            // Anyway, in this case, take the mid-point of the line as circumcenter.
            const float minX = std::min(std::min(x0, x1), x2);
            const float minY = std::min(std::min(y0, y1), y2);
            const float maxX = std::max(std::max(x0, x1), x2);
            const float maxY = std::max(std::max(y0, y1), y2);
            m_circumCircleCenter.Set((minX + maxX)/2, (minY + maxY)/2);
        }
        else 
        {
            m_circumCircleCenter.SetX((d*e - b*f) / g);
            m_circumCircleCenter.SetY((a*f - c*e) / g);
        }
        
        m_circumCircleRadiusSqr = (points[vert0] - m_circumCircleCenter).GetLengthSq();
    }


    DelaunayTriangulator::DelaunayTriangulator()
    {
    }

    DelaunayTriangulator::~DelaunayTriangulator()
    {
    }

    const DelaunayTriangulator::Triangles& DelaunayTriangulator::Triangulate(const AZStd::vector<AZ::Vector2>& points)
    {
        m_points = points;
        m_triangles.clear();
        const size_t numPoints = points.size();
        DoTriangulation(0, (int)numPoints - 1);
        return m_triangles;
    }

    // "SuperTraingle" is some triangle that is big enough that all the sample
    // points are guaranteed to be inside it. 
    //
    void DelaunayTriangulator::AddSuperTriangle()
    {
        const float valueRange = 2.0f; // This is the range when
            // the point coordinates are in the normalized -1 to 1 range. 
            // Change this value if that is not the case.

        const int numPoints = (int)m_points.size();
        m_points.resize(numPoints + 3);
        m_points[numPoints].Set(-20 * valueRange, -valueRange);
        m_points[numPoints + 1].Set(0, 20 * valueRange);
        m_points[numPoints + 2].Set(20 * valueRange, -valueRange);
        m_triangles.push_back(Triangle(m_points, numPoints, numPoints + 1, numPoints + 2));
    }

    // This is an incremental algorithm. We iteratively add one point at a time and update the tessellation.
    // By definition, in Delaunauy triangulation, the circumcircle of any triangle does not contain any sample 
    // point other than its three vertices. So, when we insert a new sample point, we gather all the current
    // triangles whose circum-circles contain the newly added point and fix them.
    //
    // The overall algorithm:
    //    Add the supertriangle
    //    for each point in the vertex list:
    //        for each triangle whose circumcircle contains the point
    //          add the edges of the triangles into "edgeInfos" list
    //        (We want to retain the outer, unshared edges of those triangles
    //         but remove the inner edges that are shared by two triangles. In the
    //         implementation below the edges to be removed will have a count of 2 while
    //         the edges to be retained will have a count of 1.)
    //         The idea made use of here is that in a quadrilateral, if one of the diagonals violates
    //         the Delaunay property, the other diagonal will satisfy it.
    //         Add new triangles formed by the two end vertices of each edge to be retained and the new point.
    //          
    //    Remove any triangles from the triangle list that use the supertriangle vertices.
    //                
    void DelaunayTriangulator::DoTriangulation(int newPointsStart, int newPointsEnd)
    {
        // Add a big triangle that is guaranteed to contain all the points. This and any other triangle that shares 
        // vertices with it get removed after the completion of tessellation.
        AddSuperTriangle();

        TriangleEdgeInfos triEdgeInfos;

        for (int newPtIdx=newPointsStart; newPtIdx <= newPointsEnd; ++newPtIdx)
        {
            const AZ::Vector2& newPt = m_points[newPtIdx];

            triEdgeInfos.clear();

            for (size_t i=0, numTriangles=m_triangles.size(); i < numTriangles;)
            {
                const Triangle& tri = m_triangles[i];
                if (tri.DoesCircumCircleContainPoint(newPt))
                {
                    AddTriangleEdge(triEdgeInfos, TriangleEdge(tri.VertIndex(0), tri.VertIndex(1)));
                    AddTriangleEdge(triEdgeInfos, TriangleEdge(tri.VertIndex(1), tri.VertIndex(2)));
                    AddTriangleEdge(triEdgeInfos, TriangleEdge(tri.VertIndex(2), tri.VertIndex(0)));

                    // Delete this triangle from m_triangles
                    m_triangles[i] = m_triangles.back();
                    m_triangles.pop_back();
                    numTriangles--;
                }
                else
                {
                    i++;
                }
            }

            for (const TriangleEdgeInfo& edgeInfo : triEdgeInfos)
            {
                AZ_Assert(edgeInfo.m_count == 1 || edgeInfo.m_count == 2, "A triangle edge should be shared with at most one other triangle");
                // if the edge count is 2, that is an edge we want to get rid of. If it is 1, we want to add a new triangle formed by the two end vertices
                // of that edge and the new point.
                if (edgeInfo.m_count == 1)
                {
                    const TriangleEdge& edge = edgeInfo.m_edge;
                    m_triangles.push_back(Triangle(m_points, edge.m_vert1, edge.m_vert2, newPtIdx));
                }
            }
        }

        AZ_Assert(m_points.size() >= 3, "At least the 3 verts of SuperTriangle should be there");
        const size_t numActualPts = m_points.size() - 3;

        // Delete all the triangles that share a vertex with the SuperTriangle.
        for (size_t i = 0, numTriangles = m_triangles.size(); i < numTriangles;)
        {
            const Triangle& tri = m_triangles[i];
            if ((tri.VertIndex(0) >= numActualPts) || (tri.VertIndex(1) >= numActualPts) ||
                (tri.VertIndex(2) >= numActualPts))
            {
                m_triangles[i] = m_triangles.back();
                m_triangles.pop_back();
                numTriangles--;
            }
            else
            {
                i++;
            }
        }
        
        // Erase the vertices of the SuperTriangle
        m_points.erase(m_points.begin() + numActualPts, m_points.end());
    }

}   // namespace MCore

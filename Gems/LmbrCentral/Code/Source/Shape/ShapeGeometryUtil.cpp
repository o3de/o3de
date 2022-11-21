/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShapeGeometryUtil.h"

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    AZ::u32* WriteTriangle(AZ::u32 a, AZ::u32 b, AZ::u32 c, AZ::u32* indices)
    {
        indices[0] = a;
        indices[1] = b;
        indices[2] = c;
        return indices + 3;
    }

    AZ::Vector3* WriteVertex(const AZ::Vector3& vertex, AZ::Vector3* vertices)
    {
        *vertices = vertex;
        return vertices + 1;
    }

    void DrawShape(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const ShapeDrawParams& shapeDrawParams,
        const ShapeMesh& shapeMesh,
        const AZ::Vector3& shapeOffset)
    {
        debugDisplay.PushMatrix(AZ::Transform::CreateTranslation(shapeOffset));

        if (shapeDrawParams.m_filled)
        {
            if (!shapeMesh.m_vertexBuffer.empty() && !shapeMesh.m_indexBuffer.empty())
            {
                debugDisplay.DrawTrianglesIndexed(shapeMesh.m_vertexBuffer, shapeMesh.m_indexBuffer, shapeDrawParams.m_shapeColor);
            }
        }

        if (!shapeMesh.m_lineBuffer.empty())
        {
            debugDisplay.DrawLines(shapeMesh.m_lineBuffer, shapeDrawParams.m_wireColor);
        }

        debugDisplay.PopMatrix();
    }

    /// Determine if a list of vertices constitute a simple polygon
    /// (none of the edges are self intersecting).
    /// https://en.wikipedia.org/wiki/Simple_polygon
    static bool SimplePolygon(const AZStd::vector<AZ::Vector2>& vertices)
    {
        const size_t vertexCount = vertices.size();

        if (vertexCount < 3)
        {
            return false;
        }

        if (vertexCount == 3)
        {
            return true;
        }

        for (size_t i = 0; i < vertexCount; ++i)
        {
            // make it easy to nicely wrap indices
            const size_t safeIndex = i + vertexCount;

            const size_t endIndex = (safeIndex - 1) % vertexCount;
            const size_t beginIndex = (safeIndex + 2) % vertexCount;

            for (size_t j = beginIndex; j != endIndex; j = (j + 1) % vertexCount)
            {
                float aa, bb;
                AZ::Vector3 a, b;
                AZ::Intersect::ClosestSegmentSegment(
                    AZ::Vector3(vertices[i]),
                    AZ::Vector3(vertices[(i + 1) % vertexCount]),
                    AZ::Vector3(vertices[j]),
                    AZ::Vector3(vertices[(j + 1) % vertexCount]),
                    aa, bb, a, b);

                if ((a - b).GetLength() < 0.001f)
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool ClockwiseOrder(const AZStd::vector<AZ::Vector2>& vertices)
    {
        const size_t vertexCount = vertices.size();

        float total = 0.0f;
        for (size_t i = 0; i < vertexCount; ++i)
        {
            const AZ::Vector2 a = vertices[i];
            const AZ::Vector2 b = vertices[(i + 1) % vertexCount];

            total += (b.GetX() - a.GetX()) * (b.GetY() + a.GetY());
        }

        return total > 0.0f;
    }

    /// Calculate the Wedge Product of two vectors (the area of the parallelogram formed by them)
    static float Wedge(const AZ::Vector2& v1, const AZ::Vector2& v2)
    {
        return v1.GetX() * v2.GetY() - v1.GetY() * v2.GetX();
    }

    AZStd::vector<AZ::Vector3> GenerateTriangles(AZStd::vector<AZ::Vector2> vertices)
    {
        AZStd::vector<AZ::Vector3> triangles;

        // we only support simple polygons (ones with no self intersections)
        if (!SimplePolygon(vertices))
        {
            return triangles;
        }

        // vertices must be in anti-clockwise winding order
        if (ClockwiseOrder(vertices))
        {
            AZStd::reverse(vertices.begin(), vertices.end());
        }

        // while we still have vertices remaining
        for (;;)
        {
            for (size_t i = 0; i < vertices.size(); ++i)
            {
                // make it easy to nicely wrap indices
                const size_t safeIndex = i + vertices.size();

                const size_t prevIndex = (safeIndex - 1) % vertices.size();
                const size_t currIndex = safeIndex % vertices.size();
                const size_t nextIndex = (safeIndex + 1) % vertices.size();

                // vertices making up the triangle
                const AZ::Vector2 prev = vertices[prevIndex];
                const AZ::Vector2 curr = vertices[currIndex];
                const AZ::Vector2 next = vertices[nextIndex];

                const AZ::Vector2 edgeBefore = prev - curr;
                const AZ::Vector2 edgeAfter = next - curr;

                const float triangleArea = Wedge(edgeBefore, edgeAfter);
                const float tolerance = 0.001f;
                const bool interiorVertex = triangleArea <= 0.0f;

                // if triangle is not an 'ear' and we have other vertices, continue.
                if (!interiorVertex && vertices.size() > 3)
                {
                    continue;
                }

                // check if this is a large enough triangle, that there are no other vertices
                // inside the triangle formed, otherwise, continue to next vertex.
                if (vertices.size() > 3 && !AZ::IsClose(triangleArea, 0.f, tolerance))
                {
                    bool pointInside = false;
                    for (size_t j = (nextIndex + 1) % vertices.size(); j != prevIndex; j = (j + 1) % vertices.size())
                    {
                        if (AZ::Intersect::TestPointTriangle(
                            AZ::Vector3(vertices[j]),
                            AZ::Vector3(prev),
                            AZ::Vector3(curr),
                            AZ::Vector3(next)))
                        {
                            pointInside = true;
                            break;
                        }
                    }

                    if (pointInside)
                    {
                        continue;
                    }
                }

                // form new triangle from 'ear'
                triangles.push_back(AZ::Vector3(prev));
                triangles.push_back(AZ::Vector3(curr));
                triangles.push_back(AZ::Vector3(next));

                // if work is still do be done, remove vertex from list and iterate again
                if (vertices.size() > 3)
                {
                    for (size_t k = i; k < vertices.size() - 1; ++k)
                    {
                        vertices[k] = vertices[k + 1];
                    }

                    vertices.pop_back();
                }
                else
                {
                    return triangles;
                }
            }
        }
    }

    namespace CapsuleTubeUtil
    {
        AZ::Vector3 CalculatePositionOnSphere(
            const AZ::Vector3& localPosition, const AZ::Vector3& forwardAxis, const AZ::Vector3& sideAxis,
            const float radius, const float angle)
        {
            return localPosition +
                (forwardAxis * sinf(angle) * radius) +
                (sideAxis * cosf(angle) * radius);
        }

        AZ::Vector3* GenerateWireCap(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction, const AZ::Vector3& side,
            const float radius, const AZ::u32 capSegments, AZ::Vector3* vertices)
        {
            const AZ::Vector3 up = side.Cross(direction);
            // number of cap segments is tesselation of end - total is double, as we need lines for first
            // 90 degrees, then the same tesselation completing the semi-cicle for the next 90 degrees
            const AZ::u32 totalCapSegments = capSegments * 2;
            const float deltaAngle = AZ::Constants::Pi / static_cast<float>(totalCapSegments);

            float angle = 0.0f;
            for (size_t i = 0; i < totalCapSegments; ++i)
            {
                const float nextAngle = angle + deltaAngle;

                // horizontal semi-circle arc
                vertices = WriteVertex(
                    CalculatePositionOnSphere(localPosition, direction, side, radius, angle),
                    vertices);
                vertices = WriteVertex(
                    CalculatePositionOnSphere(localPosition, direction, side, radius, nextAngle),
                    vertices);

                // vertical semi-circle arc
                vertices = WriteVertex(
                    CalculatePositionOnSphere(localPosition, direction, up, radius, angle),
                    vertices);
                vertices = WriteVertex(
                    CalculatePositionOnSphere(localPosition, direction, up, radius, nextAngle),
                    vertices);

                angle += deltaAngle;
            }

            return vertices;
        }


        AZ::Vector3* GenerateWireLoop(
            const AZ::Vector3& position, const AZ::Vector3& direction, const AZ::Vector3& side,
            const AZ::u32 sides, const float radius, AZ::Vector3* vertices)
        {
            const auto deltaRot = AZ::Quaternion::CreateFromAxisAngle(
                direction, AZ::Constants::TwoPi / static_cast<float>(sides));

            auto currentNormal = side;
            for (size_t i = 0; i < sides; ++i)
            {
                const auto nextNormal = deltaRot.TransformVector(currentNormal);
                const auto localPosition = position + currentNormal * radius;
                const auto nextlocalPosition = position + nextNormal * radius;

                vertices = WriteVertex(localPosition, vertices);
                vertices = WriteVertex(nextlocalPosition, vertices);

                currentNormal = deltaRot.TransformVector(currentNormal);
            }

            return vertices;
        }

        /// Generate verts to be used when drawing triangles for cap (top vertex is ommitted and is
        /// added in concrete Start/End cap because of ordering - StartCap must at top vertex first,
        /// EndCap must add bottom vertex last.
        static AZ::Vector3* GenerateSolidCap(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction,
            const AZ::Vector3& side, const float radius, const AZ::u32 sides, const AZ::u32 capSegments,
            const float angleOffset, const float sign, AZ::Vector3* vertices)
        {
            const float angleDelta = AZ::Constants::HalfPi / static_cast<float>(capSegments);
            float angle = 0.0f;
            for (size_t i = 1; i <= capSegments; ++i)
            {
                const AZ::Vector3 capSegmentPosition = localPosition + direction * sign * cosf(angle - angleOffset) * radius;
                vertices = GenerateSegmentVertices(
                    capSegmentPosition, direction, side, sinf(angle + angleOffset) * radius, sides, vertices);

                angle += angleDelta;
            }

            return vertices;
        }

        AZ::Vector3* GenerateSolidStartCap(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction,
            const AZ::Vector3& side, const float radius, const AZ::u32 sides,
            const AZ::u32 capSegments, AZ::Vector3* vertices)
        {
            vertices = WriteVertex( // cap end vertex
                localPosition - direction * radius, vertices);
            return GenerateSolidCap(  // circular segments of cap vertices
                localPosition, direction,
                side, radius, sides, capSegments, 0.0f, -1.0f, vertices);
        }

        AZ::Vector3* GenerateSolidEndCap(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction,
            const AZ::Vector3& side, const float radius, const AZ::u32 sides,
            const AZ::u32 capSegments, AZ::Vector3* vertices)
        {
            vertices = GenerateSolidCap(  // circular segments of cap vertices
                localPosition, direction,
                side, radius, sides, capSegments, AZ::Constants::HalfPi, 1.0f, vertices);
            return WriteVertex( // cap end vertex
                localPosition + direction * radius, vertices);
        }

        /// Generates a single segment of vertices - Extrudes the point using the normal * radius,
        /// then rotates it around the axis 'sides' times.
        AZ::Vector3* GenerateSegmentVertices(
            const AZ::Vector3& point,
            const AZ::Vector3& axis,
            const AZ::Vector3& normal,
            const float radius,
            const AZ::u32 sides,
            AZ::Vector3* vertices)
        {
            const auto deltaRot = AZ::Quaternion::CreateFromAxisAngle(
                axis, AZ::Constants::TwoPi / static_cast<float>(sides));

            auto currentNormal = normal;
            for (size_t i = 0; i < sides; ++i)
            {
                const auto localPosition = point + currentNormal * radius;
                vertices = WriteVertex(localPosition, vertices);
                currentNormal = deltaRot.TransformVector(currentNormal);
            }

            return vertices;
        }

        void GenerateSolidMeshIndices(
            const AZ::u32 sides, const AZ::u32 segments, const AZ::u32 capSegments,
            AZ::u32* indices)
        {
            const AZ::u32 capSegmentTipVerts = capSegments > 0 ? 1 : 0;
            const AZ::u32 totalSegments = segments + capSegments * 2;
            const AZ::u32 numVerts = sides * (totalSegments + 1) + 2 * capSegmentTipVerts;
            const AZ::u32 hasEnds = capSegments > 0;

            // Start Faces (start point of tube)
            // Each starting face shares the same vertex at the beginning of the vertex buffer
            // like a triangle fan
            // 1 triangle per face
            // 1 face per side
            if (hasEnds)
            {
                for (AZ::u32 i = 0; i < sides; ++i)
                {
                    AZ::u32 a = i + 1;
                    AZ::u32 b = a + 1;
                    AZ::u32 start = 0; // First vertex
                    if (i == sides - 1)
                    {
                        b -= sides;
                    }

                    indices = WriteTriangle(start, b, a, indices);
                }
            }

            // Middle Faces
            // 2 triangles per face.
            // 1 face per side.
            for (AZ::u32 i = 0; i < totalSegments; ++i)
            {
                for (AZ::u32 j = 0; j < sides; ++j)
                {
                    // 4 corners for each face
                    // a ------ d
                    // |        |
                    // |        |
                    // b ------ c
                    AZ::u32 a = (i + 0) * sides + (j + 0) + capSegmentTipVerts;
                    AZ::u32 b = (i + 0) * sides + (j + 1) + capSegmentTipVerts;
                    AZ::u32 c = (i + 1) * sides + (j + 1) + capSegmentTipVerts;
                    AZ::u32 d = (i + 1) * sides + (j + 0) + capSegmentTipVerts;

                    // Wraps back to beginning vertices
                    if (j == sides - 1)
                    {
                        b -= sides;
                        c -= sides;
                    }

                    indices = WriteTriangle(a, b, d, indices);
                    indices = WriteTriangle(b, c, d, indices);
                }
            }

            // End Faces (end point of tube)
            // Each face shares the same vertex at the end of the vertex buffer
            // like a triangle fan
            // 1 triangle per face
            // 1 face per side
            if (hasEnds)
            {
                for (AZ::u32 i = 0; i < sides; ++i)
                {
                    AZ::u32 a = totalSegments * sides + i + 1;
                    AZ::u32 b = a + 1;
                    AZ::u32 end = numVerts - 1; // Last vertex

                    if (i == sides - 1)
                    {
                        b -= sides;
                    }
                    indices = WriteTriangle(a, b, end, indices);
                }
            }
        }
    }
} // namespace LmbrCentral

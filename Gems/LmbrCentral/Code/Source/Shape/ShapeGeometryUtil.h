/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    /// Buffers used for rendering Shapes.
    /// Generated from shape properties.
    struct ShapeMesh
    {
        AZStd::vector<AZ::Vector3> m_vertexBuffer; ///< Vertices of the shape.
        AZStd::vector<AZ::u32> m_indexBuffer; ///< Indices of the shape.
        AZStd::vector<AZ::Vector3> m_lineBuffer; ///< Lines of the shape.
    };

    /// Writes 3 indices (1 tri) to the buffer and returns a pointer to the next index.
    AZ::u32* WriteTriangle(AZ::u32 a, AZ::u32 b, AZ::u32 c, AZ::u32* indices);

    /// Writes a vertex to the buffer and returns a pointer to the next vertex.
    AZ::Vector3* WriteVertex(const AZ::Vector3& vertex, AZ::Vector3* vertices);

    /// Draw a ShapeMesh (previously generated vertices, indices and lines).
    void DrawShape(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const ShapeDrawParams& shapeDrawParams,
        const ShapeMesh& shapeMesh,
        const AZ::Vector3& shapeOffset = AZ::Vector3::CreateZero());

    /// Return a vector of vertices representing a list of triangles to render (CCW).
    /// This is implemented using the Ear Clipping method:
    ///  (https://www.gamedev.net/articles/programming/graphics/polygon-triangulation-r3334/)
    /// @param vertices List of vertices to process (pass by value as vertices is
    /// modified inside the function so must be copied).
    AZStd::vector<AZ::Vector3> GenerateTriangles(AZStd::vector<AZ::Vector2> vertices);

    /// Determine if a list of ordered vertices have clockwise winding order.
    /// http://blog.element84.com/polygon-winding.html
    bool ClockwiseOrder(const AZStd::vector<AZ::Vector2>& vertices);

    namespace CapsuleTubeUtil
    {
        /// Generates all indices, assumes index pointer is valid.
        void GenerateSolidMeshIndices(
            AZ::u32 sides, AZ::u32 segments, AZ::u32 capSegments, AZ::u32* indices);

        /// Generate verts to be used when drawing triangles for the start cap.
        AZ::Vector3* GenerateSolidStartCap(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction,
            const AZ::Vector3& side, float radius, AZ::u32 sides, AZ::u32 capSegments, AZ::Vector3* vertices);

        /// Generate verts to be used when drawing triangles for the end cap.
        AZ::Vector3* GenerateSolidEndCap(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction,
            const AZ::Vector3& side, float radius, AZ::u32 sides, AZ::u32 capSegments, AZ::Vector3* vertices);

        /// Generate vertices to be used for a loop of a segment along a tube or capsule (for use with index buffer).
        AZ::Vector3* GenerateSegmentVertices(
            const AZ::Vector3& point,
            const AZ::Vector3& axis,
            const AZ::Vector3& normal,
            float radius,
            AZ::u32 sides,
            AZ::Vector3* vertices);

        /// Generate a circle/loop for a given segment along the capsule/tube - Produces a series of begin/end
        /// line segments to draw in DrawLines.
        AZ::Vector3* GenerateWireLoop(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction, const AZ::Vector3& side,
            AZ::u32 sides, float radius, AZ::Vector3* vertices);

        /// Generate a series of lines to be drawn, arcing around the end of a capsule/tube
        /// Two arcs, one horizontal and one vertical, arcing 180 degrees of a sphere.
        AZ::Vector3* GenerateWireCap(
            const AZ::Vector3& localPosition, const AZ::Vector3& direction, const AZ::Vector3& side,
            float radius, AZ::u32 capSegments, AZ::Vector3* vertices);

        /// Given a position, forward axis, side axis and angle (radians), calculate
        /// the position of a final point on a sphere by summing the rotation of those
        /// two axis from their starting orientation.
        AZ::Vector3 CalculatePositionOnSphere(
            const AZ::Vector3& localPosition, const AZ::Vector3& forwardAxis, const AZ::Vector3& sideAxis,
            float radius, float angle);
    }
}

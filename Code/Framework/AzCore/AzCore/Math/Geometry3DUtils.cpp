/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Geometry3DUtils.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::Geometry3dUtils
{
    // Recursive method for subdividing each face of the polygon.
    namespace
    {
        void SubdivideTriangle(
            AZStd::vector<AZ::Vector3>& vertices, const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c, const uint8_t depth)
        {
            // If we've reached the end, push the triangle face onto the vertices list and stop recursing.
            if (depth == 0)
            {
                vertices.push_back(a);
                vertices.push_back(b);
                vertices.push_back(c);
                return;
            }

            /** For each triangle that's passed in, split it into 4 triangles and recursively subdivide those.
             *           a
             *           /\
             *          /  \
             *     ab  /----\  ca
             *        / \  / \
             *       /   \/   \
             *    b ------------ c
             *           bc
             */
            const AZ::Vector3 ab = (a + b).GetNormalized();
            const AZ::Vector3 bc = (b + c).GetNormalized();
            const AZ::Vector3 ca = (c + a).GetNormalized();

            SubdivideTriangle(vertices, a, ab, ca, depth - 1);
            SubdivideTriangle(vertices, b, bc, ab, depth - 1);
            SubdivideTriangle(vertices, ab, bc, ca, depth - 1);
            SubdivideTriangle(vertices, c, ca, bc, depth - 1);
        }
    }

    AZStd::vector<AZ::Vector3> GenerateIcoSphere(const uint8_t subdivisionDepth)
    {
        // The algorithm for generating an icosphere is to start with a regular icosahedron (20-sided polygon with triangluar faces),
        // and then for each triangle, subdivide it further into 4 triangles at the midpoint of each edge. This is repeated recursively
        // to the given depth to reach a desired number of subdivided faces.

        constexpr int IcosahedronFaces = 20;

        // "phi" is also known as the "golden ratio", (1 + sqrt(5)) / 2
        const float phi = (1.0f + AZStd::sqrt(5.0f)) * 0.5f;

        // The 12 vertices of an icosahedron centered at the origin with edge length 1 are
        // (+- 1, +- 1/phi, 0), (0, +- 1, +- 1/phi), (+- 1/phi, 0, +- 1)

        // normalization factor to make the radius 1
        const float norm = 1.0f / AZStd::sqrt(1.0f + 1.0f / (phi * phi));
        const AZ::Vector3 icosahedronVertices[] = {
            // (+- 1, +- 1/phi, 0)
            { -norm, norm/phi, 0.0f }, { norm, norm/phi, 0.0f }, { -norm, -norm/phi, 0.0f }, { norm, -norm/phi, 0.0f },
            // (0, +- 1, +- 1/phi)
            { 0.0f, -norm, norm/phi }, { 0.0f, norm, norm/phi }, { 0.0f, -norm, -norm/phi }, { 0.0f, norm, -norm/phi },
            // (+- 1/phi, 0, +- 1)
            { norm/phi, 0.0f, -norm }, { norm/phi, 0.0f, norm }, { -norm/phi, 0.0f, -norm }, { -norm/phi, 0.0f, norm } };

        // The 20 triangles that make up the faces of the icosahedron.
        int faceIndices[IcosahedronFaces][3] = {
            // The first 5 faces are around vertex 0
            { 0, 11, 5 }, { 0, 5, 1 }, { 0, 1, 7 }, { 0, 7, 10 }, { 0, 10, 11 },
            // The second 5 faces are the ones adjacent to those
            { 1, 5, 9 }, { 5, 11, 4 }, { 11, 10, 2 }, { 10, 7, 6 }, { 7, 1, 8 },
            // The third 5 faces are around vertex 3, which is directly opposite to vertex 0
            { 3, 9, 4 }, { 3, 4, 2 }, { 3, 2, 6 }, { 3, 6, 8 }, { 3, 8, 9 },
            // The final 5 faces are the ones adjacent to those
            { 4, 9, 5 }, { 2, 4, 11 }, { 6, 2, 10 }, { 8, 6, 7 }, { 9, 8, 1 }
        };

        // We're intentionally limiting this algorithm to a max recursion depth of 9 right now as that generates 5M triangles (15M verts)
        // which should be more than sufficient for most purposes. If it ever turns out not to be enough, this limit can be increased as
        // needed.
        uint8_t cappedDepth = AZStd::min(subdivisionDepth, static_cast<uint8_t>(9));

        // Preallocate enough space for all the vertices we're going to generate.
        AZStd::vector<AZ::Vector3> sphereVertices;
        sphereVertices.reserve(IcosahedronFaces * 3 * aznumeric_cast<int>(AZStd::pow(4, cappedDepth)));

        for (int face = 0; face < IcosahedronFaces; face++)
        {
            SubdivideTriangle(
                sphereVertices,
                icosahedronVertices[faceIndices[face][0]],
                icosahedronVertices[faceIndices[face][1]],
                icosahedronVertices[faceIndices[face][2]],
                cappedDepth);
        }

        return sphereVertices;
    }

} // namespace AZ::Geometry3dUtils

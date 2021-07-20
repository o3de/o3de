/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TriangleInputHelper.h>

namespace UnitTest
{
    TriangleInput CreatePlane(float width, float height, AZ::u32 segmentsX, AZ::u32 segmentsY)
    {
        TriangleInput plane;

        plane.m_vertices.resize((segmentsX + 1) * (segmentsY + 1));
        plane.m_uvs.resize((segmentsX + 1) * (segmentsY + 1));
        plane.m_indices.resize((segmentsX * segmentsY * 2) * 3);

        const NvCloth::SimParticleFormat topLeft(
            -width * 0.5f,
            -height * 0.5f,
            0.0f,
            0.0f);

        // Vertices and UVs
        for (AZ::u32 y = 0; y < segmentsY + 1; ++y)
        {
            for (AZ::u32 x = 0; x < segmentsX + 1; ++x)
            {
                const AZ::u32 segmentIndex = x + y * (segmentsX + 1);
                const float fractionX = ((float)x / (float)segmentsX);
                const float fractionY = ((float)y / (float)segmentsY);

                NvCloth::SimParticleFormat position(
                    fractionX * width,
                    fractionY * height,
                    0.0f,
                    (y > 0) ? 1.0f : 0.0f);

                plane.m_vertices[segmentIndex] = topLeft + position;
                plane.m_uvs[segmentIndex] = NvCloth::SimUVType(fractionX, fractionY);
            }
        }

        // Triangles indices
        for (AZ::u32 y = 0; y < segmentsY; ++y)
        {
            for (AZ::u32 x = 0; x < segmentsX; ++x)
            {
                const AZ::u32 segmentIndex = (x + y * segmentsX) * 2 * 3;

                const AZ::u32 firstTriangleStartIndex = segmentIndex;
                const AZ::u32 secondTriangleStartIndex = segmentIndex + 3;

                //Top left to bottom right

                plane.m_indices[firstTriangleStartIndex + 0] = static_cast<NvCloth::SimIndexType>((x + 0) + (y + 0) * (segmentsX + 1));
                plane.m_indices[firstTriangleStartIndex + 1] = static_cast<NvCloth::SimIndexType>((x + 1) + (y + 0) * (segmentsX + 1));
                plane.m_indices[firstTriangleStartIndex + 2] = static_cast<NvCloth::SimIndexType>((x + 1) + (y + 1) * (segmentsX + 1));

                plane.m_indices[secondTriangleStartIndex + 0] = static_cast<NvCloth::SimIndexType>((x + 0) + (y + 0) * (segmentsX + 1));
                plane.m_indices[secondTriangleStartIndex + 1] = static_cast<NvCloth::SimIndexType>((x + 1) + (y + 1) * (segmentsX + 1));
                plane.m_indices[secondTriangleStartIndex + 2] = static_cast<NvCloth::SimIndexType>((x + 0) + (y + 1) * (segmentsX + 1));
            }
        }

        return plane;
    }
} // namespace UnitTest

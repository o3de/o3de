/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxRenderData.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace WhiteBox
{
    void WhiteBoxRenderData::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxFace::Reflect(context);
        WhiteBoxMaterial::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxRenderData>()
                ->Version(2)
                ->Field("Faces", &WhiteBoxRenderData::m_faces)
                ->Field("Material", &WhiteBoxRenderData::m_material);
        }
    }

    void WhiteBoxVertex::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxVertex>()
                ->Version(1)
                ->Field("Position", &WhiteBoxVertex::m_position)
                ->Field("UV", &WhiteBoxVertex::m_uv);
        }
    }

    void WhiteBoxFace::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxVertex::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxFace>()
                ->Version(1)
                ->Field("Vertex1", &WhiteBoxFace::m_v1)
                ->Field("Vertex2", &WhiteBoxFace::m_v2)
                ->Field("Vertex3", &WhiteBoxFace::m_v3)
                ->Field("Normal", &WhiteBoxFace::m_normal);
        }
    }

    WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& inFaces)
    {
        // actual face count can be less than the original face count if any degenerate
        // faces are detected and removed
        const size_t inFaceCount = inFaces.size();
        size_t outFaceCount = 0;

        WhiteBoxFaces outFaces;

        // resize to the worst case number of faces
        outFaces.resize(inFaceCount);

        for (size_t inFaceIndex = 0; inFaceIndex < inFaceCount; ++inFaceIndex)
        {
            const WhiteBoxFace face = inFaces[inFaceIndex];
            const AZ::Vector3& vertex1 = face.m_v1.m_position;
            const AZ::Vector3& vertex2 = face.m_v2.m_position;
            const AZ::Vector3& vertex3 = face.m_v3.m_position;

            const auto areaSquared = 0.5f * ((vertex2 - vertex1).Cross(vertex3 - vertex1)).GetLengthSq();

            // only copy non-degenerate triangles (area is less than zero)
            if (areaSquared > DegenerateTriangleAreaSquareEpsilon)
            {
                outFaces[outFaceCount] = face;
                outFaceCount++;
            }
        }

        // now that we know the number of faces we need we can safely resize the vector without any
        // additional allocations as the number of faces will be equal to or less than the original size
        outFaces.resize(outFaceCount);

        return outFaces;
    }
} // namespace WhiteBox

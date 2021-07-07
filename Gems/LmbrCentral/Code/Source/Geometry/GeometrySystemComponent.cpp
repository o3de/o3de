/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral_precompiled.h>
#include "GeometrySystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral_precompiled.h>
#include <Shape/ShapeGeometryUtil.h>
#include <AzCore/std/containers/array.h>

namespace LmbrCentral
{
    void GeometrySystemComponent::Activate()
    {
        CapsuleGeometrySystemRequestBus::Handler::BusConnect();
    }

    void GeometrySystemComponent::Deactivate()
    {
        CapsuleGeometrySystemRequestBus::Handler::BusDisconnect();
    }

    /**
     * Generate vertices for triangles to make up a complete capsule.
     */
    static void GenerateSolidCapsuleMeshVertices(
        const float radius, const float height,
        const AZ::u32 sides, const AZ::u32 capSegments,
        AZ::Vector3* vertices)
    {
        const float middleHeight = AZ::GetMax(height - radius * 2.0f, 0.0f);
        const float halfMiddleHeight = middleHeight * 0.5f;

        vertices = CapsuleTubeUtil::GenerateSolidStartCap(
            AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(),
            radius, sides, capSegments, vertices);

        AZStd::array<float, 2> endPositions = { { -halfMiddleHeight, halfMiddleHeight } };
        for (size_t i = 0; i < endPositions.size(); ++i)
        {
            const AZ::Vector3 position = AZ::Vector3::CreateAxisZ(endPositions[i]);
            vertices = CapsuleTubeUtil::GenerateSegmentVertices(
                position, AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(),
                radius, sides, vertices);
        }

        CapsuleTubeUtil::GenerateSolidEndCap(
            AZ::Vector3::CreateAxisZ(halfMiddleHeight),
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(),
            radius, sides, capSegments, vertices);
    }

    /**
     * Generate vertices (via GenerateSolidCapsuleMeshVertices) and then build index
     * list for capsule shape for solid rendering.
     */
    static void GenerateSolidCapsuleMesh(
        const float radius, const float height, const AZ::u32 sides, const AZ::u32 capSegments,
        AZStd::vector<AZ::Vector3>& vertexBufferOut, AZStd::vector<AZ::u32>& indexBufferOut)
    {
        const AZ::u32 segments = 1;
        const AZ::u32 totalSegments = segments + capSegments * 2;

        const size_t numVerts = sides * (totalSegments + 1) + 2;
        const size_t numTriangles = (sides * totalSegments) * 2 + (sides * 2);

        vertexBufferOut.resize(numVerts);
        indexBufferOut.resize(numTriangles * 3);

        GenerateSolidCapsuleMeshVertices(
            radius, height, sides, capSegments, &vertexBufferOut[0]);

        CapsuleTubeUtil::GenerateSolidMeshIndices(
            sides, segments, capSegments, &indexBufferOut[0]);
    }

    /**
     * Generate full wire mesh for capsule (end caps, loops, and lines along length).
     */
    static void GenerateWireCapsuleMesh(
        const float radius, const float height, const AZ::u32 sides,
        const AZ::u32 capSegments, AZStd::vector<AZ::Vector3>& lineBufferOut)
    {
        // notes on vert buffer size
        // total end segments
        // 2 verts for each segment
        //  2 * capSegments for one full half arc
        //   2 arcs per end
        //    2 ends
        // total segments
        // 2 verts for each segment
        //  2 lines - top and bottom
        //   2 lines - left and right
        // total loops
        // 2 verts for each segment
        //  loops == sides
        //   1 extra segment needed for last loop
        //    1 extra segment needed for centre loop
        const AZ::u32 segments = 1;
        const AZ::u32 totalEndSegments = capSegments * 2 * 2 * 2 * 2;
        const AZ::u32 totalSegments = segments * 2 * 2 * 2;
        const AZ::u32 totalLoops = sides * 2 * (segments + 2);

        const size_t numVerts = totalEndSegments + totalSegments + totalLoops;
        lineBufferOut.resize(numVerts);

        const float middleHeight = AZ::GetMax(height - radius * 2.0f, 0.0f);
        const float halfMiddleHeight = middleHeight * 0.5f;

        // start cap
        AZ::Vector3* vertices = CapsuleTubeUtil::GenerateWireCap(
            AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
            -AZ::Vector3::CreateAxisZ(),
            AZ::Vector3::CreateAxisX(),
            radius,
            capSegments,
            &lineBufferOut[0]);

        // first loop
        vertices = CapsuleTubeUtil::GenerateWireLoop(
            AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), sides, radius, vertices);

        // centre loop
        vertices = CapsuleTubeUtil::GenerateWireLoop(
            AZ::Vector3::CreateZero(),
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), sides, radius, vertices);

        // body
        // left line
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), radius, 0.0f),
                vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), radius, 0.0f),
            vertices);
        // right line
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), radius, AZ::Constants::Pi),
            vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), radius, AZ::Constants::Pi),
            vertices);
        // top line
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), radius, 0.0f),
            vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), radius, 0.0f),
            vertices);
        // bottom line
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), radius, AZ::Constants::Pi),
            vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), radius, AZ::Constants::Pi),
            vertices);

        // final loop
        vertices = CapsuleTubeUtil::GenerateWireLoop(
            AZ::Vector3::CreateAxisZ(halfMiddleHeight), AZ::Vector3::CreateAxisZ(),
            AZ::Vector3::CreateAxisX(), sides, radius, vertices);

        // end cap
        CapsuleTubeUtil::GenerateWireCap(
            AZ::Vector3::CreateAxisZ(halfMiddleHeight),
            AZ::Vector3::CreateAxisZ(),
            AZ::Vector3::CreateAxisX(), radius,
            capSegments, vertices);
    }

    void GeometrySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GeometrySystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void GeometrySystemComponent::GenerateCapsuleMesh(
        const float radius, const float height,
        const AZ::u32 sides, const AZ::u32 capSegments,
        AZStd::vector<AZ::Vector3>& vertexBufferOut,
        AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut)
    {
        GenerateSolidCapsuleMesh(
            radius, height, sides, capSegments, vertexBufferOut, indexBufferOut);

        GenerateWireCapsuleMesh(
            radius, height, sides, capSegments, lineBufferOut);
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysXTestFixtures.h"
#include "EditorTestUtilities.h"
#include "PhysXTestCommon.h"

#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>

#include <Shape.h>
#include <Utils.h>

namespace PhysXEditorTests
{
    bool NormalPointsAwayFromPosition(const AZ::Vector3& vertexA, const AZ::Vector3& vertexB, const AZ::Vector3& vertexC, const AZ::Vector3& position)
    {
        const AZ::Vector3 edge1 = (vertexB - vertexA).GetNormalized();
        const AZ::Vector3 edge2 = (vertexC - vertexA).GetNormalized();
        const AZ::Vector3 normal = edge1.Cross(edge2);
        return normal.Dot(vertexA - position) >= 0.f;
    }

    bool TriangleWindingOrderIsValid(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Vector3& insidePosition)
    {
        if (!vertices.empty() && !indices.empty())
        {
            // use indices to construct triangles
            for (auto i = 0; i < indices.size(); i+=3)
            {
                const AZ::Vector3 vertexA = vertices[indices[i + 0]];
                const AZ::Vector3 vertexB = vertices[indices[i + 1]];
                const AZ::Vector3 vertexC = vertices[indices[i + 2]];
                if (!NormalPointsAwayFromPosition(vertexA, vertexB, vertexC, insidePosition))
                {
                    return false;
                }
            }
        }
        else if (!vertices.empty())
        {
            // assume a triangle list order
            for (auto i = 0; i < vertices.size(); i+=3)
            {
                const AZ::Vector3 vertexA = vertices[i + 0];
                const AZ::Vector3 vertexB = vertices[i + 1];
                const AZ::Vector3 vertexC = vertices[i + 2];
                if (!NormalPointsAwayFromPosition(vertexA, vertexB, vertexC, insidePosition))
                {
                    return false;
                }
            }
        }

        return true;
    }

    TEST_F(PhysXEditorFixture, BoxShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        const AZ::Vector3 boxDimensions(1.f,1.f,1.f);

        // Given there is a box shape
        auto physicsSystem = AZ::Interface<Physics::System>::Get();
        AZ_Assert(physicsSystem, "Physics System required");

        auto shape = physicsSystem->CreateShape(Physics::ColliderConfiguration(), 
            Physics::BoxShapeConfiguration(boxDimensions));
        
        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.

        // valid number of vertices and indices
        EXPECT_TRUE(vertices.size() == 8);

        // 6 sides, 2 triangles per side, 3 indices per triangle
        EXPECT_TRUE(indices.size() == 6 * 2 * 3);

        // all vertices are inside dimensions AABB
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        for (auto vertex : vertices)
        {
            bounds.AddPoint(vertex);
        }
        EXPECT_NEAR(bounds.GetXExtent(), boxDimensions.GetX(), AZ::Constants::Tolerance);
        EXPECT_NEAR(bounds.GetYExtent(), boxDimensions.GetY(), AZ::Constants::Tolerance);
        EXPECT_NEAR(bounds.GetZExtent(), boxDimensions.GetZ(), AZ::Constants::Tolerance);

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, SphereShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        // Given there is a sphere shape
        constexpr float radius = 1.f;
        auto shape = AZ::Interface<Physics::System>::Get()->CreateShape(Physics::ColliderConfiguration(), 
            Physics::SphereShapeConfiguration(radius));

        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // valid radius from center
        bool vertexRadiusIsValid = true;
        for (auto vertex : vertices)
        {
            if (!AZ::IsClose(vertex.GetLength(), radius))
            {
                vertexRadiusIsValid = false;
                break;
            }
        }
        EXPECT_TRUE(vertexRadiusIsValid);

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, CapsuleShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        // Given there is a shape
        constexpr float height = 1.f;
        constexpr float radius = .25f;
        auto shape = AZ::Interface<Physics::System>::Get()->CreateShape(Physics::ColliderConfiguration(), 
            Physics::CapsuleShapeConfiguration(height, radius));

        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // all vertices are inside dimensions AABB
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        for (auto vertex : vertices)
        {
            bounds.AddPoint(vertex);
        }
        constexpr float halfHeight = height * .5f;
        const AZ::Vector3 min(-radius,-radius,-halfHeight);
        const AZ::Vector3 max(radius,radius,halfHeight);
        const AZ::Aabb expectedBounds = AZ::Aabb::CreateFromMinMax(min, max);
        EXPECT_TRUE(expectedBounds.Contains(bounds));

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, ConvexHullShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        // Given there is a shape
        auto shape = PhysX::TestUtils::CreatePyramidShape(1.0f);

        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }

    TEST_F(PhysXEditorFixture, TriangleMeshShape_Geometry_IsValid_FT)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;
        auto physicsSystem = AZ::Interface<Physics::System>::Get();

        // Given there is a shape
        PhysX::VertexIndexData cubeMeshData = PhysX::TestUtils::GenerateCubeMeshData(3.0f);
        AZStd::vector<AZ::u8> cookedData;
        physicsSystem->CookTriangleMeshToMemory(
            cubeMeshData.first.data(), static_cast<AZ::u32>(cubeMeshData.first.size()),
            cubeMeshData.second.data(), static_cast<AZ::u32>(cubeMeshData.second.size()),
            cookedData);

        // Setup shape & collider configurations
        Physics::CookedMeshShapeConfiguration shapeConfig;
        shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(), 
            Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);

        // we need a valid triangle mesh shape
        auto shape = physicsSystem->CreateShape(Physics::ColliderConfiguration(), shapeConfig);

        // When geometry is requested
        shape->GetGeometry(vertices, indices);

        // Then valid geometry is returned.
        EXPECT_FALSE(vertices.empty());

        // valid winding order
        const AZ::Vector3 insidePosition = AZ::Vector3::CreateZero();
        EXPECT_TRUE(TriangleWindingOrderIsValid(vertices, indices, insidePosition));
    }
} // namespace UnitTest

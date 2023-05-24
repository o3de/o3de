/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <Editor/PolygonPrismMeshUtils.h>
#include <poly2tri.h>
#include <AzCore/Math/Geometry2DUtils.h>
#include <AzCore/Math/Vector3.h>

namespace PolygonPrismMeshUtils
{
    using PolygonPrismMeshUtilsTest = ::testing::Test;
    
    TEST_F(PolygonPrismMeshUtilsTest, CalculateAngles_ArbitraryTriangle_AnglesCorrect)
    {
        const float tolerance = 1e-3f;

        p2t::Point a(1.3, 2.7);
        p2t::Point b(1.7, 3.2);
        p2t::Point c(0.8, 2.9);
        p2t::Triangle triangle(a, b, c);

        const float expectedAngleA = atan2f(0.4f, 0.5f) + atan2f(0.5f, 0.2f);
        const float expectedAngleB = atan2f(0.5f, 0.4f) - atan2f(0.3f, 0.9f);
        const float expectedAngleC = atan2f(0.2f, 0.5f) + atan2f(0.3f, 0.9f);

        AZ::Vector3 angles = CalculateAngles(triangle);

        EXPECT_NEAR(angles.GetX(), expectedAngleA, tolerance);
        EXPECT_NEAR(angles.GetY(), expectedAngleB, tolerance);
        EXPECT_NEAR(angles.GetZ(), expectedAngleC, tolerance);
    }

    TEST_F(PolygonPrismMeshUtilsTest, CalculateAngles_DegenerateTriangles_AnglesSane)
    {
        // Test to ensure floating point precision issues are handled in CalculateAngles.
        // Test a series of triangles where the points are collinear, which in exact arithmetic should make two of the
        // angles 0 and the other 180, but might generate invalid floating point numbers if there are precision issues.
        // Multiple values are tested because it is hard to predict which values could lead to precision issues.
        const float epsilon = 1e-3f;
        for (double x = 0.1; x < 1.0; x += 0.1)
        {
            p2t::Point a(0.0, 0.0);
            p2t::Point b(0.2 * x, 0.2 * x);
            p2t::Point c(x, x);
            p2t::Triangle triangle(a, b, c);

            AZ::Vector3 angles = CalculateAngles(triangle);

            for (int elementIndex = 0; elementIndex < 3; elementIndex++)
            {
                const float elementValue = angles.GetElement(elementIndex);
                EXPECT_FALSE(AZStd::isnan(elementValue));
                EXPECT_GE(elementValue, -epsilon);
                EXPECT_LE(elementValue, AZ::Constants::Pi + epsilon);
            }
        }
    }

    Mesh2D CreateFromPolygon(const AZStd::vector<AZ::Vector2>& vertices)
    {
        AZStd::vector<p2t::Point> p2tVertices;
        std::vector<p2t::Point*> polyline;
        p2tVertices.reserve(vertices.size());
        polyline.reserve(vertices.size());

        int vertexIndex = 0;
        for (const AZ::Vector2& vert : vertices)
        {
            p2tVertices.emplace_back(p2t::Point(vert.GetX(), vert.GetY()));
            polyline.emplace_back(&(p2tVertices.data()[vertexIndex++]));
        }

        p2t::CDT constrainedDelaunayTriangulation(polyline);
        constrainedDelaunayTriangulation.Triangulate();
        Mesh2D mesh2D;
        mesh2D.CreateFromPoly2Tri(constrainedDelaunayTriangulation.GetTriangles());

        return mesh2D;
    }

    struct TestData
    {
        const AZStd::vector<AZ::Vector2> polygonHShape = {
            AZ::Vector2(0.0f, 0.0f),
            AZ::Vector2(0.0f, 3.0f),
            AZ::Vector2(1.0f, 3.0f),
            AZ::Vector2(1.0f, 2.0f),
            AZ::Vector2(2.0f, 2.0f),
            AZ::Vector2(2.0f, 3.0f),
            AZ::Vector2(3.0f, 3.0f),
            AZ::Vector2(3.0f, 0.0f),
            AZ::Vector2(2.0f, 0.0f),
            AZ::Vector2(2.0f, 1.0f),
            AZ::Vector2(1.0f, 1.0f),
            AZ::Vector2(1.0f, 0.0f)
        };

        const AZStd::vector<AZ::Vector2> pentagon = {
            AZ::Vector2(0.0f, 0.0f),
            AZ::Vector2(1.0f, 2.0f),
            AZ::Vector2(-1.0f, 3.0f),
            AZ::Vector2(-3.0f, 2.0f),
            AZ::Vector2(-2.0f, 0.0f)
        };
    };

    TEST_F(PolygonPrismMeshUtilsTest, CreateFromPoly2Tri_HShapedPolygon_ValidMesh)
    {
        TestData testData;
        const Mesh2D mesh2d = CreateFromPolygon(testData.polygonHShape);

        const AZStd::vector<Face>& faces = mesh2d.GetFaces();
        const size_t numFaces = faces.size();

        // the triangulation of an n-sided polygon should have n - 2 triangles
        // the H-shape has 12 sides, so we expect 10 faces in the triangulation
        EXPECT_EQ(numFaces, 10);

        // the number of internal edges should be one less than the number of triangles
        int numTwinnedHalfEdges = 0;

        for (const auto& face : faces)
        {
            // each face should be triangular
            EXPECT_EQ(face.m_numEdges, 3);
            EXPECT_FALSE(face.m_removed);

            HalfEdge* currentEdge = face.m_edge;
            for (int i = 0; i < 3; i++)
            {
                // the prev and next pointers for each half-edge should cycle correctly
                EXPECT_TRUE(currentEdge->m_next->m_prev == currentEdge);
                EXPECT_TRUE(currentEdge->m_prev->m_next == currentEdge);
                if (currentEdge->m_twin != nullptr)
                {
                    numTwinnedHalfEdges++;
                    EXPECT_TRUE(currentEdge->m_twin->m_twin == currentEdge);
                }
                currentEdge = currentEdge->m_next;
            }

            // after going round the whole face we should be back where we started
            EXPECT_TRUE(currentEdge == face.m_edge);
        }

        // there should be two half-edges for each internal edge
        EXPECT_EQ(numTwinnedHalfEdges, 18);

        const InternalEdgePriorityQueue& internalEdges = mesh2d.GetInternalEdges();

        EXPECT_EQ(internalEdges.size(), 9);
    }

    TEST_F(PolygonPrismMeshUtilsTest, CreateFromSimpleConvexPolygon_Pentagon_ValidMesh)
    {
        TestData testData;
        Mesh2D mesh2d;
        mesh2d.CreateFromSimpleConvexPolygon(testData.pentagon);

        const AZStd::vector<Face>& faces = mesh2d.GetFaces();
        const size_t numFaces = faces.size();

        // there should be a single, 5-sided face
        EXPECT_EQ(numFaces, 1);
        EXPECT_EQ(faces[0].m_numEdges, 5);
        EXPECT_FALSE(faces[0].m_removed);

        HalfEdge* currentEdge = faces[0].m_edge;
        for (int i = 0; i < 5; i++)
        {
            EXPECT_TRUE(currentEdge->m_origin.IsClose(testData.pentagon[i]));
            // the prev and next pointers for each half-edge should cycle correctly
            EXPECT_TRUE(currentEdge->m_next->m_prev == currentEdge);
            EXPECT_TRUE(currentEdge->m_prev->m_next == currentEdge);
            // none of the half-edges should have a twin
            EXPECT_TRUE(currentEdge->m_twin == nullptr);
            currentEdge = currentEdge->m_next;
        }

        // after going round the whole face we should be back where we started
        EXPECT_TRUE(currentEdge == faces[0].m_edge);
    }

    TEST_F(PolygonPrismMeshUtilsTest, RemoveInternalEdge_HShapedPolygonTriangulation_ValidMesh)
    {
        TestData testData;
        Mesh2D mesh2d = CreateFromPolygon(testData.polygonHShape);

        const AZStd::priority_queue<InternalEdge, AZStd::vector<InternalEdge>,
            InternalEdgeCompare>& internalEdges = mesh2d.GetInternalEdges();
        const InternalEdge& internalEdge = internalEdges.top();
        mesh2d.RemoveInternalEdge(internalEdge);

        const AZStd::vector<Face>& faces = mesh2d.GetFaces();
        const size_t numFaces = faces.size();

        // the triangulation of an n-sided polygon should have n - 2 triangles
        // the H-shape has 12 sides, so we expect 10 faces in the triangulation
        // after the edge removal, there should still be 10 faces, but one of them should be marked as removed
        EXPECT_EQ(numFaces, 10);

        // all the non-removed faces should still be valid
        int numRemovedFaces = 0;
        int numTwinnedHalfEdges = 0;
        for (const auto& face : faces)
        {
            if (face.m_removed)
            {
                numRemovedFaces++;
            }
            else
            {
                const int numEdges = face.m_numEdges;
                HalfEdge* currentEdge = face.m_edge;
                for (int edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
                {
                    EXPECT_TRUE(currentEdge->m_next->m_prev == currentEdge);
                    EXPECT_TRUE(currentEdge->m_prev->m_next == currentEdge);
                    if (currentEdge->m_twin != nullptr)
                    {
                        numTwinnedHalfEdges++;
                        EXPECT_TRUE(currentEdge->m_twin->m_twin == currentEdge);
                    }
                    currentEdge = currentEdge->m_next;
                }

                // after going round the whole face we should be back where we started
                EXPECT_TRUE(currentEdge == face.m_edge);
            }
        }

        // there should have been 18 twinned half-edges prior to the internal edge removal, and 2 should now have
        // been removed
        EXPECT_EQ(numTwinnedHalfEdges, 16);

        // one face should have been removed
        EXPECT_EQ(numRemovedFaces, 1);
    }

    TEST_F(PolygonPrismMeshUtilsTest, ConvexMerge_HShapedPolygon_ValidConvexDecomposition)
    {
        TestData testData;
        Mesh2D mesh2d = CreateFromPolygon(testData.polygonHShape);
        mesh2d.ConvexMerge();
        const AZStd::vector<Face>& faces = mesh2d.GetFaces();
        const size_t numFaces = faces.size();

        // the triangulation of an n-sided polygon should have n - 2 triangles
        // the H-shape has 12 sides, so we expect 10 faces in the triangulation
        // after the convex merge, there should still be 10 faces, but some of them should be marked as removed
        EXPECT_EQ(numFaces, 10);

        // all the non-removed faces should be valid and should be convex
        for (const auto& face : faces)
        {
            if (face.m_removed)
            {
                continue;
            }

            AZStd::vector<AZ::Vector2> vertices;
            HalfEdge* currentEdge = face.m_edge;
            int numEdges = face.m_numEdges;
            for (int edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
            {
                vertices.emplace_back(currentEdge->m_origin);

                EXPECT_TRUE(currentEdge->m_next->m_prev == currentEdge);
                EXPECT_TRUE(currentEdge->m_prev->m_next == currentEdge);
                if (currentEdge->m_twin != nullptr)
                {
                    EXPECT_TRUE(currentEdge->m_twin->m_twin == currentEdge);
                }

                currentEdge = currentEdge->m_next;
            }

            // after going round the whole face we should be back where we started
            EXPECT_TRUE(currentEdge == face.m_edge);

            // the origin vertices from the edges should form a simple convex polygon
            EXPECT_TRUE(AZ::Geometry2DUtils::IsSimplePolygon(vertices));
            EXPECT_TRUE(AZ::Geometry2DUtils::IsConvex(vertices));
        }
    }

    TEST_F(PolygonPrismMeshUtilsTest, GetDebugDrawPoints_HShapedPolygonDecomposition_SaneValues)
    {
        TestData testData;
        Mesh2D mesh2d = CreateFromPolygon(testData.polygonHShape);
        mesh2d.ConvexMerge();
        const float height = 1.5f;
        const AZ::Vector3 scale(0.2f);
        const AZStd::vector<AZ::Vector3>& debugDrawPoints = mesh2d.GetDebugDrawPoints(height, scale);

        // the points should appear in pairs, so there should be an even number of them
        EXPECT_EQ(debugDrawPoints.size() % 2, 0);

        // the H-shape has a bounding box from (0.0, 0.0) to (3.0, 3.0), so given the height and scale values
        // all the points should be inside a bounding box from (0.0, 0.0, 0.0) to (0.6, 0.6, 0.3)

        AZ::Vector3 min(1000.0f);
        AZ::Vector3 max = -min;

        for (const auto& point : debugDrawPoints)
        {
            min = point.GetMin(min);
            max = point.GetMax(max);
        }

        EXPECT_TRUE(min.IsClose(AZ::Vector3::CreateZero()));
        EXPECT_TRUE(max.IsClose(scale * AZ::Vector3(3.0f, 3.0f, height)));
    }
} // namespace PolygonPrismMeshUtils


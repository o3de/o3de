/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cmath>
#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"
#include "Util/WhiteBoxMeshUtil.h"
#include "Viewport/WhiteBoxShapeBuilders.h"
#include "Core/WhiteBoxCsgCore.h"
#include "Rendering/WhiteBoxRenderData.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <QApplication>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace UnitTest
{
    TEST(WhiteBoxTest, HandlesInitializedInvalid)
    {
        namespace Api = WhiteBox::Api;

        Api::VertexHandle vertexHandle;
        Api::FaceHandle faceHandle;
        Api::HalfedgeHandle halfedgeHandle;
        Api::EdgeHandle edgeHandle;

        EXPECT_FALSE(vertexHandle.IsValid());
        EXPECT_FALSE(faceHandle.IsValid());
        EXPECT_FALSE(halfedgeHandle.IsValid());
        EXPECT_FALSE(edgeHandle.IsValid());
    }

    TEST(WhiteBoxTest, VertexHandlesNotEqual)
    {
        namespace Api = WhiteBox::Api;

        Api::VertexHandle firstVertexHandle{1};
        Api::VertexHandle secondVertexHandle{2};

        EXPECT_TRUE(firstVertexHandle != secondVertexHandle);
        EXPECT_FALSE(firstVertexHandle == secondVertexHandle);
    }

    TEST(WhiteBoxTest, FaceHandlesNotEqual)
    {
        namespace Api = WhiteBox::Api;

        Api::FaceHandle firstFaceHandle{1};
        Api::FaceHandle secondFaceHandle{2};

        EXPECT_TRUE(firstFaceHandle != secondFaceHandle);
        EXPECT_FALSE(firstFaceHandle == secondFaceHandle);
    }

    TEST(WhiteBoxTest, HalfedgeHandlesNotEqual)
    {
        namespace Api = WhiteBox::Api;

        Api::HalfedgeHandle firstHalfedgeHandle{1};
        Api::HalfedgeHandle secondHalfedgeHandle{2};

        EXPECT_TRUE(firstHalfedgeHandle != secondHalfedgeHandle);
        EXPECT_FALSE(firstHalfedgeHandle == secondHalfedgeHandle);
    }

    TEST(WhiteBoxTest, EdgeHandlesNotEqual)
    {
        namespace Api = WhiteBox::Api;

        Api::EdgeHandle firstEdgeHandle{1};
        Api::EdgeHandle secondEdgeHandle{2};

        EXPECT_TRUE(firstEdgeHandle != secondEdgeHandle);
        EXPECT_FALSE(firstEdgeHandle == secondEdgeHandle);
    }

    TEST_F(WhiteBoxTestFixture, ClearRemovesMeshData)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::Clear(*m_whiteBox);

        const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
        const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
        const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
        const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);
        const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
        const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);

        EXPECT_EQ(faceCount, 0);
        EXPECT_EQ(faceHandles.size(), 0);
        EXPECT_EQ(vertexCount, 0);
        EXPECT_EQ(vertexHandles.size(), 0);
        EXPECT_EQ(halfedgeHandleCount, 0);
        EXPECT_EQ(polygonHandles.size(), 0);
    }

    TEST_F(WhiteBoxTestFixture, FirstFaceOfCubeIsTop)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const AZ::Vector3 normal = Api::FaceNormal(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(WhiteBoxTestFixture, FaceEdgeHandlesEmptyEdgeHandlesReturnedWithInvalidInput)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const Api::EdgeHandles edgeHandles = Api::FaceEdgeHandles(*m_whiteBox, Api::FaceHandle{});

        EXPECT_THAT(edgeHandles.empty(), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, FaceVertexHandlesEmptyVertexHandlesReturnedWithInvalidInput)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const Api::VertexHandles vertexHandles = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{});

        EXPECT_THAT(vertexHandles.empty(), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, ConnectedPolyFacesWithSameNormalReturned)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;

        AZStd::array<Api::VertexHandle, 8> vhandles;
        // verts must be added in CCW order
        vhandles[0] = Api::AddVertex(*m_whiteBox, AZ::Vector3(-1, 1, 0));
        vhandles[1] = Api::AddVertex(*m_whiteBox, AZ::Vector3(-2, 0, 0));
        vhandles[2] = Api::AddVertex(*m_whiteBox, AZ::Vector3(-1, -1, 0));
        vhandles[3] = Api::AddVertex(*m_whiteBox, AZ::Vector3(0, -3, 0));
        vhandles[4] = Api::AddVertex(*m_whiteBox, AZ::Vector3(1, -1, 0));
        vhandles[5] = Api::AddVertex(*m_whiteBox, AZ::Vector3(2, 0, 0));
        vhandles[6] = Api::AddVertex(*m_whiteBox, AZ::Vector3(1, 1, 0));
        vhandles[7] = Api::AddVertex(*m_whiteBox, AZ::Vector3(0, 3, 0));

        AZStd::vector<Api::FaceHandle> fhandles;
        for (size_t i = 1; i < vhandles.size() - 1; ++i)
        {
            // triangle fan topology setup
            fhandles.push_back(Api::AddFace(*m_whiteBox, vhandles[0], vhandles[i], vhandles[i + 1]));
        }

        Api::CalculateNormals(*m_whiteBox);
        Api::ZeroUVs(*m_whiteBox);

        const auto sideFaceHandles = Api::SideFaceHandles(*m_whiteBox, Api::FaceHandle{0});
        const auto sideVertexHandles = Api::SideVertexHandles(*m_whiteBox, Api::FaceHandle{0});
        const auto faceNormal = Api::FaceNormal(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(sideFaceHandles, ElementsAreArray(fhandles));
        EXPECT_THAT(sideVertexHandles, ElementsAreArray(vhandles));
        EXPECT_THAT(faceNormal, IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(WhiteBoxTestFixture, OutgoingHalfedgesFromVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // Note to future maintainers, a half-edge is an edge between two vertices, with a direction.
        // Think of it like an arrow starting at one vertex, and then pointing to another at its tip.
        // The other vertex then has a corresponding half edge, from itself, pointing back to the first vertex.

        // Given a vertex handle, you can ask Open Mesh to iterate over half-edges from that vertex.
        // If you ask it for "outgoing" half-edges, you'll get all of the ones which start at the given vertex and point at others.
        // If you ask it for "incoming" half-edges, you'll get all of the ones which point at the given vertex and start at others.

        // Most importantly, the "handle" of a half edge is an opaque integer that is implementation version specific and does not
        // impart meaning, similar to how the actual integer file handle returned when you open a user file using 'open' does not
        // impart meaning.  You can only use it to refer to the half-edge in functions which query other aspects, such as which faces
        // it is beside or which vertices it points at or comes from.  The acutal integer number is not guarinteed to remain in order
        // or to be stable.

        // Users, however to specify vertices, and in that case, the order matters. For this unit cube,
        // vertex 0 is at the corner of the top of the cube.
        // Vertex 0 has 5 outgoing half-edges, one for each of the 5 edges that connect from vertex 0 to other vertices as the model
        // is actually made of triangles (not quads), so each quad face is actually 2 triangles.
        // It has no 6th outgoing edge because the triangle-strip like zig-zag pattern which
        // the triangles are constructed from means that vertex 0 connects to 5 other vertices via edges but isn't the nexus of all
        // 3 cube sides it touches, as that would form a triangle fan around it instead of a strip.

        AZStd::vector<Api::HalfedgeHandle> outgoingHalfedgeHandles =
            Api::VertexOutgoingHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        ASSERT_EQ(outgoingHalfedgeHandles.size(), 5); // we are going to assume this going forward, so stop here if its not.

        for (const auto& halfedgeHandle : outgoingHalfedgeHandles)
        {
            // when we are iterating over half-edges, the "tail" is the vertex that the half-edge is coming from, so in this case,
            // because we asked it to get all the outgoing half-edges 0, that means 0 should be the tail of every such 'arrow'.
            auto tailHandle = Api::VertexHandle{ 0 };
            EXPECT_EQ(Api::HalfedgeVertexHandleAtTail(*m_whiteBox, halfedgeHandle), tailHandle);
        }

        // On the other hand, when we ask it for which vertex is at the tip of the edge, its the vertex on the other side of the
        // half-edge, from vertex 0 - We expect it to connect to exactly 5 other vertices since there are 5 half-edges and no duplicates.
        // Specifically the closest 5 from that corner in the cube.  Because the iterator always runs counter clockwise, this order is FIXED.
        const Api::VertexHandle expectedOtherSideVertexHandles[] = {
             Api::VertexHandle{ 7 },
             Api::VertexHandle{ 4 },
             Api::VertexHandle{ 1 },
             Api::VertexHandle{ 2 },
             Api::VertexHandle{ 3 }
        };

        ASSERT_EQ(AZ_ARRAY_SIZE(expectedOtherSideVertexHandles), outgoingHalfedgeHandles.size());
        for (size_t halfEdgeIndex = 0; halfEdgeIndex < outgoingHalfedgeHandles.size(); ++halfEdgeIndex)
        {
            EXPECT_EQ(Api::HalfedgeVertexHandleAtTip(*m_whiteBox, outgoingHalfedgeHandles[halfEdgeIndex]), expectedOtherSideVertexHandles[halfEdgeIndex]);
        }
    }

    TEST_F(WhiteBoxTestFixture, IncomingHalfedgesFromVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        AZStd::vector<Api::HalfedgeHandle> incomingHalfedgeHandles =
            Api::VertexIncomingHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        // see the note in the above check.
        ASSERT_EQ(incomingHalfedgeHandles.size(), 5); // we are going to assume this going forward, so stop here if its not.
        for (const auto& halfedgeHandle : incomingHalfedgeHandles)
        {
            // when we are iterating over half-edges, the "tip" is the vertex that the half-edge is pointing at,
            // because we asked it to get all the incoming half-edges pointing at vertex 0, we expect that all of them return vertex 0
            auto tailHandle = Api::VertexHandle{ 0 };
            EXPECT_EQ(Api::HalfedgeVertexHandleAtTip(*m_whiteBox, halfedgeHandle), tailHandle);
        }

        // On the other hand, when we ask it for which vertex is at the tail of the edge, its the one on the opposite side to vertex 0
        // this should be exactly 5 other vertices, in counter clockwise order (so it should be a fixed order).
        const Api::VertexHandle expectedOtherSideVertexHandles[] = {
            Api::VertexHandle{ 7 },
            Api::VertexHandle{ 4 },
            Api::VertexHandle{ 1 },
            Api::VertexHandle{ 2 },
            Api::VertexHandle{ 3 }
        };

        ASSERT_EQ(AZ_ARRAY_SIZE(expectedOtherSideVertexHandles), incomingHalfedgeHandles.size());
        for (size_t halfEdgeIndex = 0; halfEdgeIndex < incomingHalfedgeHandles.size(); ++halfEdgeIndex)
        {
            EXPECT_EQ(
                Api::HalfedgeVertexHandleAtTail(*m_whiteBox, incomingHalfedgeHandles[halfEdgeIndex]),
                expectedOtherSideVertexHandles[halfEdgeIndex]);
        }
    }

    TEST_F(WhiteBoxTestFixture, AllHalfedgesFromVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // see the note on the previous 2 functions.  This one gets ALL of the half edges related to a vertex
        // both incoming and outgiong.  Handles are opaque data types, so its unreliable to expect the handles
        // to be any particular value.  However, each handle should point at vertex 0 at its head or tail
        // and there is always a fixed number of them (5 incoming, 5 outgoing = 10 total).
        // However, the iterator returns them in counter clockwise order, so the verts they connect to on each end are predictable.
        AZStd::vector<Api::HalfedgeHandle> allHalfedgeHandles =
            Api::VertexHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        ASSERT_EQ(allHalfedgeHandles.size(), 10);

        // notice that when we ask for all halfedges, we get 5 incoming and 5 outgoing
        // and it gets the outgoing first (so the tip will be all the other verts)
        // then the incoming, so the tip will all be the vert we asked for, which is zero.
        const Api::VertexHandle expectedTipVertexHandles[] = {
            Api::VertexHandle{ 7 },
            Api::VertexHandle{ 4 },
            Api::VertexHandle{ 1 },
            Api::VertexHandle{ 2 },
            Api::VertexHandle{ 3 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 } };

        // Similarly when we ask for all halfedges, the tail of each one is going be the outgoing
        // first, so all 0's (since we asked for 0's halfedges), and then the incoming, so the other verts.
        const Api::VertexHandle expectedTailVertexHandles[] = {
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 7 },
            Api::VertexHandle{ 4 },
            Api::VertexHandle{ 1 },
            Api::VertexHandle{ 2 },
            Api::VertexHandle{ 3 } };

        AZStd::vector<Api::VertexHandle> actualTipVertexHandles;
        AZStd::vector<Api::VertexHandle> actualTailVertexHandles;

        ASSERT_EQ(AZ_ARRAY_SIZE(expectedTipVertexHandles),  allHalfedgeHandles.size());
        ASSERT_EQ(AZ_ARRAY_SIZE(expectedTailVertexHandles), allHalfedgeHandles.size());

        for (size_t halfEdgeIndex = 0; halfEdgeIndex < allHalfedgeHandles.size(); ++halfEdgeIndex)
        {
            actualTipVertexHandles.push_back(Api::HalfedgeVertexHandleAtTip(*m_whiteBox, allHalfedgeHandles[halfEdgeIndex]));
            actualTailVertexHandles.push_back(Api::HalfedgeVertexHandleAtTail(*m_whiteBox, allHalfedgeHandles[halfEdgeIndex]));
        }

        EXPECT_THAT(actualTipVertexHandles, ElementsAreArray(expectedTipVertexHandles, std::size(expectedTipVertexHandles)));
        EXPECT_THAT(actualTailVertexHandles, ElementsAreArray(expectedTailVertexHandles, std::size(expectedTailVertexHandles)));
    }

    TEST_F(WhiteBoxTestFixture, VerticesForFace)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexHandlesForFace = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        const Api::VertexHandle expectedVertexHandles[] = {
            Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}};

        EXPECT_THAT(vertexHandlesForFace, ElementsAreArray(expectedVertexHandles, std::size(expectedVertexHandles)));
    }

    TEST_F(WhiteBoxTestFixture, SideHalfedgesForFace)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto sideHalfEdgeHandlesCollection = Api::SideBorderHalfedgeHandles(*m_whiteBox, Api::FaceHandle{1});

        const Api::HalfedgeHandle expectedHalfedgeHandles[] = {
            Api::HalfedgeHandle{2}, Api::HalfedgeHandle{6}, Api::HalfedgeHandle{8}, Api::HalfedgeHandle{0}};

        EXPECT_THAT(sideHalfEdgeHandlesCollection.size(), Eq(1));
        EXPECT_THAT(
            sideHalfEdgeHandlesCollection.front(),
            ElementsAreArray(expectedHalfedgeHandles, std::size(expectedHalfedgeHandles)));
    }

    TEST_F(WhiteBoxTestFixture, VerticesOrderedForSide)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexHandlesCollection = Api::SideBorderVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        const Api::VertexHandle vhs[] = {
            Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}, Api::VertexHandle{3}};

        EXPECT_THAT(vertexHandlesCollection.size(), Eq(1));
        EXPECT_THAT(vertexHandlesCollection.front(), ElementsAreArray(vhs, std::size(vhs)));
    }

    TEST_F(WhiteBoxTestFixture, VertexPositionsFromFaceHandle)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        AZStd::vector<AZ::Vector3> vertices = {
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 1.0f, 0.0f)};

        Api::VertexHandles vhs;
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[0]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[1]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[2]));

        auto faceHandle = Api::AddFace(*m_whiteBox, vhs[0], vhs[1], vhs[2]);

        EXPECT_THAT(vertices, Pointwise(ContainerIsClose(), Api::FaceVertexPositions(*m_whiteBox, faceHandle)));
    }

    TEST_F(WhiteBoxTestFixture, VertexPositionsFromPolygonHandle)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::Pointwise;

        AZStd::vector<AZ::Vector3> vertices = {
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 1.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f)};

        Api::VertexHandles vhs;
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[0]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[1]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[2]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[3]));

        Api::PolygonHandle polygonHandle{Api::FaceHandles{
            Api::AddFace(*m_whiteBox, vhs[0], vhs[1], vhs[2]), Api::AddFace(*m_whiteBox, vhs[0], vhs[2], vhs[3])}};

        const auto vertexPositions = Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        EXPECT_THAT(vertices.size(), Eq(vertexPositions.size()));
        EXPECT_THAT(vertices, Pointwise(ContainerIsClose(), vertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, VertexPositionsFromVertexHandles)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        AZStd::vector<AZ::Vector3> vertices = {AZ::Vector3(0, 0, 0), AZ::Vector3(1, 0, 0), AZ::Vector3(1, 1, 0)};

        Api::VertexHandles vhs;
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[0]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[1]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[2]));

        Api::AddFace(*m_whiteBox, vhs[0], vhs[1], vhs[2]);

        EXPECT_THAT(vertices, Pointwise(ContainerIsClose(), Api::VertexPositions(*m_whiteBox, vhs)));
    }

    TEST_F(WhiteBoxTestFixture, PolygonHandlesFromUnitQuad)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);

        EXPECT_EQ(polygonHandles.size(), 1);
    }

    TEST_F(WhiteBoxTestFixture, PolygonHandlesFromUnitCube)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);

        EXPECT_EQ(polygonHandles.size(), 6);
    }

    TEST_F(WhiteBoxTestFixture, UniqueVertexPositionsFromUnitQuad)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto vertexPositions =
            Api::PolygonVertexPositions(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}));

        EXPECT_EQ(vertexPositions.size(), 4);
    }

    TEST_F(WhiteBoxTestFixture, MultiplePolygonExtrusions)
    {
        namespace Api = WhiteBox::Api;
        using Vh = Api::VertexHandle;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{7}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{14}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{17}), 1.0f);

        const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);
        const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
        const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
        const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
        const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
        const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);

        EXPECT_EQ(polygonHandles.size(), 22);
        EXPECT_EQ(faceCount, 44);
        EXPECT_EQ(faceHandles.size(), 44);
        EXPECT_EQ(vertexCount, 24);
        EXPECT_EQ(vertexHandles.size(), 24);
        EXPECT_THAT(vertexHandles, ElementsAreArray({Vh{0},  Vh{1},  Vh{2},  Vh{3},  Vh{4},  Vh{5},  Vh{6},  Vh{7},
                                                     Vh{8},  Vh{9},  Vh{10}, Vh{11}, Vh{12}, Vh{13}, Vh{14}, Vh{15},
                                                     Vh{16}, Vh{17}, Vh{18}, Vh{19}, Vh{20}, Vh{21}, Vh{22}, Vh{23}}));
        EXPECT_EQ(halfedgeHandleCount, 132);
    }

    TEST_F(WhiteBoxTestFixture, PolygonExtrusionEmptyWithEmptyMesh)
    {
        namespace Api = WhiteBox::Api;

        const auto polygon = Api::TranslatePolygonAppend(*m_whiteBox, Api::PolygonHandle{}, 1.0f);

        EXPECT_EQ(polygon.m_faceHandles.size(), 0);
    }

    TEST_F(WhiteBoxTestFixture, MeshSerializedAndDeserialized)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{15}), 1.0f);

        {
            const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);
            const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
            const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
            const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
            const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
            const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);

            EXPECT_EQ(polygonHandles.size(), 14);
            EXPECT_EQ(faceCount, 28);
            EXPECT_EQ(faceHandles.size(), 28);
            EXPECT_EQ(vertexCount, 16);
            EXPECT_EQ(vertexHandles.size(), 16);
            EXPECT_EQ(halfedgeHandleCount, 84);
        }

        AZStd::vector<AZ::u8> whiteBoxMeshData;
        Api::WriteMesh(*m_whiteBox, whiteBoxMeshData);

        m_whiteBox.reset();
        m_whiteBox = Api::CreateWhiteBoxMesh();

        Api::ReadMesh(*m_whiteBox, whiteBoxMeshData);

        {
            const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);
            const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
            const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
            const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
            const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
            const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);

            EXPECT_EQ(polygonHandles.size(), 14);
            EXPECT_EQ(faceCount, 28);
            EXPECT_EQ(faceHandles.size(), 28);
            EXPECT_EQ(vertexCount, 16);
            EXPECT_EQ(vertexHandles.size(), 16);
            EXPECT_EQ(halfedgeHandleCount, 84);
        }
    }

    TEST_F(WhiteBoxTestFixture, MeshNotDeserializedWithSkipWhiteSpaceStream)
    {
        namespace Api = WhiteBox::Api;
        using testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);
        AZStd::vector<AZ::u8> serializedWhiteBox;
        Api::WriteMesh(*m_whiteBox, serializedWhiteBox);

        std::string serializedWhiteBoxStr;
        serializedWhiteBoxStr.reserve(serializedWhiteBox.size());
        AZStd::copy(
            serializedWhiteBox.cbegin(), serializedWhiteBox.cend(), AZStd::back_inserter(serializedWhiteBoxStr));

        std::stringstream whiteBoxStream;
        whiteBoxStream.str(serializedWhiteBoxStr);
        // note: std::stringstream will default to skip white space characters

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_THAT(Api::ReadMesh(*m_whiteBox, whiteBoxStream), Eq(Api::ReadResult::Error));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(WhiteBoxTestFixture, InitialiseAsUnitQuad)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        AZStd::vector<AZ::Vector3> vertexPositions = Api::MeshVertexPositions(*m_whiteBox);
        AZStd::vector<AZ::Vector3> expectedVertexPositions = {
            AZ::Vector3(-0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, 0.5f),
            AZ::Vector3(-0.5f, 0.0f, 0.5f)};

        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), expectedVertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, MeshScalePolygon)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto midpoint = Api::PolygonMidpoint(*m_whiteBox, polygonHandle);

        Api::ScalePolygonRelative(*m_whiteBox, polygonHandle, midpoint, 0.5f);

        const auto vertexPositions = Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        // result of scaling each vertex by 0.5 towards the midpoint of the quad
        const AZStd::vector<AZ::Vector3> scaledUnitQuad = {
            AZ::Vector3(-0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, 0.75f),
            AZ::Vector3(-0.75f, 0.0f, 0.75f)};

        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), scaledUnitQuad));
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), 1);
    }

    TEST_F(WhiteBoxTestFixture, MeshScalePolygonAppend)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        polygonHandle = Api::ScalePolygonAppendRelative(*m_whiteBox, polygonHandle, 0.5f);

        const auto vertexPositions = Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        // result of scaling each vertex by 0.5 towards the midpoint of the quad
        const AZStd::vector<AZ::Vector3> scaledUnitQuad = {
            AZ::Vector3(-0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, 0.75f),
            AZ::Vector3(-0.75f, 0.0f, 0.75f)};

        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), scaledUnitQuad));
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), 5);
    }

    TEST_F(WhiteBoxTestFixture, MeshPolygonUniqueVertexHandles)
    {
        namespace Api = WhiteBox::Api;
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto vertexHandles = Api::PolygonVertexHandles(*m_whiteBox, polygonHandle);
        EXPECT_EQ(vertexHandles.size(), 4);
    }

    TEST_F(WhiteBoxTestFixture, MeshMidPointOfPolygon)
    {
        namespace Api = WhiteBox::Api;
        auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        EXPECT_THAT(AZ::Vector3::CreateZero(), IsClose(Api::PolygonMidpoint(*m_whiteBox, polygonHandle)));
    }

    TEST_F(WhiteBoxTestFixture, MeshMidPointOfEdge)
    {
        namespace Api = WhiteBox::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto edgeHandles = Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandle);

        for (const auto& edgeHandle : edgeHandles)
        {
            // when
            const auto tail = Api::HalfedgeVertexPositionAtTail(
                *m_whiteBox, Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First));
            const auto tip = Api::HalfedgeVertexPositionAtTip(
                *m_whiteBox, Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First));

            // computed differently to EdgeMidpoint
            const auto midpoint = tail + (tip - tail) * 0.5f;

            // then
            EXPECT_THAT(midpoint, IsClose(Api::EdgeMidpoint(*m_whiteBox, edgeHandle)));
        }
    }

    TEST_F(WhiteBoxTestFixture, MeshMidPointOfFace)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto faceMidpoint = Api::FaceMidpoint(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(faceMidpoint, IsClose(AZ::Vector3(0.1666f, -0.1666f, 0.5f)));
    }

    TEST_F(WhiteBoxTestFixture, MeshFacesReturned)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto faces = Api::MeshFaces(*m_whiteBox);

        EXPECT_EQ(faces.size(), 2);

        const AZStd::vector<AZ::Vector3> vertexPositions = {
            AZ::Vector3(-0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, 0.5f),
            AZ::Vector3(-0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, 0.5f),  AZ::Vector3(-0.5f, 0.0f, 0.5f),
        };

        AZStd::vector<AZ::Vector3> faceVertexPositions;
        faceVertexPositions.insert(faceVertexPositions.end(), faces[0].begin(), faces[0].end());
        faceVertexPositions.insert(faceVertexPositions.end(), faces[1].begin(), faces[1].end());

        EXPECT_THAT(faceVertexPositions, Pointwise(ContainerIsClose(), vertexPositions));
    }

    // note: here we sum and then normalize unit normals of each face (the normals are not weighted)
    TEST_F(WhiteBoxTestFixture, PolygonNormalIsAverageOfFaces)
    {
        namespace Api = WhiteBox::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        const auto vertexHandles = Api::PolygonVertexHandles(*m_whiteBox, polygonHandle);
        const auto vertexPositions = Api::VertexPositions(*m_whiteBox, vertexHandles);

        // update the position of a single vertex to make the faces in the polygon not co-planar
        Api::SetVertexPosition(*m_whiteBox, vertexHandles[0], vertexPositions[0] + AZ::Vector3::CreateAxisY());

        // ensure we refresh normals after modifications
        Api::CalculateNormals(*m_whiteBox);

        // when
        const auto polygonNormal = PolygonNormal(*m_whiteBox, polygonHandle);

        // then
        const auto faceNormal22 = FaceNormal(*m_whiteBox, polygonHandle.m_faceHandles[0]);
        const auto faceNormal23 = FaceNormal(*m_whiteBox, polygonHandle.m_faceHandles[1]);
        const auto averageNormal = (faceNormal22 + faceNormal23).GetNormalized();

        EXPECT_THAT(polygonNormal, IsClose(averageNormal));
    }

    TEST_F(WhiteBoxTestFixture, PolygonTranslateAlongNormal)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        // given
        // use default position (Y is at origin)
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        // when
        Api::TranslatePolygon(*m_whiteBox, polygonHandle, 1.0f);

        AZStd::vector<AZ::Vector3> expectedVertexPositions = {
            AZ::Vector3(-0.5f, -1.0f, -0.5f), AZ::Vector3(0.5f, -1.0f, -0.5f), AZ::Vector3(0.5f, -1.0f, 0.5f),
            AZ::Vector3(-0.5f, -1.0f, 0.5f)};

        const AZStd::vector<AZ::Vector3> polygonVertexPositions =
            Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        // then
        EXPECT_THAT(expectedVertexPositions, Pointwise(ContainerIsClose(), polygonVertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, SpaceCreatedForPolygonIsOrthogonal)
    {
        namespace Api = WhiteBox::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        // when
        const AZ::Vector3 polygonMidpoint = Api::PolygonMidpoint(*m_whiteBox, polygonHandle);
        const AZ::Transform polygonSpace = Api::PolygonSpace(*m_whiteBox, polygonHandle, polygonMidpoint);

        // then
        EXPECT_TRUE(polygonSpace.IsOrthogonal());
    }

    TEST_F(WhiteBoxTestFixture, SpaceCreatedForEdgeIsOrthogonal)
    {
        namespace Api = WhiteBox::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto edgeHandles = Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandle);

        // when
        const AZ::Vector3 edgeMidpoint = Api::EdgeMidpoint(*m_whiteBox, edgeHandles[0]);
        const AZ::Transform edgeSpace = Api::EdgeSpace(*m_whiteBox, edgeHandles[0], edgeMidpoint);

        // then
        EXPECT_TRUE(edgeSpace.IsOrthogonal());
    }

    TEST_F(WhiteBoxTestFixture, EdgeToHalfEdgeConversionsMapCorrectly)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        // given
        const auto edgeHandles = Api::MeshEdgeHandles(*m_whiteBox);
        for (const auto& edgeHandle : edgeHandles)
        {
            const auto firstHalfedgeHandle = Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First);
            const auto secondHalfedgeHandle =
                Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First);

            // when
            const auto edgeHandleFromFirst = Api::HalfedgeEdgeHandle(*m_whiteBox, firstHalfedgeHandle);
            const auto edgeHandleFromSecond = Api::HalfedgeEdgeHandle(*m_whiteBox, secondHalfedgeHandle);

            // then
            EXPECT_EQ(edgeHandle, edgeHandleFromFirst);
            EXPECT_EQ(edgeHandle, edgeHandleFromSecond);
        }
    }

    class WhiteBoxTestUpdateVerticesFixture : public WhiteBoxTestFixture
    {
    public:
        WhiteBoxTestUpdateVerticesFixture()
            : WhiteBoxTestFixture()
        {
        }

        void SetUpEditorFixtureImpl() override
        {
            namespace Api = WhiteBox::Api;

            Api::InitializeAsUnitCube(*m_whiteBox);

            // triangle A of the unit cube's left face tri pair
            WhiteBox::Api::FaceHandle leftFaceHandle = WhiteBox::Api::FaceHandle{0};

            // triangle A of the unit cube's top face tri pair
            WhiteBox::Api::FaceHandle topFaceHandle = WhiteBox::Api::FaceHandle{10};

            // top face polygon comprised of triangles A and B
            WhiteBox::Api::PolygonHandle topFacePolyHandle =
                WhiteBox::Api::FacePolygonHandle(*m_whiteBox, topFaceHandle);

            Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, leftFaceHandle), 1.0f);

            m_polygonVertexHandles = Api::PolygonVertexHandles(*m_whiteBox, topFacePolyHandle);

            const auto vertexPositions = Api::VertexPositions(*m_whiteBox, m_polygonVertexHandles);

            // translate top face upwards one unit
            size_t index = 0;
            for (auto vertexHandle : m_polygonVertexHandles)
            {
                Api::SetVertexPositionAndUpdateUVs(
                    *m_whiteBox, vertexHandle, vertexPositions[index++] + AZ::Vector3::CreateAxisZ());
            }
        }

        void TearDownEditorFixtureImpl()
        {
            // ensure we deallocate memory for the vector before the allocator is destroyed
            m_polygonVertexHandles = {};
        }

        WhiteBox::Api::VertexHandles m_polygonVertexHandles;
    };

    TEST_F(WhiteBoxTestUpdateVerticesFixture, MeshCanUpdateVertexPositions)
    {
        namespace Api = WhiteBox::Api;

        const AZ::Vector3 updatedVertexPositions[] = {
            AZ::Vector3{-0.5f, -0.5f, 2.5f}, AZ::Vector3{0.5f, -0.5f, 2.5f}, AZ::Vector3{0.5f, 0.5f, 2.5f},
            AZ::Vector3{-0.5f, 0.5f, 2.5f}};

        size_t index = 0;
        for (auto vertexHandle : m_polygonVertexHandles)
        {
            const auto vertexPosition = Api::VertexPosition(*m_whiteBox, vertexHandle);
            EXPECT_THAT(vertexPosition, IsClose(updatedVertexPositions[index++]));
        }
    }

    TEST_F(WhiteBoxTestUpdateVerticesFixture, MeshCanUpdateVertexUVs)
    {
        namespace Api = WhiteBox::Api;

        // iterate over all vertex handles associated with a polygon and get all outgoing
        // half edges - check the uvs at each halfedge at the outer edge of the extruded
        // face (halfedges are on the lateral faces, opposite of halfedges on the extruded
        // polygon/face) - we lookup the uv from the halfedge handle and verify the tiling
        using Heh = Api::HalfedgeHandle;
        const Heh topHalfedgeHandles[] = {Heh{35}, Heh{37}, Heh{41}, Heh{43}};

        // expected uv coordinates given the z-axis translation applied to the top face
        const AZ::Vector2 expectedUVs[] = {
            AZ::Vector2(-2.0f, 0.0f), AZ::Vector2(1.0f, -2.0f), AZ::Vector2(-2.0f, 1.0f), AZ::Vector2(0.0f, -2.0f)};

        unsigned i = 0;
        for (auto vertexHandle : m_polygonVertexHandles)
        {
            const auto outgoingHalfedges = Api::VertexOutgoingHalfedgeHandles(*m_whiteBox, vertexHandle);
            for (auto halfedgeHandle : outgoingHalfedges)
            {
                auto halfedgeHandleIt =
                    AZStd::find(std::begin(topHalfedgeHandles), std::end(topHalfedgeHandles), halfedgeHandle);

                if (halfedgeHandleIt != std::end(topHalfedgeHandles))
                {
                    const AZ::Vector2 uv = Api::HalfedgeUV(*m_whiteBox, halfedgeHandle);
                    EXPECT_THAT(uv, IsClose(expectedUVs[i++]));
                }
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeAppendedToWhiteBoxCubeConnectedByQuadPolygons)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        const auto polygonCountBefore = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountBefore = Api::MeshFaceCount(*m_whiteBox);

        const auto nextEdgeHandle =
            Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{1}, AZ::Vector3{-0.5f, 0.0f, 0.5f});

        const auto edgeMidpoint = Api::EdgeMidpoint(*m_whiteBox, nextEdgeHandle);
        const auto polygonCountAfter = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountAfter = Api::MeshFaceCount(*m_whiteBox);

        EXPECT_THAT(nextEdgeHandle, Eq(Api::EdgeHandle{19}));
        EXPECT_THAT(edgeMidpoint, IsClose(AZ::Vector3{0.0f, 0.0f, 1.0f}));
        EXPECT_THAT(polygonCountAfter - polygonCountBefore, Eq(3));
        EXPECT_THAT(faceCountAfter - faceCountBefore, Eq(4));
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeAppendedToWhiteBoxCubeConnectedByTriPolygons)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        // quad edge extrusion - same as EdgeCanBeAppendedToWhiteBoxCubeConnectedByTwoQuadPolygons
        Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{1}, AZ::Vector3{-0.5f, 0.0f, 0.5f});

        const auto polygonCountBefore = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountBefore = Api::MeshFaceCount(*m_whiteBox);

        // triangle edge extrusion
        const auto nextEdgeHandle =
            Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{0}, AZ::Vector3{0.0f, -0.25f, 0.25f});

        const auto edgeMidpoint = Api::EdgeMidpoint(*m_whiteBox, nextEdgeHandle);
        const auto polygonCountAfter = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountAfter = Api::MeshFaceCount(*m_whiteBox);

        EXPECT_THAT(nextEdgeHandle, Eq(Api::EdgeHandle{26}));
        EXPECT_THAT(edgeMidpoint, IsClose(AZ::Vector3{0.0f, -0.75f, 0.75f}));
        EXPECT_THAT(polygonCountAfter - polygonCountBefore, Eq(3));
        EXPECT_THAT(faceCountAfter - faceCountBefore, Eq(4));
    }

    TEST_F(WhiteBoxTestFixture, HidingEdgeCreatesNewPolygonHandleWithCombinedFaceHandles)
    {
        namespace Api = WhiteBox::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{7}), 1.0f);

        const auto beforeHidePolygonHandleFromFaceHandle_0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto beforeHidePolygonHandleFromFaceHandle_1 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1});
        const auto beforeHidePolygonHandleFromFaceHandle_16 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16});
        const auto beforeHidePolygonHandleFromFaceHandle_17 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{17});

        // hide top edge
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{1});

        const auto afterHidePolygonHandleFromFaceHandle_0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto afterHidePolygonHandleFromFaceHandle_1 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1});
        const auto afterHidePolygonHandleFromFaceHandle_16 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16});
        const auto afterHidePolygonHandleFromFaceHandle_17 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{17});

        const auto beforeHidePolygonHandlesFaceHandle_0_1_Expected =
            Api::PolygonHandle{Api::FaceHandles{Api::FaceHandle{0}, Api::FaceHandle{1}}};
        const auto beforeHidePolygonHandlesFaceHandle_16_17_Expected =
            Api::PolygonHandle{Api::FaceHandles{Api::FaceHandle{16}, Api::FaceHandle{17}}};

        // two separate top polygons after append/extrusion
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_0, beforeHidePolygonHandlesFaceHandle_0_1_Expected);
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_1, beforeHidePolygonHandlesFaceHandle_0_1_Expected);
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_16, beforeHidePolygonHandlesFaceHandle_16_17_Expected);
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_17, beforeHidePolygonHandlesFaceHandle_16_17_Expected);

        const auto afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected = Api::PolygonHandle{
            Api::FaceHandles{{Api::FaceHandle{0}, Api::FaceHandle{1}, Api::FaceHandle{16}, Api::FaceHandle{17}}}};

        // single top polygon after hiding edge
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_0, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_1, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_16, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_17, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
    }

    TEST_F(WhiteBoxTestFixture, FlipInternalEdgeOfQuadSucceeds)
    {
        namespace Api = WhiteBox::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitQuad(*m_whiteBox);

        const auto beforeFlipVertexHandles =
            AZStd::array<Api::VertexHandle, 2>{Api::VertexHandle{2}, Api::VertexHandle{0}};
        const auto afterFlipVertexHandles =
            AZStd::array<Api::VertexHandle, 2>{Api::VertexHandle{3}, Api::VertexHandle{1}};

        const auto beforeFlipExpectedVertexHandles = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{2});
        EXPECT_EQ(beforeFlipExpectedVertexHandles, beforeFlipVertexHandles);

        // flip diagonal edge
        bool result = Api::FlipEdge(*m_whiteBox, Api::EdgeHandle{2});

        EXPECT_TRUE(result);

        const auto afterFlipExpectedVertexHandles = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{2});
        EXPECT_EQ(afterFlipExpectedVertexHandles, afterFlipVertexHandles);
    }

    TEST_F(WhiteBoxTestFixture, FlipOuterEdgeOfQuadReturnsFalse)
    {
        namespace Api = WhiteBox::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitQuad(*m_whiteBox);

        // attempt to flip outer edge
        bool result = Api::FlipEdge(*m_whiteBox, Api::EdgeHandle{0});

        EXPECT_EQ(result, false);
    }

    TEST_F(WhiteBoxTestFixture, FlipVisibleEdgeReturnsFalse)
    {
        namespace Api = WhiteBox::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitQuad(*m_whiteBox);

        {
            Api::EdgeHandles restoringEdgeHandles;
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{2}, restoringEdgeHandles);
        }

        // attempt to flip outer edge
        bool result = Api::FlipEdge(*m_whiteBox, Api::EdgeHandle{2});

        EXPECT_EQ(result, false);
    }

    TEST_F(WhiteBoxTestFixture, EdgeCannotBeAppendedWhenPolygonHasMoreThanTwoFaces)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        const auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        // quad face extrusion
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{7}), 1.0f);
        // hide top edge
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{1});

        const auto polygonCountBefore = Api::MeshPolygonHandles(*m_whiteBox).size();

        // attempt to perform an edge append
        const auto nextEdgeHandle =
            Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{20}, AZ::Vector3{-0.5f, 0.0f, 0.5f});

        const auto polygonCountAfter = Api::MeshPolygonHandles(*m_whiteBox).size();

        // same edge handle is returned, no append/extrusion is performed
        EXPECT_THAT(nextEdgeHandle, Eq(Api::EdgeHandle{20}));
        EXPECT_THAT(polygonCountBefore, Eq(polygonCountAfter));
    }

    TEST_F(WhiteBoxTestFixture, PolygonCannotBeAppendedWithNoEdges)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        const auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        // hide all 'logical'/'visible' edges (those that define the bounds of a polygon)
        for (const auto& edgeHandle :
             {Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{4}, Api::EdgeHandle{0}, Api::EdgeHandle{6}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        const auto polygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto polygonCount = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto borderPolygonVertexHandlesCollection = Api::PolygonBorderVertexHandles(*m_whiteBox, polygonHandle);

        // attempt appending a polygon
        const auto nextPolygonHandle =
            Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), 1.0f);

        // mesh is unchanged, polygon count is as before, same polygon handle is returned
        EXPECT_THAT(polygonCount, Eq(1));
        EXPECT_THAT(nextPolygonHandle, Eq(polygonHandle));
        EXPECT_THAT(borderPolygonVertexHandlesCollection, Eq(Api::VertexHandlesCollection{}));
    }

    TEST_F(WhiteBoxTestFixture, MeshReturnsBothBordersOfPolygonWithHole)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAreArray;

        const auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        const auto polygonHandle = Api::ScalePolygonAppendRelative(
            *m_whiteBox, Api::PolygonHandle{Api::FaceHandles{{Api::FaceHandle{4}, Api::FaceHandle{5}}}}, -0.25f);

        // hide all 'logical'/'visible' edges for scale appended face
        for (const auto& edgeHandle : {Api::EdgeHandle{25}, Api::EdgeHandle{27}, Api::EdgeHandle{24}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        const auto expectedLoopFaceHandles =
            Api::FaceHandles{Api::FaceHandle{16}, Api::FaceHandle{17}, Api::FaceHandle{14}, Api::FaceHandle{15},
                             Api::FaceHandle{12}, Api::FaceHandle{13}, Api::FaceHandle{19}, Api::FaceHandle{18}};

        const auto expectedFirstBorderVertexHandles = Api::VertexHandles{
            Api::VertexHandle{11}, Api::VertexHandle{10}, Api::VertexHandle{9}, Api::VertexHandle{8}};

        const auto expectedSecondBorderVertexHandles =
            Api::VertexHandles{Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{5}, Api::VertexHandle{4}};

        const auto loopPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{13});
        const auto borderVertexHandlesCollection = Api::PolygonBorderVertexHandles(*m_whiteBox, loopPolygonHandle);

        EXPECT_THAT(borderVertexHandlesCollection.size(), Eq(2));
        EXPECT_THAT(
            loopPolygonHandle.m_faceHandles,
            UnorderedElementsAreArray(expectedLoopFaceHandles.cbegin(), expectedLoopFaceHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[0],
            UnorderedElementsAreArray(
                expectedFirstBorderVertexHandles.cbegin(), expectedFirstBorderVertexHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[1],
            UnorderedElementsAreArray(
                expectedSecondBorderVertexHandles.cbegin(), expectedSecondBorderVertexHandles.cend()));
    }

    TEST_F(WhiteBoxTestFixture, MeshReturnsMultipleBordersOfHollowCylinderPolygon)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // hide all vertical 'logical'/'visible' edges
        for (const auto& edgeHandle : {Api::EdgeHandle{13}, Api::EdgeHandle{15}, Api::EdgeHandle{12}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        const auto expectedLoopFaceHandles =
            Api::FaceHandles{Api::FaceHandle{9}, Api::FaceHandle{8}, Api::FaceHandle{7},  Api::FaceHandle{6},
                             Api::FaceHandle{5}, Api::FaceHandle{4}, Api::FaceHandle{11}, Api::FaceHandle{10}};

        const auto expectedFirstBorderVertexHandles =
            Api::VertexHandles{Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}, Api::VertexHandle{3}};
        const auto expectedSecondBorderVertexHandles =
            Api::VertexHandles{Api::VertexHandle{4}, Api::VertexHandle{5}, Api::VertexHandle{6}, Api::VertexHandle{7}};

        const auto loopPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11});
        const auto borderVertexHandlesCollection = Api::PolygonBorderVertexHandles(*m_whiteBox, loopPolygonHandle);

        EXPECT_THAT(borderVertexHandlesCollection.size(), Eq(2));
        EXPECT_THAT(
            loopPolygonHandle.m_faceHandles,
            UnorderedElementsAreArray(expectedLoopFaceHandles.cbegin(), expectedLoopFaceHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[0],
            UnorderedElementsAreArray(
                expectedFirstBorderVertexHandles.cbegin(), expectedFirstBorderVertexHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[1],
            UnorderedElementsAreArray(
                expectedSecondBorderVertexHandles.cbegin(), expectedSecondBorderVertexHandles.cend()));
    }

    TEST_F(WhiteBoxTestFixture, SingleEdgeCanBeRestored)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::EdgeHandles restoringEdgeHandles;
        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> splitPolygons =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{2}, restoringEdgeHandles);

        // no left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(0));
        // split polygon was returned
        EXPECT_THAT(splitPolygons.has_value(), Eq(true));
        // each polygon has a single face
        EXPECT_THAT((*splitPolygons)[0].m_faceHandles.size(), Eq(1));
        EXPECT_THAT((*splitPolygons)[1].m_faceHandles.size(), Eq(1));
        // each polygon has 3 edges
        EXPECT_THAT(
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, (*splitPolygons)[0]),
            UnorderedElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{2}));
        EXPECT_THAT(
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, (*splitPolygons)[1]),
            UnorderedElementsAre(Api::EdgeHandle{2}, Api::EdgeHandle{3}, Api::EdgeHandle{4}));
    }

    TEST_F(WhiteBoxTestFixture, MultipleEdgesCanBeRestored)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const auto edgeHandlesToRestore = {
            Api::EdgeHandle{48}, Api::EdgeHandle{47}, Api::EdgeHandle{27}, Api::EdgeHandle{59}, Api::EdgeHandle{41}};

        int restoreCount = 0;
        Api::EdgeHandles restoringEdgeHandles; // inout param
        AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> splitPolygons;
        for (const Api::EdgeHandle& edgeHandleToRestore : edgeHandlesToRestore)
        {
            splitPolygons = Api::RestoreEdge(*m_whiteBox, edgeHandleToRestore, restoringEdgeHandles);
            restoreCount++;

            if (splitPolygons.has_value())
            {
                break;
            }
        }

        EXPECT_THAT((*splitPolygons)[0].m_faceHandles.size(), Eq(8));
        EXPECT_THAT((*splitPolygons)[1].m_faceHandles.size(), Eq(10));
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(0));
        EXPECT_THAT(restoreCount, Eq(edgeHandlesToRestore.size()));
    }

    TEST_F(WhiteBoxTestFixture, RestoreExistingUserEdgeHasNoEffect)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::EdgeHandles restoringEdgeHandles;
        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> splitPolygons =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{12}, restoringEdgeHandles);

        // no left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(0));
        // split polygon was not returned
        EXPECT_THAT(splitPolygons.has_value(), Eq(false));
    }

    TEST_F(WhiteBoxTestFixture, RestoreInnerOuterBorderSplitsPolygonLoop)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);

        // hide edges to form a polygon with a hole (inner and outer edge list - two borders)
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{41});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{12});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{20});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{0});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{4});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{48});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{85});

        Api::EdgeHandles restoringEdgeHandles;
        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> firstSplitPolygonsAttempt =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{88}, restoringEdgeHandles);

        // one left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(1));
        // split polygon was not returned
        EXPECT_THAT(firstSplitPolygonsAttempt.has_value(), Eq(false));

        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> secondSplitPolygonsAttempt =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{28}, restoringEdgeHandles);

        // no left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.empty(), Eq(true));
        // split polygon was returned
        EXPECT_THAT(secondSplitPolygonsAttempt.has_value(), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, PolygonWithMultipleFacesHalfedgeHandles)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::PolygonHandle topPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{24});
        const Api::HalfedgeHandles polygonHalfedgeHandles = Api::PolygonHalfedgeHandles(*m_whiteBox, topPolygonHandle);

        // halfedge handles
        EXPECT_THAT(polygonHalfedgeHandles.size(), Eq(54));
    }

    TEST_F(WhiteBoxTestFixture, PolygonWithTwoFacesHalfedgeHandles)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const Api::PolygonHandle topPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const Api::HalfedgeHandles polygonHalfedgeHandles = Api::PolygonHalfedgeHandles(*m_whiteBox, topPolygonHandle);

        // halfedge handles
        EXPECT_THAT(polygonHalfedgeHandles.size(), Eq(6));
        EXPECT_THAT(
            polygonHalfedgeHandles,
            UnorderedElementsAre(
                Api::HalfedgeHandle{2}, Api::HalfedgeHandle{0}, Api::HalfedgeHandle{8}, Api::HalfedgeHandle{6},
                Api::HalfedgeHandle{5}, Api::HalfedgeHandle{4}));
    }

    TEST_F(WhiteBoxTestFixture, PolygonMultipleFacesBorderVertexPositionsInOrder)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::Pointwise;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::PolygonHandle topPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{24});
        Api::VertexPositionsCollection polygonBorderVertexPositionsCollection =
            Api::PolygonBorderVertexPositions(*m_whiteBox, topPolygonHandle);

        const AZStd::vector<AZ::Vector3> expectedBorderVertexPositions = {
            AZ::Vector3{0.5f, 0.5f, 0.5f},   AZ::Vector3{-0.5f, 0.5f, 0.5f},  AZ::Vector3{-1.5f, 0.5f, 0.5f},
            AZ::Vector3{-2.5f, 0.5f, 0.5f},  AZ::Vector3{-2.5f, -0.5f, 0.5f}, AZ::Vector3{-2.5f, -1.5f, 0.5f},
            AZ::Vector3{-2.5f, -2.5f, 0.5f}, AZ::Vector3{-1.5f, -2.5f, 0.5f}, AZ::Vector3{-0.5f, -2.5f, 0.5f},
            AZ::Vector3{0.5f, -2.5f, 0.5f},  AZ::Vector3{0.5f, -1.5f, 0.5f},  AZ::Vector3{0.5f, -0.5f, 0.5f}};

        // find the bottom corner to start from (used as the pivot position)
        auto vertexPositionIt = AZStd::find_if(
            polygonBorderVertexPositionsCollection.front().begin(),
            polygonBorderVertexPositionsCollection.front().end(),
            [](const AZ::Vector3& vertexPosition)
            {
                return vertexPosition.IsClose(AZ::Vector3{0.5f, 0.5f, 0.5f});
            });

        // rotate about the pivot to make the ordering of the vertices a little easier to understand
        AZStd::rotate(
            polygonBorderVertexPositionsCollection.front().begin(), vertexPositionIt,
            polygonBorderVertexPositionsCollection.front().end());

        // check the vertex positions are what we expect
        EXPECT_THAT(polygonBorderVertexPositionsCollection.size(), Eq(1));
        EXPECT_THAT(polygonBorderVertexPositionsCollection.front().size(), Eq(12));
        EXPECT_THAT(
            expectedBorderVertexPositions,
            Pointwise(ContainerIsClose(), polygonBorderVertexPositionsCollection.front()));
    }

    TEST_F(WhiteBoxTestFixture, PolygonFacePositionsForMultiFacePolygon)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::Pointwise;

        Create2x2CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor2x2Grid(*m_whiteBox);

        AZStd::vector<AZ::Vector3> polygonTriangles =
            Api::PolygonFacesPositions(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{29}));

        const AZStd::vector<AZ::Vector3> expectedPolygonFacePositions = {
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{0.5, -0.5, 0.5},  AZ::Vector3{0.5, 0.5, 0.5},
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{0.5, 0.5, 0.5},   AZ::Vector3{-0.5, 0.5, 0.5},
            AZ::Vector3{0.5, -0.5, 0.5},  AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-0.5, -1.5, 0.5},
            AZ::Vector3{0.5, -0.5, 0.5},  AZ::Vector3{-0.5, -1.5, 0.5}, AZ::Vector3{0.5, -1.5, 0.5},
            AZ::Vector3{-0.5, -1.5, 0.5}, AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-1.5, -0.5, 0.5},
            AZ::Vector3{-0.5, -1.5, 0.5}, AZ::Vector3{-1.5, -0.5, 0.5}, AZ::Vector3{-1.5, -1.5, 0.5},
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-0.5, 0.5, 0.5},  AZ::Vector3{-1.5, 0.5, 0.5},
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-1.5, 0.5, 0.5},  AZ::Vector3{-1.5, -0.5, 0.5}};

        EXPECT_THAT(polygonTriangles.size(), Eq(24));
        EXPECT_THAT(expectedPolygonFacePositions, Pointwise(ContainerIsClose(), polygonTriangles));
    }

    TEST_F(WhiteBoxTestFixture, HalfedgeHandleNext)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // next - CCW order
        EXPECT_THAT(Api::HalfedgeHandleNext(*m_whiteBox, Api::HalfedgeHandle{5}), Eq(Api::HalfedgeHandle{6}));
        EXPECT_THAT(Api::HalfedgeHandleNext(*m_whiteBox, Api::HalfedgeHandle{34}), Eq(Api::HalfedgeHandle{19}));
        EXPECT_THAT(Api::HalfedgeHandleNext(*m_whiteBox, Api::HalfedgeHandle{30}), Eq(Api::HalfedgeHandle{32}));
    }

    TEST_F(WhiteBoxTestFixture, HalfedgeHandlePrevious)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // previous - CW order
        EXPECT_THAT(Api::HalfedgeHandlePrevious(*m_whiteBox, Api::HalfedgeHandle{6}), Eq(Api::HalfedgeHandle{5}));
        EXPECT_THAT(Api::HalfedgeHandlePrevious(*m_whiteBox, Api::HalfedgeHandle{19}), Eq(Api::HalfedgeHandle{34}));
        EXPECT_THAT(Api::HalfedgeHandlePrevious(*m_whiteBox, Api::HalfedgeHandle{32}), Eq(Api::HalfedgeHandle{30}));
    }

    TEST_F(WhiteBoxTestFixture, CloneOperationProducesIdenticalResults)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::NotNull;

        Create3x3CubeGrid(*m_whiteBox);

        Api::WhiteBoxMeshPtr whiteBoxClone = Api::CloneMesh(*m_whiteBox);

        // ensure all important data is identical
        EXPECT_THAT(whiteBoxClone, NotNull());
        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(Api::MeshVertexCount(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshVertexHandles(*m_whiteBox), Eq(Api::MeshVertexHandles(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshFaceHandles(*m_whiteBox), Eq(Api::MeshFaceHandles(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshEdgeHandles(*m_whiteBox), Eq(Api::MeshEdgeHandles(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshHalfedgeCount(*m_whiteBox), Eq(Api::MeshHalfedgeCount(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshFaces(*m_whiteBox), Eq(Api::MeshFaces(*whiteBoxClone)));
    }

    TEST_F(WhiteBoxTestFixture, ClonedMeshGeometryAndFacePropertiesAreIndependent)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto vertex = Api::MeshVertexHandles(*m_whiteBox).front();
        const auto position = Api::VertexPosition(*m_whiteBox, vertex);
        const auto polygons = Api::MeshPolygonHandles(*m_whiteBox);
        const auto face = polygons.front().m_faceHandles.front();
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:9");
        Api::SetFaceMaterial(*m_whiteBox, face, material);
        Api::SetFacePaintColor(*m_whiteBox, face, 0xFF3366CC);
        auto clone = Api::CloneMesh(*m_whiteBox);
        ASSERT_NE(clone, nullptr);
        EXPECT_EQ(Api::FacePolygonHandle(*clone, face), Api::FacePolygonHandle(*m_whiteBox, face));
        EXPECT_EQ(Api::FaceMaterial(*clone, face), material);
        EXPECT_EQ(Api::FacePaintColor(*clone, face), 0xFF3366CC);

        Api::SetVertexPosition(*clone, vertex, position + AZ::Vector3::CreateAxisX());
        Api::SetFaceMaterial(*clone, face, {});
        Api::SetFacePaintColor(*clone, face, 0);
        EXPECT_TRUE(Api::VertexPosition(*m_whiteBox, vertex).IsClose(position));
        EXPECT_EQ(Api::FaceMaterial(*m_whiteBox, face), material);
        EXPECT_EQ(Api::FacePaintColor(*m_whiteBox, face), 0xFF3366CC);
        Api::Clear(*clone);
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox), polygons);
    }

    TEST_F(WhiteBoxTestFixture, EdgeVertexHandlesTailTipAreExpected)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const AZStd::array<Api::VertexHandle, 2> edge0VertexHandles = {Api::VertexHandle{0}, Api::VertexHandle{1}};

        const AZStd::array<Api::VertexHandle, 2> edge1VertexHandles = {Api::VertexHandle{1}, Api::VertexHandle{2}};

        const AZStd::array<Api::VertexHandle, 2> edge2VertexHandles = {Api::VertexHandle{2}, Api::VertexHandle{0}};

        const AZStd::array<Api::VertexHandle, 2> edge3VertexHandles = {Api::VertexHandle{2}, Api::VertexHandle{3}};

        const AZStd::array<Api::VertexHandle, 2> edge4VertexHandles = {Api::VertexHandle{3}, Api::VertexHandle{0}};

        // note: currently 'first' halfedge handle is always returned internally as
        // it will be the CCW direction of the halfedge
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{0}), UnorderedElementsAreArray(edge0VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{1}), UnorderedElementsAreArray(edge1VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{2}), UnorderedElementsAreArray(edge2VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{3}), UnorderedElementsAreArray(edge3VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{4}), UnorderedElementsAreArray(edge4VertexHandles));
    }

    TEST_F(WhiteBoxTestFixture, MultipleLoops)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;
        using ::testing::Eq;

        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        Api::ScalePolygonAppendRelative(*m_whiteBox, polygonHandle, -0.25f);
        // hide edges to create a polygon loop (two vertex lists)
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{13});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{10});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{6});

        // retrieve the halfedge and vertex handles for the polygon loop
        const auto halfedgeHandlesCollection =
            PolygonBorderHalfedgeHandles(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{2}));
        const auto vertexHandlesCollection =
            PolygonBorderVertexHandles(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{2}));

        const AZStd::array<Api::VertexHandle, 4> vertexHandlesFirstBorder = {
            Api::VertexHandle{6}, Api::VertexHandle{5}, Api::VertexHandle{4}, Api::VertexHandle{7}};
        const AZStd::array<Api::VertexHandle, 4> vertexHandlesSecondBorder = {
            Api::VertexHandle{3}, Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}};
        const AZStd::array<Api::HalfedgeHandle, 4> halfedgeHandlesFirstBorder = {
            Api::HalfedgeHandle{7}, Api::HalfedgeHandle{3}, Api::HalfedgeHandle{1}, Api::HalfedgeHandle{9}};
        const AZStd::array<Api::HalfedgeHandle, 4> halfedgeHandlesSecondBorder = {
            Api::HalfedgeHandle{24}, Api::HalfedgeHandle{30}, Api::HalfedgeHandle{10}, Api::HalfedgeHandle{18}};

        EXPECT_THAT(vertexHandlesCollection.size(), Eq(2));
        EXPECT_THAT(vertexHandlesCollection[0], ElementsAreArray(vertexHandlesFirstBorder));
        EXPECT_THAT(vertexHandlesCollection[1], ElementsAreArray(vertexHandlesSecondBorder));
        EXPECT_THAT(halfedgeHandlesCollection.size(), Eq(2));
        EXPECT_THAT(halfedgeHandlesCollection[0], ElementsAreArray(halfedgeHandlesFirstBorder));
        EXPECT_THAT(halfedgeHandlesCollection[1], ElementsAreArray(halfedgeHandlesSecondBorder));
    }

    TEST_F(WhiteBoxTestFixture, ExtrusionFromQuadWithBoundaryEdges)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitQuad(*m_whiteBox);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), 1.0f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(8));
        // note: MeshFaceCount should be 12 when 2D extrusion case is correctly handled
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(10));
    }

    TEST_F(WhiteBoxTestFixture, ImpressionOneConnectedEdge)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);
        // append another cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{9}), 1.0f);
        // use impression to squash one of the cubes down
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(14)); // 2 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(24)); // 4 faces added (2 for side polygon, 2 for linking)
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(13)); // 3 polygons added
    }

    TEST_F(WhiteBoxTestFixture, ImpressionTwoConnectedEdges)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);
        // append another cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{9}), 1.0f);
        // append another cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), 1.0f);
        // use impression to squash center cube down
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(20)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(36)); // 8 faces added (4 for side polygons, 4 for linking)
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(20)); // 6 polygons added
    }

    TEST_F(WhiteBoxTestFixture, ImpressionFourConnectedEdges)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);

        // use impression to squash center cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{27}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(36)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(68)); // 16 faces added (8 for side polygons, 8 for linking)
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(32)); // 12 polygons added
    }

    TEST_F(WhiteBoxTestFixture, ImpressionInsideLoop)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // scale append polygon in
        Api::ScalePolygonAppendRelative(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), -0.25f);

        // hide connecting edges (make loop)
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{45}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{50}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{54}));

        // use impression to squash center polygon
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(16)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(28)); // 28 faces added
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(11)); // 11 polygons added (should be 7 before?)
    }

    TEST_F(WhiteBoxTestFixture, ImpressionOutsideLoop)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // scale append polygon in
        Api::ScalePolygonAppendRelative(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), -0.25f);

        // hide connecting edges (make loop)
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{45}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{50}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{54}));

        // use impression to squash outer polygon loop
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{19}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(16)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(28)); // 28 faces added
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(11)); // 11 polygons added (should be 7 before?)
    }

    TEST_F(WhiteBoxTestFixture, AdvancedPolygonAppendReturnsExpectedRestoredPolygonHandles)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAre;
        using ::testing::UnorderedElementsAreArray;
        using fh = Api::FaceHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const auto appendedPolygonHandles = Api::TranslatePolygonAppendAdvanced(
            *m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{34}), -1.0f);

        const fh expectedFirstRestoredBefore[] = {fh(54), fh(55), fh(16), fh(17), fh(56), fh(57),
                                                  fh(0),  fh(1),  fh(5),  fh(4),  fh(52), fh(53),
                                                  fh(36), fh(37), fh(27), fh(26), fh(25), fh(24)};
        const fh expectedFirstRestoredAfter[] = {fh(38), fh(39), fh(40), fh(41), fh(42), fh(43),
                                                 fh(44), fh(45), fh(46), fh(47), fh(48), fh(49),
                                                 fh(50), fh(51), fh(52), fh(53), fh(54), fh(55)};

        const fh expectedSecondRestoredBefore[] = {fh(32), fh(33)};
        const fh expectedSecondRestoredAfter[] = {fh(56), fh(57)};

        EXPECT_THAT(
            appendedPolygonHandles.m_appendedPolygonHandle.m_faceHandles,
            UnorderedElementsAre(Api::FaceHandle{62}, Api::FaceHandle{63}));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[0].m_before.m_faceHandles,
            UnorderedElementsAreArray(expectedFirstRestoredBefore));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[0].m_after.m_faceHandles,
            UnorderedElementsAreArray(expectedFirstRestoredAfter));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[1].m_before.m_faceHandles,
            UnorderedElementsAreArray(expectedSecondRestoredBefore));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[1].m_after.m_faceHandles,
            UnorderedElementsAreArray(expectedSecondRestoredAfter));
    }

    TEST_F(WhiteBoxTestFixture, EdgeHandlesConnectedToVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto edgeHandles = Api::VertexEdgeHandles(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_THAT(
            edgeHandles,
            UnorderedElementsAre(
                Api::EdgeHandle{4}, Api::EdgeHandle{0}, Api::EdgeHandle{12}, Api::EdgeHandle{17}, Api::EdgeHandle{2}));
    }

    TEST_F(WhiteBoxTestFixture, EdgeHandlesConnectedToVertexAfterPolygonAppend)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{10}), 1.0f);

        const auto edgeHandles = Api::VertexEdgeHandles(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_THAT(
            edgeHandles,
            UnorderedElementsAre(
                Api::EdgeHandle{4}, Api::EdgeHandle{0}, Api::EdgeHandle{12}, Api::EdgeHandle{25}, Api::EdgeHandle{2},
                Api::EdgeHandle{28}));
    }

    TEST_F(WhiteBoxTestFixture, VertexCanBeHidden)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::HideVertex(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_TRUE(Api::VertexIsHidden(*m_whiteBox, Api::VertexHandle{0}));
    }

    TEST_F(WhiteBoxTestFixture, VertexCanBeRestored)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideVertex(*m_whiteBox, Api::VertexHandle{0});

        // verify precondition
        EXPECT_TRUE(Api::VertexIsHidden(*m_whiteBox, Api::VertexHandle{0}));

        Api::RestoreVertex(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_TRUE(!Api::VertexIsHidden(*m_whiteBox, Api::VertexHandle{0}));
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeHidden)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        EXPECT_FALSE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));

        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{0});

        EXPECT_TRUE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeRestored)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{0});

        // verify precondition
        EXPECT_TRUE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));

        {
            Api::EdgeHandles restoringEdgeHandles;
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{0}, restoringEdgeHandles);
        }

        EXPECT_FALSE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));
    }

    // note: no boundaries implies the mesh is closed (like a cube) as opposed
    // to having unconnected halfedges in the case of a quad
    TEST_F(WhiteBoxTestFixture, HalfedgeHandlesOfEdgeHandleWithoutBoundaries)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        {
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{0});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{0}, Api::HalfedgeHandle{1}));
        }

        {
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{17});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{35}, Api::HalfedgeHandle{34}));
        }
    }

    TEST_F(WhiteBoxTestFixture, HalfedgeHandlesOfEdgeHandleWithBoundaries)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        {
            // edge will only have a single halfedge (one connected face)
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{1});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{2}));
        }

        {
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{2});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{5}, Api::HalfedgeHandle{4}));
        }
    }

    TEST_F(WhiteBoxTestFixture, MeshUserEdgeHandlesForDefaultQuad)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const auto userMeshEdgeHandles = Api::MeshUserEdgeHandles(*m_whiteBox);

        EXPECT_THAT(userMeshEdgeHandles.m_mesh, UnorderedElementsAre(Api::EdgeHandle{2}));
        EXPECT_THAT(
            userMeshEdgeHandles.m_user,
            UnorderedElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{4}));
    }

    // no hidden vertices means only a single edge (the one passed in) will be returned
    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfUserEdgeWithoutHiddenVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{0});
            EXPECT_THAT(edgeGrouping, ElementsAre(Api::EdgeHandle{0}));
        }

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{15});
            EXPECT_THAT(edgeGrouping, ElementsAre(Api::EdgeHandle{15}));
        }
    }

    // requesting an edge grouping for a 'mesh' edge (not selectable by
    // the user) will return an empty grouping
    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfMeshEdgeWithoutHiddenVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{2});
            EXPECT_THAT(edgeGrouping.size(), Eq(0));
        }
    }

    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfUserEdgeWithHiddenVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideVertex(*m_whiteBox, Api::VertexHandle{0});

        const auto expectedEdgeGrouping = Api::EdgeHandles{Api::EdgeHandle{0}, Api::EdgeHandle{4}, Api::EdgeHandle{12}};

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{0});
            EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
        }

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{12});
            EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
        }
    }

    // here verify hidden connected edges will not be added to the grouping
    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfUserEdgeWithHiddenVertexAndConnectedHiddenEdge)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideVertex(*m_whiteBox, Api::VertexHandle{3});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{15});

        const auto expectedEdgeGrouping = Api::EdgeHandles{Api::EdgeHandle{3}, Api::EdgeHandle{4}};

        const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{4});
        EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
    }

    TEST_F(WhiteBoxTestFixture, EdgeGroupingForTopLoopOfCube)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // hide all top vertices
        for (const auto& vertexHandle :
             {Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}, Api::VertexHandle{3}})
        {
            Api::HideVertex(*m_whiteBox, vertexHandle);
        }

        // hide all vertical edges
        for (const auto& edgeHandle :
             {Api::EdgeHandle{15}, Api::EdgeHandle{13}, Api::EdgeHandle{12}, Api::EdgeHandle{10}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        // edge grouping is the top loop
        const auto expectedEdgeGrouping =
            Api::EdgeHandles{Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{4}};

        const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{3});
        EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
    }

    TEST_F(WhiteBoxTestFixture, TriPolygonCreated)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitTriangle(*m_whiteBox);

        const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);
        const auto vertexPositions = Api::MeshVertexPositions(*m_whiteBox);

        const auto expectedVertexHandles =
            Api::VertexHandles{Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}};

        const auto expectedVertexPositions = AZStd::vector<AZ::Vector3>{
            AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{-0.866f, -0.5f, 0.0f}, AZ::Vector3{0.866f, -0.5f, 0.0f}};

        EXPECT_THAT(vertexHandles, UnorderedElementsAreArray(expectedVertexHandles));
        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), expectedVertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, SplitUserEdgeCausesNewlyFormedFacesToBeAddedToCorrespondingPolygons)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAre;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // verify preconditions
        const auto edgeFaceHandlesBefore = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto edgeVertexHandlesBefore = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto firstConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[0]);
        const auto secondConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[1]);

        // given
        EXPECT_THAT(edgeFaceHandlesBefore, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{0}));
        EXPECT_THAT(
            firstConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}));
        EXPECT_THAT(
            secondConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}));
        EXPECT_THAT(edgeVertexHandlesBefore, UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{1}));

        // when
        const Api::VertexHandle splitVertexHandle =
            Api::SplitEdge(*m_whiteBox, Api::EdgeHandle{0}, Api::EdgeMidpoint(*m_whiteBox, Api::EdgeHandle{3}));

        // then
        const auto splitVertexEdgeHandles = Api::VertexEdgeHandles(*m_whiteBox, splitVertexHandle);
        const auto faceHandlesForEdge20 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{20});
        const auto faceHandlesForEdge19 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{19});
        const auto faceHandlesForEdge18 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{18});
        const auto faceHandlesForEdge0 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto polygonHandleForFace0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto polygonHandleForFace5 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{5});
        const auto bordedEdgeHandlesPolygon0 =
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandleForFace0);
        const auto bordedEdgeHandlesPolygon5 =
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandleForFace5);

        EXPECT_THAT(splitVertexHandle, Eq(Api::VertexHandle{8}));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, splitVertexHandle), Eq(false));
        EXPECT_THAT(
            splitVertexEdgeHandles,
            UnorderedElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{18}, Api::EdgeHandle{20}, Api::EdgeHandle{19}));
        EXPECT_THAT(faceHandlesForEdge0, UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{5}));
        EXPECT_THAT(faceHandlesForEdge18, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{13}));
        EXPECT_THAT(faceHandlesForEdge19, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{0}));
        EXPECT_THAT(faceHandlesForEdge20, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{13}));
        EXPECT_THAT(
            polygonHandleForFace0.m_faceHandles, // top face
            UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{12}, Api::FaceHandle{1}));
        EXPECT_THAT(
            polygonHandleForFace5.m_faceHandles, // near (side) face
            UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}, Api::FaceHandle{13}));
        EXPECT_THAT(
            bordedEdgeHandlesPolygon0,
            ElementsAre(
                Api::EdgeHandle{4}, Api::EdgeHandle{18}, Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{3}));
        EXPECT_THAT(
            bordedEdgeHandlesPolygon5,
            ElementsAre(
                Api::EdgeHandle{12}, Api::EdgeHandle{8}, Api::EdgeHandle{10}, Api::EdgeHandle{0}, Api::EdgeHandle{18}));
    }

    TEST_F(WhiteBoxTestFixture, SplitMeshEdgeCausesNewlyFormedFacesToBeAddedToCorrespondingPolygons)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAre;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // verify preconditions
        const auto edgeFaceHandlesBefore = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto edgeVertexHandlesBefore = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto firstConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[0]);
        const auto secondConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[1]);

        // given
        EXPECT_THAT(edgeFaceHandlesBefore, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{0}));
        EXPECT_THAT(
            firstConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}));
        EXPECT_THAT(
            secondConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}));
        EXPECT_THAT(edgeVertexHandlesBefore, UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{1}));

        // when
        const Api::VertexHandle splitVertexHandle =
            Api::SplitEdge(*m_whiteBox, Api::EdgeHandle{11}, Api::EdgeMidpoint(*m_whiteBox, Api::EdgeHandle{11}));

        // then
        const auto splitVertexEdgeHandles = Api::VertexEdgeHandles(*m_whiteBox, splitVertexHandle);
        const auto faceHandlesForEdge20 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{20});
        const auto faceHandlesForEdge19 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{19});
        const auto faceHandlesForEdge18 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{18});
        const auto faceHandlesForEdge11 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{11});
        const auto polygonHandleForFace5 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{5});
        const auto bordedEdgeHandlesPolygon5 =
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandleForFace5);

        EXPECT_THAT(splitVertexHandle, Eq(Api::VertexHandle{8}));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, splitVertexHandle), Eq(true));
        EXPECT_THAT(
            splitVertexEdgeHandles,
            UnorderedElementsAre(Api::EdgeHandle{20}, Api::EdgeHandle{18}, Api::EdgeHandle{11}, Api::EdgeHandle{19}));
        EXPECT_THAT(faceHandlesForEdge11, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}));
        EXPECT_THAT(faceHandlesForEdge18, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{13}));
        EXPECT_THAT(faceHandlesForEdge19, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{4}));
        EXPECT_THAT(faceHandlesForEdge20, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{13}));
        EXPECT_THAT(
            polygonHandleForFace5.m_faceHandles, // near (side) face
            UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}, Api::FaceHandle{13}, Api::FaceHandle{12}));
        EXPECT_THAT(
            bordedEdgeHandlesPolygon5,
            ElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{12}, Api::EdgeHandle{8}, Api::EdgeHandle{10}));
    }

    TEST_F(WhiteBoxTestFixture, SplitFaceCausesNewlyFormedFacesToBeAddedToCorrespondingPolygons)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto polygonHandleForFace0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto faceVertexHandles = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(
            polygonHandleForFace0.m_faceHandles, // top face
            UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}));
        EXPECT_THAT(
            faceVertexHandles, UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}));

        const auto splitVertexHandle =
            Api::SplitFace(*m_whiteBox, Api::FaceHandle{0}, Api::FaceMidpoint(*m_whiteBox, Api::FaceHandle{0}));

        const auto edgeHandles = Api::VertexEdgeHandles(*m_whiteBox, splitVertexHandle);
        const auto polygonHandleForFace0After = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto faceVertexHandlesAfter = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(splitVertexHandle, Eq(Api::VertexHandle{8}));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, splitVertexHandle), Eq(true));
        EXPECT_THAT(edgeHandles, UnorderedElementsAre(Api::EdgeHandle{18}, Api::EdgeHandle{19}, Api::EdgeHandle{20}));
        EXPECT_THAT(
            polygonHandleForFace0After.m_faceHandles, // top face
            UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}, Api::FaceHandle{12}, Api::FaceHandle{13}));
        EXPECT_THAT(
            faceVertexHandlesAfter,
            UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{8}, Api::VertexHandle{2}));
    }

    TEST_F(WhiteBoxTestFixture, UserEdgeHandlesReturnedForVertexHandle)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexUserEdgeHandles = Api::VertexUserEdgeHandles(*m_whiteBox, Api::VertexHandle{2});

        // in the unit cube, vertex handle 2 has 5 connected edge handles but only two of these
        // are user edges (two are internal edges of a cube face)
        EXPECT_THAT(
            vertexUserEdgeHandles, UnorderedElementsAre(Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{13}));
    }

    TEST_F(WhiteBoxTestFixture, UserEdgeAxesReturnedForVertexHandle)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexUserEdgeVectors = Api::VertexUserEdgeVectors(*m_whiteBox, Api::VertexHandle{2});

        // The order of these can change between versions of OM3D but the values should be the same (one for each basis)
        // Winding is counter clockwise so it tends to go -X ... -Y ... -Z in latest OpenMesh 11.x
        const auto expectedUserEdgeVectors = AZStd::vector<AZ::Vector3>{
            -AZ::Vector3::CreateAxisX(), -AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisZ()};

        EXPECT_THAT(vertexUserEdgeVectors, Pointwise(ContainerIsClose(), expectedUserEdgeVectors));
    }

    TEST_F(WhiteBoxTestFixture, EdgeAxisReturnedForEdgeHandle)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), 1.0f);
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1}), 0.5f);

        const auto edgeVector_4 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{4});
        const auto edgeVector_17 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{17});
        const auto edgeVector_12 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{12});
        const auto edgeVector_0 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{0});
        const auto edgeVector_2 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{2});

        EXPECT_THAT(edgeVector_4, IsClose(AZ::Vector3(0.0f, -1.0f, 0.0f)));
        EXPECT_THAT(edgeVector_17, IsClose(AZ::Vector3(0.0f, 1.0f, -1.5f)));
        EXPECT_THAT(edgeVector_12, IsClose(AZ::Vector3(0.0f, 0.0f, -1.5f)));
        EXPECT_THAT(edgeVector_0, IsClose(AZ::Vector3(2.0f, 0.0f, 0.0f)));
        EXPECT_THAT(edgeVector_2, IsClose(AZ::Vector3(-2.0f, -1.0f, 0.0f)));
    }

    TEST_F(WhiteBoxTestFixture, UserEdgesWithZeroLengthNotReturned)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // squash a cube to be flat (where certain edges will have zero length)
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1}), 1.0f);
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{5}), 2.0f);
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), -1.0f);

        const auto vertexUserEdgeVectors = Api::VertexUserEdgeVectors(*m_whiteBox, Api::VertexHandle{2});
        const auto vertexUserEdgeAxes = Api::VertexUserEdgeAxes(*m_whiteBox, Api::VertexHandle{2});

        // Again, these could change in order between versions of OpenMesh.  But the values will be present.
        // In the OpenMesh 11.0 it tends to go (-x ... -y ... -z) so in this case its (-y, -z) becuase we have 
        // collapsed along x, which will have a zero length and should no be returned.

        const auto expectedUserEdgeVectors = AZStd::vector<AZ::Vector3>{ -AZ::Vector3::CreateAxisY(3.0f), -AZ::Vector3::CreateAxisZ(2.0f) };
        const auto expectedUserEdgeAxes    = AZStd::vector<AZ::Vector3>{ -AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisZ() };

        EXPECT_THAT(vertexUserEdgeVectors, Pointwise(ContainerIsClose(), expectedUserEdgeVectors));
        EXPECT_THAT(vertexUserEdgeAxes, Pointwise(ContainerIsClose(), expectedUserEdgeAxes));
    }

    TEST_F(WhiteBoxTestFixture, IsolatedVerticesAreHiddenWhenCreatingNewPolygons)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::VertexHandle internalVertexHandles[] = {vh{0}, vh{11}, vh{20}, vh{16}};

        bool internalVertexHandlesHidden[std::size(internalVertexHandles)];
        bool internalVertexHandlesIsolated[std::size(internalVertexHandles)];

        AZStd::transform(
            AZStd::cbegin(internalVertexHandles), AZStd::cend(internalVertexHandles),
            AZStd::begin(internalVertexHandlesHidden),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsHidden(*whiteBox, vertexHandle);
            });

        AZStd::transform(
            AZStd::cbegin(internalVertexHandles), AZStd::cend(internalVertexHandles),
            AZStd::begin(internalVertexHandlesIsolated),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsIsolated(*whiteBox, vertexHandle);
            });

        EXPECT_THAT(internalVertexHandlesHidden, Each(Eq(true)));
        EXPECT_THAT(internalVertexHandlesIsolated, Each(Eq(true)));
    }

    TEST_F(WhiteBoxTestFixture, HiddenVerticesConnectedToRestoredEdgesAreRestored)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::VertexHandle reconnectedInternalVertexHandles[] = {vh{11}, vh{20}, vh{16}};

        const auto edgeHandlesToRestore = {
            Api::EdgeHandle{85}, Api::EdgeHandle{45}, Api::EdgeHandle{59}, Api::EdgeHandle{12}};

        bool edgeRestored = false;
        Api::EdgeHandles restoringEdgeHandles; // inout param
        for (const Api::EdgeHandle& edgeHandleToRestore : edgeHandlesToRestore)
        {
            if (Api::RestoreEdge(*m_whiteBox, edgeHandleToRestore, restoringEdgeHandles))
            {
                edgeRestored = true;
                break;
            }
        }

        bool internalVertexHandlesHidden[std::size(reconnectedInternalVertexHandles)];
        bool internalVertexHandlesIsolated[std::size(reconnectedInternalVertexHandles)];

        AZStd::transform(
            AZStd::cbegin(reconnectedInternalVertexHandles), AZStd::cend(reconnectedInternalVertexHandles),
            AZStd::begin(internalVertexHandlesHidden),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsHidden(*whiteBox, vertexHandle);
            });

        AZStd::transform(
            AZStd::cbegin(reconnectedInternalVertexHandles), AZStd::cend(reconnectedInternalVertexHandles),
            AZStd::begin(internalVertexHandlesIsolated),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsIsolated(*whiteBox, vertexHandle);
            });

        // ensure the edge was correctly restored
        EXPECT_THAT(edgeRestored, Eq(true));

        // vertex handles connected to restored edges will be no longer be hidden or isolated
        EXPECT_THAT(internalVertexHandlesHidden, Each(Eq(false)));
        EXPECT_THAT(internalVertexHandlesIsolated, Each(Eq(false)));

        // unaffected vertex will remain hidden and isolated
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(true));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, TryingToRestoreIsolatedHidddenVerticesFails)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        // precondition check
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(true));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));

        const bool vertexRestored = Api::TryRestoreVertex(*m_whiteBox, vh{0});

        // postcondition check - values remain the same
        EXPECT_THAT(vertexRestored, Eq(false));
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(true));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, TryingToRestoreConnectedHidddenVerticesSucceeds)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);

        // precondition check
        Api::HideVertex(*m_whiteBox, vh{0});

        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(false));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));

        const bool vertexRestored = Api::TryRestoreVertex(*m_whiteBox, vh{0});

        // postcondition check - values have changed
        EXPECT_THAT(vertexRestored, Eq(true));
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(false));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(false));
    }

} // namespace UnitTest

// Required to support running integration tests with Qt
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();

namespace UnitTest
{
    TEST_F(WhiteBoxTestFixture, EdgePatternsFollowQuadTopologyAndIgnoreTriangleDiagonals)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        Api::VertexHandles vertices;
        for (int y = 0; y <= 3; ++y)
        {
            for (int x = 0; x <= 3; ++x)
            {
                vertices.push_back(Api::AddVertex(*mesh, AZ::Vector3(static_cast<float>(x), static_cast<float>(y), 0)));
            }
        }
        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                const int i = y * 4 + x;
                Api::AddQuadPolygon(*mesh, vertices[i], vertices[i + 1], vertices[i + 5], vertices[i + 4]);
            }
        }
        const auto edgeBetween = [&](int a, int b)
        {
            for (const auto edge : Api::MeshEdgeHandles(*mesh))
            {
                const auto pair = Api::EdgeVertexHandles(*mesh, edge);
                if ((pair[0] == vertices[a] && pair[1] == vertices[b]) ||
                    (pair[1] == vertices[a] && pair[0] == vertices[b])) { return edge; }
            }
            return Api::EdgeHandle{};
        };
        const auto seed = edgeBetween(5, 6);
        const auto otherRow = edgeBetween(9, 10);
        const auto border = edgeBetween(0, 1);
        const auto diagonal = edgeBetween(0, 5);
        ASSERT_TRUE(seed.IsValid());
        ASSERT_TRUE(diagonal.IsValid());
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        const auto loop = Api::FindEdgeLoop(*mesh, {seed});
        ASSERT_EQ(loop.size(), 3);
        for (const auto edge : loop)
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            EXPECT_FLOAT_EQ(points[0].GetY(), 1);
            EXPECT_FLOAT_EQ(points[1].GetY(), 1);
        }
        const auto ring = Api::FindEdgeRing(*mesh, {seed});
        ASSERT_EQ(ring.size(), 4);
        for (const auto edge : ring)
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            EXPECT_FLOAT_EQ(points[0].GetY(), points[1].GetY());
            EXPECT_FLOAT_EQ(AZStd::min(points[0].GetX(), points[1].GetX()), 1);
            EXPECT_FLOAT_EQ(AZStd::max(points[0].GetX(), points[1].GetX()), 2);
        }
        EXPECT_EQ(Api::FindEdgeLoop(*mesh, {border}).size(), 12);
        EXPECT_EQ(Api::FindEdgeLoop(*mesh, {seed, otherRow, seed}).size(), 6);
        EXPECT_EQ(Api::FindEdgeRing(*mesh, {seed, otherRow, seed}).size(), 4);
        EXPECT_TRUE(Api::FindEdgeLoop(*mesh, {diagonal, Api::EdgeHandle{999999}}).empty());
        EXPECT_TRUE(Api::FindEdgeRing(*mesh, {diagonal, Api::EdgeHandle{999999}}).empty());
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxTestFixture, EdgePatternsCloseWithoutBranchingAtPoles)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto seed = Api::MeshPolygonEdgeHandles(*m_whiteBox).front();
        EXPECT_EQ(Api::FindEdgeRing(*m_whiteBox, {seed}).size(), 4);
        EXPECT_EQ(Api::FindEdgeLoop(*m_whiteBox, {seed}).size(), 1); // valence-three corners
        AZStd::string error;
        ASSERT_TRUE(Api::InsertEdgeLoop(*m_whiteBox, seed, 0.5f, error)) << error.c_str();
        Api::EdgeHandle cut;
        for (const auto edge : Api::MeshPolygonEdgeHandles(*m_whiteBox))
        {
            const auto pair = Api::EdgeVertexHandles(*m_whiteBox, edge);
            if (pair[0].Index() >= 8 && pair[1].Index() >= 8) { cut = edge; break; }
        }
        ASSERT_TRUE(cut.IsValid());
        EXPECT_EQ(Api::FindEdgeLoop(*m_whiteBox, {cut}).size(), 4);
    }

    TEST_F(WhiteBoxTestFixture, EdgeRingStopsAtTriangle)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        const auto a = Api::AddVertex(*mesh, AZ::Vector3(0, 0, 0));
        const auto b = Api::AddVertex(*mesh, AZ::Vector3(1, 0, 0));
        const auto c = Api::AddVertex(*mesh, AZ::Vector3(1, 1, 0));
        const auto d = Api::AddVertex(*mesh, AZ::Vector3(0, 1, 0));
        const auto e = Api::AddVertex(*mesh, AZ::Vector3(2, 0.5f, 0));
        Api::AddQuadPolygon(*mesh, a, b, c, d);
        Api::AddTriPolygon(*mesh, c, b, e);
        Api::EdgeHandle seed;
        for (const auto edge : Api::MeshPolygonEdgeHandles(*mesh))
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            if (points[0].GetX() == 0 && points[1].GetX() == 0) { seed = edge; }
        }
        ASSERT_TRUE(seed.IsValid());
        const auto ring = Api::FindEdgeRing(*mesh, {seed});
        ASSERT_EQ(ring.size(), 2);
        for (const auto edge : ring)
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            EXPECT_FLOAT_EQ(points[0].GetX(), points[1].GetX());
        }
    }

    TEST_F(WhiteBoxTestFixture, BevelCubeEdgesPreservesClosedMeshAndFaceProperties)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:1");
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            Api::SetFaceMaterial(*m_whiteBox, face, material);
            Api::SetFacePaintColor(*m_whiteBox, face, 0xFF00FF00u);
        }
        for (const auto edge : Api::MeshPolygonEdgeHandles(*m_whiteBox))
        {
            for (const int segments : {1, 3, 8})
            {
                auto mesh = Api::CloneMesh(*m_whiteBox);
                AZStd::string error;
                ASSERT_TRUE(Api::BevelEdges(*mesh, {edge}, 0.1f, segments, error)) << error.c_str();
                EXPECT_EQ(Api::MeshVertexCount(*mesh), 8 + 2 * segments);
                EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), static_cast<size_t>(6 + segments));
                for (const auto resultEdge : Api::MeshEdgeHandles(*mesh))
                {
                    EXPECT_EQ(Api::EdgeFaceHandles(*mesh, resultEdge).size(), 2);
                }
                double volume = 0.0;
                for (const auto face : Api::MeshFaceHandles(*mesh))
                {
                    EXPECT_EQ(Api::FaceMaterial(*mesh, face), material);
                    EXPECT_EQ(Api::FacePaintColor(*mesh, face), 0xFF00FF00u);
                    const auto positions = Api::FaceVertexPositions(*mesh, face);
                    EXPECT_GT((positions[1] - positions[0]).Cross(positions[2] - positions[0]).GetLengthSq(), 1e-12f);
                    volume += positions[0].Dot(positions[1].Cross(positions[2])) / 6.0;
                }
                EXPECT_LT(volume, 1.0);
                EXPECT_GT(volume, 0.99);
                if (segments == 1) { EXPECT_NEAR(volume, 0.995, 1e-5); }
                Api::WhiteBoxMeshStream saved;
                ASSERT_TRUE(Api::WriteMesh(*mesh, saved));
                auto restored = Api::CreateWhiteBoxMesh();
                ASSERT_EQ(Api::ReadMesh(*restored, saved), Api::ReadResult::Full);
                EXPECT_EQ(Api::MeshPolygonHandles(*restored).size(), Api::MeshPolygonHandles(*mesh).size());
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, BevelKeepsOpenWallBoundaryAndRejectsBoundaryEdge)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        const auto a = Api::AddVertex(*mesh, AZ::Vector3(0, 0, 0));
        const auto b = Api::AddVertex(*mesh, AZ::Vector3(0, 0, 1));
        const auto c = Api::AddVertex(*mesh, AZ::Vector3(1, 0, 0));
        const auto d = Api::AddVertex(*mesh, AZ::Vector3(1, 0, 1));
        const auto e = Api::AddVertex(*mesh, AZ::Vector3(0, 1, 0));
        const auto f = Api::AddVertex(*mesh, AZ::Vector3(0, 1, 1));
        Api::AddQuadPolygon(*mesh, a, c, d, b);
        Api::AddQuadPolygon(*mesh, a, b, f, e);
        Api::CalculateNormals(*mesh);
        Api::EdgeHandle seam;
        Api::EdgeHandle boundary;
        for (const auto edge : Api::MeshPolygonEdgeHandles(*mesh))
        {
            if (Api::EdgeIsBoundary(*mesh, edge)) { boundary = edge; }
            else { seam = edge; }
        }
        ASSERT_TRUE(seam.IsValid());
        ASSERT_TRUE(boundary.IsValid());
        AZStd::string error;
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        EXPECT_FALSE(Api::BevelEdges(*mesh, {boundary}, 0.1f, 3, error));
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
        ASSERT_TRUE(Api::BevelEdges(*mesh, {seam}, 0.1f, 3, error)) << error.c_str();
        EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 5);
        size_t boundaryCount = 0;
        for (const auto edge : Api::MeshEdgeHandles(*mesh))
        {
            if (Api::EdgeIsBoundary(*mesh, edge)) { ++boundaryCount; }
            else { EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2); }
        }
        EXPECT_EQ(boundaryCount, 12);
    }

    TEST_F(WhiteBoxTestFixture, BevelAcceptsTouchingEdgesAndRejectsExcessiveWidthWithoutChangingMesh)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto edges = Api::MeshPolygonEdgeHandles(*m_whiteBox);
        const auto endpoints = Api::EdgeVertexHandles(*m_whiteBox, edges.front());
        Api::EdgeHandle touching;
        Api::EdgeHandle separate;
        for (const auto edge : edges)
        {
            if (edge == edges.front()) { continue; }
            const auto vertices = Api::EdgeVertexHandles(*m_whiteBox, edge);
            if (vertices[0] == endpoints[0] || vertices[1] == endpoints[0] ||
                vertices[0] == endpoints[1] || vertices[1] == endpoints[1])
            {
                touching = edge;
            }
            else { separate = edge; }
        }
        ASSERT_TRUE(touching.IsValid());
        ASSERT_TRUE(separate.IsValid());
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        AZStd::string error;
        auto connected = Api::CloneMesh(*m_whiteBox);
        ASSERT_TRUE(Api::BevelEdges(*connected, {edges.front(), touching}, 0.1f, 3, error)) << error.c_str();
        for (const auto edge : Api::MeshEdgeHandles(*connected))
        {
            EXPECT_EQ(Api::EdgeFaceHandles(*connected, edge).size(), 2);
        }
        EXPECT_FALSE(Api::BevelEdges(*m_whiteBox, {edges.front()}, 2.0f, 1, error));
        EXPECT_FALSE(Api::BevelEdges(*m_whiteBox, {edges.front()}, 0.1f, 0, error));
        EXPECT_FALSE(Api::BevelEdges(*m_whiteBox, {Api::EdgeHandle{999999}}, 0.1f, 1, error));
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(before, after);
        ASSERT_TRUE(Api::BevelEdges(*m_whiteBox, {edges.front(), separate}, 0.1f, 3, error)) << error.c_str();
        for (const auto edge : Api::MeshEdgeHandles(*m_whiteBox))
        {
            EXPECT_EQ(Api::EdgeFaceHandles(*m_whiteBox, edge).size(), 2);
        }
    }

    TEST_F(WhiteBoxTestFixture, ThreeEdgeBevelCornerUsesProfileGridWithoutGrooves)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto cornerHandle = Api::MeshVertexHandles(*m_whiteBox).front();
        const auto corner = Api::VertexPosition(*m_whiteBox, cornerHandle);
        Api::EdgeHandles selection;
        for (const auto edge : Api::MeshPolygonEdgeHandles(*m_whiteBox))
        {
            const auto vertices = Api::EdgeVertexHandles(*m_whiteBox, edge);
            if (vertices[0] == cornerHandle || vertices[1] == cornerHandle) { selection.push_back(edge); }
        }
        ASSERT_EQ(selection.size(), 3);
        constexpr float width = 0.1f;
        AZ::Vector3 center = corner;
        for (int axis = 0; axis < 3; ++axis)
        {
            center.SetElement(axis, corner.GetElement(axis) + (corner.GetElement(axis) > 0.0f ? -width : width));
        }
        for (const int segments : {1, 2, 3, 4, 5, 6, 7, 8, 9})
        {
            for (const float profile : {0.25f, 0.5f, 0.75f, 0.9f, 0.95f})
            {
                auto mesh = Api::CloneMesh(*m_whiteBox);
                AZStd::string error;
                ASSERT_TRUE(Api::BevelEdges(*mesh, selection, width, segments, error, profile)) << error.c_str();
                WhiteBox::WhiteBoxFaces renderFaces;
                for (const auto handle : Api::MeshFaceHandles(*mesh))
                {
                    const auto points = Api::FaceVertexPositions(*mesh, handle);
                    WhiteBox::WhiteBoxFace face{};
                    face.m_v1 = {points[0], AZ::Vector2(0, 0)};
                    face.m_v2 = {points[1], AZ::Vector2(1, 0)};
                    face.m_v3 = {points[2], AZ::Vector2(0, 1)};
                    face.m_normal = Api::FaceNormal(*mesh, handle);
                    renderFaces.push_back(face);
                }
                EXPECT_EQ(WhiteBox::BuildCulledWhiteBoxFaces(renderFaces).size(), renderFaces.size());
                size_t patchCount = 0;
                size_t triangleCount = 0;
                for (const auto& polygon : Api::MeshPolygonHandles(*mesh))
                {
                    const auto borders = Api::PolygonBorderVertexHandles(*mesh, polygon);
                    ASSERT_EQ(borders.size(), 1);
                    bool inCorner = true;
                    for (const auto vertex : borders.front())
                    {
                        const auto delta = Api::VertexPosition(*mesh, vertex) - corner;
                        for (int axis = 0; axis < 3; ++axis)
                        {
                            inCorner = inCorner && AZStd::abs(delta.GetElement(axis)) <= width + 1e-5f;
                        }
                    }
                    if (!inCorner) { continue; }
                    ++patchCount;
                    if (segments % 2 == 0)
                    {
                        EXPECT_EQ(borders.front().size(), 4);
                        EXPECT_EQ(polygon.m_faceHandles.size(), 2);
                    }
                    if (borders.front().size() == 3)
                    {
                        ++triangleCount;
                        // The sole odd-segment triangle belongs at the center:
                        // its centroid must be on the corner's symmetry axis.
                        AZ::Vector3 centroid = AZ::Vector3::CreateZero();
                        for (const auto vertex : borders.front()) { centroid += Api::VertexPosition(*mesh, vertex); }
                        centroid = (centroid / 3.0f - center) / width;
                        EXPECT_NEAR(AZStd::abs(centroid.GetX()), AZStd::abs(centroid.GetY()), 1e-5f);
                        EXPECT_NEAR(AZStd::abs(centroid.GetY()), AZStd::abs(centroid.GetZ()), 1e-5f);
                    }
                    else { EXPECT_EQ(borders.front().size(), 4); }
                    // Every corner sample lies on the strips' superellipsoid (catches pulled-in borders).
                    for (const auto vertex : borders.front())
                    {
                        const auto relative = (Api::VertexPosition(*mesh, vertex) - center) / width;
                        float implicitSurface = 0.0f;
                        for (int axis = 0; axis < 3; ++axis)
                        {
                            implicitSurface += powf(AZStd::abs(relative.GetElement(axis)), 1.0f / (1.0f - profile));
                        }
                        EXPECT_NEAR(implicitSurface, 1.0f, 1e-4f);
                    }
                }
                const int expectedPatches = segments % 2 == 0
                    ? 3 * (segments / 2) * (segments / 2) : 3 * (segments / 2) * (segments / 2 + 1) + 1;
                EXPECT_EQ(patchCount, static_cast<size_t>(expectedPatches));
                EXPECT_EQ(triangleCount, static_cast<size_t>(segments % 2));
                for (const auto edge : Api::MeshEdgeHandles(*mesh))
                {
                    EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
                }
                Api::WhiteBoxMeshStream saved;
                ASSERT_TRUE(Api::WriteMesh(*mesh, saved));
                auto restored = Api::CreateWhiteBoxMesh();
                ASSERT_EQ(Api::ReadMesh(*restored, saved), Api::ReadResult::Full);
                EXPECT_EQ(Api::MeshPolygonHandles(*restored).size(), Api::MeshPolygonHandles(*mesh).size());
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, BevelPolygonPerimeterAndAllCubeEdgesBuildClosedCornerJoins)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto polygon = Api::MeshPolygonHandles(*m_whiteBox).front();
        Api::EdgeHandles perimeter;
        for (const auto& border : Api::PolygonBorderHalfedgeHandles(*m_whiteBox, polygon))
        {
            for (const auto halfedge : border) { perimeter.push_back(Api::HalfedgeEdgeHandle(*m_whiteBox, halfedge)); }
        }
        for (const auto& selection : {perimeter, Api::MeshPolygonEdgeHandles(*m_whiteBox)})
        {
            for (const float profile : {0.25f, 0.5f, 0.75f})
            {
                for (const int segments : {1, 3, 8})
                {
                    auto mesh = Api::CloneMesh(*m_whiteBox);
                    AZStd::string error;
                    ASSERT_TRUE(Api::BevelEdges(*mesh, selection, 0.1f, segments, error, profile)) << error.c_str();
                    for (const auto edge : Api::MeshEdgeHandles(*mesh))
                    {
                        EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
                    }
                    double volume = 0.0;
                    for (const auto face : Api::MeshFaceHandles(*mesh))
                    {
                        const auto points = Api::FaceVertexPositions(*mesh, face);
                        EXPECT_GT((points[1] - points[0]).Cross(points[2] - points[0]).GetLengthSq(), 1e-12f);
                        volume += points[0].Dot(points[1].Cross(points[2])) / 6.0;
                    }
                    EXPECT_GT(volume, 0.8);
                    EXPECT_LT(volume, 1.0);
                }
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, InsertLoopAroundCubePreservesVolumeAndFaceProperties)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:1");
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            Api::SetFaceMaterial(*m_whiteBox, face, material);
            Api::SetFacePaintColor(*m_whiteBox, face, 0xFF00FF00u);
        }
        const auto seed = Api::MeshPolygonEdgeHandles(*m_whiteBox).front();
        const auto endpoints = Api::EdgeVertexPositions(*m_whiteBox, seed);
        const auto axis = (endpoints[1] - endpoints[0]).GetNormalized();
        for (const float fraction : {0.25f, 0.5f, 0.75f})
        {
            auto mesh = Api::CloneMesh(*m_whiteBox);
            AZStd::string error;
            ASSERT_TRUE(Api::InsertEdgeLoop(*mesh, seed, fraction, error)) << error.c_str();
            EXPECT_EQ(Api::MeshVertexCount(*mesh), 12);
            EXPECT_EQ(Api::MeshFaceCount(*mesh), 20);
            EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 10);
            const auto cut = endpoints[0].Lerp(endpoints[1], fraction);
            for (const auto vertex : Api::MeshVertexHandles(*mesh))
            {
                if (vertex.Index() >= 8)
                {
                    EXPECT_NEAR((Api::VertexPosition(*mesh, vertex) - cut).Dot(axis), 0.0f, 1e-5f);
                }
            }
            for (const auto edge : Api::MeshEdgeHandles(*mesh))
            {
                EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
            }
            double volume = 0.0;
            for (const auto face : Api::MeshFaceHandles(*mesh))
            {
                EXPECT_EQ(Api::FaceMaterial(*mesh, face), material);
                EXPECT_EQ(Api::FacePaintColor(*mesh, face), 0xFF00FF00u);
                const auto points = Api::FaceVertexPositions(*mesh, face);
                volume += points[0].Dot(points[1].Cross(points[2])) / 6.0;
            }
            EXPECT_NEAR(volume, 1.0, 1e-5);
            for (const auto& polygon : Api::MeshPolygonHandles(*mesh))
            {
                const auto borders = Api::PolygonBorderVertexHandles(*mesh, polygon);
                ASSERT_EQ(borders.size(), 1);
                EXPECT_EQ(borders[0].size(), 4);
                for (const auto face : polygon.m_faceHandles) { EXPECT_EQ(Api::FacePolygonHandle(*mesh, face), polygon); }
            }
            ASSERT_TRUE(Api::InsertEdgeLoop(*mesh, Api::MeshPolygonEdgeHandles(*mesh).front(), 0.5f, error)) << error.c_str();
            for (const auto edge : Api::MeshEdgeHandles(*mesh))
            {
                EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, MultipleLoopPreviewMatchesInsertedEdgesAndDoesNotModifyMesh)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        // Exercise all seed orientations and both odd/even cut counts.
        for (const auto seed : Api::MeshPolygonEdgeHandles(*m_whiteBox))
        {
            for (const int count : {1, 2, 3, 8})
            {
                for (const float slide : {-1.0f, 0.0f, 0.5f, 1.0f})
                {
                    AZStd::string error;
                    AZStd::vector<AZ::Vector3> preview;
                    ASSERT_TRUE(Api::PreviewEdgeLoops(*m_whiteBox, seed, count, preview, error, slide)) << error.c_str();
                    EXPECT_EQ(preview.size(), static_cast<size_t>(8 * count));
                    Api::WhiteBoxMeshStream afterPreview;
                    ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, afterPreview));
                    EXPECT_EQ(before, afterPreview);
                    auto mesh = Api::CloneMesh(*m_whiteBox);
                    ASSERT_TRUE(Api::InsertEdgeLoops(*mesh, seed, count, error, slide)) << error.c_str();
                    EXPECT_EQ(Api::MeshVertexCount(*mesh), 8 + 4 * count);
                    EXPECT_EQ(Api::MeshFaceCount(*mesh), 12 + 8 * count);
                    EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), static_cast<size_t>(6 + 4 * count));
                    const auto edges = Api::MeshPolygonEdgeHandles(*mesh);
                    for (size_t i = 0; i < preview.size(); i += 2)
                    {
                        bool found = false;
                        for (const auto edge : edges)
                        {
                            const auto points = Api::EdgeVertexPositions(*mesh, edge);
                            found = found || (points[0].IsClose(preview[i]) && points[1].IsClose(preview[i + 1])) ||
                                (points[1].IsClose(preview[i]) && points[0].IsClose(preview[i + 1]));
                        }
                        EXPECT_TRUE(found);
                    }
                    for (const auto edge : Api::MeshEdgeHandles(*mesh))
                    {
                        EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
                    }
                    double volume = 0.0;
                    for (const auto face : Api::MeshFaceHandles(*mesh))
                    {
                        const auto points = Api::FaceVertexPositions(*mesh, face);
                        volume += points[0].Dot(points[1].Cross(points[2])) / 6.0;
                    }
                    EXPECT_NEAR(volume, 1.0, 1e-5);
                    const auto endpoints = Api::EdgeVertexPositions(*m_whiteBox, seed);
                    const auto axis = (endpoints[1] - endpoints[0]).GetNormalized();
                    for (int cut = 1; cut <= count; ++cut)
                    {
                        int verticesOnCut = 0;
                        const auto point = endpoints[0].Lerp(endpoints[1], (static_cast<float>(cut) + slide * 0.98f) / (count + 1));
                        for (const auto vertex : Api::MeshVertexHandles(*mesh))
                        {
                            if (vertex.Index() >= 8 &&
                                AZStd::abs((Api::VertexPosition(*mesh, vertex) - point).Dot(axis)) < 1e-5f)
                            {
                                ++verticesOnCut;
                            }
                        }
                        EXPECT_EQ(verticesOnCut, 4);
                    }
                }
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, InsertLoopCrossesOpenQuadStripWithoutCracks)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        Api::VertexHandles lower;
        Api::VertexHandles upper;
        for (int x = 0; x < 3; ++x)
        {
            lower.push_back(Api::AddVertex(*mesh, AZ::Vector3(static_cast<float>(x), 0, 0)));
            upper.push_back(Api::AddVertex(*mesh, AZ::Vector3(static_cast<float>(x), 1, 0)));
        }
        for (size_t i = 0; i < 2; ++i)
        {
            Api::AddQuadPolygon(*mesh, lower[i], lower[i + 1], upper[i + 1], upper[i]);
        }
        Api::CalculateNormals(*mesh);
        Api::EdgeHandle seed;
        for (const auto edge : Api::MeshPolygonEdgeHandles(*mesh))
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            if (points[0].GetX() == 0 && points[1].GetX() == 0) { seed = edge; break; }
        }
        ASSERT_TRUE(seed.IsValid());
        AZStd::string error;
        ASSERT_TRUE(Api::InsertEdgeLoop(*mesh, seed, 0.3f, error)) << error.c_str();
        EXPECT_EQ(Api::MeshVertexCount(*mesh), 9);
        EXPECT_EQ(Api::MeshFaceCount(*mesh), 8);
        EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 4);
        size_t boundary = 0;
        for (const auto edge : Api::MeshEdgeHandles(*mesh))
        {
            boundary += Api::EdgeIsBoundary(*mesh, edge) ? 1 : 0;
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            if (points[0].GetX() == 1 && points[1].GetX() == 1)
            {
                EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
            }
        }
        EXPECT_EQ(boundary, 8);
        for (const auto face : Api::MeshFaceHandles(*mesh)) { EXPECT_GT(Api::FaceNormal(*mesh, face).GetZ(), 0.99f); }
    }

    TEST_F(WhiteBoxTestFixture, InsertLoopRejectsTriangleTerminationWithoutChangingMesh)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        const auto a = Api::AddVertex(*mesh, AZ::Vector3(0, 0, 0));
        const auto b = Api::AddVertex(*mesh, AZ::Vector3(1, 0, 0));
        const auto c = Api::AddVertex(*mesh, AZ::Vector3(1, 1, 0));
        const auto d = Api::AddVertex(*mesh, AZ::Vector3(0, 1, 0));
        const auto e = Api::AddVertex(*mesh, AZ::Vector3(2, 0.5f, 0));
        Api::AddQuadPolygon(*mesh, a, b, c, d);
        Api::AddTriPolygon(*mesh, c, b, e);
        Api::CalculateNormals(*mesh);
        Api::CalculatePlanarUVs(*mesh);
        Api::EdgeHandle seed;
        for (const auto edge : Api::MeshPolygonEdgeHandles(*mesh))
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            if (points[0].GetX() == 0 && points[1].GetX() == 0) { seed = edge; break; }
        }
        ASSERT_TRUE(seed.IsValid());
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        AZStd::string error;
        EXPECT_FALSE(Api::InsertEdgeLoop(*mesh, seed, 0.5f, error));
        EXPECT_FALSE(error.empty());
        AZStd::vector<AZ::Vector3> preview;
        EXPECT_FALSE(Api::PreviewEdgeLoops(*mesh, seed, 3, preview, error));
        EXPECT_TRUE(preview.empty());
        EXPECT_FALSE(Api::InsertEdgeLoops(*mesh, seed, 3, error));
        EXPECT_FALSE(Api::InsertEdgeLoops(*mesh, seed, 0, error));
        EXPECT_FALSE(Api::InsertEdgeLoops(*mesh, seed, 65, error));
        EXPECT_FALSE(Api::InsertEdgeLoop(*mesh, seed, 0.0f, error));
        EXPECT_FALSE(Api::InsertEdgeLoop(*mesh, Api::EdgeHandle{999999}, 0.5f, error));
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxTestFixture, WeldSelectedVerticesSupportsCenterAndLastTargetsAndPreservesFaceData)
    {
        namespace Api = WhiteBox::Api;
        auto source = Api::CreateWhiteBoxMesh();
        const auto a = Api::AddVertex(*source, AZ::Vector3(0, 0, 0));
        const auto b = Api::AddVertex(*source, AZ::Vector3(1, 0, 0));
        const auto c = Api::AddVertex(*source, AZ::Vector3(1, 1, 0));
        const auto d = Api::AddVertex(*source, AZ::Vector3(0, 1, 0));
        const auto polygon = Api::AddQuadPolygon(*source, a, b, c, d);
        Api::CalculateNormals(*source);
        Api::CalculatePlanarUVs(*source);
        Api::HideVertex(*source, d);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:1");
        Api::SetPolygonMaterial(*source, polygon, material);
        for (const auto face : polygon.m_faceHandles) { Api::SetFacePaintColor(*source, face, 0xFF00FF00u); }
        Api::FaceHandle surviving;
        for (const auto face : polygon.m_faceHandles)
        {
            const auto vertices = Api::FaceVertexHandles(*source, face);
            const bool hasA = AZStd::find(vertices.begin(), vertices.end(), a) != vertices.end();
            const bool hasB = AZStd::find(vertices.begin(), vertices.end(), b) != vertices.end();
            if (!(hasA && hasB)) { surviving = face; }
        }
        ASSERT_TRUE(surviving.IsValid());
        for (const bool toLast : {false, true})
        {
            auto mesh = Api::CloneMesh(*source);
            AZStd::string error;
            ASSERT_TRUE(Api::WeldVertices(*mesh, {a, b}, toLast, error)) << error.c_str();
            EXPECT_EQ(Api::MeshVertexCount(*mesh), 3);
            EXPECT_EQ(Api::MeshFaceCount(*mesh), 1);
            EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 1);
            const auto target = toLast ? AZ::Vector3(1, 0, 0) : AZ::Vector3(0.5f, 0, 0);
            size_t targetCount = 0;
            for (const auto vertex : Api::MeshVertexHandles(*mesh))
            {
                const auto position = Api::VertexPosition(*mesh, vertex);
                targetCount += position.IsClose(target) ? 1 : 0;
                if (position.IsClose(AZ::Vector3(0, 1, 0))) { EXPECT_TRUE(Api::VertexIsHidden(*mesh, vertex)); }
            }
            EXPECT_EQ(targetCount, 1);
            const auto result = Api::MeshFaceHandles(*mesh).front();
            EXPECT_EQ(Api::FaceMaterial(*mesh, result), material);
            EXPECT_EQ(Api::FacePaintColor(*mesh, result), 0xFF00FF00u);
            EXPECT_GT(Api::FaceNormal(*mesh, result).GetZ(), 0.99f);
            for (const auto oldHalfedge : Api::FaceHalfedgeHandles(*source, surviving))
            {
                const auto oldVertex = Api::HalfedgeVertexHandleAtTip(*source, oldHalfedge);
                const auto expected = (oldVertex == a || oldVertex == b) ? target : Api::VertexPosition(*source, oldVertex);
                bool found = false;
                for (const auto halfedge : Api::FaceHalfedgeHandles(*mesh, result))
                {
                    if (Api::HalfedgeVertexPositionAtTip(*mesh, halfedge).IsClose(expected))
                    {
                        EXPECT_TRUE(Api::HalfedgeUV(*mesh, halfedge).IsClose(Api::HalfedgeUV(*source, oldHalfedge)));
                        found = true;
                    }
                }
                EXPECT_TRUE(found);
            }
            Api::WhiteBoxMeshStream saved;
            ASSERT_TRUE(Api::WriteMesh(*mesh, saved));
            auto restored = Api::CreateWhiteBoxMesh();
            ASSERT_EQ(Api::ReadMesh(*restored, saved), Api::ReadResult::Full);
            EXPECT_EQ(Api::MeshPolygonHandles(*restored).size(), 1);
            EXPECT_EQ(Api::FaceMaterial(*restored, Api::MeshFaceHandles(*restored).front()), material);
        }
    }

    TEST_F(WhiteBoxTestFixture, WeldCornerEdgeKeepsBothWallsAtOneMergedVertex)
    {
        namespace Api = WhiteBox::Api;
        auto source = Api::CreateWhiteBoxMesh();
        const auto bottom = Api::AddVertex(*source, AZ::Vector3(0, 0, 0));
        const auto top = Api::AddVertex(*source, AZ::Vector3(0, 0, 2));
        const auto leftBottom = Api::AddVertex(*source, AZ::Vector3(-2, 0, 0));
        const auto leftTop = Api::AddVertex(*source, AZ::Vector3(-2, 0, 2));
        const auto rightBottom = Api::AddVertex(*source, AZ::Vector3(0, 2, 0));
        const auto rightTop = Api::AddVertex(*source, AZ::Vector3(0, 2, 2));
        const auto left = Api::AddQuadPolygon(*source, leftBottom, bottom, top, leftTop);
        const auto right = Api::AddQuadPolygon(*source, bottom, rightBottom, rightTop, top);
        const auto leftMaterial = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:1");
        const auto rightMaterial = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:2");
        Api::SetPolygonMaterial(*source, left, leftMaterial);
        Api::SetPolygonMaterial(*source, right, rightMaterial);
        for (const auto face : left.m_faceHandles) { Api::SetFacePaintColor(*source, face, 0xFF0000FFu); }
        for (const auto face : right.m_faceHandles) { Api::SetFacePaintColor(*source, face, 0xFF00FF00u); }
        Api::CalculateNormals(*source);
        Api::CalculatePlanarUVs(*source);
        for (const bool toLast : {false, true})
        {
            auto mesh = Api::CloneMesh(*source);
            AZStd::string error;
            ASSERT_TRUE(Api::WeldVertices(*mesh, {bottom, top}, toLast, error)) << error.c_str();
            EXPECT_EQ(Api::MeshVertexCount(*mesh), 5);
            EXPECT_EQ(Api::MeshFaceCount(*mesh), 2);
            EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 2);
            const auto target = AZ::Vector3(0, 0, toLast ? 2.0f : 1.0f);
            Api::VertexHandle merged;
            size_t targetCount = 0;
            for (const auto vertex : Api::MeshVertexHandles(*mesh))
            {
                if (Api::VertexPosition(*mesh, vertex).IsClose(target))
                {
                    merged = vertex;
                    ++targetCount;
                }
            }
            ASSERT_EQ(targetCount, 1); // One shared vertex, not overlapping duplicates.
            EXPECT_EQ(Api::VertexEdgeHandles(*mesh, merged).size(), 4);
            size_t leftFaces = 0;
            for (const auto face : Api::MeshFaceHandles(*mesh))
            {
                const auto vertices = Api::FaceVertexHandles(*mesh, face);
                EXPECT_NE(AZStd::find(vertices.begin(), vertices.end(), merged), vertices.end());
                const bool isLeft = Api::FaceMaterial(*mesh, face) == leftMaterial;
                leftFaces += isLeft ? 1 : 0;
                EXPECT_EQ(Api::FaceMaterial(*mesh, face), isLeft ? leftMaterial : rightMaterial);
                EXPECT_EQ(Api::FacePaintColor(*mesh, face), isLeft ? 0xFF0000FFu : 0xFF00FF00u);
                EXPECT_GT(Api::FaceNormal(*mesh, face).Dot(
                    isLeft ? -AZ::Vector3::CreateAxisY() : AZ::Vector3::CreateAxisX()), 0.99f);
            }
            EXPECT_EQ(leftFaces, 1);
            Api::WhiteBoxMeshStream saved;
            ASSERT_TRUE(Api::WriteMesh(*mesh, saved));
            auto restored = Api::CreateWhiteBoxMesh();
            ASSERT_EQ(Api::ReadMesh(*restored, saved), Api::ReadResult::Full);
            EXPECT_EQ(Api::MeshVertexCount(*restored), 5);
            EXPECT_EQ(Api::MeshFaceCount(*restored), 2);
            EXPECT_EQ(Api::MeshPolygonHandles(*restored).size(), 2);
        }
    }

    TEST_F(WhiteBoxTestFixture, WeldRejectsPointOnlyConnectionsAndInvalidSelectionsWithoutMutation)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        const auto a = Api::AddVertex(*mesh, AZ::Vector3(0, 0, 0));
        const auto b = Api::AddVertex(*mesh, AZ::Vector3(1, 0, 0));
        const auto c = Api::AddVertex(*mesh, AZ::Vector3(0, 1, 0));
        const auto d = Api::AddVertex(*mesh, AZ::Vector3(3, 0, 0));
        const auto e = Api::AddVertex(*mesh, AZ::Vector3(4, 0, 0));
        const auto f = Api::AddVertex(*mesh, AZ::Vector3(3, 1, 0));
        Api::AddTriPolygon(*mesh, a, b, c);
        Api::AddTriPolygon(*mesh, d, e, f);
        Api::CalculateNormals(*mesh);
        Api::CalculatePlanarUVs(*mesh);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        AZStd::string error;
        EXPECT_FALSE(Api::WeldVertices(*mesh, {b, d}, false, error));
        EXPECT_FALSE(error.empty());
        EXPECT_FALSE(Api::WeldVertices(*mesh, {a, a}, false, error));
        EXPECT_FALSE(Api::WeldVertices(*mesh, {a, Api::VertexHandle{999999}}, true, error));
        EXPECT_FALSE(Api::WeldVertices(*mesh, {a, b, c, d, e, f}, false, error));
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxTestFixture, ExtrudeOpenBoundaryExtendsSurfaceAndPreservesPaint)
    {
        namespace Api = WhiteBox::Api;
        auto original = Api::CreateWhiteBoxMesh();
        const auto a = Api::AddVertex(*original, AZ::Vector3(0, 0, 0));
        const auto b = Api::AddVertex(*original, AZ::Vector3(1, 0, 0));
        const auto c = Api::AddVertex(*original, AZ::Vector3(1, 1, 0));
        const auto d = Api::AddVertex(*original, AZ::Vector3(0, 1, 0));
        const auto polygon = Api::AddQuadPolygon(*original, a, b, c, d);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:1");
        Api::SetPolygonMaterial(*original, polygon, material);
        for (const auto face : polygon.m_faceHandles) { Api::SetFacePaintColor(*original, face, 0xFF00FF00u); }
        Api::CalculateNormals(*original);
        Api::CalculatePlanarUVs(*original);
        // Exercise all four boundary orientations, including either halfedge being empty.
        for (const auto edge : Api::MeshEdgeHandles(*original))
        {
            if (!Api::EdgeIsBoundary(*original, edge)) { continue; }
            auto mesh = Api::CloneMesh(*original);
            const auto displacement = (Api::EdgeMidpoint(*mesh, edge) - AZ::Vector3(0.5f, 0.5f, 0)).GetNormalized();
            const auto extended = Api::TranslateEdgeAppend(*mesh, edge, displacement);
            EXPECT_NE(extended, edge);
            ASSERT_TRUE(extended.IsValid());
            EXPECT_EQ(Api::MeshVertexCount(*mesh), 6);
            EXPECT_EQ(Api::MeshFaceCount(*mesh), 4);
            EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 2);
            EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
            EXPECT_EQ(Api::EdgeFaceHandles(*mesh, extended).size(), 1);
            for (const auto face : Api::MeshFaceHandles(*mesh))
            {
                EXPECT_GT(Api::FaceNormal(*mesh, face).GetZ(), 0.99f);
                EXPECT_EQ(Api::FaceMaterial(*mesh, face), material);
                EXPECT_EQ(Api::FacePaintColor(*mesh, face), 0xFF00FF00u);
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, ExtrudeInternalOrInvalidEdgeDoesNotMutateMesh)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        for (const auto edge : Api::MeshEdgeHandles(*m_whiteBox))
        {
            const auto faces = Api::EdgeFaceHandles(*m_whiteBox, edge);
            if (faces.size() == 2 &&
                Api::FacePolygonHandle(*m_whiteBox, faces[0]) == Api::FacePolygonHandle(*m_whiteBox, faces[1]))
            {
                EXPECT_EQ(Api::TranslateEdgeAppend(*m_whiteBox, edge, AZ::Vector3(1, 1, 1)), edge);
            }
        }
        EXPECT_EQ(Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{}, AZ::Vector3::CreateAxisX()), Api::EdgeHandle{});
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxTestFixture, BridgeBoundaryEdgesSharesVerticesAndPreservesExistingUvs)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        for (const float x : {0.0f, 2.0f})
        {
            const auto a = Api::AddVertex(*mesh, AZ::Vector3(x, 0, 0));
            const auto b = Api::AddVertex(*mesh, AZ::Vector3(x + 1, 0, 0));
            const auto c = Api::AddVertex(*mesh, AZ::Vector3(x + 1, 1, 0));
            const auto d = Api::AddVertex(*mesh, AZ::Vector3(x, 1, 0));
            Api::AddQuadPolygon(*mesh, a, b, c, d);
        }
        Api::CalculateNormals(*mesh);
        Api::CalculatePlanarUVs(*mesh);
        Api::HalfedgeHandles oldHalfedges;
        AZStd::vector<AZ::Vector2> oldUvs;
        for (const auto face : Api::MeshFaceHandles(*mesh))
        {
            for (const auto halfedge : Api::FaceHalfedgeHandles(*mesh, face))
            {
                oldHalfedges.push_back(halfedge);
                oldUvs.push_back(Api::HalfedgeUV(*mesh, halfedge));
            }
        }
        Api::EdgeHandles edges;
        for (const auto edge : Api::MeshEdgeHandles(*mesh))
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            if ((points[0].GetX() == 1.0f && points[1].GetX() == 1.0f) ||
                (points[0].GetX() == 2.0f && points[1].GetX() == 2.0f))
            {
                edges.push_back(edge);
            }
        }
        ASSERT_EQ(edges.size(), 2);
        AZStd::string error;
        ASSERT_TRUE(Api::BridgeSelection(*mesh, {}, edges, error)) << error.c_str();
        EXPECT_EQ(Api::MeshVertexCount(*mesh), 8);
        EXPECT_EQ(Api::MeshFaceCount(*mesh), 6);
        EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 3);
        for (const auto edge : edges) { EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2); }
        for (size_t i = 0; i < oldHalfedges.size(); ++i)
        {
            EXPECT_TRUE(Api::HalfedgeUV(*mesh, oldHalfedges[i]).IsClose(oldUvs[i]));
        }
        for (const auto face : Api::MeshFaceHandles(*mesh))
        {
            EXPECT_GT(Api::FaceNormal(*mesh, face).GetZ(), 0.99f);
        }
    }

    TEST_F(WhiteBoxTestFixture, BridgeFreeformPolygonsKeepsOriginalFacesAndConnectsNearestBoundary)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        const AZStd::vector<AZ::Vector3> quad{
            AZ::Vector3(0, 0, 0), AZ::Vector3(1, 0, 0), AZ::Vector3(1, 1, 0), AZ::Vector3(0, 1, 0)};
        const AZStd::vector<AZ::Vector3> triangle{
            AZ::Vector3(2, 0, 0), AZ::Vector3(3, 0.5f, 0), AZ::Vector3(2, 1, 0)};
        ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
            *mesh, AZ::Transform::CreateIdentity(), quad, AZ::Vector3::CreateAxisZ()));
        ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
            *mesh, AZ::Transform::CreateIdentity(), triangle, AZ::Vector3::CreateAxisZ()));
        Api::CalculateNormals(*mesh);
        const auto polygons = Api::MeshPolygonHandles(*mesh);
        ASSERT_EQ(polygons.size(), 2);
        AZStd::string error;
        ASSERT_TRUE(Api::BridgeSelection(*mesh, polygons, {}, error)) << error.c_str();
        EXPECT_EQ(Api::MeshVertexCount(*mesh), 7);
        EXPECT_EQ(Api::MeshFaceCount(*mesh), 5);
        EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 3);
        for (const auto& polygon : polygons)
        {
            for (const auto face : polygon.m_faceHandles)
            {
                EXPECT_EQ(Api::FacePolygonHandle(*mesh, face), polygon);
            }
        }
        for (const auto edge : Api::MeshEdgeHandles(*mesh))
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            if ((points[0].GetX() == 1.0f && points[1].GetX() == 1.0f) ||
                (points[0].GetX() == 2.0f && points[1].GetX() == 2.0f))
            {
                EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, BridgeFacingOpenPolygonsDoesNotDeleteSourceFaces)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        const AZStd::vector<AZ::Vector3> lower{
            AZ::Vector3(0, 0, 0), AZ::Vector3(1, 0, 0), AZ::Vector3(1, 1, 0), AZ::Vector3(0, 1, 0)};
        const AZStd::vector<AZ::Vector3> upper{
            AZ::Vector3(0, 0, 1), AZ::Vector3(0, 1, 1), AZ::Vector3(1, 1, 1), AZ::Vector3(1, 0, 1)};
        ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
            *mesh, AZ::Transform::CreateIdentity(), lower, AZ::Vector3::CreateAxisZ()));
        ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
            *mesh, AZ::Transform::CreateIdentity(), upper, -AZ::Vector3::CreateAxisZ()));
        Api::CalculateNormals(*mesh);
        const auto polygons = Api::MeshPolygonHandles(*mesh);
        ASSERT_EQ(polygons.size(), 2);
        const auto oldFaces = Api::MeshFaceHandles(*mesh);
        const auto oldPositions = Api::FacesPositions(*mesh, oldFaces);
        AZStd::string error;
        ASSERT_TRUE(Api::BridgeSelection(*mesh, polygons, {}, error)) << error.c_str();
        EXPECT_EQ(Api::MeshVertexCount(*mesh), 8);
        EXPECT_EQ(Api::MeshFaceCount(*mesh), 6);
        EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 3);
        EXPECT_EQ(Api::FacesPositions(*mesh, oldFaces), oldPositions);
        for (const auto& polygon : polygons)
        {
            for (const auto face : polygon.m_faceHandles)
            {
                EXPECT_EQ(Api::FacePolygonHandle(*mesh, face), polygon);
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, BridgePolygonCapsProducesClosedSolidAndPreservesFaceProperties)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto other = Api::CreateWhiteBoxMesh();
        Api::InitializeAsUnitCube(*other);
        for (const auto vertex : Api::MeshVertexHandles(*other))
        {
            Api::SetVertexPosition(*other, vertex, Api::VertexPosition(*other, vertex) + AZ::Vector3(3, 0, 0));
        }
        WhiteBox::AppendMesh(*m_whiteBox, *other);
        Api::CalculateNormals(*m_whiteBox);
        Api::PolygonHandles caps(2);
        const auto bridgeMaterial = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:1");
        const auto retainedMaterial = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:2");
        for (const auto& polygon : Api::MeshPolygonHandles(*m_whiteBox))
        {
            const auto normal = Api::PolygonNormal(*m_whiteBox, polygon);
            const float x = Api::FaceVertexPositions(*m_whiteBox, polygon.m_faceHandles.front())[0].GetX();
            if (normal.GetX() > 0.99f && x < 1.0f) { caps[0] = polygon; }
            if (normal.GetX() < -0.99f && x > 2.0f) { caps[1] = polygon; }
            Api::SetPolygonMaterial(*m_whiteBox, polygon, retainedMaterial);
            for (const auto face : polygon.m_faceHandles) { Api::SetFacePaintColor(*m_whiteBox, face, 0xFFFF0000u); }
        }
        ASSERT_FALSE(caps[0].m_faceHandles.empty());
        ASSERT_FALSE(caps[1].m_faceHandles.empty());
        Api::SetPolygonMaterial(*m_whiteBox, caps[0], bridgeMaterial);
        for (const auto face : caps[0].m_faceHandles) { Api::SetFacePaintColor(*m_whiteBox, face, 0xFF0000FFu); }
        AZStd::string error;
        ASSERT_TRUE(Api::BridgeSelection(*m_whiteBox, caps, {}, error)) << error.c_str();
        EXPECT_EQ(Api::MeshVertexCount(*m_whiteBox), 16);
        EXPECT_EQ(Api::MeshFaceCount(*m_whiteBox), 28);
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), 14);
        for (const auto edge : Api::MeshEdgeHandles(*m_whiteBox))
        {
            EXPECT_EQ(Api::EdgeFaceHandles(*m_whiteBox, edge).size(), 2);
        }
        size_t bridgeFaces = 0;
        double volume = 0.0;
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            const bool bridge = Api::FaceMaterial(*m_whiteBox, face) == bridgeMaterial;
            bridgeFaces += bridge ? 1 : 0;
            EXPECT_EQ(Api::FaceMaterial(*m_whiteBox, face), bridge ? bridgeMaterial : retainedMaterial);
            EXPECT_EQ(Api::FacePaintColor(*m_whiteBox, face), bridge ? 0xFF0000FFu : 0xFFFF0000u);
            const auto points = Api::FaceVertexPositions(*m_whiteBox, face);
            volume += points[0].Dot(points[1].Cross(points[2])) / 6.0;
        }
        EXPECT_EQ(bridgeFaces, 8);
        EXPECT_NEAR(volume, 4.0, 1e-5);
        // Face groups must remain valid after removing caps and compacting face handles.
        size_t groupedFaces = 0;
        for (const auto& polygon : Api::MeshPolygonHandles(*m_whiteBox))
        {
            for (const auto face : polygon.m_faceHandles)
            {
                EXPECT_EQ(Api::FacePolygonHandle(*m_whiteBox, face), polygon);
                ++groupedFaces;
            }
        }
        EXPECT_EQ(groupedFaces, 28);
    }

    TEST_F(WhiteBoxTestFixture, BridgeCollapsedStripDiscardsCandidateWithoutChangingInput)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        // Separate vertex handles, but coincident boundaries: topology selection is valid,
        // while the connecting strip has zero area and must fail after candidate creation.
        for (int copy = 0; copy < 2; ++copy)
        {
            const auto a = Api::AddVertex(*mesh, AZ::Vector3(0, 0, 0));
            const auto b = Api::AddVertex(*mesh, AZ::Vector3(1, 0, 0));
            const auto c = Api::AddVertex(*mesh, AZ::Vector3(1, 1, 0));
            const auto d = Api::AddVertex(*mesh, AZ::Vector3(0, 1, 0));
            Api::AddQuadPolygon(*mesh, a, b, c, d);
        }
        Api::EdgeHandles edges;
        for (const auto edge : Api::MeshEdgeHandles(*mesh))
        {
            const auto points = Api::EdgeVertexPositions(*mesh, edge);
            if (points[0].GetX() == 1.0f && points[1].GetX() == 1.0f) { edges.push_back(edge); }
        }
        ASSERT_EQ(edges.size(), 2);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        AZStd::string error;
        EXPECT_FALSE(Api::BridgeSelection(*mesh, {}, edges, error));
        EXPECT_FALSE(error.empty());
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxTestFixture, BridgeInvalidSelectionLeavesMeshUnchanged)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        const auto polygons = Api::MeshPolygonHandles(*m_whiteBox);
        const auto edges = Api::MeshEdgeHandles(*m_whiteBox);
        AZStd::string error;
        EXPECT_FALSE(Api::BridgeSelection(*m_whiteBox, {}, {edges[0], edges[1]}, error));
        EXPECT_FALSE(error.empty());
        EXPECT_FALSE(Api::BridgeSelection(*m_whiteBox, {polygons[0], polygons[0]}, {}, error));
        EXPECT_FALSE(Api::BridgeSelection(*m_whiteBox, {}, {}, error));
        EXPECT_FALSE(Api::BridgeSelection(*m_whiteBox, {}, {Api::EdgeHandle{999999}, edges[0]}, error));
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxTestFixture, RingPrimitivesAreClosedAndPlaneHasAnOpenBoundary)
    {
        namespace Api = WhiteBox::Api;
        for (const auto shape : {WhiteBox::DrawShapeType::Pipe, WhiteBox::DrawShapeType::Torus})
        {
            SCOPED_TRACE(static_cast<int>(shape));
            for (const float hole : {0.1f, 0.5f, 0.9f})
            {
                auto mesh = WhiteBox::BuildParametricShapeMesh(
                    shape, 2.0f, 3.0f, 0.5f, 24, 8, 0.15f, 0.1f, true, false,
                    true, 0.0f, 0.5f, 270.0f, false, 0.25f, hole);
                ASSERT_FALSE(Api::MeshFaceHandles(*mesh).empty());
                for (const auto edge : Api::MeshEdgeHandles(*mesh))
                {
                    EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
                }
                double volume = 0.0;
                for (const auto face : Api::MeshFaceHandles(*mesh))
                {
                    const auto points = Api::FaceVertexPositions(*mesh, face);
                    volume += points[0].Dot(points[1].Cross(points[2])) / 6.0;
                }
                EXPECT_GT(volume, 0.0);
            }
        }
        auto plane = WhiteBox::BuildParametricShapeMesh(WhiteBox::DrawShapeType::Plane, 2, 3, 1, 4, 8);
        EXPECT_EQ(Api::MeshFaceHandles(*plane).size(), 2);
        size_t boundaryEdges = 0;
        for (const auto edge : Api::MeshEdgeHandles(*plane))
        {
            boundaryEdges += Api::EdgeFaceHandles(*plane, edge).size() == 1 ? 1 : 0;
        }
        EXPECT_EQ(boundaryEdges, 4);
    }

    TEST_F(WhiteBoxTestFixture, ExtrudeAndInsetKeepAdjacentPolygonsAsOneRegion)
    {
        namespace Api = WhiteBox::Api;
        for (const bool inset : {false, true})
        {
            auto mesh = Api::CreateWhiteBoxMesh();
            Api::VertexHandles vertices;
            for (int y = 0; y < 2; ++y)
            {
                for (int x = 0; x < 3; ++x)
                {
                    vertices.push_back(Api::AddVertex(*mesh, AZ::Vector3(float(x), float(y), 0)));
                }
            }
            const Api::PolygonHandles selection{
                Api::AddQuadPolygon(*mesh, vertices[0], vertices[1], vertices[4], vertices[3]),
                Api::AddQuadPolygon(*mesh, vertices[1], vertices[2], vertices[5], vertices[4])};
            const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:7");
            Api::SetPolygonMaterial(*mesh, selection[0], material);
            for (const auto face : selection[0].m_faceHandles) { Api::SetFacePaintColor(*mesh, face, 0xff443322); }
            Api::CalculateNormals(*mesh);
            Api::CalculatePlanarUVs(*mesh);
            Api::PolygonHandles result;
            AZStd::string error;
            ASSERT_TRUE(Api::ExtrudeInsetRegions(*mesh, selection, inset ? 0.2f : 0.25f, inset, result, error))
                << error.c_str();
            ASSERT_EQ(result.size(), 2);
            EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 8); // two caps + six perimeter walls, no interior wall
            EXPECT_EQ(Api::MeshFaceHandles(*mesh).size(), 16);
            size_t materialFaces = 0;
            float capArea = 0.0f;
            Api::VertexHandles capVertices;
            for (const auto& polygon : result)
            {
                for (const auto vertex : Api::PolygonVertexHandles(*mesh, polygon))
                {
                    if (AZStd::find(capVertices.begin(), capVertices.end(), vertex) == capVertices.end())
                    {
                        capVertices.push_back(vertex);
                    }
                    EXPECT_NEAR(Api::VertexPosition(*mesh, vertex).GetZ(), inset ? 0.0f : 0.25f, 1e-5f);
                }
                for (const auto face : polygon.m_faceHandles)
                {
                    if (Api::FaceMaterial(*mesh, face) == material)
                    {
                        ++materialFaces;
                        EXPECT_EQ(Api::FacePaintColor(*mesh, face), 0xff443322);
                    }
                    const auto points = Api::FaceVertexPositions(*mesh, face);
                    capArea += (points[1] - points[0]).Cross(points[2] - points[0]).GetLength() * 0.5f;
                }
            }
            EXPECT_EQ(capVertices.size(), 6); // shared cap edge still shares its two vertices
            EXPECT_EQ(materialFaces, 2);
            EXPECT_NEAR(capArea, inset ? 1.28f : 2.0f, 1e-5f);
        }
    }

    TEST_F(WhiteBoxTestFixture, ExtrudeRegionOnCubeIsClosedAndUnsupportedInsetLeavesMeshUnchanged)
    {
        namespace Api = WhiteBox::Api;
        auto mesh = Api::CreateWhiteBoxMesh();
        const auto polygons = Api::InitializeAsUnitCube(*mesh);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        Api::PolygonHandles result;
        AZStd::string error;
        // The top and front form one connected, non-planar region.
        EXPECT_FALSE(Api::ExtrudeInsetRegions(*mesh, {polygons[0], polygons[2]}, 0.2f, true, result, error));
        EXPECT_TRUE(result.empty());
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
        ASSERT_TRUE(Api::ExtrudeInsetRegions(*mesh, {polygons[0]}, 0.25f, false, result, error)) << error.c_str();
        ASSERT_EQ(result.size(), 1);
        double volume = 0.0;
        for (const auto edge : Api::MeshEdgeHandles(*mesh)) { EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2); }
        for (const auto face : Api::MeshFaceHandles(*mesh))
        {
            const auto p = Api::FaceVertexPositions(*mesh, face);
            volume += p[0].Dot(p[1].Cross(p[2])) / 6.0;
        }
        EXPECT_NEAR(volume, 1.25, 1e-5);
    }

    TEST_F(WhiteBoxTestFixture, FreeformPolygonSupportsConcaveOutlinesAndRejectsCrossingsWithoutMutation)
    {
        namespace Api = WhiteBox::Api;
        const AZStd::vector<AZ::Vector3> outline{
            AZ::Vector3(0, 0, 0), AZ::Vector3(2, 0, 0), AZ::Vector3(2, 2, 0),
            AZ::Vector3(1, 1, 0), AZ::Vector3(0, 2, 0)};
        auto mesh = Api::CreateWhiteBoxMesh();
        ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
            *mesh, AZ::Transform::CreateIdentity(), outline, AZ::Vector3::CreateAxisZ()));
        EXPECT_EQ(Api::MeshFaceHandles(*mesh).size(), 3);
        EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 1);
        float area = 0.0f;
        for (const auto face : Api::MeshFaceHandles(*mesh))
        {
            const auto p = Api::FaceVertexPositions(*mesh, face);
            const float signedArea = (p[1] - p[0]).Cross(p[2] - p[0]).GetZ() * 0.5f;
            EXPECT_GT(signedArea, 0.0f);
            area += signedArea;
        }
        EXPECT_NEAR(area, 3.0f, 1e-5f);
        const auto before = Api::MeshVertexCount(*mesh);
        const AZStd::vector<AZ::Vector3> crossing{
            AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 0), AZ::Vector3(0, 2, 0), AZ::Vector3(2, 0, 0)};
        EXPECT_FALSE(WhiteBox::Detail::BuildPolygonFace(
            *mesh, AZ::Transform::CreateIdentity(), crossing, AZ::Vector3::CreateAxisZ()));
        EXPECT_FALSE(WhiteBox::Detail::BuildPolygonFace(
            *mesh, AZ::Transform::CreateIdentity(), crossing, AZ::Vector3::CreateAxisZ(), 2.0f));
        EXPECT_EQ(Api::MeshVertexCount(*mesh), before);
        EXPECT_EQ(Api::MeshFaceHandles(*mesh).size(), 3);
    }

    TEST_F(WhiteBoxTestFixture, FreeformPolygonExtrusionIsClosedForConcaveOutlinesBothWindingsAndDepthSigns)
    {
        namespace Api = WhiteBox::Api;
        const AZStd::vector<AZ::Vector3> outline{
            AZ::Vector3(0, 0, 0), AZ::Vector3(2, 0, 0), AZ::Vector3(2, 2, 0),
            AZ::Vector3(1, 1, 0), AZ::Vector3(0, 2, 0)};
        for (const bool reverse : {false, true})
        {
            for (const float height : {-2.0f, 2.0f})
            {
                // Draw on a tilted world plane and store on a scaled entity.
                auto planeToWorld = AZ::Transform::CreateRotationX(1.0f);
                planeToWorld.SetTranslation(AZ::Vector3(5.0f, 7.0f, 3.0f));
                AZStd::vector<AZ::Vector3> points;
                for (size_t i = 0; i < outline.size(); ++i)
                {
                    points.push_back(planeToWorld.TransformPoint(outline[reverse ? outline.size() - 1 - i : i]));
                }
                auto mesh = Api::CreateWhiteBoxMesh();
                ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
                    *mesh, AZ::Transform::CreateUniformScale(0.5f), points,
                    planeToWorld.TransformVector(AZ::Vector3::CreateAxisZ()), height));
                EXPECT_EQ(Api::MeshVertexCount(*mesh), 10);
                EXPECT_EQ(Api::MeshFaceHandles(*mesh).size(), 16);
                EXPECT_EQ(Api::MeshPolygonHandles(*mesh).size(), 7);
                for (const auto edge : Api::MeshEdgeHandles(*mesh))
                {
                    EXPECT_EQ(Api::EdgeFaceHandles(*mesh, edge).size(), 2);
                }
                double signedVolume = 0.0;
                for (const auto face : Api::MeshFaceHandles(*mesh))
                {
                    const auto positions = Api::FaceVertexPositions(*mesh, face);
                    signedVolume += positions[0].Dot(positions[1].Cross(positions[2])) / 6.0;
                }
                // Concave outline area = 3; depth = 2; local uniform scale = 0.5.
                EXPECT_NEAR(signedVolume, 0.75, 1e-4);
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, ExtrudedConcavePolygonCanCarveAndUnionInBothPullDirections)
    {
        namespace Api = WhiteBox::Api;
        struct BooleanCase
        {
            Api::BooleanOperation m_operation;
            float m_baseHeight;
            float m_depth;
            double m_expectedVolume;
        };
        const BooleanCase cases[]{
            {Api::BooleanOperation::Subtraction, 4.01f, -2.01f, 58.0},
            {Api::BooleanOperation::Union, 3.0f, 2.0f, 67.0},
            {Api::BooleanOperation::Union, 1.0f, -2.0f, 67.0}};
        for (const auto& example : cases)
        {
            auto target = Api::CreateWhiteBoxMesh();
            auto cutter = Api::CreateWhiteBoxMesh();
            ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
                *target, AZ::Transform::CreateIdentity(),
                {AZ::Vector3(0, 0, 0), AZ::Vector3(4, 0, 0), AZ::Vector3(4, 4, 0), AZ::Vector3(0, 4, 0)},
                AZ::Vector3::CreateAxisZ(), 4.0f));
            const float z = example.m_baseHeight;
            // Concave footprint area is 3. The subtraction cuts a two-unit-deep pocket;
            // either union extends one unit beyond the target, adding volume 3.
            ASSERT_TRUE(WhiteBox::Detail::BuildPolygonFace(
                *cutter, AZ::Transform::CreateIdentity(),
                {AZ::Vector3(1, 1, z), AZ::Vector3(3, 1, z), AZ::Vector3(3, 3, z),
                 AZ::Vector3(2, 2, z), AZ::Vector3(1, 3, z)},
                AZ::Vector3::CreateAxisZ(), example.m_depth));
            Api::CalculateNormals(*target);
            Api::CalculateNormals(*cutter);
            ASSERT_TRUE(Api::ApplyMeshBoolean(
                *target, *cutter, AZ::Transform::CreateIdentity(), example.m_operation));
            double volume = 0.0;
            for (const auto face : Api::MeshFaceHandles(*target))
            {
                const auto positions = Api::FaceVertexPositions(*target, face);
                volume += positions[0].Dot(positions[1].Cross(positions[2])) / 6.0;
            }
            EXPECT_NEAR(volume, example.m_expectedVolume, 1e-3);
            for (const auto edge : Api::MeshEdgeHandles(*target))
            {
                EXPECT_EQ(Api::EdgeFaceHandles(*target, edge).size(), 2);
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, ParametricStairsDeriveStepCountFromTargetRiserHeight)
    {
        namespace Api = WhiteBox::Api;
        struct Case
        {
            float m_height;
            float m_stepHeight;
            int m_expectedSteps;
        };
        for (const auto shape : {WhiteBox::DrawShapeType::Staircase, WhiteBox::DrawShapeType::CircularStairs})
        {
            for (const auto& example : {Case{2.1f, 0.25f, 8}, Case{3.0f, 0.2f, 15},
                     Case{0.5f, 2.0f, 1}, Case{3.0f, 0.001f, 128}})
            {
                SCOPED_TRACE(static_cast<int>(shape));
                SCOPED_TRACE(example.m_expectedSteps);
                auto derived = WhiteBox::BuildParametricShapeMesh(
                    shape, 1.0f, 2.0f, example.m_height, 16, 3,
                    0.15f, 0.1f, true, false, true, 0.0f, 0.5f, 270.0f, true, example.m_stepHeight);
                auto counted = WhiteBox::BuildParametricShapeMesh(
                    shape, 1.0f, 2.0f, example.m_height, 16, example.m_expectedSteps);
                EXPECT_EQ(Api::MeshVertexCount(*derived), Api::MeshVertexCount(*counted));
                EXPECT_THAT(Api::MeshFaces(*derived), ::testing::Eq(Api::MeshFaces(*counted)));
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, ParametricDoorsAndCircularStairsHaveClosedNondegenerateOutwardMeshes)
    {
        namespace Api = WhiteBox::Api;
        const auto checkSolid = [](const WhiteBox::WhiteBoxMesh& mesh)
        {
            EXPECT_FALSE(Api::MeshFaceHandles(mesh).empty());
            for (const auto edge : Api::MeshEdgeHandles(mesh))
            {
                EXPECT_EQ(Api::EdgeFaceHandles(mesh, edge).size(), 2);
            }
            double volume = 0.0;
            for (const auto face : Api::MeshFaceHandles(mesh))
            {
                const auto points = Api::FaceVertexPositions(mesh, face);
                ASSERT_EQ(points.size(), 3);
                EXPECT_GT((points[1] - points[0]).Cross(points[2] - points[0]).GetLengthSq(), 1e-12f);
                volume += static_cast<double>(points[0].Dot(points[1].Cross(points[2]))) / 6.0;
            }
            EXPECT_GT(volume, 0.0);
        };
        for (const bool frame : {false, true})
        {
            for (const float arch : {0.0f, 0.5f, 1.5f})
            {
                SCOPED_TRACE(frame);
                SCOPED_TRACE(arch);
                auto mesh = WhiteBox::BuildParametricShapeMesh(
                    WhiteBox::DrawShapeType::Door, 1.0f, 0.2f, 2.1f, 16, 8,
                    0.15f, 0.0f, false, false, frame, arch);
                checkSolid(*mesh);
            }
        }
        for (const int steps : {1, 3, 16})
        {
            for (const float sweep : {90.0f, 270.0f, 360.0f})
            {
                SCOPED_TRACE(steps);
                SCOPED_TRACE(sweep);
                auto mesh = WhiteBox::BuildParametricShapeMesh(
                    WhiteBox::DrawShapeType::CircularStairs, 1.0f, 1.0f, 3.0f, 16, steps,
                    0.15f, 0.0f, false, false, true, 0.0f, 0.5f, sweep);
                checkSolid(*mesh);
                if (steps == 16 && sweep == 270.0f)
                {
                    // Ordinary quads must not regain a center vertex and four triangles.
                    EXPECT_LT(Api::MeshFaceHandles(*mesh).size(), 400);
                }
                float maximumHeight = 0.0f;
                for (const auto vertex : Api::MeshVertexHandles(*mesh))
                {
                    maximumHeight = AZStd::max(maximumHeight, Api::VertexPosition(*mesh, vertex).GetZ());
                }
                EXPECT_NEAR(maximumHeight, 3.0f, 1e-5f);
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, FacePaintSurvivesRepeatedInitializationAndSerialization)
    {
        namespace Api = WhiteBox::Api;
        // Reinitialization must reuse persistent face properties, not create duplicate names.
        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto faces = Api::MeshFaceHandles(*m_whiteBox);
        ASSERT_GT(faces.size(), 1);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:6");
        constexpr AZ::u32 color = 0xFF3366CC;
        Api::SetFaceMaterial(*m_whiteBox, faces.front(), material);
        Api::SetFacePaintColor(*m_whiteBox, faces.front(), color);
        auto clone = Api::CloneMesh(*m_whiteBox);
        ASSERT_NE(clone, nullptr);
        EXPECT_EQ(Api::FaceMaterial(*clone, faces.front()), material);
        EXPECT_EQ(Api::FacePaintColor(*clone, faces.front()), color);
        EXPECT_EQ(Api::FacePaintColor(*clone, faces.back()), 0);

        Api::WhiteBoxMeshStream saved;
        ASSERT_TRUE(Api::WriteMesh(*clone, saved));
        Api::SetFacePaintColor(*clone, faces.front(), 0);
        ASSERT_EQ(Api::ReadMesh(*clone, saved), Api::ReadResult::Full);
        EXPECT_EQ(Api::FacePaintColor(*clone, faces.front()), color);
        EXPECT_EQ(Api::FaceMaterial(*clone, faces.front()), material);
    }

    TEST_F(WhiteBoxTestFixture, PolygonMaterialsSurviveSerializationAndClone)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto polygons = Api::MeshPolygonHandles(*m_whiteBox);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:1");
        Api::SetPolygonMaterial(*m_whiteBox, polygons.front(), material);

        auto clone = Api::CloneMesh(*m_whiteBox);
        ASSERT_NE(clone, nullptr);
        for (const auto face : polygons.front().m_faceHandles)
        {
            EXPECT_EQ(Api::FaceMaterial(*clone, face), material);
        }
        EXPECT_FALSE(Api::FaceMaterial(*clone, polygons.back().m_faceHandles.front()).IsValid());

        Api::WhiteBoxMeshStream saved;
        ASSERT_TRUE(Api::WriteMesh(*clone, saved));
        Api::SetPolygonMaterial(*clone, polygons.front(), {});
        EXPECT_FALSE(Api::FaceMaterial(*clone, polygons.front().m_faceHandles.front()).IsValid());
        ASSERT_EQ(Api::ReadMesh(*clone, saved), Api::ReadResult::Full);
        EXPECT_EQ(Api::FaceMaterial(*clone, polygons.front().m_faceHandles.front()), material);
    }

    TEST_F(WhiteBoxTestFixture, PolygonMaterialsSurviveLayerAppendAndWindingFlip)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:2");
        const auto polygon = Api::MeshPolygonHandles(*m_whiteBox).front();
        Api::SetPolygonMaterial(*m_whiteBox, polygon, material);
        auto appended = Api::CreateWhiteBoxMesh();
        WhiteBox::AppendMesh(*appended, *m_whiteBox);
        auto flipped = WhiteBox::FlippedMeshWinding(*appended);
        size_t assigned = 0;
        for (const auto face : Api::MeshFaceHandles(*flipped))
        {
            assigned += Api::FaceMaterial(*flipped, face) == material ? 1 : 0;
        }
        EXPECT_EQ(assigned, polygon.m_faceHandles.size());
    }

    TEST_F(WhiteBoxTestFixture, MeshRepairKeepsCoplanarMaterialBoundaries)
    {
        namespace Api = WhiteBox::Api;
        Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:3");
        const auto faces = Api::MeshFaceHandles(*m_whiteBox);
        ASSERT_EQ(faces.size(), 2);
        Api::SetFaceMaterial(*m_whiteBox, faces.front(), material);
        ASSERT_TRUE(Api::RepairMesh(*m_whiteBox));
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), 2);
        size_t assigned = 0;
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            assigned += Api::FaceMaterial(*m_whiteBox, face) == material ? 1 : 0;
        }
        EXPECT_EQ(assigned, 1);
    }

    TEST_F(WhiteBoxTestFixture, BooleanMaterialsFollowBothOperandsForBothSolvers)
    {
        namespace Api = WhiteBox::Api;
        const auto materialA = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:4");
        const auto materialB = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:5");
        for (const auto solver : {Api::CsgSolver::Fast, Api::CsgSolver::Manifold})
        {
            for (const auto operation : {Api::BooleanOperation::Union, Api::BooleanOperation::Subtraction,
                                        Api::BooleanOperation::Intersection})
            {
                SCOPED_TRACE(static_cast<int>(solver));
                SCOPED_TRACE(static_cast<int>(operation));
                auto target = Api::CreateWhiteBoxMesh();
                auto cutter = Api::CreateWhiteBoxMesh();
                Api::InitializeAsUnitCube(*target);
                Api::InitializeAsUnitCube(*cutter);
                for (const auto& polygon : Api::MeshPolygonHandles(*target))
                {
                    Api::SetPolygonMaterial(*target, polygon, materialA);
                }
                for (const auto& polygon : Api::MeshPolygonHandles(*cutter))
                {
                    Api::SetPolygonMaterial(*cutter, polygon, materialB);
                }
                ASSERT_TRUE(Api::ApplyMeshBoolean(*target, *cutter,
                    AZ::Transform::CreateTranslation(AZ::Vector3(0.3f, 0.2f, 0.1f)), operation, solver));
                size_t fromA = 0;
                size_t fromB = 0;
                for (const auto face : Api::MeshFaceHandles(*target))
                {
                    const auto material = Api::FaceMaterial(*target, face);
                    EXPECT_TRUE(material == materialA || material == materialB);
                    fromA += material == materialA ? 1 : 0;
                    fromB += material == materialB ? 1 : 0;
                }
                EXPECT_GT(fromA, 0);
                EXPECT_GT(fromB, 0);
                // Coplanar regrouping must never hide a material boundary inside a polygon.
                for (const auto& polygon : Api::MeshPolygonHandles(*target))
                {
                    const auto material = Api::FaceMaterial(*target, polygon.m_faceHandles.front());
                    for (const auto face : polygon.m_faceHandles)
                    {
                        EXPECT_EQ(Api::FaceMaterial(*target, face), material);
                    }
                }
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, WeldingDegenerateTrianglesKeepsMaterialsAligned)
    {
        WhiteBox::Csg::TriangleMesh mesh;
        mesh.m_positions = {0,0,0, 1,0,0, 0,1,0, 0,0,0};
        mesh.m_indices = {0,3,1, 0,1,2};
        mesh.m_materials = {"discarded", "retained"};
        WhiteBox::Csg::WeldVertices(mesh, 1e-6);
        ASSERT_EQ(mesh.TriangleCount(), 1);
        ASSERT_EQ(mesh.m_materials.size(), 1);
        EXPECT_EQ(mesh.m_materials.front(), "retained");
    }
}

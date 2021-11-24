/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"

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

        AZStd::vector<Api::HalfedgeHandle> outgoingHalfedgeHandles =
            Api::VertexOutgoingHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        const Api::HalfedgeHandle expectedHalfedgeHandles[] = {
            Api::HalfedgeHandle{9}, Api::HalfedgeHandle{34}, Api::HalfedgeHandle{24}, Api::HalfedgeHandle{0},
            Api::HalfedgeHandle{5}};

        EXPECT_THAT(
            outgoingHalfedgeHandles, ElementsAreArray(expectedHalfedgeHandles, std::size(expectedHalfedgeHandles)));
    }

    TEST_F(WhiteBoxTestFixture, IncomingHalfedgesFromVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        AZStd::vector<Api::HalfedgeHandle> incomingHalfedgeHandles =
            Api::VertexIncomingHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        const Api::HalfedgeHandle expectedHalfedgeHandles[] = {
            Api::HalfedgeHandle{8}, Api::HalfedgeHandle{35}, Api::HalfedgeHandle{25}, Api::HalfedgeHandle{1},
            Api::HalfedgeHandle{4}};

        EXPECT_THAT(
            incomingHalfedgeHandles, ElementsAreArray(expectedHalfedgeHandles, std::size(expectedHalfedgeHandles)));
    }

    TEST_F(WhiteBoxTestFixture, AllHalfedgesFromVertex)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        AZStd::vector<Api::HalfedgeHandle> allHalfedgeHandles =
            Api::VertexHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        const Api::HalfedgeHandle expectedHalfedgeHandles[] = {
            Api::HalfedgeHandle{9}, Api::HalfedgeHandle{34}, Api::HalfedgeHandle{24}, Api::HalfedgeHandle{0},
            Api::HalfedgeHandle{5}, Api::HalfedgeHandle{8},  Api::HalfedgeHandle{35}, Api::HalfedgeHandle{25},
            Api::HalfedgeHandle{1}, Api::HalfedgeHandle{4}};

        EXPECT_THAT(allHalfedgeHandles, ElementsAreArray(expectedHalfedgeHandles, std::size(expectedHalfedgeHandles)));
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

        EXPECT_THAT(faceMidpoint, IsClose(AZ::Vector3(0.1666f, -0.1666, 0.5f)));
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

        const auto expectedUserEdgeVectors = AZStd::vector<AZ::Vector3>{
            -AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisX(), -AZ::Vector3::CreateAxisY()};

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

        const auto expectedUserEdgeVectors =
            AZStd::vector<AZ::Vector3>{-AZ::Vector3::CreateAxisZ(2.0f), -AZ::Vector3::CreateAxisY(3.0f)};
        const auto expectedUserEdgeAxes =
            AZStd::vector<AZ::Vector3>{-AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisY()};

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

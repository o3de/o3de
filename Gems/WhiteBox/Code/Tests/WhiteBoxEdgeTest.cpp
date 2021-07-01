/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBox_precompiled.h"

#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace UnitTest
{
    class WhiteBoxEdgeTests : public WhiteBoxTestFixture
    {
    };

    TEST_F(WhiteBoxEdgeTests, TestPolygonEdgeHandles)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto polygonHandle = Api::MeshPolygonHandles(*m_whiteBox)[0];
        auto edgeHandles = Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandle);

        EXPECT_EQ(edgeHandles.size(), 4);
    }

    TEST_F(WhiteBoxEdgeTests, TestMeshPolygonEdgeHandlesAsCube)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        auto edgeHandles = Api::MeshPolygonEdgeHandles(*m_whiteBox);

        EXPECT_EQ(edgeHandles.size(), 12);
    }

    TEST_F(WhiteBoxEdgeTests, TestMeshPolygonEdgeHandlesAsQuad)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto edgeHandles = Api::MeshPolygonEdgeHandles(*m_whiteBox);

        EXPECT_EQ(edgeHandles.size(), 4);
    }

    TEST_F(WhiteBoxEdgeTests, TestMeshEdgeHandlesCube)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        auto edgeHandles = Api::MeshEdgeHandles(*m_whiteBox);

        EXPECT_EQ(edgeHandles.size(), 18);
    }

    TEST_F(WhiteBoxEdgeTests, TestMeshEdgeHandlesQuad)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto edgeHandles = Api::MeshEdgeHandles(*m_whiteBox);

        EXPECT_EQ(edgeHandles.size(), 5);
    }

    TEST_F(WhiteBoxEdgeTests, TestFaceEdgeHandles)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto face = Api::MeshPolygonHandles(*m_whiteBox)[0].m_faceHandles[0];
        auto edgeHandles = Api::FaceEdgeHandles(*m_whiteBox, face);

        EXPECT_EQ(edgeHandles.size(), 3);
    }

    TEST_F(WhiteBoxEdgeTests, PolygonEdgeHandles)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto polygon = Api::MeshPolygonHandles(*m_whiteBox)[0];
        auto edgeHandles = Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygon);

        EXPECT_EQ(edgeHandles.size(), 4);
    }

    TEST_F(WhiteBoxEdgeTests, EdgeFaceHandles)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        auto edgeHandle = Api::MeshEdgeHandles(*m_whiteBox)[0];
        auto faceHandles = Api::EdgeFaceHandles(*m_whiteBox, edgeHandle);

        EXPECT_TRUE(faceHandles[0].IsValid());
        EXPECT_TRUE(faceHandles[1].IsValid());
    }

    TEST_F(WhiteBoxEdgeTests, EdgeFacehandlesAtBorder)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto edgeHandle = Api::MeshEdgeHandles(*m_whiteBox)[0];
        auto faceHandles = Api::EdgeFaceHandles(*m_whiteBox, edgeHandle);

        ASSERT_EQ(1, faceHandles.size());
        EXPECT_TRUE(faceHandles[0].IsValid());
    }

    TEST_F(WhiteBoxEdgeTests, EdgeVertexhandles)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto edgeHandle = Api::MeshEdgeHandles(*m_whiteBox)[0];
        auto vertexHandles = Api::EdgeVertexHandles(*m_whiteBox, edgeHandle);

        EXPECT_TRUE(vertexHandles[0].IsValid());
        EXPECT_TRUE(vertexHandles[1].IsValid());
    }

    TEST_F(WhiteBoxEdgeTests, EdgeTranslate)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        AZStd::vector<AZ::Vector3> expectedVertexPositions = {
            AZ::Vector3(-0.5f, 0.0f, -1.0f), AZ::Vector3(0.5f, 0.0f, -1.0f), AZ::Vector3(0.5f, 0.0f, 0.5f),
            AZ::Vector3(-0.5f, 0.0f, 0.5f)};

        auto edgeHandle = Api::MeshEdgeHandles(*m_whiteBox)[0];

        Api::TranslateEdge(*m_whiteBox, edgeHandle, AZ::Vector3(0.0, 0.0, -0.5f));

        auto vertexPositions = Api::MeshVertexPositions(*m_whiteBox);

        EXPECT_THAT(vertexPositions, ::testing::Pointwise(ContainerIsClose(), expectedVertexPositions));
    }
} // namespace UnitTest

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/std/smart_ptr/make_shared.h>

#include <Pipeline/SceneAPIExt/ClothRule.h>

#include <UnitTestHelper.h>
#include <MeshVertexColorDataStub.h>

namespace UnitTest
{
    class NvClothRule
        : public ::testing::Test
    {
    public:
        // [inverse mass, motion constrain radius, backstop offset, backstop radius]
        const AZ::Color DefaultClothVertexData = AZ::Color(1.0f, 1.0f, 0.5f, 0.0f);

        const AZStd::vector<AZ::Color> MeshClothData = {{
            AZ::Color(0.75f, 0.6f, 0.5f, 0.1f),
            AZ::Color(1.0f, 0.16f, 0.1f, 1.0f),
            AZ::Color(0.25f, 1.0f, 0.9f, 0.5f)
        }};
    };

    TEST_F(NvClothRule, ClothRule_ExtractClothDataNoSceneGraph_ReturnsEmptyData)
    {
        NvCloth::Pipeline::ClothRule clothRule;

        AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData({}, 0);

        EXPECT_TRUE(clothData.empty());
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothDataWithNonExistentNode_ReturnsEmptyData)
    {
        AZ::SceneAPI::Containers::SceneGraph graph;
        graph.AddChild(graph.GetRoot(), "child_node");

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName("mesh_node");

        AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, MeshClothData.size());

        EXPECT_TRUE(clothData.empty());
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothDataWithAllStreamsDisabled_ReturnsDefaultClothData)
    {
        const AZStd::string nodeName = "mesh_node";

        AZ::SceneAPI::Containers::SceneGraph graph;
        graph.AddChild(graph.GetRoot(), nodeName.c_str());

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName(nodeName);
        clothRule.SetInverseMassesStreamName(NvCloth::Pipeline::ClothRule::DefaultInverseMassesString);
        clothRule.SetMotionConstraintsStreamName(NvCloth::Pipeline::ClothRule::DefaultMotionConstraintsString);
        clothRule.SetBackstopStreamName(NvCloth::Pipeline::ClothRule::DefaultBackstopString);

        AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, MeshClothData.size());

        EXPECT_EQ(clothData.size(), MeshClothData.size());
        for (size_t i = 0; i < clothData.size(); ++i)
        {
            EXPECT_THAT(clothData[i], IsCloseTolerance(DefaultClothVertexData, Tolerance));
        }
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothDataWithStreamsNonPresentInGraph_ReturnsDefaultClothData)
    {
        const AZStd::string nodeName = "mesh_node";

        AZ::SceneAPI::Containers::SceneGraph graph;
        graph.AddChild(graph.GetRoot(), nodeName.c_str());

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName(nodeName);
        clothRule.SetInverseMassesStreamName("inverse_masses_stream");
        clothRule.SetMotionConstraintsStreamName("motion_constraints_stream");
        clothRule.SetBackstopStreamName("backstop_stream");

        AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, MeshClothData.size());

        EXPECT_EQ(clothData.size(), MeshClothData.size());
        for (size_t i = 0; i < clothData.size(); ++i)
        {
            EXPECT_THAT(clothData[i], IsCloseTolerance(DefaultClothVertexData, Tolerance));
        }
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothDataWithUnmachingNumVertices_ReturnsDefaultClothData)
    {
        const AZStd::string nodeName = "mesh_node";
        const AZStd::string inverseMassesStreamName = "inverse_masses_stream";

        auto inverseMassesStream = AZStd::make_shared<MeshVertexColorDataStub>();
        inverseMassesStream->m_colors.reserve(MeshClothData.size());
        for (const auto& color : MeshClothData)
        {
            inverseMassesStream->m_colors.emplace_back(
                    color.GetR(), 0.0f, 0.0f, 0.0f);
        }

        AZ::SceneAPI::Containers::SceneGraph graph;
        auto rootIndex = graph.AddChild(graph.GetRoot(), nodeName.c_str());
        graph.AddChild(rootIndex, inverseMassesStreamName.c_str(), inverseMassesStream);

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName(nodeName);
        clothRule.SetInverseMassesStreamName(inverseMassesStreamName);
        clothRule.SetMotionConstraintsStreamName(NvCloth::Pipeline::ClothRule::DefaultMotionConstraintsString);
        clothRule.SetBackstopStreamName(NvCloth::Pipeline::ClothRule::DefaultBackstopString);

        const size_t numVertices = MeshClothData.size() * 2; // Unmatching number of vertices

        const AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, numVertices);

        EXPECT_EQ(clothData.size(), numVertices);
        for (size_t i = 0; i < clothData.size(); ++i)
        {
            EXPECT_THAT(clothData[i], IsCloseTolerance(DefaultClothVertexData, Tolerance));
        }
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothData_ReturnsInverseMassesData)
    {
        const AZStd::string nodeName = "mesh_node";
        const AZStd::string inverseMassesStreamName = "inverse_masses_stream";

        auto inverseMassesStream = AZStd::make_shared<MeshVertexColorDataStub>();
        inverseMassesStream->m_colors.reserve(MeshClothData.size());
        for (const auto& color : MeshClothData)
        {
            inverseMassesStream->m_colors.emplace_back(
                    color.GetR(), 0.0f, 0.0f, 0.0f);
        }

        AZ::SceneAPI::Containers::SceneGraph graph;
        auto rootIndex = graph.AddChild(graph.GetRoot(), nodeName.c_str());
        graph.AddChild(rootIndex, inverseMassesStreamName.c_str(), inverseMassesStream);

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName(nodeName);
        clothRule.SetInverseMassesStreamName(inverseMassesStreamName);
        clothRule.SetMotionConstraintsStreamName(NvCloth::Pipeline::ClothRule::DefaultMotionConstraintsString);
        clothRule.SetBackstopStreamName(NvCloth::Pipeline::ClothRule::DefaultBackstopString);

        const AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, MeshClothData.size());

        EXPECT_EQ(clothData.size(), MeshClothData.size());
        for (size_t i = 0; i < clothData.size(); ++i)
        {
            EXPECT_NEAR(clothData[i].GetR(), MeshClothData[i].GetR(), Tolerance);
        }
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothDataInSeparateStreams_ReturnsClothData)
    {
        const AZStd::string nodeName = "mesh_node";
        const AZStd::string inverseMassesStreamName = "inverse_masses_stream";
        const AZStd::string motionConstraintsStreamName = "motion_constraints_stream";
        const AZStd::string backstopStreamName = "backstop_stream";

        auto inverseMassesStream = AZStd::make_shared<MeshVertexColorDataStub>();
        auto motionConstraintsStream = AZStd::make_shared<MeshVertexColorDataStub>();
        auto backstopStream = AZStd::make_shared<MeshVertexColorDataStub>();
        inverseMassesStream->m_colors.reserve(MeshClothData.size());
        motionConstraintsStream->m_colors.reserve(MeshClothData.size());
        backstopStream->m_colors.reserve(MeshClothData.size());
        for (const auto& color : MeshClothData)
        {
            inverseMassesStream->m_colors.emplace_back(
                    color.GetR(), 0.0f, 0.0f, 0.0f);
            motionConstraintsStream->m_colors.emplace_back(
                    color.GetG(), 0.0f, 0.0f, 0.0f);
            backstopStream->m_colors.emplace_back(
                    color.GetB(), color.GetA(), 0.0f, 0.0f);
        }

        AZ::SceneAPI::Containers::SceneGraph graph;
        auto rootIndex = graph.AddChild(graph.GetRoot(), nodeName.c_str());
        graph.AddChild(rootIndex, inverseMassesStreamName.c_str(), inverseMassesStream);
        graph.AddChild(rootIndex, motionConstraintsStreamName.c_str(), motionConstraintsStream);
        graph.AddChild(rootIndex, backstopStreamName.c_str(), backstopStream);

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName(nodeName);
        clothRule.SetInverseMassesStreamName(inverseMassesStreamName);
        clothRule.SetMotionConstraintsStreamName(motionConstraintsStreamName);
        clothRule.SetBackstopStreamName(backstopStreamName);
        clothRule.SetInverseMassesStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Red);
        clothRule.SetMotionConstraintsStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Red);
        clothRule.SetBackstopOffsetStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Red);
        clothRule.SetBackstopRadiusStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Green);

        const AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, MeshClothData.size());

        EXPECT_THAT(clothData, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), MeshClothData));
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothDataInOneStream_ReturnsClothData)
    {
        const AZStd::string nodeName = "mesh_node";
        const AZStd::string clothDataStreamName = "cloth_data_stream";

        auto clothDataStream = AZStd::make_shared<MeshVertexColorDataStub>();
        clothDataStream->m_colors.reserve(MeshClothData.size());
        for (const auto& color : MeshClothData)
        {
            clothDataStream->m_colors.emplace_back(
                    color.GetR(), color.GetG(), color.GetB(), color.GetA());
        }

        AZ::SceneAPI::Containers::SceneGraph graph;
        auto rootIndex = graph.AddChild(graph.GetRoot(), nodeName.c_str());
        graph.AddChild(rootIndex, clothDataStreamName.c_str(), clothDataStream);

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName(nodeName);
        clothRule.SetInverseMassesStreamName(clothDataStreamName);
        clothRule.SetMotionConstraintsStreamName(clothDataStreamName);
        clothRule.SetBackstopStreamName(clothDataStreamName);
        clothRule.SetInverseMassesStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Red);
        clothRule.SetMotionConstraintsStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Green);
        clothRule.SetBackstopOffsetStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Blue);
        clothRule.SetBackstopRadiusStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Alpha);

        const AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, MeshClothData.size());

        EXPECT_THAT(clothData, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), MeshClothData));
    }

    TEST_F(NvClothRule, ClothRule_ExtractClothDataInOneStreamDifferentLayout_ReturnsClothData)
    {
        const AZStd::string nodeName = "mesh_node";
        const AZStd::string clothDataStreamName = "cloth_data_stream";

        auto clothDataStream = AZStd::make_shared<MeshVertexColorDataStub>();
        clothDataStream->m_colors.reserve(MeshClothData.size());
        for (const auto& color : MeshClothData)
        {
            clothDataStream->m_colors.emplace_back(
                    color.GetG(), color.GetA(), color.GetR(), color.GetB());
        }

        AZ::SceneAPI::Containers::SceneGraph graph;
        auto rootIndex = graph.AddChild(graph.GetRoot(), nodeName.c_str());
        graph.AddChild(rootIndex, clothDataStreamName.c_str(), clothDataStream);

        NvCloth::Pipeline::ClothRule clothRule;
        clothRule.SetMeshNodeName(nodeName);
        clothRule.SetInverseMassesStreamName(clothDataStreamName);
        clothRule.SetMotionConstraintsStreamName(clothDataStreamName);
        clothRule.SetBackstopStreamName(clothDataStreamName);
        clothRule.SetInverseMassesStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Blue);
        clothRule.SetMotionConstraintsStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Red);
        clothRule.SetBackstopOffsetStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Alpha);
        clothRule.SetBackstopRadiusStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel::Green);

        const AZStd::vector<AZ::Color> clothData = clothRule.ExtractClothData(graph, MeshClothData.size());

        EXPECT_THAT(clothData, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), MeshClothData));
    }
} // namespace UnitTest

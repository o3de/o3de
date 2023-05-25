/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinWeightData.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <Generation/Components/MeshOptimizer/MeshOptimizerComponent.h>

#include <InitSceneAPIFixture.h>

namespace AZ::SceneAPI::DataTypes
{
    void PrintTo(const ISkinWeightData::Link& link, ::std::ostream* os)
    {
        *os << '{' << link.boneId << ", " << link.weight << '}';
    }
}

MATCHER(VectorOfLinksEq, "")
{
    return testing::ExplainMatchResult(
        testing::AllOf(
            testing::Field(&AZ::SceneData::GraphData::SkinWeightData::Link::boneId, testing::Eq(testing::get<0>(arg).boneId)),
            testing::Field(&AZ::SceneData::GraphData::SkinWeightData::Link::weight, testing::FloatEq(testing::get<0>(arg).weight))),
        testing::get<1>(arg), result_listener);
}

MATCHER(VectorOfVectorOfLinksEq, "")
{
    return testing::ExplainMatchResult(
        testing::UnorderedPointwise(VectorOfLinksEq(), testing::get<0>(arg)), testing::get<1>(arg), result_listener);
}

namespace SceneProcessing
{
    class VertexDeduplicationFixture
        : public SceneProcessing::InitSceneAPIFixture
    {
    public:
        void SetUp() override
        {
            SceneProcessing::InitSceneAPIFixture::SetUp();

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_systemEntity = m_app.Create({}, startupParameters);
            m_systemEntity->AddComponent(aznew AZ::JobManagerComponent());
            m_systemEntity->Init();
            m_systemEntity->Activate();
        }
        void TearDown() override
        {
            m_systemEntity->Deactivate();
            SceneProcessing::InitSceneAPIFixture::TearDown();
        }

        static AZStd::unique_ptr<AZ::SceneAPI::DataTypes::IMeshData> MakePlaneMesh()
        {
            // Create a simple plane with 2 triangles, 6 total vertices, 2 shared vertices
            // 0,5 --- 1
            // | \     |
            // |  \    |
            // |   \   |
            // |    \  |
            // 4 --- 2,3
            const AZStd::array planeVertexPositions = {
                AZ::Vector3{0.0f, 0.0f, 0.0f},
                AZ::Vector3{0.0f, 0.0f, 1.0f},
                AZ::Vector3{1.0f, 0.0f, 1.0f},
                AZ::Vector3{1.0f, 0.0f, 1.0f},
                AZ::Vector3{1.0f, 0.0f, 0.0f},
                AZ::Vector3{0.0f, 0.0f, 0.0f},
            };

            auto mesh = AZStd::make_unique<AZ::SceneData::GraphData::MeshData>();

            int i = 0;
            for (const AZ::Vector3& position : planeVertexPositions)
            {
                mesh->AddPosition(position);
                mesh->AddNormal(AZ::Vector3::CreateAxisY());

                // This assumes that the data coming from the import process gives a unique control point
                // index to every vertex. This follows the behavior of the AssImp library.
                mesh->SetVertexIndexToControlPointIndexMap(i, i);
                ++i;
            }

            mesh->AddFace({0, 1, 2}, 0);
            mesh->AddFace({3, 4, 5}, 0);

            return mesh;
        }

        static AZStd::unique_ptr<AZ::SceneData::GraphData::SkinWeightData> MakeSkinData(
            const AZStd::vector<AZStd::vector<AZ::SceneData::GraphData::SkinWeightData::Link>>& sourceLinks)
        {
            auto skinWeights = AZStd::make_unique<AZ::SceneData::GraphData::SkinWeightData>();

            skinWeights->ResizeContainerSpace(sourceLinks.size());

            for (size_t vertexIndex = 0; vertexIndex < sourceLinks.size(); ++vertexIndex)
            {
                for (const auto& link : sourceLinks[vertexIndex])
                {
                    // Make sure the bone is added to the skin weights
                    skinWeights->GetBoneId(AZStd::to_string(link.boneId));

                    skinWeights->AppendLink(vertexIndex, link);
                }
            }

            return skinWeights;
        }

        static AZStd::unique_ptr<AZ::SceneData::GraphData::SkinWeightData> MakeDuplicateSkinData()
        {
            auto skinWeights = AZStd::make_unique<AZ::SceneData::GraphData::SkinWeightData>();

            skinWeights->ResizeContainerSpace(6);

            // Add bones 0 and 1 to the skin weights
            skinWeights->GetBoneId("0");
            skinWeights->GetBoneId("1");

            // Vertices 0,5 and 2,3 have duplicate skin data, in addition to duplicate positions
            skinWeights->AppendLink(0, { /*.boneId=*/0, /*.weight=*/1 });
            skinWeights->AppendLink(1, { /*.boneId=*/1, /*.weight=*/1 });
            skinWeights->AppendLink(2, { /*.boneId=*/0, /*.weight=*/1 });
            skinWeights->AppendLink(3, { /*.boneId=*/0, /*.weight=*/1 });
            skinWeights->AppendLink(4, { /*.boneId=*/2, /*.weight=*/1 });
            skinWeights->AppendLink(5, { /*.boneId=*/0, /*.weight=*/1 });

            return skinWeights;
        }

        static void TestSkinDuplication(
            const AZStd::shared_ptr<AZ::SceneData::GraphData::SkinWeightData> skinData,
            const AZStd::vector<AZStd::vector<AZ::SceneData::GraphData::SkinWeightData::Link>>& expectedLinks)
        {
            AZ::SceneAPI::Containers::Scene scene("testScene");
            AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

            const auto meshNodeIndex = graph.AddChild(graph.GetRoot(), "testMesh", MakePlaneMesh());
            const auto skinDataNodeIndex = graph.AddChild(meshNodeIndex, "skinData", skinData);
            graph.MakeEndPoint(skinDataNodeIndex);

            // The original source mesh should have 6 vertices
            EXPECT_EQ(
                AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshData>(graph.GetNodeContent(meshNodeIndex))->GetVertexCount(), 6);

            auto meshGroup = AZStd::make_unique<AZ::SceneAPI::SceneData::MeshGroup>();
            meshGroup->GetSceneNodeSelectionList().AddSelectedNode("testMesh");
            scene.GetManifest().AddEntry(AZStd::move(meshGroup));

            AZ::SceneGenerationComponents::MeshOptimizerComponent component;
            AZ::SceneAPI::Events::GenerateSimplificationEventContext context(scene, "pc");
            component.OptimizeMeshes(context);

            AZ::SceneAPI::Containers::SceneGraph::NodeIndex optimizedNodeIndex =
                graph.Find(AZStd::string("testMesh_").append(AZ::SceneAPI::Utilities::OptimizedMeshSuffix));
            ASSERT_TRUE(optimizedNodeIndex.IsValid()) << "Mesh optimizer did not add an optimized version of the mesh";

            const auto& optimizedMesh =
                AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshData>(graph.GetNodeContent(optimizedNodeIndex));
            ASSERT_TRUE(optimizedMesh);

            AZ::SceneAPI::Containers::SceneGraph::NodeIndex optimizedSkinDataNodeIndex =
                graph.Find(AZStd::string("testMesh_").append(AZ::SceneAPI::Utilities::OptimizedMeshSuffix).append(".skinWeights"));
            ASSERT_TRUE(optimizedSkinDataNodeIndex.IsValid()) << "Mesh optimizer did not add an optimized version of the skin data";

            const auto& optimizedSkinWeights =
                AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::ISkinWeightData>(graph.GetNodeContent(optimizedSkinDataNodeIndex));
            ASSERT_TRUE(optimizedSkinWeights);

            AZStd::vector<AZStd::vector<AZ::SceneData::GraphData::SkinWeightData::Link>> gotLinks(optimizedMesh->GetVertexCount());
            for (unsigned int vertexIndex = 0; vertexIndex < optimizedMesh->GetVertexCount(); ++vertexIndex)
            {
                for (size_t linkIndex = 0; linkIndex < optimizedSkinWeights->GetLinkCount(vertexIndex); ++linkIndex)
                {
                    gotLinks[vertexIndex].emplace_back(optimizedSkinWeights->GetLink(vertexIndex, linkIndex));
                }
            }
            EXPECT_THAT(gotLinks, testing::Pointwise(VectorOfVectorOfLinksEq(), expectedLinks));

            EXPECT_EQ(optimizedMesh->GetVertexCount(), expectedLinks.size());
        }

    private:
        AZ::ComponentApplication m_app;
        AZ::Entity* m_systemEntity;
    };

    TEST_F(VertexDeduplicationFixture, CanDeduplicateVertices)
    {
        AZ::SceneAPI::Containers::Scene scene("testScene");
        AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

        const auto meshNodeIndex = graph.AddChild(graph.GetRoot(), "testMesh", MakePlaneMesh());

        // The original source mesh should have 6 vertices
        EXPECT_EQ(AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshData>(graph.GetNodeContent(meshNodeIndex))->GetVertexCount(), 6);

        auto meshGroup = AZStd::make_unique<AZ::SceneAPI::SceneData::MeshGroup>();
        meshGroup->GetSceneNodeSelectionList().AddSelectedNode("testMesh");
        scene.GetManifest().AddEntry(AZStd::move(meshGroup));

        AZ::SceneGenerationComponents::MeshOptimizerComponent component;
        AZ::SceneAPI::Events::GenerateSimplificationEventContext context(scene, "pc");
        component.OptimizeMeshes(context);

        AZ::SceneAPI::Containers::SceneGraph::NodeIndex optimizedNodeIndex = graph.Find(AZStd::string("testMesh_").append(AZ::SceneAPI::Utilities::OptimizedMeshSuffix));
        ASSERT_TRUE(optimizedNodeIndex.IsValid()) << "Mesh optimizer did not add an optimized version of the mesh";

        const auto& optimizedMesh = AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshData>(graph.GetNodeContent(optimizedNodeIndex));
        ASSERT_TRUE(optimizedMesh);

        // The optimized mesh should have 4 vertices, the 2 shared vertices are welded together
        EXPECT_EQ(optimizedMesh->GetVertexCount(), 4);
    }

    TEST_F(VertexDeduplicationFixture, DeduplicatedVerticesKeepUniqueSkinInfluences)
    {
        // Vertices 0,5 and 2,3 have duplicate positions, but unique links,
        // so none of the vertices should be de-duplicated
        // and the sourceLinks should be the same as the expected links
        const AZStd::vector<AZStd::vector<AZ::SceneData::GraphData::SkinWeightData::Link>> sourceLinks{
            /*0*/ { { 0, 1.0f } },
            /*1*/ { { 0, 1.0f } },
            /*2*/ { { 0, 1.0f } },
            /*3*/ { { 1, 1.0f } },
            /*4*/ { { 1, 1.0f } },
            /*5*/ { { 1, 1.0f } },
        };

        TestSkinDuplication(MakeSkinData(sourceLinks), sourceLinks);
    }

    TEST_F(VertexDeduplicationFixture, DeduplicatedVerticesDeduplicateSkinInfluences)
    {
        // Vertices 0,5 and 2,3 have duplicate positions, and also duplicate links,
        // so they should be de-duplicated and the expected links
        // should have two fewer links
        const AZStd::vector<AZStd::vector<AZ::SceneData::GraphData::SkinWeightData::Link>> sourceLinks{
            /*0*/ { { 0, 1.0f } },
            /*1*/ { { 1, 1.0f } },
            /*2*/ { { 0, 1.0f } },
            /*3*/ { { 0, 1.0f } },
            /*4*/ { { 2, 1.0f } },
            /*5*/ { { 0, 1.0f } },
        };
        const AZStd::vector<AZStd::vector<AZ::SceneData::GraphData::SkinWeightData::Link>> expectedLinks{
            /*0*/ { { 0, 1.0f } },
            /*1*/ { { 1, 1.0f } },
            /*2*/ { { 0, 1.0f } },
            /*3*/ { { 2, 1.0f } },
        };

        TestSkinDuplication(MakeSkinData(sourceLinks), expectedLinks);
    }
} // namespace SceneProcessing

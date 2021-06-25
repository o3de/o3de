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
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <Generation/Components/MeshOptimizer/MeshOptimizerComponent.h>

#include <InitSceneAPIFixture.h>

namespace SceneProcessing
{
    using VertexDeduplicationFixture = SceneProcessing::InitSceneAPIFixture;

    TEST_F(VertexDeduplicationFixture, CanDeduplicateVertices)
    {
        AZ::ComponentApplication app;
        AZ::Entity* systemEntity = app.Create({}, {});
        systemEntity->AddComponent(aznew AZ::MemoryComponent());
        systemEntity->AddComponent(aznew AZ::JobManagerComponent());
        systemEntity->Init();
        systemEntity->Activate();

        AZ::SceneAPI::Containers::Scene scene("testScene");
        AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

        // Create a simple plane with 2 triangles, 6 total vertices, 2 shared vertices
        // 0 --- 1
        // |   / |
        // |  /  |
        // | /   |
        // 2 --- 3
        const AZStd::array planeVertexPositions = {
            AZ::Vector3{0.0f, 0.0f, 0.0f},
            AZ::Vector3{0.0f, 0.0f, 1.0f},
            AZ::Vector3{1.0f, 0.0f, 1.0f},
            AZ::Vector3{1.0f, 0.0f, 1.0f},
            AZ::Vector3{1.0f, 0.0f, 0.0f},
            AZ::Vector3{0.0f, 0.0f, 0.0f},
        };
        {
            auto mesh = AZStd::make_unique<AZ::SceneData::GraphData::MeshData>();
            {
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
            }
            mesh->AddFace({0, 1, 2}, 0);
            mesh->AddFace({3, 4, 5}, 0);

            // The original source mesh should have 6 vertices
            EXPECT_EQ(mesh->GetVertexCount(), planeVertexPositions.size());

            graph.AddChild(graph.GetRoot(), "testMesh", AZStd::move(mesh));
        }

        auto meshGroup = AZStd::make_unique<AZ::SceneAPI::SceneData::MeshGroup>();
        meshGroup->GetSceneNodeSelectionList().AddSelectedNode("testMesh");
        scene.GetManifest().AddEntry(AZStd::move(meshGroup));

        AZ::SceneGenerationComponents::MeshOptimizerComponent component;
        AZ::SceneAPI::Events::GenerateSimplificationEventContext context(scene, "pc");
        component.OptimizeMeshes(context);

        AZ::SceneAPI::Containers::SceneGraph::NodeIndex optimizedNodeIndex = graph.Find(AZStd::string("testMesh").append(AZ::SceneAPI::Utilities::OptimizedMeshSuffix));
        ASSERT_TRUE(optimizedNodeIndex.IsValid()) << "Mesh optimizer did not add an optimized version of the mesh";

        const auto& optimizedMesh = AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshData>(graph.GetNodeContent(optimizedNodeIndex));
        ASSERT_TRUE(optimizedMesh);

        // The optimized mesh should have 4 vertices, the 2 shared vertices are welded together
        EXPECT_EQ(optimizedMesh->GetVertexCount(), 4);

        systemEntity->Deactivate();
    }
} // namespace SceneProcessing

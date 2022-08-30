/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabGroup/tests/PrefabBehaviorTestMocks.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

namespace AZ::Render
{
    void EditorMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AZ::Render::EditorMeshComponent>();
        }
        
        if (BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorMeshComponent>("AZ::Render::EditorMeshComponent");
        }
    }

    void EditorMeshComponentHelper::Reflect(AZ::ReflectContext* reflection) const
    {
        AZ::Render::EditorMeshComponent::Reflect(reflection);
    }
}

namespace UnitTest
{
    AZStd::unique_ptr<AZ::SceneAPI::Containers::Scene> CreateMockScene(
        const AZStd::string_view manifestFilename,
        const AZStd::string_view sourceFileName,
        const AZStd::string_view watchFolder)
    {
        auto scene = CreateEmptyMockSceneWithRoot(manifestFilename, sourceFileName, watchFolder);
        BuildMockScene(*scene.get());
        return scene;
    }

    AZStd::unique_ptr<AZ::SceneAPI::Containers::Scene> CreateEmptyMockSceneWithRoot(
        const AZStd::string_view manifestFilename,
        const AZStd::string_view sourceFileName,
        const AZStd::string_view watchFolder)
    {
        using namespace AZ::SceneAPI;

        auto scene = AZStd::make_unique<Containers::Scene>("mock_scene");
        scene->SetManifestFilename(manifestFilename.data());
        scene->SetSource(sourceFileName.data(), AZ::Uuid::CreateRandom());
        scene->SetWatchFolder(watchFolder.data());

        auto root = scene->GetGraph().GetRoot();
        scene->GetGraph().SetContent(root, AZStd::make_shared<DataTypes::MockIGraphObject>(0));

        return AZStd::move(scene);
    }

    void BuildMockScene(AZ::SceneAPI::Containers::Scene& scene)
    {
        using namespace AZ::SceneAPI;

        /*---------------------------------------\
                    Root
                     |
                     1
                     |
                     2
                   /   \
            ------3m    7
           /  /  /        \
          6  5  4t         8m-------
                            \   \   \
                             9t 10  11
        \---------------------------------------*/

        // Build up the graph
        auto root = scene.GetGraph().GetRoot();
        auto index1 = scene.GetGraph().AddChild(root, "1", AZStd::make_shared<DataTypes::MockIGraphObject>(1));
        auto index2 = scene.GetGraph().AddChild(index1, "2", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
        auto index3 = scene.GetGraph().AddChild(index2, "3", AZStd::make_shared<AZ::SceneData::GraphData::MeshData>());
        auto index4 = scene.GetGraph().AddChild(index3, "4", AZStd::make_shared<MockTransform>());
        auto index5 = scene.GetGraph().AddChild(index3, "5", AZStd::make_shared<DataTypes::MockIGraphObject>(5));
        auto index6 = scene.GetGraph().AddChild(index3, "6", AZStd::make_shared<DataTypes::MockIGraphObject>(6));
        auto index7 = scene.GetGraph().AddChild(index2, "7", AZStd::make_shared<DataTypes::MockIGraphObject>(7));
        auto index8 = scene.GetGraph().AddChild(index7, "8", AZStd::make_shared<AZ::SceneData::GraphData::MeshData>());
        auto index9 = scene.GetGraph().AddChild(index8, "9", AZStd::make_shared<MockTransform>());
        auto index10 = scene.GetGraph().AddChild(index8, "10", AZStd::make_shared<DataTypes::MockIGraphObject>(10));
        auto index11 = scene.GetGraph().AddChild(index8, "11", AZStd::make_shared<DataTypes::MockIGraphObject>(11));

        scene.GetGraph().MakeEndPoint(index4);
        scene.GetGraph().MakeEndPoint(index5);
        scene.GetGraph().MakeEndPoint(index6);
        scene.GetGraph().MakeEndPoint(index9);
        scene.GetGraph().MakeEndPoint(index10);
        scene.GetGraph().MakeEndPoint(index11);
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Tests/InitSceneAPIFixture.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <SceneAPI/SceneCore/Mocks/Containers/MockScene.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Pipeline/RCExt/Actor/ActorBuilder.h>
#include <EMotionFX/Pipeline/RCExt/Actor/ActorGroupExporter.h>
#include <EMotionFX/Pipeline/RCExt/ExportContexts.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Groups/ActorGroup.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Rules/SimulatedObjectSetupRule.h>
#include <EMotionFX/Source/Actor.h>

namespace EMotionFX
{
    using ActorCanSaveSimulatedObjectSetupFixtureBase = InitSceneAPIFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent,
        EMotionFX::Pipeline::ActorBuilder
    >;

    class ActorCanSaveSimulatedObjectSetupFixture
        : public ActorCanSaveSimulatedObjectSetupFixtureBase
    {
    public:
        void SetUp() override
        {
            ActorCanSaveSimulatedObjectSetupFixtureBase::SetUp();

            // Set up the scene graph
            m_scene = AZStd::make_unique<AZ::SceneAPI::Containers::MockScene>("MockScene");
            m_scene->SetOriginalSceneOrientation(AZ::SceneAPI::Containers::Scene::SceneOrientation::ZUp);

            AZ::SceneAPI::Containers::SceneGraph& graph = m_scene->GetGraph();

            graph.AddChild(graph.GetRoot(), "testRootBone", AZStd::make_shared<AZ::SceneData::GraphData::BoneData>());
        }

        AZStd::unique_ptr<AZ::SceneAPI::Containers::Scene> m_scene{};
    };

    TEST_F(ActorCanSaveSimulatedObjectSetupFixture, Test)
    {
        EMotionFX::Pipeline::Group::ActorGroup actorGroup;
        actorGroup.SetName("TestSimulatedObjectSaving");
        actorGroup.SetSelectedRootBone("testRootBone");
        auto simulatedObjectSetupRule = AZStd::make_shared<EMotionFX::Pipeline::Rule::SimulatedObjectSetupRule>();
        auto simulatedObjectSetup = AZStd::make_shared<EMotionFX::SimulatedObjectSetup>();
        simulatedObjectSetup->AddSimulatedObject("testSimulatedObject");
        simulatedObjectSetupRule->SetData(simulatedObjectSetup);
        actorGroup.GetRuleContainer().AddRule(simulatedObjectSetupRule);

        AZ::SceneAPI::Events::ExportProductList products;

        // Only run the Filling phase, to avoid any file writes
        EMotionFX::Pipeline::ActorGroupExportContext actorGroupExportContext(
            products,
            *m_scene,
            /*outputDirectory =*/"tmp",
            actorGroup,
            AZ::RC::Phase::Filling
        );

        EMotionFX::Pipeline::ActorGroupExporter exporter;
        AZ::SceneAPI::Events::ProcessingResult result = exporter.ProcessContext(actorGroupExportContext);

        ASSERT_EQ(result, AZ::SceneAPI::Events::ProcessingResult::Success) << "Failed to build actor";

        Actor* actor = exporter.GetActor();
        ASSERT_TRUE(actor);
        ASSERT_TRUE(actor->GetSimulatedObjectSetup());
        ASSERT_EQ(actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 1);
        EXPECT_STREQ(actor->GetSimulatedObjectSetup()->GetSimulatedObject(0)->GetName().c_str(), "testSimulatedObject");
    }
} // namespace EMotionFX

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/Behaviors/LodRuleBehavior.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneData/Behaviors/Registry.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>

namespace AZ
{
    namespace SceneData
    {
        struct SoftNameMock
            : SceneAPI::Events::GraphMetaInfoBus::Handler
        {
            SoftNameMock()
            {
                BusConnect();
            }

            ~SoftNameMock() override
            {
                BusDisconnect();
            }

            void GetVirtualTypes(
                SceneAPI::Events::GraphMetaInfo::VirtualTypesSet& types,
                const SceneAPI::Containers::Scene&,
                SceneAPI::Containers::SceneGraph::NodeIndex) override
            {
                // Indicate this node is a LOD1 type
                types.emplace(AZ_CRC_CE("LODMesh1"));
            }
        };

        TEST(LOD, LODRuleTest)
        {
            // Test that UpdateManifest doesn't crash when trying to auto-add new LOD levels
            SoftNameMock softNameMock;

            SceneAPI::SceneData::LodRuleBehavior lod;
            SceneAPI::Containers::Scene scene("test");

            auto lodRule = AZStd::shared_ptr<SceneAPI::SceneData::LodRule>(aznew SceneAPI::SceneData::LodRule());
            scene.GetManifest().AddEntry(lodRule);

            auto group = AZStd::shared_ptr<SceneAPI::SceneData::MeshGroup>(aznew SceneAPI::SceneData::MeshGroup());

            // Add a bunch of other rules first
            // This is necessary to replicate the bug condition where the index of the rule is used instead of the index of the LOD
            for (int i = 0; i < 5; ++i)
            {
                auto tangentsRule = AZStd::shared_ptr<SceneAPI::SceneData::TangentsRule>(aznew SceneAPI::SceneData::TangentsRule());
                group->GetRuleContainer().AddRule(tangentsRule);
            }

            group->GetRuleContainer().AddRule(lodRule);
            scene.GetManifest().AddEntry(group);

            auto meshData = AZStd::shared_ptr<GraphData::MeshData>(new GraphData::MeshData());
            scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "test", meshData);

            EXPECT_EQ(lodRule->GetLodCount(), 0);

            // This should auto-add 1 LOD because of the "test" node we added above along with the SoftNameMock which will report it as an LOD1
            lod.UpdateManifest(scene, SceneAPI::Events::AssetImportRequest::Update, SceneAPI::Events::AssetImportRequest::Generic);

            EXPECT_EQ(lodRule->GetLodCount(), 1);
        }
    }
}

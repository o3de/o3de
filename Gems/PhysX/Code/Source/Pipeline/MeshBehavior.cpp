/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PhysX_precompiled.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>

#include <Source/Pipeline/MeshBehavior.h>
#include <Source/Pipeline/MeshGroup.h>

namespace PhysX
{
    namespace Pipeline
    {
        void MeshBehavior::Activate()
        {
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
        }

        void MeshBehavior::Deactivate()
        {
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
        }

        void MeshBehavior::Reflect(AZ::ReflectContext* context)
        {
            MeshGroup::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MeshBehavior, BehaviorComponent>()->Version(1);
            }
        }

        void MeshBehavior::GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene)
        {
            if (AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IMeshData>(scene, false))
            {
                categories.emplace_back("PhysX", MeshGroup::TYPEINFO_Uuid());
            }
        }

        void MeshBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
        {
            if (!target.RTTI_IsTypeOf(MeshGroup::TYPEINFO_Uuid()))
            {
                return;
            }

            MeshGroup* group = azrtti_cast<MeshGroup*>(&target);
            group->SetName(AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<MeshGroup>(scene.GetName(), scene.GetManifest()));

            const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

            // Gather the nodes that should be selected by default
            AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& nodeSelectionList = group->GetSceneNodeSelectionList();
            AZ::SceneAPI::Utilities::SceneGraphSelector::UnselectAll(graph, nodeSelectionList);
            AZ::SceneAPI::Containers::SceneGraph::ContentStorageConstData graphContent = graph.GetContentStorage();
            auto view = AZ::SceneAPI::Containers::Views::MakeFilterView(graphContent, AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IMeshData>());
            for (auto iter = view.begin(), iterEnd = view.end(); iter != iterEnd; ++iter)
            {
                AZ::SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.ConvertToNodeIndex(iter.GetBaseIterator());

                AZStd::set<AZ::Crc32> types;
                AZ::SceneAPI::Events::GraphMetaInfoBus::Broadcast(&AZ::SceneAPI::Events::GraphMetaInfoBus::Events::GetVirtualTypes, types, scene, nodeIndex);

                if (types.count(AZ_CRC("PhysicsMesh", 0xc75d4ff1)) == 1)
                {
                    nodeSelectionList.AddSelectedNode(graph.GetNodeName(nodeIndex).GetPath());
                }
            }
        }

        AZ::SceneAPI::Events::ProcessingResult MeshBehavior::UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
            RequestingApplication /*requester*/)
        {
            if (action == ManifestAction::ConstructDefault)
            {
                return BuildDefault(scene);
            }
            else if (action == ManifestAction::Update)
            {
                return UpdatePhysXMeshGroups(scene);
            }
            else
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
        }

        AZ::SceneAPI::Events::ProcessingResult MeshBehavior::BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const
        {
            if (!AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IMeshData>(scene, true))
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            AZStd::shared_ptr<MeshGroup> group = AZStd::make_shared<MeshGroup>();

            // This is a group that's generated automatically so may not be saved to disk but would need to be recreated
            //      in the same way again. To guarantee the same uuid, generate a stable one instead.
            group->OverrideId(AZ::SceneAPI::DataTypes::Utilities::CreateStableUuid(scene, MeshGroup::TYPEINFO_Uuid()));

            EBUS_EVENT(AZ::SceneAPI::Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
            scene.GetManifest().AddEntry(AZStd::move(group));

            return AZ::SceneAPI::Events::ProcessingResult::Success;
        }

        AZ::SceneAPI::Events::ProcessingResult MeshBehavior::UpdatePhysXMeshGroups(AZ::SceneAPI::Containers::Scene& scene) const
        {
            bool updated = false;
            AZ::SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
            auto valueStorage  = manifest.GetValueStorage();
            auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<MeshGroup>(valueStorage);
            for (MeshGroup& group : view)
            {
                if (group.GetName().empty())
                {
                    group.SetName(AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<AZ::SceneAPI::DataTypes::IMeshGroup>(scene.GetName(), scene.GetManifest()));
                }

                AZ::SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), group.GetSceneNodeSelectionList());
                updated = true;
            }

            return updated ? AZ::SceneAPI::Events::ProcessingResult::Success : AZ::SceneAPI::Events::ProcessingResult::Ignored;
        }
    } // namespace SceneAPI
} // namespace AZ

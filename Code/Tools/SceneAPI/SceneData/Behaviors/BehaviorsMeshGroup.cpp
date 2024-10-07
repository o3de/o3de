/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <SceneAPI/SceneData/Behaviors/MeshGroup.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            void MeshGroup::Activate()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void MeshGroup::Deactivate()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void MeshGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MeshGroup, BehaviorComponent>()->Version(1);
                }
            }

            void MeshGroup::GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene)
            {
                if (SceneHasMeshGroup(scene) || Utilities::DoesSceneGraphContainDataLike<DataTypes::IMeshData>(scene, false))
                {
                    categories.emplace_back("Meshes", SceneData::MeshGroup::TYPEINFO_Uuid(), s_meshGroupPreferredTabOrder);
                }
            }

            void MeshGroup::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(SceneData::MeshGroup::TYPEINFO_Uuid()))
                {
                    return;
                }
                    
                SceneData::MeshGroup* group = azrtti_cast<SceneData::MeshGroup*>(&target);
                group->SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IMeshGroup>(scene.GetName(), scene.GetManifest()));
                Utilities::SceneGraphSelector::SelectAll(scene.GetGraph(), group->GetSceneNodeSelectionList());

                const Containers::SceneGraph& graph = scene.GetGraph();
                auto nameStorage = graph.GetNameStorage();
                auto contentStorage = graph.GetContentStorage();
                
                auto keyValueView = Containers::Views::MakePairView(nameStorage, contentStorage);
                auto filteredView = Containers::Views::MakeFilterView(keyValueView, Containers::DerivedTypeFilter<DataTypes::IMeshData>());
                for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                {
                    Events::GraphMetaInfo::VirtualTypesSet types;
                    auto keyValueIterator = it.GetBaseIterator();
                    Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                    EBUS_EVENT(Events::GraphMetaInfoBus, GetVirtualTypes, types, scene, index);
                    if (!types.empty())
                    {
                        // Mesh is not a standard static mesh, but a special type so remove it from the selected list.
                        group->GetSceneNodeSelectionList().RemoveSelectedNode(it->first.GetPath());
                    }
                }
            }

            Events::ProcessingResult MeshGroup::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateMeshGroups(scene);
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            Events::ProcessingResult MeshGroup::BuildDefault(Containers::Scene& scene) const
            {
                if (SceneHasMeshGroup(scene) ||
                    !Utilities::DoesSceneGraphContainDataLike<DataTypes::IMeshData>(scene, true))
                {
                    return Events::ProcessingResult::Ignored;
                }

                // There are meshes but no mesh group, so add a default mesh group to the manifest.
                AZStd::shared_ptr<SceneData::MeshGroup> group = AZStd::make_shared<SceneData::MeshGroup>();
                
                // This is a group that's generated automatically so may not be saved to disk but would need to be recreated
                //      in the same way again. To guarantee the same uuid, generate a stable one instead.
                group->OverrideId(DataTypes::Utilities::CreateStableUuid(scene, MeshGroup::TYPEINFO_Uuid()));
                
                EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return Events::ProcessingResult::Success;
            }

            Events::ProcessingResult MeshGroup::UpdateMeshGroups(Containers::Scene& scene) const
            {
                bool updated = false;
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage  = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<SceneData::MeshGroup>(valueStorage);
                for (SceneData::MeshGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IMeshGroup>(scene.GetName(), scene.GetManifest()));
                    }
                    if (group.GetId().IsNull())
                    {
                        // When the uuid it's null is likely because the manifest has been updated from an older version. Include the 
                        // name of the group as there could be multiple groups.
                        group.OverrideId(DataTypes::Utilities::CreateStableUuid(scene, MeshGroup::TYPEINFO_Uuid(), group.GetName()));
                    }
                    Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), group.GetSceneNodeSelectionList());
                    updated = true;
                }

                return updated ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
            }

            bool MeshGroup::SceneHasMeshGroup(const Containers::Scene& scene) const
            {
                const Containers::SceneManifest& manifest = scene.GetManifest();
                Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto meshGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), Containers::DerivedTypeFilter<DataTypes::IMeshGroup>());
                return meshGroup != manifestData.end();
            }
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ

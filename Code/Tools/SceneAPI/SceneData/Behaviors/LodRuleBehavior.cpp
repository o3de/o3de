/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneData/Behaviors/LodRuleBehavior.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            static AZStd::fixed_vector < AZ::Crc32, LodRule::m_maxLods > s_lodVirtualTypeKeys =
            {
                AZ_CRC("LODMesh1", 0xcbea988c),
                AZ_CRC("LODMesh2", 0x52e3c936),
                AZ_CRC("LODMesh3", 0x25e4f9a0),
                AZ_CRC("LODMesh4", 0xbb806c03),
                AZ_CRC("LODMesh5", 0xcc875c95)
            };

            void LodRuleBehavior::Activate()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
                Events::GraphMetaInfoBus::Handler::BusConnect();
            }

            void LodRuleBehavior::Deactivate()
            {
                Events::GraphMetaInfoBus::Handler::BusDisconnect();
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void LodRuleBehavior::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<LodRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            void LodRuleBehavior::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                //Initialize Mesh Groups. 
                if (target.RTTI_IsTypeOf(DataTypes::IMeshGroup::TYPEINFO_Uuid()) || target.RTTI_IsTypeOf(DataTypes::ISkinGroup::TYPEINFO_Uuid()))
                {
                    AZStd::shared_ptr<LodRule> lodRule = nullptr;
                    for (size_t lodLevel = 0; lodLevel < LodRule::m_maxLods; ++lodLevel)
                    {
                        SceneNodeSelectionList selection;
                        size_t lodCount = SelectLodMeshes(scene, selection, lodLevel);
                        if (lodCount > 0)
                        {
                            //Only create a lodRule if we have the first lod level. 
                            if (lodLevel == 0 && !lodRule)
                            {
                                lodRule = AZStd::make_shared<LodRule>();
                            }
                            lodRule->AddLod();
                            selection.CopyTo(lodRule->GetNodeSelectionList(lodLevel));
                        }
                        else
                        {
                            //Stop processing if we hit an empty lod.
                            break;
                        }
                    }
 
                    if(lodRule) 
                    {
                        DataTypes::IGroup* group = azrtti_cast<DataTypes::IGroup*>(&target);
                        group->GetRuleContainer().AddRule(AZStd::move(lodRule));
                    }
                }
                else if (target.RTTI_IsTypeOf(LodRule::TYPEINFO_Uuid()))
                {
                    LodRule* rule = azrtti_cast<LodRule*>(&target);
             
                    for (size_t lodLevel = 0; lodLevel < rule->GetLodCount(); ++lodLevel)
                    {
                        SelectLodMeshes(scene, rule->GetSceneNodeSelectionList(lodLevel), lodLevel);
                    }
                }
            }

            size_t LodRuleBehavior::SelectLodMeshes(const Containers::Scene& scene, DataTypes::ISceneNodeSelectionList& selection, size_t lodLevel) const
            {
                Utilities::SceneGraphSelector::SelectAll(scene.GetGraph(), selection);
                size_t lodMeshCount = 0;

                const Containers::SceneGraph& graph = scene.GetGraph();
                auto contentStorage = graph.GetContentStorage();
                auto nameStorage = graph.GetNameStorage();
                
                auto keyValueView = Containers::Views::MakePairView(nameStorage, contentStorage);
                auto filteredView = Containers::Views::MakeFilterView(keyValueView, Containers::DerivedTypeFilter<DataTypes::IMeshData>());
                for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                {
                    AZStd::set<Crc32> types;
                    auto keyValueIterator = it.GetBaseIterator();
                    Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                    EBUS_EVENT(Events::GraphMetaInfoBus, GetVirtualTypes, types, scene, index);

                    if (types.find(Events::GraphMetaInfo::GetIgnoreVirtualType()) != types.end() ||
                        types.find(s_lodVirtualTypeKeys[lodLevel]) == types.end())
                    {
                        selection.RemoveSelectedNode(it->first.GetPath());
                    }
                    else
                    {
                        lodMeshCount++;
                    }
                }
                return lodMeshCount;
            }

            Events::ProcessingResult LodRuleBehavior::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::Update)
                {
                    UpdateLodRules(scene);
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            
            void LodRuleBehavior::UpdateLodRules(Containers::Scene& scene) const
            {
                Containers::SceneManifest& manifest = scene.GetManifest();

                //Process Mesh or Skin Groups.
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<DataTypes::ISceneNodeGroup>(valueStorage);
                for (DataTypes::ISceneNodeGroup& group : view)
                {
                    AZ_TraceContext("Mesh/Skin Group", group.GetName());
                    const Containers::RuleContainer& rules = group.GetRuleContainer();
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t index = 0; index < ruleCount; ++index)
                    {
                        LodRule* rule = azrtti_cast<LodRule*>(rules.GetRule(index).get());
                        if (rule)
                        {
                            //update existing lods.
                            for (size_t lodLevel = 0; lodLevel < rule->GetLodCount(); ++lodLevel)
                            {
                                Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), rule->GetSceneNodeSelectionList(lodLevel));
                            }
                            //Check for new lods.
                            for (size_t lodLevel = rule->GetLodCount(); lodLevel < LodRule::m_maxLods; ++lodLevel)
                            {
                                SceneNodeSelectionList selection;
                                size_t lodCount = SelectLodMeshes(scene, selection, lodLevel);
                                if (lodCount > 0)
                                {
                                    rule->AddLod();
                                    selection.CopyTo(rule->GetNodeSelectionList(lodLevel));
                                }
                                else
                                {
                                    //Stop processing if we hit an empty lod.
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            void LodRuleBehavior::GetVirtualTypeName(AZStd::string& name, Crc32 type)
            {
                if (type == AZ_CRC("LODMesh1", 0xcbea988c)) { name = "LODMesh1"; }
                else if (type == AZ_CRC("LODMesh2", 0x52e3c936)) { name = "LODMesh2"; }
                else if (type == AZ_CRC("LODMesh3", 0x25e4f9a0)) { name = "LODMesh3"; }
                else if (type == AZ_CRC("LODMesh4", 0xbb806c03)) { name = "LODMesh4"; }
                else if (type == AZ_CRC("LODMesh5", 0xcc875c95)) { name = "LODMesh5"; }
            }

            void LodRuleBehavior::GetAllVirtualTypes(AZStd::set<Crc32>& types)
            {
                AZStd::copy(s_lodVirtualTypeKeys.begin(), s_lodVirtualTypeKeys.end(), AZStd::inserter(types, types.begin()));
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ

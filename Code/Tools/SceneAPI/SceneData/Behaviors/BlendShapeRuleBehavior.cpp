/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneData/Rules/BlendShapeRule.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Behaviors/BlendShapeRuleBehavior.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            void BlendShapeRuleBehavior::Activate()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void BlendShapeRuleBehavior::Deactivate()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void BlendShapeRuleBehavior::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<BlendShapeRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            void BlendShapeRuleBehavior::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(DataTypes::ISkinGroup::TYPEINFO_Uuid()))
                {
                    SceneData::SceneNodeSelectionList selection;
                    size_t blendShapeCount = SelectBlendShapes(scene, selection);

                    if (blendShapeCount > 0)
                    {
                        AZStd::shared_ptr<SceneData::BlendShapeRule> blendShapeRule = AZStd::make_shared<SceneData::BlendShapeRule>();
                        selection.CopyTo(blendShapeRule->GetNodeSelectionList());
                        
                        DataTypes::ISkinGroup* skinGroup = azrtti_cast<DataTypes::ISkinGroup*>(&target);
                        skinGroup->GetRuleContainer().AddRule(AZStd::move(blendShapeRule));
                    }
                }
                else if (target.RTTI_IsTypeOf(SceneData::BlendShapeRule::TYPEINFO_Uuid()))
                {
                    SceneData::BlendShapeRule* rule = azrtti_cast<SceneData::BlendShapeRule*>(&target);
                    SelectBlendShapes(scene, rule->GetSceneNodeSelectionList());
                }
            }

            size_t BlendShapeRuleBehavior::SelectBlendShapes(const Containers::Scene& scene, DataTypes::ISceneNodeSelectionList& selection) const
            {
                Utilities::SceneGraphSelector::UnselectAll(scene.GetGraph(), selection);
                size_t blendShapeCount = 0;

                const Containers::SceneGraph& graph = scene.GetGraph();
                auto contentStorage = graph.GetContentStorage();
                auto nameStorage = graph.GetNameStorage();
                
                auto keyValueView = Containers::Views::MakePairView(nameStorage, contentStorage);
                auto filteredView = Containers::Views::MakeFilterView(keyValueView, Containers::DerivedTypeFilter<DataTypes::IBlendShapeData>());
                for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                {
                    Events::GraphMetaInfo::VirtualTypesSet types;
                    auto keyValueIterator = it.GetBaseIterator();
                    Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                    EBUS_EVENT(Events::GraphMetaInfoBus, GetVirtualTypes, types, scene, index);
                    if (types.find(Events::GraphMetaInfo::GetIgnoreVirtualType()) == types.end())
                    {
                        selection.AddSelectedNode(it->first.GetPath());
                        blendShapeCount++;
                    }
                }
                return blendShapeCount;
            }

            Events::ProcessingResult BlendShapeRuleBehavior::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::Update)
                {
                    UpdateBlendShapeRules(scene);
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            void BlendShapeRuleBehavior::UpdateBlendShapeRules(Containers::Scene& scene) const
            {
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                
                auto view = Containers::MakeDerivedFilterView<DataTypes::ISkinGroup>(valueStorage);
                for (DataTypes::ISkinGroup& group : view)
                {
                    AZ_TraceContext("Skin group", group.GetName());
                    const Containers::RuleContainer& rules = group.GetRuleContainer();
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t index = 0; index < ruleCount; ++index)
                    {
                        SceneData::BlendShapeRule* rule = azrtti_cast<SceneData::BlendShapeRule*>(rules.GetRule(index).get());
                        if (rule)
                        {
                            Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), rule->GetSceneNodeSelectionList());
                        }
                    }
                }
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ

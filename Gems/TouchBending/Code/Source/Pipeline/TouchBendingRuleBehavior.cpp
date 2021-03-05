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
#include "TouchBending_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>

#include <Pipeline/TouchBendingRuleBehavior.h>
#include <Pipeline/TouchBendingRule.h>

namespace TouchBending
{
    namespace Pipeline
    {
        using namespace AZ;

        void TouchBendingRuleBehavior::Activate()
        {
            SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
            SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
        }

        void TouchBendingRuleBehavior::Deactivate()
        {
            SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
            SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
        }

        void TouchBendingRuleBehavior::Reflect(ReflectContext* context)
        {
            TouchBendingRule::Reflect(context);

            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<TouchBendingRuleBehavior, BehaviorComponent>()->Version(1);
            }
        }

        void TouchBendingRuleBehavior::InitializeObject(const SceneAPI::Containers::Scene& scene, SceneAPI::DataTypes::IManifestObject& target)
        {
            if (target.RTTI_IsTypeOf(SceneAPI::DataTypes::IMeshGroup::TYPEINFO_Uuid()))
            {
                SceneAPI::DataTypes::IMeshGroup* meshGroup = azrtti_cast<SceneAPI::DataTypes::IMeshGroup*>(&target);

                AZStd::shared_ptr<TouchBendingRule> touchBendingRule = AZStd::make_shared<TouchBendingRule>();

                //Let's see if there's a Bone with Virtual Type "TouchBend", the first bone that follows this naming
                //convention is the root bone.
                AZStd::string rootBoneName = FindRootBoneName(scene, nullptr);

                //Let's see if there's a mesh with Virtual Type "TouchBend", the mesh(es) that follows this naming
                //convention is the proximity trigger mesh.
                SceneAPI::SceneData::SceneNodeSelectionList selection;
                size_t proximityTriggerMeshCount = SelectProximityTriggerMeshes(scene, &selection);
                if (proximityTriggerMeshCount > 0)
                {
                    selection.CopyTo(touchBendingRule->m_proximityTriggerMeshes);
                }

                //Add the rule only in case there's default data.
                if (rootBoneName.empty() && (proximityTriggerMeshCount < 1))
                {
                    //It seems the user did not follow virtual type soft naming conventions
                    //for this asset.
                    return;
                }

                touchBendingRule->m_rootBoneName = rootBoneName;

                SceneAPI::Containers::RuleContainer& meshRules = meshGroup->GetRuleContainer();
                meshRules.AddRule(AZStd::move(touchBendingRule));
            }
            else if (target.RTTI_IsTypeOf(TouchBendingRule::TYPEINFO_Uuid()))
            {
                TouchBendingRule* rule = azrtti_cast<TouchBendingRule*>(&target);
                rule->m_rootBoneName = FindRootBoneName(scene, rule->m_rootBoneName.empty() ? nullptr : rule->m_rootBoneName.c_str());
                SelectProximityTriggerMeshes(scene, &rule->m_proximityTriggerMeshes);
            }
        }

        void TouchBendingRuleBehavior::GetAvailableModifiers(SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers, const SceneAPI::Containers::Scene& /*scene*/,
            const SceneAPI::DataTypes::IManifestObject& target)
        {
            if (!target.RTTI_IsTypeOf(SceneAPI::DataTypes::IMeshGroup::TYPEINFO_Uuid()))
            {
                return;
            }

            //When the "Add Modifier" Button in the FBX Settings Editor is clicked,
            //the expectation is that only those Modifiers(aka Rules) that have not been added to the mesh group
            //should be displayed for further selection.
            const SceneAPI::DataTypes::IMeshGroup* group = azrtti_cast<const SceneAPI::DataTypes::IMeshGroup*>(&target);
            const SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainerConst();

            const size_t ruleCount = rules.GetRuleCount();
            for (size_t i = 0; i < ruleCount; ++i)
            {
                AZStd::shared_ptr< SceneAPI::DataTypes::IRule> rule = rules.GetRule(i);
                if (rule->RTTI_IsTypeOf(SceneAPI::DataTypes::ITouchBendingRule::TYPEINFO_Uuid()))
                {
                    //ITouchBendingRule is already added into the MeshGroup.
                    return;
                }
            }

            modifiers.push_back(TouchBendingRule::TYPEINFO_Uuid());
        }

        SceneAPI::Events::ProcessingResult TouchBendingRuleBehavior::UpdateManifest(SceneAPI::Containers::Scene& scene, ManifestAction action,
            RequestingApplication /*requester*/)
        {
            // If there's not a corresponding *.assetinfo manifest file for a given *.fbx file this method is called
            // with action == ManifestAction::ConstructDefault. If the assetinfo file exist, then this called with action == ManifestAction::Update.
            if (action != ManifestAction::Update)
            {
                return SceneAPI::Events::ProcessingResult::Ignored;
            }

            //The assetinfo file exists, it is parsed and loaded in memory, and it is our mission to update (or maybe add) the TouchBendingRule in case the FBX file
            //has changed after the assetinfo file was initially created.
            return UpdateTouchBendingRule(scene);
        }

        SceneAPI::Events::ProcessingResult TouchBendingRuleBehavior::UpdateTouchBendingRule(SceneAPI::Containers::Scene& scene) const
        {
            SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
            auto valueStorage = manifest.GetValueStorage();
            auto view = SceneAPI::Containers::MakeDerivedFilterView<SceneAPI::DataTypes::IMeshGroup>(valueStorage);
            for (SceneAPI::DataTypes::IMeshGroup& group : view)
            {
                const SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainerConst();
                const size_t ruleCount = rules.GetRuleCount();
                for (size_t index = 0; index < ruleCount; ++index)
                {
                    TouchBendingRule* rule = azrtti_cast<TouchBendingRule*>(rules.GetRule(index).get());
                    if (rule)
                    {
                        rule->m_rootBoneName = FindRootBoneName(scene, rule->m_rootBoneName.empty() ? nullptr : rule->m_rootBoneName.c_str());
                        SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), rule->m_proximityTriggerMeshes);
                    }
                }
            }
            return AZ::SceneAPI::Events::ProcessingResult::Success;
        }

        size_t TouchBendingRuleBehavior::SelectProximityTriggerMeshes(const SceneAPI::Containers::Scene& scene, SceneAPI::DataTypes::ISceneNodeSelectionList* selection) const
        {
            if (selection)
            {
                SceneAPI::Utilities::SceneGraphSelector::SelectAll(scene.GetGraph(), *selection);
            }

            size_t proximityTriggerMeshCount = 0;

            const SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

            auto contentStorage = graph.GetContentStorage();
            auto nameStorage = graph.GetNameStorage();
            auto keyValueView = SceneAPI::Containers::Views::MakePairView(nameStorage, contentStorage);
            auto filteredView = SceneAPI::Containers::Views::MakeFilterView(keyValueView, SceneAPI::Containers::DerivedTypeFilter<SceneAPI::DataTypes::IMeshData>());
            for (auto nameAndContentMeshFilterIterator = filteredView.begin();
                nameAndContentMeshFilterIterator != filteredView.end();
                ++nameAndContentMeshFilterIterator)
            {
                AZStd::set<Crc32> types;
                auto keyValueIterator = nameAndContentMeshFilterIterator.GetBaseIterator();
                SceneAPI::Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                EBUS_EVENT(SceneAPI::Events::GraphMetaInfoBus, GetVirtualTypes, types, scene, index);
                if (types.find(AZ_CRC("TouchBend", 0xb56d5fbf)) == types.end() ||
                    types.find(SceneAPI::Events::GraphMetaInfo::GetIgnoreVirtualType()) != types.end())
                {
                    if (selection)
                    {
                        selection->RemoveSelectedNode(nameAndContentMeshFilterIterator->first.GetPath());
                    }
                }
                else
                {
                    proximityTriggerMeshCount++;
                    if (!selection)
                    {
                        break;
                    }
                }
            }
            return proximityTriggerMeshCount;
        }

        AZStd::string TouchBendingRuleBehavior::FindRootBoneName(const AZ::SceneAPI::Containers::Scene& scene, const char *selectedBoneName) const
        {
            AZStd::string retBoneName;
            const SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
            auto contentStorage = graph.GetContentStorage();
            auto nameStorage = graph.GetNameStorage();

            auto keyValueView = SceneAPI::Containers::Views::MakePairView(nameStorage, contentStorage);
            auto filteredView = SceneAPI::Containers::Views::MakeFilterView(keyValueView, SceneAPI::Containers::DerivedTypeFilter<SceneAPI::DataTypes::IBoneData>());
            for (auto nameAndContentBoneFilterIterator = filteredView.begin();
                nameAndContentBoneFilterIterator != filteredView.end();
                ++nameAndContentBoneFilterIterator)
            {
                AZStd::set<Crc32> types;
                auto keyValueIterator = nameAndContentBoneFilterIterator.GetBaseIterator();
                SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                EBUS_EVENT(SceneAPI::Events::GraphMetaInfoBus, GetVirtualTypes, types, scene, nodeIndex);
                if (types.find(SceneAPI::Events::GraphMetaInfo::GetIgnoreVirtualType()) != types.end())
                {
                    continue;
                }
                //This bone should not be ignored, does it have the name we are looking for?
                AZStd::string boneName = nameAndContentBoneFilterIterator->first.GetPath();
                if (selectedBoneName && !strcmp(selectedBoneName, boneName.c_str()))
                {
                    return boneName;
                }

                if (types.find(AZ_CRC("TouchBend", 0xb56d5fbf)) == types.end())
                {
                    continue;
                }

                if (!selectedBoneName)
                {
                    //If we are not looking for a bone in particular, then let's return the first bone that
                    //has the virtual type.
                    return boneName;
                }

                //So, we found a touch bendable bone, but its name does not match selectedBoneName,
                //record this bone name, but let's keep looking for a bone named selectedBoneName.
                retBoneName = boneName;
            }
            return retBoneName;
        }

    } // namespace Pipeline
} // namespace TouchBending

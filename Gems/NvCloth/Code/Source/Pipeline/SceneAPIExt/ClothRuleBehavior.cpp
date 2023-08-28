/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>

#include <Pipeline/SceneAPIExt/ClothRuleBehavior.h>
#include <Pipeline/SceneAPIExt/ClothRule.h>

namespace NvCloth
{
    namespace Pipeline
    {
        void ClothRuleBehavior::Activate()
        {
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
        }

        void ClothRuleBehavior::Deactivate()
        {
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
        }

        void ClothRuleBehavior::Reflect(AZ::ReflectContext* context)
        {
            ClothRule::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ClothRuleBehavior, BehaviorComponent>()->Version(1);
            }
        }

        void ClothRuleBehavior::GetAvailableModifiers(
            AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
            const AZ::SceneAPI::Containers::Scene& scene,
            const AZ::SceneAPI::DataTypes::IManifestObject& target)
        {
            AZ_UNUSED(scene);
            if (target.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::ISceneNodeGroup::TYPEINFO_Uuid()))
            {
                const AZ::SceneAPI::DataTypes::ISceneNodeGroup* group = azrtti_cast<const AZ::SceneAPI::DataTypes::ISceneNodeGroup*>(&target);
                if (IsValidGroupType(*group))
                {
                    modifiers.push_back(ClothRule::TYPEINFO_Uuid());
                }
            }
        }

        void ClothRuleBehavior::InitializeObject(
            [[maybe_unused]] const AZ::SceneAPI::Containers::Scene& scene,
            AZ::SceneAPI::DataTypes::IManifestObject& target)
        {
            // When a cloth rule is created in the Scene Settings...
            if (target.RTTI_IsTypeOf(ClothRule::TYPEINFO_Uuid()))
            {
                ClothRule* clothRule = azrtti_cast<ClothRule*>(&target);

                // Set default values
                clothRule->SetMeshNodeName(ClothRule::DefaultChooseNodeName);
                clothRule->SetInverseMassesStreamName(ClothRule::DefaultInverseMassesString);
                clothRule->SetMotionConstraintsStreamName(ClothRule::DefaultMotionConstraintsString);
                clothRule->SetBackstopStreamName(ClothRule::DefaultBackstopString);
            }
        }

        AZ::SceneAPI::Events::ProcessingResult ClothRuleBehavior::UpdateManifest(
            AZ::SceneAPI::Containers::Scene& scene,
            ManifestAction action,
            RequestingApplication requester)
        {
            AZ_UNUSED(requester);
            // When the manifest is updated let's check the content is still valid for cloth rules
            if (action == ManifestAction::Update)
            {
                bool updated = UpdateClothRules(scene);
                return updated ? AZ::SceneAPI::Events::ProcessingResult::Success : AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
            else
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
        }

        bool ClothRuleBehavior::IsValidGroupType(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group) const
        {
            // Cloth rules are available in Mesh Groups
            return group.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IMeshGroup::TYPEINFO_Uuid());
        }

        bool ClothRuleBehavior::UpdateClothRules(AZ::SceneAPI::Containers::Scene& scene)
        {
            bool rulesUpdated = false;

            auto& manifest = scene.GetManifest();
            auto valueStorage = manifest.GetValueStorage();
            auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<AZ::SceneAPI::DataTypes::ISceneNodeGroup>(valueStorage);

            // For each scene group...
            for (auto& group : view)
            {
                bool isValidGroupType = IsValidGroupType(group);

                AZStd::vector<size_t> rulesToRemove;

                auto& groupRules = group.GetRuleContainer();
                for (size_t index = 0; index < groupRules.GetRuleCount(); ++index)
                {
                    ClothRule* clothRule = azrtti_cast<ClothRule*>(groupRules.GetRule(index).get());
                    if (clothRule)
                    {
                        if (isValidGroupType)
                        {
                            rulesUpdated = UpdateClothRule(scene.GetGraph(), group, *clothRule) || rulesUpdated;
                        }
                        else
                        {
                            // Cloth rule found in a group that shouldn't have cloth rules, add for removal.
                            rulesToRemove.push_back(index);
                            rulesUpdated = true;
                        }
                    }
                }

                // Remove in reversed order, as otherwise the indices will be wrong. For example if we remove index 3, then index 6 would really be 5 afterwards.
                // By doing this in reversed order we remove items at the end of the list first so it won't impact the indices of previous ones.
                for (AZStd::vector<size_t>::reverse_iterator it = rulesToRemove.rbegin(); it != rulesToRemove.rend(); ++it)
                { 
                    groupRules.RemoveRule(*it);
                }
            }

            return rulesUpdated;
        }

        bool ClothRuleBehavior::UpdateClothRule(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group, ClothRule& clothRule)
        {
            bool ruleUpdated = false;

            if (clothRule.GetMeshNodeName() != ClothRule::DefaultChooseNodeName)
            {
                bool foundMeshNode = false;
                const AZStd::string& meshNodeName = clothRule.GetMeshNodeName();

                if (!meshNodeName.empty())
                {
                    const auto& selectedNodesList = group.GetSceneNodeSelectionList();
                    foundMeshNode = selectedNodesList.IsSelectedNode(meshNodeName);
                }

                // Mesh node selected in the cloth rule is not part of the list of selected nodes anymore, set the default value.
                if (!foundMeshNode)
                {
                    clothRule.SetMeshNodeName(ClothRule::DefaultChooseNodeName);
                    ruleUpdated = true;
                }
            }

            // If the Vertex Color Stream selected for the inverse masses doesn't exist anymore, set the default value.
            if (!clothRule.IsInverseMassesStreamDisabled() &&
                !ContainsVertexColorStream(graph, clothRule.GetInverseMassesStreamName()))
            {
                clothRule.SetInverseMassesStreamName(ClothRule::DefaultInverseMassesString);
                ruleUpdated = true;
            }

            // If the Vertex Color Stream selected for the motion constraints doesn't exist anymore, set the default value.
            if (!clothRule.IsMotionConstraintsStreamDisabled() &&
                !ContainsVertexColorStream(graph, clothRule.GetMotionConstraintsStreamName()))
            {
                clothRule.SetMotionConstraintsStreamName(ClothRule::DefaultMotionConstraintsString);
                ruleUpdated = true;
            }

            // If the Vertex Color Stream selected for the backstop doesn't exist anymore, set the default value.
            if (!clothRule.IsBackstopStreamDisabled() &&
                !ContainsVertexColorStream(graph, clothRule.GetBackstopStreamName()))
            {
                clothRule.SetBackstopStreamName(ClothRule::DefaultBackstopString);
                ruleUpdated = true;
            }

            return ruleUpdated;
        }

        bool ClothRuleBehavior::ContainsVertexColorStream(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZStd::string& streamName) const
        {
            if (streamName.empty())
            {
                return false;
            }

            auto graphNames = graph.GetNameStorage();

            auto graphNameIt = AZStd::find_if(graphNames.cbegin(), graphNames.cend(),
                [&streamName](const AZ::SceneAPI::Containers::SceneGraph::NameStorageType& graphName)
                {
                    return streamName == graphName.GetName();
                });

            return graphNameIt != graphNames.cend();
        }
    } // namespace Pipeline
} // namespace NvCloth

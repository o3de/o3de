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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Rules/MeshRule.h>
#include <SceneAPIExt/Behaviors/MeshRuleBehavior.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void MeshRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                Rule::MeshRule::Reflect(context);

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MeshRuleBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void MeshRuleBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void MeshRuleBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
            }

            void MeshRuleBehavior::InitializeObject([[maybe_unused]] const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(Group::IActorGroup::TYPEINFO_Uuid()))
                {
                    Group::IActorGroup* actorGroup = azrtti_cast<Group::IActorGroup*>(&target);

                    // TODO: Check if the sceneGraph contain mesh data.
                    AZ::SceneAPI::Containers::RuleContainer& rules = actorGroup->GetRuleContainer();
                    if (!rules.ContainsRuleOfType<Rule::IMeshRule>())
                    {
                        rules.AddRule(AZStd::make_shared<Rule::MeshRule>());
                    }
                }
            }

            AZ::SceneAPI::Events::ProcessingResult MeshRuleBehavior::UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action, RequestingApplication requester)
            {
                AZ_UNUSED(requester);

                // When the manifest is updated let's check the content is still valid
                if (action == ManifestAction::Update)
                {
                    bool updated = UpdateMeshRules(scene);
                    return updated ? AZ::SceneAPI::Events::ProcessingResult::Success : AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }
                else
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }
            }

            bool MeshRuleBehavior::IsValidGroupType(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group) const
            {
                return group.RTTI_IsTypeOf(EMotionFX::Pipeline::Group::IActorGroup::TYPEINFO_Uuid());
            }

            bool MeshRuleBehavior::UpdateMeshRules(AZ::SceneAPI::Containers::Scene& scene)
            {
                bool rulesUpdated = false;

                auto& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<AZ::SceneAPI::DataTypes::ISceneNodeGroup>(valueStorage);

                for (auto& group : view)
                {
                    bool isValidGroupType = IsValidGroupType(group);

                    AZStd::vector<size_t> rulesToRemove;

                    auto& groupRules = group.GetRuleContainer();
                    for (size_t index = 0; index < groupRules.GetRuleCount(); ++index)
                    {
                        EMotionFX::Pipeline::Rule::MeshRule* meshRule = azrtti_cast<EMotionFX::Pipeline::Rule::MeshRule*>(groupRules.GetRule(index).get());
                        if (meshRule)
                        {
                            if (isValidGroupType)
                            {
                                rulesUpdated = UpdateMeshRule(scene, group, *meshRule) || rulesUpdated;
                            }
                            else
                            {
                                // Mesh rule found in a group that shouldn't have mesh rules, add for removal.
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

            bool MeshRuleBehavior::UpdateMeshRule(const AZ::SceneAPI::Containers::Scene& scene, [[maybe_unused]] const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group, EMotionFX::Pipeline::Rule::MeshRule& meshRule)
            {
                bool ruleUpdated = false;

                if (!meshRule.IsVertexColorsDisabled())
                {
                    const AZStd::string& vertexColorStreamName = meshRule.GetVertexColorStreamName();

                    const auto& graph = scene.GetGraph();
                    auto graphNames = graph.GetNameStorage();

                    auto found = AZStd::find_if(graphNames.cbegin(), graphNames.cend(), 
                        [&vertexColorStreamName](const AZ::SceneAPI::Containers::SceneGraph::NameStorageType& graphName)
                        {
                            return vertexColorStreamName == graphName.GetName();
                        });

                    // Vertex Color Stream selected in the mesh rule doesn't exist anymore, disable it.
                    if (found == graphNames.cend())
                    {
                        meshRule.DisableVertexColors();
                        ruleUpdated = true;
                    }
                }

                return ruleUpdated;
            }
        } // Behavior
    } // Pipeline
} // EMotionFX

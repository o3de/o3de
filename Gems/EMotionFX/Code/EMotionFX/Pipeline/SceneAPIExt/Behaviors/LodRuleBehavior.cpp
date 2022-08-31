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
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>

#include <SceneAPIExt/Rules/LodRule.h>
#include <SceneAPIExt/Behaviors/LodRuleBehavior.h>
#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Utilities/LODSelector.h>
#include <SceneAPIExt/Data/LodNodeSelectionList.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void LodRuleBehavior::Activate()
            {
                SceneEvents::ManifestMetaInfoBus::Handler::BusConnect();
                SceneEvents::AssetImportRequestBus::Handler::BusConnect();
                SceneEvents::GraphMetaInfoBus::Handler::BusConnect();
            }

            void LodRuleBehavior::Deactivate()
            {
                SceneEvents::GraphMetaInfoBus::Handler::BusDisconnect();
                SceneEvents::AssetImportRequestBus::Handler::BusDisconnect();
                SceneEvents::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void LodRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<LodRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            void LodRuleBehavior::InitializeObject(const SceneContainers::Scene& scene, SceneDataTypes::IManifestObject& target)
            {
                if (azrtti_istypeof<Rule::LodRule>(&target))
                {
                    Rule::LodRule* rule = azrtti_cast<Rule::LodRule*>(&target);

                    const size_t lodRuleCount = rule->GetLodRuleCount();
                    for (size_t ruleIndex = 0; ruleIndex < lodRuleCount; ++ruleIndex)
                    {
                        Utilities::LODSelector::SelectLODBones(scene, rule->GetSceneNodeSelectionList(ruleIndex), ruleIndex);
                    }
                }
            }

            void LodRuleBehavior::BuildLODRuleForActor(const SceneContainers::Scene& scene, SceneDataTypes::IManifestObject& target)
            {
                if (azrtti_istypeof<Group::IActorGroup>(&target))
                {
                    AZStd::shared_ptr<Rule::LodRule> lodRule;
                    for (size_t lodLevel = 0; lodLevel < g_maxLods; ++lodLevel)
                    {
                        Data::LodNodeSelectionList selection;
                        const size_t lodCount = Utilities::LODSelector::SelectLODBones(scene, selection, lodLevel);
                        if (lodCount > 0)
                        {
                            //Only create a lodRule if we have the first lod level. 
                            if (lodLevel == 0 && !lodRule)
                            {
                                lodRule = AZStd::make_shared<Rule::LodRule>();
                            }
                            lodRule->AddLod();
                            selection.CopyTo(lodRule->GetSceneNodeSelectionList(lodLevel));
                        }
                        else
                        {
                            //Stop processing if we hit an empty lod.
                            break;
                        }
                    }

                    if (lodRule)
                    {
                        Group::IActorGroup* group = azrtti_cast<Group::IActorGroup*>(&target);
                        group->GetRuleContainer().AddRule(AZStd::move(lodRule));

                        // Change the default root node.
                        const char* rootPath = Utilities::LODSelector::FindLODRootPath(scene, 0);
                        if (rootPath)
                        {
                            group->SetSelectedRootBone(rootPath);
                        }
                    }
                }
            }

            SceneEvents::ProcessingResult LodRuleBehavior::UpdateManifest(SceneContainers::Scene& scene, ManifestAction action,
                RequestingApplication requester)
            {
                AZ_UNUSED(requester);
                if (action == ManifestAction::Update)
                {
                    UpdateLodRules(scene);
                    return SceneEvents::ProcessingResult::Success;
                }
                else
                {
                    return SceneEvents::ProcessingResult::Ignored;
                }
            }

            void LodRuleBehavior::UpdateLodRules(SceneContainers::Scene& scene) const
            {
                SceneContainers::SceneManifest& manifest = scene.GetManifest();

                //Process Mesh or Skin Groups.
                auto valueStorage = manifest.GetValueStorage();
                auto view = SceneContainers::MakeDerivedFilterView<SceneDataTypes::ISceneNodeGroup>(valueStorage);
                for (SceneDataTypes::ISceneNodeGroup& group : view)
                {
                    AZ_TraceContext("Mesh/Skin Group", group.GetName());
                    const SceneContainers::RuleContainer& rules = group.GetRuleContainer();
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t index = 0; index < ruleCount; ++index)
                    {
                        Rule::LodRule* rule = azrtti_cast<Rule::LodRule*>(rules.GetRule(index).get());
                        if (rule)
                        {
                            // Update existing lods.
                            const size_t lodRuleCount = rule->GetLodRuleCount();
                            for (size_t ruleIndex = 0; ruleIndex < lodRuleCount; ++ruleIndex)
                            {
                                SceneUtilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), rule->GetSceneNodeSelectionList(ruleIndex));
                            }

                            // Check for new lods.
                            for (size_t ruleIndex = lodRuleCount; ruleIndex < g_maxLods; ++ruleIndex)
                            {
                                Data::LodNodeSelectionList selection;
                                const size_t lodCount = Utilities::LODSelector::SelectLODBones(scene, selection, ruleIndex);
                                if (lodCount > 0)
                                {
                                    rule->AddLod();
                                    selection.CopyTo(rule->GetSceneNodeSelectionList(ruleIndex));
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

            void LodRuleBehavior::GetVirtualTypeName(AZStd::string& name, AZ::Crc32 type)
            {
                if (type == AZ_CRC_CE("LODMesh1")) { name = "LODMesh1"; }
                else if (type == AZ_CRC_CE("LODMesh2")) { name = "LODMesh2"; }
                else if (type == AZ_CRC_CE("LODMesh3")) { name = "LODMesh3"; }
                else if (type == AZ_CRC_CE("LODMesh4")) { name = "LODMesh4"; }
                else if (type == AZ_CRC_CE("LODMesh5")) { name = "LODMesh5"; }
            }
        }
    }
}

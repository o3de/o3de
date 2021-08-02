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
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneData/Rules/StaticMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/SkinMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Behaviors/MeshAdvancedRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            void MeshAdvancedRule::Activate()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void MeshAdvancedRule::Deactivate()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void MeshAdvancedRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MeshAdvancedRule, BehaviorComponent>()->Version(1);
                }
            }

            void MeshAdvancedRule::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                AZStd::string firstVertexColorStream = GetFirstVertexColorStream(scene);
                if (target.RTTI_IsTypeOf(DataTypes::ISceneNodeGroup::TYPEINFO_Uuid()))
                {
                    if (!firstVertexColorStream.empty())
                    {
                        if (target.RTTI_IsTypeOf(DataTypes::ISkinGroup::TYPEINFO_Uuid()))
                        {
                            AZStd::shared_ptr<SceneData::SkinMeshAdvancedRule> rule = AZStd::make_shared<SceneData::SkinMeshAdvancedRule>();
                            rule->SetVertexColorStreamName(firstVertexColorStream.empty() ?
                                DataTypes::s_advancedDisabledString : AZStd::move(firstVertexColorStream));
                            DataTypes::ISceneNodeGroup* sceneNodeGroup = azrtti_cast<DataTypes::ISceneNodeGroup*>(&target);
                            sceneNodeGroup->GetRuleContainer().AddRule(AZStd::move(rule));
                        }
                        else if (target.RTTI_IsTypeOf(DataTypes::IMeshGroup::TYPEINFO_Uuid()))
                        {
                            AZStd::shared_ptr<SceneData::StaticMeshAdvancedRule> rule = AZStd::make_shared<SceneData::StaticMeshAdvancedRule>();
                            rule->SetVertexColorStreamName(firstVertexColorStream.empty() ?
                                DataTypes::s_advancedDisabledString : AZStd::move(firstVertexColorStream));
                            DataTypes::ISceneNodeGroup* sceneNodeGroup = azrtti_cast<DataTypes::ISceneNodeGroup*>(&target);
                            sceneNodeGroup->GetRuleContainer().AddRule(AZStd::move(rule));
                        }
                    }
                }
                else if (target.RTTI_IsTypeOf(SceneData::StaticMeshAdvancedRule::TYPEINFO_Uuid()))
                {
                    SceneData::StaticMeshAdvancedRule* rule = azrtti_cast<SceneData::StaticMeshAdvancedRule*>(&target);
                    rule->SetVertexColorStreamName(firstVertexColorStream.empty() ?
                        DataTypes::s_advancedDisabledString : AZStd::move(firstVertexColorStream));
                }
                else if (target.RTTI_IsTypeOf(SceneData::SkinMeshAdvancedRule::TYPEINFO_Uuid()))
                {
                    SceneData::SkinMeshAdvancedRule* rule = azrtti_cast<SceneData::SkinMeshAdvancedRule*>(&target);
                    rule->SetVertexColorStreamName(firstVertexColorStream.empty() ?
                        DataTypes::s_advancedDisabledString : AZStd::move(firstVertexColorStream));
                }
            }

            Events::ProcessingResult MeshAdvancedRule::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication requester)
            {
                AZ_UNUSED(requester);

                if (action == ManifestAction::Update)
                {
                    UpdateMeshAdvancedRules(scene);
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            void MeshAdvancedRule::UpdateMeshAdvancedRules(Containers::Scene& scene) const
            {
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();                
                auto view = Containers::MakeDerivedFilterView<DataTypes::ISceneNodeGroup>(valueStorage);
                for (DataTypes::ISceneNodeGroup& group : view)
                {
                    AZ_TraceContext("Scene node group", group.GetName());
                    Containers::RuleContainer& rules = group.GetRuleContainer();
                    const size_t ruleCount = rules.GetRuleCount();

                    // The Mesh Advanced Rules were previously invalidly applied to any group containing a vertex color stream, and should be cleaned up if unnecessarily added to existing data.
                    // We use a list to track indices of rules to remove in a separate pass since the RuleContainer does not have direct iterator access.
                    bool isValidGroupType = group.RTTI_IsTypeOf(DataTypes::IMeshGroup::TYPEINFO_Uuid()) || group.RTTI_IsTypeOf(DataTypes::ISkinGroup::TYPEINFO_Uuid());
                    AZStd::vector<size_t> rulesToRemove;

                    for (size_t index = 0; index < ruleCount; ++index)
                    {
                        DataTypes::IMeshAdvancedRule* rule = azrtti_cast<DataTypes::IMeshAdvancedRule*>(rules.GetRule(index).get());
                        if (rule)
                        {
                            if (isValidGroupType)
                            {
                                UpdateMeshAdvancedRule(scene, rule);
                            }
                            else
                            {
                                rulesToRemove.push_back(index);
                            }
                        }
                    }

                    // Remove in reversed order, as otherwise the indices will be wrong. For example if we remove index 3, then index 6 would really be 5 afterwards.
                    // By doing this in reversed order we remove items at the end of the list first so it won't impact the indices of previous ones.
                    for (AZStd::vector<size_t>::reverse_iterator it = rulesToRemove.rbegin(); it != rulesToRemove.rend(); ++it)
                    { 
                        rules.RemoveRule(*it);
                    }
                }
            }

            void MeshAdvancedRule::UpdateMeshAdvancedRule(Containers::Scene& scene, DataTypes::IMeshAdvancedRule* rule) const
            {
                if (!rule)
                {
                    return;
                }
                SceneData::SkinMeshAdvancedRule* skinRule = azrtti_cast<SceneData::SkinMeshAdvancedRule*>(rule);
                SceneData::StaticMeshAdvancedRule* meshRule = azrtti_cast<SceneData::StaticMeshAdvancedRule*>(rule);
                if (!(skinRule || meshRule))
                {
                    return;
                }

                const AZStd::string& vertexColorStreamName = rule->GetVertexColorStreamName();
                bool foundColorStream = vertexColorStreamName == DataTypes::s_advancedDisabledString;
                const Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::NameStorageConstData graphNames = graph.GetNameStorage();
                for (auto it = graphNames.begin(); it != graphNames.end(); ++it)
                {
                    if (foundColorStream)
                    {
                        break;
                    }
                    const char* nodeName = it->GetName();
                    if (!foundColorStream && vertexColorStreamName == nodeName)
                    {
                        foundColorStream = true;
                    }
                }

                if (!foundColorStream)
                {
                    AZStd::string newColorStreamName = GetFirstVertexColorStream(scene);
                    AZ_TracePrintf(Utilities::WarningWindow, "Old vertex color stream name not found so renamed from '%s' to '%s'.",
                        vertexColorStreamName.c_str(), newColorStreamName.c_str());
                    if (skinRule)
                    {
                        skinRule->SetVertexColorStreamName(newColorStreamName.empty() ? DataTypes::s_advancedDisabledString : AZStd::move(newColorStreamName));
                    }
                    else if (meshRule)
                    {
                        meshRule->SetVertexColorStreamName(newColorStreamName.empty() ? DataTypes::s_advancedDisabledString : AZStd::move(newColorStreamName));
                    }
                }
            }

            AZStd::string MeshAdvancedRule::GetFirstVertexColorStream(const Containers::Scene& scene) const
            {
                const Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::ContentStorageConstData graphContent = graph.GetContentStorage();
                auto vertexColorData = AZStd::find_if(graphContent.begin(), graphContent.end(),
                    Containers::DerivedTypeFilter<DataTypes::IMeshVertexColorData>());
                if (vertexColorData != graphContent.end())
                {
                    return graph.GetNodeName(graph.ConvertToNodeIndex(vertexColorData)).GetName();
                }
                else
                {
                    return AZStd::string();
                }
            }
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ

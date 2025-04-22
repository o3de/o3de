/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabGroup/DefaultProceduralPrefab.h>
#include <PrefabGroup/PrefabGroupBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/Entity/EntityUtilityComponent.h>
#include <AzToolsFramework/Prefab/PrefabLoaderScriptingBus.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemScriptingBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ICustomPropertyData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneData/Rules/UnmodifiableRule.h>
#include <AzCore/Component/EntityId.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AZ::SceneAPI::PrefabGroupRequests::ManifestUpdates, "{B84CBFB5-4630-4484-AE69-A4155A8B0D9B}");
}

namespace AZ::SceneAPI
{
    struct PrefabGroupNotificationHandler final
        : public AZ::SceneAPI::PrefabGroupNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
        AZ_EBUS_BEHAVIOR_BINDER(
            PrefabGroupNotificationHandler,
            "{F1962BD1-D722-4C5F-A883-76F1004C3247}",
            AZ::SystemAllocator,
            OnUpdatePrefabEntity);

        virtual ~PrefabGroupNotificationHandler() = default;

        void OnUpdatePrefabEntity(const AZ::EntityId& prefabEntity) override
        {
            Call(FN_OnUpdatePrefabEntity, prefabEntity);
        }
    };

    DefaultProceduralPrefabGroup::DefaultProceduralPrefabGroup()
    {
        PrefabGroupEventBus::Handler::BusConnect();
    }

    DefaultProceduralPrefabGroup::~DefaultProceduralPrefabGroup()
    {
        PrefabGroupEventBus::Handler::BusDisconnect();
    }

    void DefaultProceduralPrefabGroup::Reflect(ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PrefabGroupNotificationBus>("PrefabGroupNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "prefab")
                ->Handler<PrefabGroupNotificationHandler>()
                ->Event("OnUpdatePrefabEntity", &PrefabGroupNotificationBus::Events::OnUpdatePrefabEntity);

            behaviorContext->EBus<PrefabGroupEventBus>("PrefabGroupEventBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(Script::Attributes::Module, "prefab")
                ->Event("GeneratePrefabGroupManifestUpdates", &PrefabGroupRequests::GeneratePrefabGroupManifestUpdates);
        }
    }

    AZStd::optional<PrefabGroupRequests::ManifestUpdates> DefaultProceduralPrefabGroup::GeneratePrefabGroupManifestUpdates(
        const Scene& scene) const
    {
        auto nodeDataMap = CalculateNodeDataMap(scene);
        if (nodeDataMap.empty())
        {
            return {};
        }

        // compute the filenames of the scene file
        AZStd::string relativeSourcePath = scene.GetSourceFilename();
        AZStd::string watchFolder = scene.GetWatchFolder() + "/";
        // the watch folder and forward slash is used in the asset hint path of the file
        AZ::StringFunc::Replace(relativeSourcePath, "\\", "/");
        AZ::StringFunc::Replace(watchFolder, "\\", "/");
        AZ::StringFunc::Replace(relativeSourcePath, watchFolder.c_str(), "");
        AZ::StringFunc::Replace(relativeSourcePath, ".", "_");
        AZStd::string filenameOnly{ relativeSourcePath };
        AZ::StringFunc::Path::GetFileName(filenameOnly.c_str(), filenameOnly);
        AZ::StringFunc::Path::ReplaceExtension(filenameOnly, "procprefab");

        ManifestUpdates manifestUpdates;

        auto nodeEntityMap = CreateNodeEntityMap(manifestUpdates, nodeDataMap, scene, relativeSourcePath);
        if (nodeEntityMap.empty())
        {
            return {};
        }

        auto entities = FixUpEntityParenting(nodeEntityMap, scene.GetGraph(), nodeDataMap);
        if (entities.empty())
        {
            return {};
        }

        if (CreatePrefabGroupManifestUpdates(manifestUpdates, scene, entities, filenameOnly, relativeSourcePath) == false )
        {
            return {};
        }

        return AZStd::make_optional(AZStd::move(manifestUpdates));
    }

    DefaultProceduralPrefabGroup::NodeDataMap DefaultProceduralPrefabGroup::CalculateNodeDataMap(
        const Containers::Scene& scene) const
    {
        auto graph = scene.GetGraph();
        const auto view = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(
            graph,
            graph.GetRoot(),
            graph.GetContentStorage().cbegin(),
            true);

        if (view.empty())
        {
            return {};
        }

        NodeDataMap nodeDataMap;
        for (auto it = view.begin(); it != view.end(); ++it)
        {
            Containers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
            const auto currentContent = graph.GetNodeContent(currentIndex);
            if (currentContent)
            {
                if (azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshData>(currentContent.get()))
                {
                    // get the MeshData child node index values for Transform and CustomPropertyData
                    auto childIndex = it.GetHierarchyIterator()->GetChildIndex();

                    NodeDataForEntity nodeDataForEntity;
                    nodeDataForEntity.m_meshIndex = currentIndex;

                    while (childIndex.IsValid())
                    {
                        const auto childContent = graph.GetNodeContent(childIndex);
                        if (childContent)
                        {
                            if (azrtti_istypeof<AZ::SceneAPI::DataTypes::ITransform>(childContent.get()))
                            {
                                if (!nodeDataForEntity.m_transformIndex.IsValid())
                                {
                                    // The first child transform of the mesh is applied to the mesh entity
                                    nodeDataForEntity.m_transformIndex = childIndex;
                                }
                                else
                                {
                                    // All other child transforms of the mesh represent unique entities
                                    NodeDataForEntity newNodeDataForEntity;
                                    newNodeDataForEntity.m_transformIndex = childIndex;
                                    nodeDataMap.emplace(NodeDataMapEntry{ childIndex, AZStd::move(newNodeDataForEntity) });
                                }
                            }
                            else if (azrtti_istypeof<AZ::SceneAPI::DataTypes::ICustomPropertyData>(childContent.get()))
                            {
                                nodeDataForEntity.m_propertyMapIndex = childIndex;
                            }
                        }
                        childIndex = graph.GetNodeSibling(childIndex);
                    }

                    nodeDataMap.emplace(NodeDataMapEntry{ currentIndex, AZStd::move(nodeDataForEntity) });
                }
                else if (azrtti_istypeof<AZ::SceneAPI::DataTypes::ITransform>(currentContent.get()))
                {
                    // Check if this transform node is not associated with any meshes
                    auto parentNodeIndex = graph.GetNodeParent(currentIndex);
                    const auto parentContent = graph.GetNodeContent(parentNodeIndex);
                    if (!azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshData>(parentContent.get()))
                    {
                        NodeDataForEntity nodeDataForEntity;
                        nodeDataForEntity.m_transformIndex = currentIndex;
                        nodeDataMap.emplace(NodeDataMapEntry{ currentIndex, AZStd::move(nodeDataForEntity) });
                    }
                }
            }
        }

        return nodeDataMap;
    }

    bool DefaultProceduralPrefabGroup::AddEditorMaterialComponent(
        const AZ::EntityId& entityId,
        const DataTypes::ICustomPropertyData& propertyData) const
    {
        const auto propertyMaterialPathIterator = propertyData.GetPropertyMap().find("o3de_default_material");
        if (propertyMaterialPathIterator == propertyData.GetPropertyMap().end())
        {
            // skip these assignment since the default material override was not provided
            return true;
        }

        const AZStd::any& propertyMaterialPath = propertyMaterialPathIterator->second;
        if (propertyMaterialPath.empty() || propertyMaterialPath.is<AZStd::string>() == false)
        {
            AZ_Error("prefab", false, "The 'o3de_default_material' custom property value type must be a string."
                "This will need to be fixed in the DCC tool and re-export the file asset.");
            return false;
        }

        // find asset path via node data
        const AZStd::string* materialAssetPath = AZStd::any_cast<AZStd::string>(&propertyMaterialPath);
        if (materialAssetPath->empty())
        {
            AZ_Error("prefab", false, "Material asset path must not be empty.");
            return false;
        }

        // create a material component for this entity's mesh to render with
        AzFramework::BehaviorComponentId editorMaterialComponent;
        AzToolsFramework::EntityUtilityBus::BroadcastResult(
            editorMaterialComponent,
            &AzToolsFramework::EntityUtilityBus::Events::GetOrAddComponentByTypeName,
            entityId,
            "EditorMaterialComponent");

        if (editorMaterialComponent.IsValid() == false)
        {
            AZ_Warning("prefab", false, "Could not add the EditorMaterialComponent component; project needs Atom enabled.");
            return {};
        }

        // the material product asset such as 'myassets/path/to/cool.azmaterial' is assigned via hint
        auto materialAssetJson = AZStd::string::format(
            R"JSON(
                    {"Controller":{"Configuration":{"materials":[{"Value":{"MaterialAsset":{"assetHint":"%s"}}}]}}}
             )JSON", materialAssetPath->c_str());

        bool result = false;
        AzToolsFramework::EntityUtilityBus::BroadcastResult(
            result,
            &AzToolsFramework::EntityUtilityBus::Events::UpdateComponentForEntity,
            entityId,
            editorMaterialComponent,
            materialAssetJson);

        AZ_Error("prefab", result, "UpdateComponentForEntity failed for EditorMaterialComponent component");
        return result;
    }

    bool DefaultProceduralPrefabGroup::AddEditorMeshComponent(
        const AZ::EntityId& entityId,
        const AZStd::string& relativeSourcePath,
        const AZStd::string& meshGroupName,
        const AZStd::string& sourceFileExtension) const
    {
        // Since the mesh component lives in a gem, then create it by name
        AzFramework::BehaviorComponentId editorMeshComponent;
        AzToolsFramework::EntityUtilityBus::BroadcastResult(
            editorMeshComponent,
            &AzToolsFramework::EntityUtilityBus::Events::GetOrAddComponentByTypeName,
            entityId,
            "{DCE68F6E-2E16-4CB4-A834-B6C2F900A7E9} AZ::Render::EditorMeshComponent");

        if (editorMeshComponent.IsValid() == false)
        {
            AZ_Warning("prefab", false, "Could not add the EditorMeshComponent component; project needs Atom enabled.");
            return {};
        }

        // assign mesh asset id hint using JSON
        AZ::IO::Path modelAssetPath(relativeSourcePath, '/');
        modelAssetPath.ReplaceFilename(AZ::IO::PathView(meshGroupName));
        modelAssetPath.ReplaceExtension(AZ::IO::PathView(sourceFileExtension));

        auto meshAssetJson = AZStd::string::format(
            R"JSON(
                   {"Controller": {"Configuration": {"ModelAsset": { "assetHint": "%s.azmodel"}}}}
             )JSON", modelAssetPath.LexicallyNormal().String().c_str());

        bool result = false;
        AzToolsFramework::EntityUtilityBus::BroadcastResult(
            result,
            &AzToolsFramework::EntityUtilityBus::Events::UpdateComponentForEntity,
            entityId,
            editorMeshComponent,
            meshAssetJson);

        AZ_Error("prefab", result, "UpdateComponentForEntity failed for EditorMeshComponent component");
        return result;
    }

    bool DefaultProceduralPrefabGroup::CreateMeshGroupAndComponents(
        ManifestUpdates& manifestUpdates,
        AZ::EntityId entityId,
        const NodeDataForEntity& nodeData,
        const NodeDataMap& nodeDataMap,
        const Containers::Scene& scene,
        const AZStd::string& relativeSourcePath) const
    {
        AZStd::shared_ptr<SceneData::MeshGroup> meshGroup(BuildMeshGroupForNode(scene, nodeData, nodeDataMap));
        manifestUpdates.emplace_back(meshGroup);

        if (AddEditorMeshComponent(entityId, relativeSourcePath, meshGroup->GetName(), scene.GetSourceExtension()) == false)
        {
            return false;
        }

        const auto& graph = scene.GetGraph();
        const auto propertyDataIndex = nodeData.m_propertyMapIndex;
        if (propertyDataIndex.IsValid())
        {
            const auto customPropertyData = azrtti_cast<const DataTypes::ICustomPropertyData*>(graph.GetNodeContent(propertyDataIndex));
            if (!customPropertyData)
            {
                AZ_Error("prefab", false, "Missing custom property data content for node.");
                return false;
            }

            if (AddEditorMaterialComponent(entityId, *(customPropertyData.get())) == false)
            {
                return false;
            }
        }

        return true;
    }

    AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>> DefaultProceduralPrefabGroup::GenerateDefaultPrefabMeshGroups(
        const Scene& scene) const
    {
        AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>> newMeshGroups;
        auto nodeDataMap = CalculateNodeDataMap(scene);
        if (nodeDataMap.empty())
        {
            return newMeshGroups;
        }

        for (const auto& entry : nodeDataMap)
        {
            newMeshGroups.push_back(BuildMeshGroupForNode(scene, entry.second, nodeDataMap));

        }

        return newMeshGroups;
    }

    AZStd::shared_ptr<SceneData::MeshGroup> DefaultProceduralPrefabGroup::BuildMeshGroupForNode(
            const Scene& scene,
            const NodeDataForEntity& nodeData,
            const NodeDataMap& nodeDataMap) const
    {
        const auto meshNodeIndex = nodeData.m_meshIndex;

        const auto& graph = scene.GetGraph();
        const auto meshNodeName = graph.GetNodeName(meshNodeIndex);
        const auto meshSubId =
            DataTypes::Utilities::CreateStableUuid(scene, azrtti_typeid<SceneData::MeshGroup>(), meshNodeName.GetPath());

        AZStd::string meshGroupName = "default_";
        meshGroupName += scene.GetName();
        meshGroupName += meshSubId.ToFixedString().c_str();

        // clean up the mesh group name
        AZStd::replace_if(
            meshGroupName.begin(),
            meshGroupName.end(),
            [](char c)
            {
                return (!AZStd::is_alnum(c) && c != '_');
            },
            '_');

        AZStd::string meshNodePath{ meshNodeName.GetPath() };
        auto meshGroup = AZStd::make_shared<SceneData::MeshGroup>();
        meshGroup->SetName(meshGroupName);
        meshGroup->GetSceneNodeSelectionList().AddSelectedNode(AZStd::move(meshNodePath));
        for (const auto& meshGoupNamePair : nodeDataMap)
        {
            if (meshGoupNamePair.second.m_meshIndex.IsValid() && meshGoupNamePair.second.m_meshIndex != meshNodeIndex)
            {
                const auto nodeName = graph.GetNodeName(meshGoupNamePair.second.m_meshIndex);
                meshGroup->GetSceneNodeSelectionList().RemoveSelectedNode(nodeName.GetPath());
            }
        }
        meshGroup->OverrideId(meshSubId);

        // tag this mesh group as a "default mesh group" using this rule
        meshGroup->GetRuleContainer().AddRule(AZStd::make_shared<AZ::SceneAPI::SceneData::ProceduralMeshGroupRule>());

        // Don't let users edit these mesh groups, because they're procedural they'll be re-generated and overwrite any changes.
        meshGroup->GetRuleContainer().AddRule(AZStd::make_shared<AZ::SceneAPI::SceneData::UnmodifiableRule>());

        // this clears out the mesh coordinates each mesh group will be rotated and translated
        // using the attached scene graph node
        auto coordinateSystemRule = AZStd::make_shared<AZ::SceneAPI::SceneData::CoordinateSystemRule>();
        coordinateSystemRule->SetUseAdvancedData(true);
        coordinateSystemRule->SetRotation(AZ::Quaternion::CreateIdentity());
        coordinateSystemRule->SetTranslation(AZ::Vector3::CreateZero());
        coordinateSystemRule->SetScale(1.0f);
        meshGroup->GetRuleContainer().AddRule(coordinateSystemRule);

        // create an empty LOD rule in order to skip the LOD buffer creation
        meshGroup->GetRuleContainer().AddRule(AZStd::make_shared<AZ::SceneAPI::SceneData::LodRule>());
        return meshGroup;
    }

    DefaultProceduralPrefabGroup::NodeEntityMap DefaultProceduralPrefabGroup::CreateNodeEntityMap(
        ManifestUpdates& manifestUpdates,
        const NodeDataMap& nodeDataMap,
        const Containers::Scene& scene,
        const AZStd::string& relativeSourcePath) const
    {
        NodeEntityMap nodeEntityMap;
        const auto& graph = scene.GetGraph();

        for (const auto& entry : nodeDataMap)
        {
            const auto thisNodeIndex = entry.first;
            const auto meshNodeIndex = entry.second.m_meshIndex;

            Containers::SceneGraph::NodeIndex nodeIndexForEntityName;
            nodeIndexForEntityName = meshNodeIndex.IsValid() ? meshNodeIndex : thisNodeIndex;
            const auto nodeNameForEntity = graph.GetNodeName(nodeIndexForEntityName);

            // create an entity for each node data entry
            AZ::EntityId entityId;
            AzToolsFramework::EntityUtilityBus::BroadcastResult(
                entityId, &AzToolsFramework::EntityUtilityBus::Events::CreateEditorReadyEntity, nodeNameForEntity.GetName());

            if (entityId.IsValid() == false)
            {
                return {};
            }

            if (meshNodeIndex.IsValid())
            {
                if (!CreateMeshGroupAndComponents(manifestUpdates,
                    entityId,
                    entry.second,
                    nodeDataMap,
                    scene,
                    relativeSourcePath))
                {
                    return {};
                }
            }

            nodeEntityMap.emplace(thisNodeIndex, AZStd::make_pair(entityId, nodeNameForEntity.GetName()));
        }

        return nodeEntityMap;
    }

    DefaultProceduralPrefabGroup::EntityIdMap DefaultProceduralPrefabGroup::FixUpEntityParenting(
        const NodeEntityMap& nodeEntityMap,
        const Containers::SceneGraph& graph,
        const NodeDataMap& nodeDataMap) const
    {
        EntityIdMap entities;

        for (const auto& nodeEntity : nodeEntityMap)
        {
            const AZStd::pair<AZ::EntityId, AzToolsFramework::Prefab::EntityAlias>& entityIdAliasPair = nodeEntity.second;
            entities.emplace(entityIdAliasPair.first, entityIdAliasPair.second);

            // find matching parent EntityId (if any)
            AZ::EntityId parentEntityId;
            const auto thisNodeIndex = nodeEntity.first;
            auto parentNodeIndex = graph.GetNodeParent(thisNodeIndex);
            while (parentNodeIndex.IsValid())
            {
                auto parentNodeIterator = nodeDataMap.find(parentNodeIndex);
                if (nodeDataMap.end() != parentNodeIterator)
                {
                    auto parentEntiyIterator = nodeEntityMap.find(parentNodeIterator->first);
                    if (nodeEntityMap.end() != parentEntiyIterator)
                    {
                        parentEntityId = parentEntiyIterator->second.first;
                        break;
                    }
                }
                else if (graph.HasNodeParent(parentNodeIndex))
                {
                    parentNodeIndex = graph.GetNodeParent(parentNodeIndex);
                }
                else
                {
                    parentNodeIndex = {};
                }
            }

            AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityIdAliasPair.first);
            auto* entityTransform = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (!entityTransform)
            {
                return {};
            }

            // parent entities
            if (parentEntityId.IsValid())
            {
                entityTransform->SetParent(parentEntityId);
            }

            auto thisNodeIterator = nodeDataMap.find(thisNodeIndex);
            AZ_Assert(thisNodeIterator != nodeDataMap.end(), "This node index missing.");
            auto thisTransformIndex = thisNodeIterator->second.m_transformIndex;

            // get node matrix data to set the entity's local transform
            const auto nodeTransform = azrtti_cast<const DataTypes::ITransform*>(graph.GetNodeContent(thisTransformIndex));
            if (nodeTransform)
            {
                entityTransform->SetLocalTM(AZ::Transform::CreateFromMatrix3x4(nodeTransform->GetMatrix()));
            }
            else
            {
                entityTransform->SetLocalTM(AZ::Transform::CreateUniformScale(1.0f));
            }

            PrefabGroupNotificationBus::Broadcast(&PrefabGroupNotificationBus::Events::OnUpdatePrefabEntity, entityIdAliasPair.first);
        }

        return entities;
    }

    bool DefaultProceduralPrefabGroup::CreatePrefabGroupManifestUpdates(
        ManifestUpdates& manifestUpdates,
        const Containers::Scene& scene,
        const EntityIdMap& entities,
        const AZStd::string& filenameOnly,
        const AZStd::string& relativeSourcePath) const
    {
        AZStd::string prefabTemplateName{ relativeSourcePath };
        AZ::StringFunc::Path::ReplaceFullName(prefabTemplateName, filenameOnly.c_str());
        AZ::StringFunc::Replace(prefabTemplateName, "\\", "/"); // the source folder uses forward slash

        // clear out any previously created prefab template for this path
        auto* prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
        AzToolsFramework::Prefab::TemplateId prefabTemplateId =
            prefabSystemComponentInterface->GetTemplateIdFromFilePath({ prefabTemplateName.c_str() });
        if (prefabTemplateId != AzToolsFramework::Prefab::InvalidTemplateId)
        {
            prefabSystemComponentInterface->RemoveTemplate(prefabTemplateId);
            prefabTemplateId = AzToolsFramework::Prefab::InvalidTemplateId;
        }

        // create prefab group for entire stack
        AzToolsFramework::Prefab::PrefabSystemScriptingBus::BroadcastResult(
            prefabTemplateId,
            &AzToolsFramework::Prefab::PrefabSystemScriptingBus::Events::CreatePrefabTemplateWithCustomEntityAliases,
            entities,
            prefabTemplateName);

        if (prefabTemplateId == AzToolsFramework::Prefab::InvalidTemplateId)
        {
            AZ_Error("prefab", false, "Could not create a prefab template for entities.");
            return false;
        }

        // Convert the prefab to a JSON string
        AZ::Outcome<AZStd::string, void> outcome;
        AzToolsFramework::Prefab::PrefabLoaderScriptingBus::BroadcastResult(
            outcome,
            &AzToolsFramework::Prefab::PrefabLoaderScriptingBus::Events::SaveTemplateToString,
            prefabTemplateId);

        if (outcome.IsSuccess() == false)
        {
            AZ_Error("prefab", false, "Could not create JSON string for template; maybe NaN values in the template?");
            return false;
        }

        AzToolsFramework::Prefab::PrefabDom prefabDom;
        prefabDom.Parse(outcome.GetValue().c_str());

        auto prefabGroup = AZStd::make_shared<AZ::SceneAPI::SceneData::PrefabGroup>();
        prefabGroup->SetName(prefabTemplateName);
        prefabGroup->SetPrefabDom(AZStd::move(prefabDom));
        prefabGroup->SetId(DataTypes::Utilities::CreateStableUuid(
            scene,
            azrtti_typeid<AZ::SceneAPI::SceneData::PrefabGroup>(),
            prefabTemplateName));

        manifestUpdates.emplace_back(prefabGroup);
        return true;
    }
}

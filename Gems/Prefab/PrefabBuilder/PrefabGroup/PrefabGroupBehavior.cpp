/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabGroup/PrefabGroupBehavior.h>
#include <PrefabGroup/PrefabGroup.h>
#include <PrefabGroup/ProceduralAssetHandler.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EntityUtilityComponent.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderScriptingBus.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemScriptingBus.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ICustomPropertyData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>

namespace AZStd
{
    template<> struct hash<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>
    {
        inline size_t operator()(const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
        {
            size_t hashValue{ 0 };
            hash_combine(hashValue, nodeIndex.AsNumber());
            return hashValue;
        }
    };
}

namespace AZ::SceneAPI::Behaviors
{
    //
    // ExportEventHandler
    //

    static constexpr const char s_PrefabGroupBehaviorCreateDefaultKey[] = "/O3DE/Preferences/Prefabs/CreateDefaults";

    struct PrefabGroupBehavior::ExportEventHandler final
        : public AZ::SceneAPI::SceneCore::ExportingComponent
        , public Events::AssetImportRequestBus::Handler
    {
        using PreExportEventContextFunction = AZStd::function<Events::ProcessingResult(Events::PreExportEventContext&)>;
        PreExportEventContextFunction m_preExportEventContextFunction;
        AZ::Prefab::PrefabGroupAssetHandler m_prefabGroupAssetHandler;

        ExportEventHandler() = delete;

        ExportEventHandler(PreExportEventContextFunction function)
            : m_preExportEventContextFunction(AZStd::move(function))
        {
            BindToCall(&ExportEventHandler::PrepareForExport);
            AZ::SceneAPI::SceneCore::ExportingComponent::Activate();
            Events::AssetImportRequestBus::Handler::BusConnect();
        }

        ~ExportEventHandler()
        {
            Events::AssetImportRequestBus::Handler::BusDisconnect();
            AZ::SceneAPI::SceneCore::ExportingComponent::Deactivate();
        }

        Events::ProcessingResult PrepareForExport(Events::PreExportEventContext& context)
        {
            return m_preExportEventContextFunction(context);
        }

        // this stores the data related with MeshData nodes
        struct MeshNodeData
        {
            Containers::SceneGraph::NodeIndex m_meshIndex = {};
            Containers::SceneGraph::NodeIndex m_transformIndex = {};
            Containers::SceneGraph::NodeIndex m_propertyMapIndex = {};
        };

        using MeshTransformEntry = AZStd::pair<Containers::SceneGraph::NodeIndex, MeshNodeData>;
        using MeshTransformMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, MeshNodeData>;
        using MeshIndexContainer = AZStd::unordered_set<Containers::SceneGraph::NodeIndex>;
        using ManifestUpdates = AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>;
        using NodeEntityMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, AZ::EntityId>;
        using EntityIdList = AZStd::vector<AZ::EntityId>;

        void AssignCustomPropertyMapIndex(
            MeshNodeData& meshNodeData,
            const Containers::SceneGraph& graph,
            const Containers::SceneGraph::NodeIndex meshIndex)
        {
            auto childIndex = graph.GetNodeChild(meshIndex);
            while (childIndex.IsValid())
            {
                const auto nodeContent = graph.GetNodeContent(childIndex);
                if (nodeContent && azrtti_istypeof<AZ::SceneAPI::DataTypes::ICustomPropertyData>(nodeContent.get()))
                {
                    meshNodeData.m_propertyMapIndex = childIndex;
                    return;
                }
                childIndex = graph.GetNodeSibling(childIndex);
            }
        }

        MeshTransformMap CalculateMeshTransformMap(const Containers::Scene& scene)
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

            MeshIndexContainer meshIndexContainer;
            MeshTransformMap meshTransformMap;
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                Containers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
                AZStd::string currentNodeName = graph.GetNodeName(currentIndex).GetPath();
                const auto currentContent = graph.GetNodeContent(currentIndex);
                if (currentContent)
                {
                    if (azrtti_istypeof<AZ::SceneAPI::DataTypes::ITransform>(currentContent.get()))
                    {
                        const auto parentIndex = graph.GetNodeParent(currentIndex);
                        if (parentIndex.IsValid() == false)
                        {
                            continue;
                        }
                        const auto parentContent = graph.GetNodeContent(parentIndex);
                        if (parentContent && azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshData>(parentContent.get()))
                        {
                            // map the node parent to the ITransform
                            meshIndexContainer.erase(parentIndex);
                            MeshNodeData meshNodeData{ parentIndex, currentIndex };
                            AssignCustomPropertyMapIndex(meshNodeData, graph, parentIndex);
                            meshTransformMap.emplace(MeshTransformEntry{ graph.GetNodeParent(parentIndex), AZStd::move(meshNodeData) });
                        }
                    }
                    else if (azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshData>(currentContent.get()))
                    {
                        meshIndexContainer.insert(currentIndex);
                    }
                }
            }

            // all mesh data nodes left in the meshIndexContainer do not have a matching TransformData node
            // since the nodes have an identity transform, so map the MeshData index with an Invalid mesh index to
            // indicate the transform should not be set to a default value
            for( const auto& meshIndex : meshIndexContainer)
            {
                MeshNodeData meshNodeData { meshIndex, Containers::SceneGraph::NodeIndex{} };
                AssignCustomPropertyMapIndex(meshNodeData, graph, meshIndex);
                meshTransformMap.emplace(MeshTransformEntry{ graph.GetNodeParent(meshIndex), AZStd::move(meshNodeData) });
            }

            return meshTransformMap;
        }

        bool AddEditorMaterialComponent(const AZ::EntityId& entityId, const DataTypes::ICustomPropertyData& propertyData)
        {
            const auto propertyMaterialPathIterator = propertyData.GetPropertyMap().find("o3de.default.material");
            if (propertyMaterialPathIterator == propertyData.GetPropertyMap().end())
            {
                // skip these assignment since the default material override was not provided
                return true;
            }

            const AZStd::any& propertyMaterialPath = propertyMaterialPathIterator->second;
            if (propertyMaterialPath.empty() || propertyMaterialPath.is<AZStd::string>() == false)
            {
                AZ_Error("prefab", false, "The 'o3de.default.material' custom property value type must be a string."
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

        bool AddEditorMeshComponent(
            const AZ::EntityId& entityId,
            const AZStd::string& relativeSourcePath,
            const AZStd::string& meshGroupName)
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
            AZStd::string modelAssetPath;
            modelAssetPath = relativeSourcePath;
            AZ::StringFunc::Path::ReplaceFullName(modelAssetPath, meshGroupName.c_str());
            AZ::StringFunc::Replace(modelAssetPath, "\\", "/"); // asset paths use forward slashes

            auto meshAssetJson = AZStd::string::format(
                R"JSON(
                        {"Controller": {"Configuration": {"ModelAsset": { "assetHint": "%s.azmodel"}}}}
                    )JSON", modelAssetPath.c_str());

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

        NodeEntityMap CreateMeshGroups(
            ManifestUpdates& manifestUpdates,
            const MeshTransformMap& meshTransformMap,
            const Containers::Scene& scene,
            const AZStd::string& relativeSourcePath)
        {
            NodeEntityMap nodeEntityMap;
            const auto& graph = scene.GetGraph();

            for (const auto& entry : meshTransformMap)
            {
                const auto thisNodeIndex = entry.first;
                const auto meshNodeIndex = entry.second.m_meshIndex;
                const auto propertyDataIndex = entry.second.m_propertyMapIndex;
                const auto meshNodeName = graph.GetNodeName(meshNodeIndex);
                const auto meshSubId = DataTypes::Utilities::CreateStableUuid(
                    scene,
                    azrtti_typeid<AZ::SceneAPI::SceneData::MeshGroup>(),
                    meshNodeName.GetPath());

                AZStd::string meshGroupName = "default_";
                meshGroupName += scene.GetName();
                meshGroupName += meshSubId.ToFixedString().c_str();

                // clean up the mesh group name
                AZStd::replace_if(
                    meshGroupName.begin(),
                    meshGroupName.end(),
                    [](char c) { return (!AZStd::is_alnum(c) && c != '_'); },
                    '_');

                AZStd::string meshNodePath{ meshNodeName.GetPath() };
                auto meshGroup = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>();
                meshGroup->SetName(meshGroupName);
                meshGroup->GetSceneNodeSelectionList().AddSelectedNode(AZStd::move(meshNodePath));
                for (const auto& meshGoupNamePair : meshTransformMap)
                {
                    if (meshGoupNamePair.first != thisNodeIndex)
                    {
                        const auto nodeName = graph.GetNodeName(meshGoupNamePair.second.m_meshIndex);
                        meshGroup->GetSceneNodeSelectionList().RemoveSelectedNode(nodeName.GetPath());
                    }
                }
                meshGroup->OverrideId(meshSubId);

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

                manifestUpdates.emplace_back(meshGroup);

                // create an entity for each MeshGroup
                AZ::EntityId entityId;
                AzToolsFramework::EntityUtilityBus::BroadcastResult(
                    entityId,
                    &AzToolsFramework::EntityUtilityBus::Events::CreateEditorReadyEntity,
                    meshNodeName.GetName());

                if (entityId.IsValid() == false)
                {
                    return {};
                }


                if (AddEditorMeshComponent(entityId, relativeSourcePath, meshGroupName) == false)
                {
                    return {};
                }

                if (propertyDataIndex.IsValid())
                {
                    const auto customPropertyData = azrtti_cast<const DataTypes::ICustomPropertyData*>(graph.GetNodeContent(propertyDataIndex));
                    if (!customPropertyData)
                    {
                        AZ_Error("prefab", false, "Missing custom propertiy data content for node.");
                        return {};
                    }

                    if (AddEditorMaterialComponent(entityId, *(customPropertyData.get())) == false)
                    {
                        return {};
                    }
                }

                nodeEntityMap.insert({ thisNodeIndex, entityId });
            }

            return nodeEntityMap;
        }

        EntityIdList FixUpEntityParenting(
            const NodeEntityMap& nodeEntityMap,
            const Containers::SceneGraph& graph,
            const MeshTransformMap& meshTransformMap)
        {
            EntityIdList entities;
            entities.reserve(nodeEntityMap.size());

            for (const auto& nodeEntity : nodeEntityMap)
            {
                entities.emplace_back(nodeEntity.second);

                // find matching parent EntityId (if any)
                AZ::EntityId parentEntityId;
                const auto thisNodeIndex = nodeEntity.first;
                auto parentNodeIndex = graph.GetNodeParent(thisNodeIndex);
                while (parentNodeIndex.IsValid())
                {
                    auto parentNodeIterator = meshTransformMap.find(parentNodeIndex);
                    if (meshTransformMap.end() != parentNodeIterator)
                    {
                        auto parentEntiyIterator = nodeEntityMap.find(parentNodeIterator->first);
                        if (nodeEntityMap.end() != parentEntiyIterator)
                        {
                            parentEntityId = parentEntiyIterator->second;
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

                AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(nodeEntity.second);
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

                auto thisNodeIterator = meshTransformMap.find(thisNodeIndex);
                AZ_Assert(thisNodeIterator != meshTransformMap.end(), "This node index missing.");
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
            }

            return entities;
        }

        bool CreatePrefabGroup(
            ManifestUpdates& manifestUpdates,
            Containers::Scene& scene,
            const EntityIdList& entities,
            const AZStd::string& filenameOnly,
            const AZStd::string& relativeSourcePath)
        {
            AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get()->RemoveAllTemplates();

            AZStd::string prefabTemplateName { relativeSourcePath };
            AZ::StringFunc::Path::ReplaceFullName(prefabTemplateName, filenameOnly.c_str());
            AZ::StringFunc::Replace(prefabTemplateName, "\\", "/"); // the source folder uses forward slash

            // create prefab group for entire stack
            AzToolsFramework::Prefab::TemplateId prefabTemplateId;
            AzToolsFramework::Prefab::PrefabSystemScriptingBus::BroadcastResult(
                prefabTemplateId,
                &AzToolsFramework::Prefab::PrefabSystemScriptingBus::Events::CreatePrefabTemplate,
                entities,
                prefabTemplateName);

            if (prefabTemplateId == AzToolsFramework::Prefab::InvalidTemplateId)
            {
                AZ_Error("prefab", false, "Could not create a prefab template for entites.");
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

            // update manifest if there where no errors
            for (auto update : manifestUpdates)
            {
                scene.GetManifest().AddEntry(update);
            }

            return true;
        }

        // AssetImportRequest
        Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action, RequestingApplication requester) override;
        Events::ProcessingResult PrepareForAssetLoading(Containers::Scene& scene, RequestingApplication requester) override;
    };

    Events::ProcessingResult PrefabGroupBehavior::ExportEventHandler::PrepareForAssetLoading(
        [[maybe_unused]] Containers::Scene& scene,
        RequestingApplication requester)
    {
        using namespace AzToolsFramework;
        if (requester == RequestingApplication::AssetProcessor)
        {
            EntityUtilityBus::Broadcast(&EntityUtilityBus::Events::ResetEntityContext);
            AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get()->RemoveAllTemplates();
        }
        return Events::ProcessingResult::Success;
    }

    Events::ProcessingResult PrefabGroupBehavior::ExportEventHandler::UpdateManifest(
        Containers::Scene& scene,
        ManifestAction action,
        RequestingApplication requester)
    {
        if (action == Events::AssetImportRequest::Update)
        {
            // ignore constructing a default procedural prefab if some tool or script is attempting
            // to update the scene manifest
            return Events::ProcessingResult::Ignored;
        }
        else if (action == Events::AssetImportRequest::ConstructDefault && requester == RequestingApplication::Editor)
        {
            // ignore constructing a default procedurla prefab if the Editor's "Edit Settings..." is being used
            // the user is trying to assign the source scene asset their own mesh groups
            return Events::ProcessingResult::Ignored;
        }

        // this toggle makes constructing default mesh groups and a prefab optional
        if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry)
        {
            bool createDefaultPrefab = true;
            settingsRegistry->Get(createDefaultPrefab, s_PrefabGroupBehaviorCreateDefaultKey);
            if (createDefaultPrefab == false)
            {
                return Events::ProcessingResult::Ignored;
            }
        }

        auto meshTransformMap = CalculateMeshTransformMap(scene);
        if (meshTransformMap.empty())
        {
            return Events::ProcessingResult::Ignored;
        }

        // compute the filenames of the scene file
        AZStd::string relativeSourcePath = scene.GetSourceFilename();
        // the watch folder and forward slash is used to in the asset hint path of the file
        AZStd::string watchFolder = scene.GetWatchFolder() + "/";
        AZ::StringFunc::Replace(relativeSourcePath, watchFolder.c_str(), "");
        AZ::StringFunc::Replace(relativeSourcePath, ".", "_");
        AZStd::string filenameOnly{ relativeSourcePath };
        AZ::StringFunc::Path::GetFileName(filenameOnly.c_str(), filenameOnly);
        AZ::StringFunc::Path::ReplaceExtension(filenameOnly, "procprefab");

        ManifestUpdates manifestUpdates;

        auto nodeEntityMap = CreateMeshGroups(manifestUpdates, meshTransformMap, scene, relativeSourcePath);
        if(nodeEntityMap.empty())
        {
            return Events::ProcessingResult::Ignored;
        }

        auto entities = FixUpEntityParenting(nodeEntityMap, scene.GetGraph(), meshTransformMap);
        if(entities.empty())
        {
            return Events::ProcessingResult::Ignored;
        }

        bool success = CreatePrefabGroup(manifestUpdates, scene, entities, filenameOnly, relativeSourcePath);
        return success ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
    }

    //
    // PrefabGroupBehavior
    //

    void PrefabGroupBehavior::Activate()
    {
        m_exportEventHandler = AZStd::make_shared<ExportEventHandler>([this](auto& context)
        {
            return this->OnPrepareForExport(context);
        });
    }

    void PrefabGroupBehavior::Deactivate()
    {
        m_exportEventHandler.reset();
    }

    AZStd::unique_ptr<rapidjson::Document> PrefabGroupBehavior::CreateProductAssetData(const SceneData::PrefabGroup* prefabGroup, const AZ::IO::Path& relativePath) const
    {
        using namespace AzToolsFramework::Prefab;

        auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
        if (!prefabLoaderInterface)
        {
            AZ_Error("prefab", false, "Could not get PrefabLoaderInterface");
            return {};
        }

        // write to a UTF-8 string buffer
        auto prefabDomRef = prefabGroup->GetPrefabDomRef();
        if (!prefabDomRef)
        {
            AZ_Error("prefab", false, "PrefabGroup(%s) missing PrefabDom", prefabGroup->GetName().c_str());
            return {};
        }

        const AzToolsFramework::Prefab::PrefabDom& prefabDom = prefabDomRef.value();
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
        if (prefabDom.Accept(writer) == false)
        {
            AZ_Error("prefab", false, "Could not write PrefabGroup(%s) to JSON", prefabGroup->GetName().c_str());
            return {};
        }

        // The originPath we pass to LoadTemplateFromString must be the relative path of the file
        AZ::IO::Path templateName(prefabGroup->GetName());
        templateName.ReplaceExtension(AZ::Prefab::PrefabGroupAssetHandler::s_Extension);
        if (!AZ::StringFunc::StartsWith(templateName.c_str(), relativePath.c_str()))
        {
            templateName = relativePath / templateName;
        }

        auto templateId = prefabLoaderInterface->LoadTemplateFromString(sb.GetString(), templateName.Native().c_str());
        if (templateId == InvalidTemplateId)
        {
            AZ_Error("prefab", false, "PrefabGroup(%s) Could not write load template", prefabGroup->GetName().c_str());
            return {};
        }

        auto* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        if (!prefabSystemComponentInterface)
        {
            AZ_Error("prefab", false, "Could not get PrefabSystemComponentInterface");
            return {};
        }

        const rapidjson::Document& generatedInstanceDom = prefabSystemComponentInterface->FindTemplateDom(templateId);
        auto proceduralPrefab = AZStd::make_unique<rapidjson::Document>(rapidjson::kObjectType);
        proceduralPrefab->CopyFrom(generatedInstanceDom, proceduralPrefab->GetAllocator(), true);

        return proceduralPrefab;
    }

    bool PrefabGroupBehavior::WriteOutProductAsset(
        Events::PreExportEventContext& context,
        const SceneData::PrefabGroup* prefabGroup,
        const rapidjson::Document& doc) const
    {
        AZStd::string filePath = AZ::SceneAPI::Utilities::FileUtilities::CreateOutputFileName(
            prefabGroup->GetName().c_str(),
            context.GetOutputDirectory(),
            AZ::Prefab::PrefabGroupAssetHandler::s_Extension);

        bool result = WriteOutProductAssetFile(filePath, context, prefabGroup, doc, false);

        if (context.GetDebug())
        {
            AZStd::string debugFilePath = AZ::SceneAPI::Utilities::FileUtilities::CreateOutputFileName(
                prefabGroup->GetName().c_str(),
                context.GetOutputDirectory(),
                AZ::Prefab::PrefabGroupAssetHandler::s_Extension);
            debugFilePath.append(".json");
            WriteOutProductAssetFile(debugFilePath, context, prefabGroup, doc, true);
        }

        return result;
    }

    bool PrefabGroupBehavior::WriteOutProductAssetFile(
        const AZStd::string& filePath,
        Events::PreExportEventContext& context,
        const SceneData::PrefabGroup* prefabGroup,
        const rapidjson::Document& doc,
        bool debug) const
    {
        AZ::IO::FileIOStream fileStream(filePath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (fileStream.IsOpen() == false)
        {
            AZ_Error("prefab", false, "File path(%s) could not open for write", filePath.c_str());
            return false;
        }

        // write to a UTF-8 string buffer
        Data::AssetType assetType = azrtti_typeid<Prefab::ProceduralPrefabAsset>();
        AZStd::string productPath {prefabGroup->GetName()};
        rapidjson::StringBuffer sb;
        bool writerResult = false;
        if (debug)
        {
            rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
            writerResult = doc.Accept(writer);
            productPath.append(".json");
            assetType = Data::AssetType::CreateNull();
        }
        else
        {
            rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
            writerResult = doc.Accept(writer);
        }
        if (writerResult == false)
        {
            AZ_Error("prefab", false, "PrefabGroup(%s) Could not buffer JSON", prefabGroup->GetName().c_str());
            return false;
        }

        const auto bytesWritten = fileStream.Write(sb.GetSize(), sb.GetString());
        if (bytesWritten > 1)
        {
            AZ::u32 subId = AZ::Crc32(productPath.c_str());
            context.GetProductList().AddProduct(
                filePath,
                context.GetScene().GetSourceGuid(),
                assetType,
                {},
                AZStd::make_optional(subId));

            return true;
        }
        return false;
    }

    Events::ProcessingResult PrefabGroupBehavior::OnPrepareForExport(Events::PreExportEventContext& context) const
    {
        AZStd::vector<const SceneData::PrefabGroup*> prefabGroupCollection;
        const Containers::SceneManifest& manifest = context.GetScene().GetManifest();

        for (size_t i = 0; i < manifest.GetEntryCount(); ++i)
        {
            const auto* group = azrtti_cast<const SceneData::PrefabGroup*>(manifest.GetValue(i).get());
            if (group)
            {
                prefabGroupCollection.push_back(group);
            }
        }

        if (prefabGroupCollection.empty())
        {
            return AZ::SceneAPI::Events::ProcessingResult::Ignored;
        }

        // Get the relative path of the source and then take just the path portion of it (no file name)
        AZ::IO::Path relativePath = context.GetScene().GetSourceFilename();
        relativePath = relativePath.LexicallyRelative(AZStd::string_view(context.GetScene().GetWatchFolder()));
        AZStd::string relativeSourcePath { AZStd::move(relativePath.ParentPath().Native()) };
        AZ::StringFunc::Replace(relativeSourcePath, "\\", "/"); // the source paths use forward slashes

        for (const auto* prefabGroup : prefabGroupCollection)
        {
            auto result = CreateProductAssetData(prefabGroup, relativeSourcePath);
            if (!result)
            {
                return Events::ProcessingResult::Failure;
            }

            if (WriteOutProductAsset(context, prefabGroup, *result.get()) == false)
            {
                return Events::ProcessingResult::Failure;
            }
        }

        return Events::ProcessingResult::Success;
    }

    void PrefabGroupBehavior::Reflect(ReflectContext* context)
    {
        AZ::SceneAPI::SceneData::PrefabGroup::Reflect(context);
        Prefab::ProceduralPrefabAsset::Reflect(context);

        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PrefabGroupBehavior, BehaviorComponent>()->Version(1);
        }

        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            using namespace AzToolsFramework::Prefab;

            auto loadTemplate = [](const AZStd::string& prefabPath)
            {
                AZ::IO::FixedMaxPath path {prefabPath};
                auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
                if (prefabLoaderInterface)
                {
                    return prefabLoaderInterface->LoadTemplateFromFile(path);
                }
                return TemplateId{};
            };

            behaviorContext->Method("LoadTemplate", loadTemplate)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "prefab");

            auto saveTemplateToString = [](TemplateId templateId) -> AZStd::string
            {
                AZStd::string output;
                auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
                if (prefabLoaderInterface)
                {
                    prefabLoaderInterface->SaveTemplateToString(templateId, output);
                }
                return output;
            };

            behaviorContext->Method("SaveTemplateToString", saveTemplateToString)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "prefab");
        }
    }
}

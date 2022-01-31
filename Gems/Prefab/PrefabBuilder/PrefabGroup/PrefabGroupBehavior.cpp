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
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

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

        using MeshTransformPair = AZStd::pair<Containers::SceneGraph::NodeIndex, Containers::SceneGraph::NodeIndex>;
        using MeshTransformEntry = AZStd::pair<Containers::SceneGraph::NodeIndex, MeshTransformPair>;
        using MeshTransformMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, MeshTransformPair>;

        MeshTransformMap CalculateMeshTransformMap(const Containers::Scene& scene)
        {
            auto graph = scene.GetGraph();
            const auto view = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(
                graph,
                graph.GetRoot(),
                graph.GetContentStorage().cbegin(),
                true);

            if (view.begin() == view.end())
            {
                return {};
            }

            MeshTransformMap meshTransformMap;
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                Containers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
                AZStd::string currentNodeName = graph.GetNodeName(currentIndex).GetPath();
                auto currentContent = graph.GetNodeContent(currentIndex);
                if (currentContent)
                {
                    if (azrtti_istypeof<AZ::SceneAPI::DataTypes::ITransform>(currentContent.get()))
                    {
                        auto parentIndex = graph.GetNodeParent(currentIndex);
                        if (parentIndex.IsValid() == false)
                        {
                            continue;
                        }
                        auto parentContent = graph.GetNodeContent(parentIndex);
                        if (parentContent && azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshData>(parentContent.get()))
                        {
                            // map the node parent to the ITransform
                            MeshTransformPair pair{ parentIndex, currentIndex };
                            meshTransformMap.emplace(MeshTransformEntry{ graph.GetNodeParent(parentIndex), AZStd::move(pair) });
                        }
                    }
                }
            }
            return meshTransformMap;
        }

        using ManifestUpdates = AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>;
        using NodeEntityMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, AZ::EntityId>;

        NodeEntityMap CreateMeshGroups(
            ManifestUpdates& manifestUpdates,
            const MeshTransformMap& meshTransformMap,
            const Containers::Scene& scene,
            AZStd::string& relativeSourcePath)
        {
            NodeEntityMap nodeEntityMap;
            const auto& graph = scene.GetGraph();

            for (const auto& entry : meshTransformMap)
            {
                const auto thisNodeIndex = entry.first;
                const auto meshNodeIndex = entry.second.first;
                const auto meshNodeName = graph.GetNodeName(meshNodeIndex);
                AZStd::string meshNodePath{ meshNodeName.GetPath() };

                AZStd::string meshNodeFullName;
                meshNodeFullName = relativeSourcePath;
                meshNodeFullName.append("_");
                meshNodeFullName.append(meshNodeName.GetName());

                auto meshGroup = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>();
                meshGroup->SetName(meshNodeFullName);
                meshGroup->GetSceneNodeSelectionList().AddSelectedNode(AZStd::move(meshNodePath));
                for (const auto& meshGoupNamePair : meshTransformMap)
                {
                    if (meshGoupNamePair.first != thisNodeIndex)
                    {
                        const auto nodeName = graph.GetNodeName(meshGoupNamePair.second.first);
                        meshGroup->GetSceneNodeSelectionList().RemoveSelectedNode(nodeName.GetPath());
                    }
                }
                meshGroup->OverrideId(DataTypes::Utilities::CreateStableUuid(
                    scene,
                    azrtti_typeid<AZ::SceneAPI::SceneData::MeshGroup>(),
                    meshNodeName.GetPath()));

                // this clears out the mesh coordinates each mesh group will be rotated and translated
                // using the attached scene graph node
                auto coordinateSystemRule = AZStd::make_shared<AZ::SceneAPI::SceneData::CoordinateSystemRule>();
                coordinateSystemRule->SetUseAdvancedData(true);
                coordinateSystemRule->SetRotation(AZ::Quaternion::CreateIdentity());
                coordinateSystemRule->SetTranslation(AZ::Vector3::CreateZero());
                coordinateSystemRule->SetScale(1.0f);
                meshGroup->GetRuleContainer().AddRule(coordinateSystemRule);

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
                auto meshAssetJson = AZStd::string::format(
                    R"JSON(
                        {"Controller": {"Configuration": {"ModelAsset": { "assetHint": "%s.azmodel"}}}}
                    )JSON", meshNodeFullName.c_str());

                bool result = false;
                AzToolsFramework::EntityUtilityBus::BroadcastResult(
                    result,
                    &AzToolsFramework::EntityUtilityBus::Events::UpdateComponentForEntity,
                    entityId,
                    editorMeshComponent,
                    meshAssetJson);

                if (result == false)
                {
                    AZ_Error("prefab", false, "UpdateComponentForEntity failed for EditorMeshComponent component");
                    return {};
                }

                nodeEntityMap.insert({ thisNodeIndex, entityId });
            }

            return nodeEntityMap;
        }

        using EntityIdList = AZStd::vector<AZ::EntityId>;

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
                auto thisTransformIndex = thisNodeIterator->second.second;

                // get node matrix data to set the entity's local transform
                const auto nodeTransform = azrtti_cast<const DataTypes::ITransform*>(graph.GetNodeContent(thisTransformIndex));
                entityTransform->SetLocalTM(AZ::Transform::CreateFromMatrix3x4(nodeTransform->GetMatrix()));
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

            // create prefab group for entire stack
            AzToolsFramework::Prefab::TemplateId prefabTemplateId;
            AzToolsFramework::Prefab::PrefabSystemScriptingBus::BroadcastResult(
                prefabTemplateId,
                &AzToolsFramework::Prefab::PrefabSystemScriptingBus::Events::CreatePrefabTemplate,
                entities,
                filenameOnly);

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
            prefabGroup->SetName(relativeSourcePath);
            prefabGroup->SetPrefabDom(AZStd::move(prefabDom));
            prefabGroup->SetId(DataTypes::Utilities::CreateStableUuid(
                scene,
                azrtti_typeid<AZ::SceneAPI::SceneData::PrefabGroup>(),
                relativeSourcePath));

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
    };

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
        AZ::StringFunc::Path::ReplaceExtension(filenameOnly, "prefab");

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
        templateName = relativePath / templateName;

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

        AZ::IO::FileIOStream fileStream(filePath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (fileStream.IsOpen() == false)
        {
            AZ_Error("prefab", false, "File path(%s) could not open for write", filePath.c_str());
            return false;
        }

        // write to a UTF-8 string buffer
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
        if (doc.Accept(writer) == false)
        {
            AZ_Error("prefab", false, "PrefabGroup(%s) Could not buffer JSON", prefabGroup->GetName().c_str());
            return false;
        }

        const auto bytesWritten = fileStream.Write(sb.GetSize(), sb.GetString());
        if (bytesWritten > 1)
        {
            AZ::u32 subId = AZ::Crc32(prefabGroup->GetName().c_str());
            context.GetProductList().AddProduct(
                filePath,
                context.GetScene().GetSourceGuid(),
                azrtti_typeid<Prefab::ProceduralPrefabAsset>(),
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
        relativePath = relativePath.ParentPath();

        for (const auto* prefabGroup : prefabGroupCollection)
        {
            auto result = CreateProductAssetData(prefabGroup, relativePath);
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
    }
}

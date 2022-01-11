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
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>

namespace AZStd
{
    template<> struct hash<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>
    {
        inline size_t operator()(const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
        {
            size_t hashValue{ 0 };
            AZStd::hash_combine(hashValue, nodeIndex.AsNumber());
            return hashValue;
        }
    };
}

namespace AZ::SceneAPI::Behaviors
{
    //
    // ExportEventHandler
    //

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

        // AssetImportRequest
        Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action, RequestingApplication requester) override;
    };

    Events::ProcessingResult PrefabGroupBehavior::ExportEventHandler::UpdateManifest(
        Containers::Scene& scene,
        ManifestAction action,
        [[maybe_unused]] RequestingApplication requester)
    {
        if (action != Events::AssetImportRequest::ConstructDefault)
        {
            return Events::ProcessingResult::Ignored;
        }

        using MeshTransformPair = AZStd::pair<Containers::SceneGraph::NodeIndex, Containers::SceneGraph::NodeIndex>;
        using MeshTransformEntry = AZStd::pair<Containers::SceneGraph::NodeIndex, MeshTransformPair>;
        using MeshTransformMap = AZStd::unordered_map<Containers::SceneGraph::NodeIndex, MeshTransformPair>;
        MeshTransformMap meshTransformMap;

        auto graph = scene.GetGraph();
        const auto view = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(
            graph,
            graph.GetRoot(),
            graph.GetContentStorage().cbegin(),
            true);

        if (view.begin() == view.end())
        {
            return Events::ProcessingResult::Ignored;
        }
        for (auto it = view.begin(); it != view.end(); ++it)
        {
            Containers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
            AZStd::string currentNodeName = graph.GetNodeName(currentIndex).GetPath();
            auto currentContent = graph.GetNodeContent(currentIndex);
AZ_Printf("debug", "name (%s) index (%d) \n", currentNodeName.c_str(), currentIndex.AsNumber());
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
                        MeshTransformPair pair {parentIndex, currentIndex};
                        meshTransformMap.emplace(MeshTransformEntry{ graph.GetNodeParent(parentIndex), AZStd::move(pair) });
                    }
                }
            }
        }

        if (meshTransformMap.empty())
        {
            return Events::ProcessingResult::Ignored;
        }

        // compute the filenames of the scene file
        AZStd::string watchFolder = scene.GetWatchFolder() + "/";
        AZStd::string relativeSourcePath = scene.GetSourceFilename();
        AZ::StringFunc::Replace(relativeSourcePath, ".", "_");
        AZ::StringFunc::Replace(relativeSourcePath, watchFolder.c_str(), "");
        AZStd::string filenameOnly{ relativeSourcePath };
        AZ::StringFunc::Path::GetFileName(filenameOnly.c_str(), filenameOnly);
        AZ::StringFunc::Path::ReplaceExtension(filenameOnly, "prefab");
        AZStd::string meshNodeFullName;

        AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>> manifestUpdates;
        AZStd::unordered_map<Containers::SceneGraph::NodeIndex, AZ::EntityId> nodeEntityMap;

        // create mesh groups and Editor entities for each pair
        for (const auto& entry : meshTransformMap)
        {
            meshNodeFullName.clear();
            meshNodeFullName = relativeSourcePath;

            auto meshNodeIndex = entry.second.first;
            auto meshNodeName = graph.GetNodeName(meshNodeIndex);
            meshNodeFullName.append("_");
            meshNodeFullName.append(meshNodeName.GetName());

            auto meshGroup = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>();
            meshGroup->SetName(meshNodeFullName.c_str());
            meshGroup->GetSceneNodeSelectionList().AddSelectedNode(meshNodeName.GetPath());
            meshGroup->OverrideId(DataTypes::Utilities::CreateStableUuid(
                scene,
                azrtti_typeid<AZ::SceneAPI::SceneData::MeshGroup>(),
                meshNodeName.GetPath()));

            manifestUpdates.emplace_back(meshGroup);

            // create an entity for each MeshGroup
            AZ::EntityId entityId;
            AzToolsFramework::EntityUtilityBus::BroadcastResult(
                entityId,
                &AzToolsFramework::EntityUtilityBus::Events::CreateEditorReadyEntity,
                meshNodeName.GetName());

            if (entityId.IsValid() == false)
            {
                return Events::ProcessingResult::Ignored;
            }

            // Since the mesh component lives in a gem, then create it by name
            AzFramework::BehaviorComponentId editorMeshComponent;
            AzToolsFramework::EntityUtilityBus::BroadcastResult(
                editorMeshComponent,
                &AzToolsFramework::EntityUtilityBus::Events::GetOrAddComponentByTypeName,
                entityId,
                "AZ::Render::EditorMeshComponent");

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
                AZ_Error("prefab", false, "UpdateComponentForEntity failed for Mesh component");
                return Events::ProcessingResult::Ignored;
            }

            nodeEntityMap.insert({ entry.first, entityId });
        }

        // fix up transforms and parenting
        AZStd::vector<AZ::EntityId> entities;
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

            // parent entities
            if (parentEntityId.IsValid())
            {
                AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(nodeEntity.second);
                auto* transform = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                transform->SetParent(parentEntityId);
            }

            // TODO: find node's immediate parent (if any)
            //auto nodeIterator = meshTransformMap.find(nodeEntity.first);

            // TODO: get matrix data for the node at pair.second
            //auto nodeTransform = azrtti_istypeof<AZ::SceneAPI::DataTypes::ITransform>(graph.GetNodeContent(nodeIterator->second));
            //TransformBus::Events::SetWorldTransform
            //void SetLocalTM(const AZ::Transform & tm) override;

            // TODO: update editor entity Transform component
        }

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
            return Events::ProcessingResult::Ignored;
        }

        // Convert the prefab to a JSON string
        AZ::Outcome<AZStd::string, void> outcome;
        AzToolsFramework::Prefab::PrefabLoaderScriptingBus::BroadcastResult(
            outcome,
            &AzToolsFramework::Prefab::PrefabLoaderScriptingBus::Events::SaveTemplateToString,
            prefabTemplateId);

        if (outcome.IsSuccess() == false)
        {
            return Events::ProcessingResult::Ignored;
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
        return Events::ProcessingResult::Success;
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

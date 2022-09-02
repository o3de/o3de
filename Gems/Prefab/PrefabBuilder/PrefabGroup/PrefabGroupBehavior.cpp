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
#include <PrefabGroup/DefaultProceduralPrefab.h>
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
        AZStd::unique_ptr<DefaultProceduralPrefabGroup> m_defaultProceduralPrefab;

        ExportEventHandler() = delete;

        ExportEventHandler(PreExportEventContextFunction function)
            : m_preExportEventContextFunction(AZStd::move(function))
        {
            m_defaultProceduralPrefab = AZStd::make_unique<DefaultProceduralPrefabGroup>();
            BindToCall(&ExportEventHandler::PrepareForExport);
            AZ::SceneAPI::SceneCore::ExportingComponent::Activate();
            Events::AssetImportRequestBus::Handler::BusConnect();
        }

        ~ExportEventHandler()
        {
            m_defaultProceduralPrefab.reset();
            Events::AssetImportRequestBus::Handler::BusDisconnect();
            AZ::SceneAPI::SceneCore::ExportingComponent::Deactivate();
        }

        Events::ProcessingResult PrepareForExport(Events::PreExportEventContext& context)
        {
            return m_preExportEventContextFunction(context);
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
            // ignore constructing a default procedural prefab if the Editor's "Edit Settings..." is being used
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

        // ignore empty scenes (i.e. only has the root node)
        if (scene.GetGraph().GetNodeCount() == 1)
        {
            return Events::ProcessingResult::Ignored;
        }

        AZStd::optional<AZ::SceneAPI::PrefabGroupRequests::ManifestUpdates> manifestUpdates;
        AZ::SceneAPI::PrefabGroupEventBus::BroadcastResult(
            manifestUpdates,
            &AZ::SceneAPI::PrefabGroupEventBus::Events::GeneratePrefabGroupManifestUpdates,
            scene);

        if (!manifestUpdates)
        {
            return Events::ProcessingResult::Failure;
        }

        // update manifest if there were no errors
        for (auto update : manifestUpdates.value())
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
        DefaultProceduralPrefabGroup::Reflect(context);

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

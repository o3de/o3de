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
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>

namespace AZ::SceneAPI::Behaviors
{
    //
    // ExportEventHandler
    //

    struct PrefabGroupBehavior::ExportEventHandler final
        : public AZ::SceneAPI::SceneCore::ExportingComponent
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
        }

        ~ExportEventHandler()
        {
            AZ::SceneAPI::SceneCore::ExportingComponent::Deactivate();
        }

        Events::ProcessingResult PrepareForExport(Events::PreExportEventContext& context)
        {
            return m_preExportEventContextFunction(context);
        }
    };

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

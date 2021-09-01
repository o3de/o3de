/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabGroup/PrefabGroupBehavior.h>
#include <PrefabGroup/PrefabGroup.h>
#include <PrefabGroup/ProceduralPrefabAsset.h>
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
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>

//#include "C:\work\o3de\user\PrefabGroupBehavior.cpp.inl"

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

    AZStd::optional<rapidjson::Document> PrefabGroupBehavior::CreateProductAssetData(const SceneData::PrefabGroup* prefabGroup) const
    {
        using namespace AzToolsFramework::Prefab;

        auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
        if (!prefabLoaderInterface)
        {
            return AZStd::nullopt;
        }

        // validate the PrefabDom will make a valid Prefab template instance
        auto templateId = prefabLoaderInterface->LoadTemplateFromString(prefabGroup->GetPrefabDomBuffer(), prefabGroup->GetName().c_str());
        if (templateId == InvalidTemplateId)
        {
            return AZStd::nullopt;
        }

        auto* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        if (!prefabSystemComponentInterface)
        {
            return AZStd::nullopt;
        }

        // create instance to update the asset hints
        auto instance = prefabSystemComponentInterface->InstantiatePrefab(templateId);
        if (!instance)
        {
            return AZStd::nullopt;
        }

        auto* instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
        if (!instanceToTemplateInterface)
        {
            return AZStd::nullopt;
        }

        // fill out a JSON DOM with all the asset IDs filled out
        rapidjson::Document generatedInstanceDom;
        instanceToTemplateInterface->GenerateDomForInstance(generatedInstanceDom, *instance.get());

        auto doc = AZStd::make_optional<rapidjson::Document>();
        doc.value().SetObject();

        auto prefabId = rapidjson::Value(rapidjson::kStringType);
        prefabId.SetString(prefabGroup->GetId().ToString<AZStd::string>().c_str(), doc.value().GetAllocator());

        auto prefabName = rapidjson::Value(rapidjson::kStringType);
        prefabName.SetString(prefabGroup->GetName().c_str(), doc.value().GetAllocator());

        doc.value().AddMember("id", prefabId.Move(), doc.value().GetAllocator());
        doc.value().AddMember("name", prefabName.Move(), doc.value().GetAllocator());
        doc.value().AddMember("data", generatedInstanceDom.Move(), doc.value().GetAllocator());
        return doc;
    }

    bool PrefabGroupBehavior::WriteOutProductAsset(
        Events::PreExportEventContext& context,
        const SceneData::PrefabGroup* prefabGroup,
        const rapidjson::Document& doc) const
    {
        //Retrieve source asset info so we can get a string with the relative path to the asset
        bool assetInfoResult;
        Data::AssetInfo info;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            assetInfoResult,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
            context.GetScene().GetSourceFilename().c_str(),
            info,
            watchFolder);

        AZ::IO::FixedMaxPath assetPath(info.m_relativePath);
        assetPath.ReplaceFilename(prefabGroup->GetName().c_str());

        AZStd::string filePath = AZ::SceneAPI::Utilities::FileUtilities::CreateOutputFileName(
            assetPath.c_str(),
            context.GetOutputDirectory(),
            "procprefab");

        AZ::IO::FileIOStream fileStream(filePath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (fileStream.IsOpen() == false)
        {
            return false;
        }

        // write to a UTF-8 string buffer
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
        if (doc.Accept(writer) == false)
        {
            return false;
        }

        const auto bytesWritten = fileStream.Write(sb.GetSize(), sb.GetString());
        if (bytesWritten > 1)
        {
            AZ::u32 subId = AZ::Crc32(filePath.c_str());
            context.GetProductList().AddProduct(
                filePath,
                context.GetScene().GetSourceGuid(),
                azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>(),
                {},
                AZStd::make_optional(subId));

            return true;
        }
        return false;
    }

    Events::ProcessingResult PrefabGroupBehavior::OnPrepareForExport(Events::PreExportEventContext& context) const
    {
#ifdef DEBUG_PREFAB
        DEBUG::FillOutPrefabGroup(context);
#endif

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

        for (const auto* prefabGroup : prefabGroupCollection)
        {
            auto result = CreateProductAssetData(prefabGroup);
            if (result.has_value() == false)
            {
                return Events::ProcessingResult::Failure;
            }

            if (WriteOutProductAsset(context, prefabGroup, result.value()) == false)
            {
                return Events::ProcessingResult::Failure;
            }
        }

        return Events::ProcessingResult::Success;
    }

    void PrefabGroupBehavior::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PrefabGroupBehavior, BehaviorComponent>()->Version(1);
        }
    }
}

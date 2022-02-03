/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabGroup/ProceduralAssetHandler.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <QMenu>

namespace AZ::Prefab
{
    // AssetTypeInfoHandler

    class PrefabGroupAssetHandler::AssetTypeInfoHandler final
        : public AZ::AssetTypeInfoBus::Handler
        , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetTypeInfoHandler, AZ::SystemAllocator, 0);
        AssetTypeInfoHandler();
        ~AssetTypeInfoHandler() override;
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;

        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        void AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;
        bool SaveAsAuthoredPrefab(const AZ::Data::AssetId& assetId, const char* destinationFilename);
    };

    PrefabGroupAssetHandler::AssetTypeInfoHandler::AssetTypeInfoHandler()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(azrtti_typeid<ProceduralPrefabAsset>());
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }

    PrefabGroupAssetHandler::AssetTypeInfoHandler::~AssetTypeInfoHandler()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(azrtti_typeid<ProceduralPrefabAsset>());
    }

    AZ::Data::AssetType PrefabGroupAssetHandler::AssetTypeInfoHandler::GetAssetType() const
    {
        return azrtti_typeid<ProceduralPrefabAsset>();
    }

    const char* PrefabGroupAssetHandler::AssetTypeInfoHandler::GetAssetTypeDisplayName() const
    {
        return "Procedural Prefab";
    }

    const char* PrefabGroupAssetHandler::AssetTypeInfoHandler::GetGroup() const
    {
        return "Prefab";
    }

    const char* PrefabGroupAssetHandler::AssetTypeInfoHandler::GetBrowserIcon() const
    {
        return "Icons/Components/Box.png";
    }

    void PrefabGroupAssetHandler::AssetTypeInfoHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(PrefabGroupAssetHandler::s_Extension);
    }

    void PrefabGroupAssetHandler::AssetTypeInfoHandler::AddContextMenuActions(
        [[maybe_unused]] QWidget* caller,
        QMenu* menu,
        const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
    {
        using namespace AzToolsFramework::AssetBrowser;
        auto entryIt = AZStd::find_if
        (
            entries.begin(),
            entries.end(),
            [](const AssetBrowserEntry* entry) -> bool
            {
                return entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product;
            }
        );

        if (entryIt == entries.end())
        {
            return;
        }
        else  if ((*entryIt)->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
        {
            ProductAssetBrowserEntry* product = azrtti_cast<ProductAssetBrowserEntry*>(*entryIt);
            if (product->GetAssetType() == azrtti_typeid<ProceduralPrefabAsset>())
            {
                AZ::Data::AssetId assetId = product->GetAssetId();
                menu->addAction("Save as Prefab...", [assetId, this]()
                {
                    QString filePath = AzQtComponents::FileDialog::GetSaveFileName(nullptr, QString("Save to file"), "", QString("Prefab file (*.prefab)"));
                    if (filePath.isEmpty())
                    {
                        return;
                    }
                    if (SaveAsAuthoredPrefab(assetId, filePath.toUtf8().data()))
                    {
                        AZ_Printf("Prefab", "Prefab was saved to a .prefab file %s", filePath.toUtf8().data());
                    }
                });
            }
        }
    }

    bool PrefabGroupAssetHandler::AssetTypeInfoHandler::SaveAsAuthoredPrefab(const AZ::Data::AssetId& assetId, const char* destinationFilename)
    {
        using namespace AzToolsFramework::Prefab;
        using namespace AZ::Data;

        auto procPrefabAsset = AssetManager::Instance().GetAsset<ProceduralPrefabAsset>(assetId, AssetLoadBehavior::Default);
        const auto status = AssetManager::Instance().BlockUntilLoadComplete(procPrefabAsset);
        if (status != AssetData::AssetStatus::Ready)
        {
            return false;
        }

        auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
        if (!prefabLoaderInterface)
        {
            return false;
        }

        const auto templateId = procPrefabAsset.GetAs<ProceduralPrefabAsset>()->GetTemplateId();
        AZStd::string outputJson;
        if (prefabLoaderInterface->SaveTemplateToString(templateId, outputJson) == false)
        {
            return false;
        }

        const auto fileMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeText;
        AZ::IO::FileIOStream outputFileStream;
        if (outputFileStream.Open(destinationFilename, fileMode) == false)
        {
            return false;
        }

        outputFileStream.Write(outputJson.size(), outputJson.data());
        return true;
    }

    // PrefabGroupAssetHandler

    AZStd::string_view PrefabGroupAssetHandler::s_Extension{ "procprefab" };

    PrefabGroupAssetHandler::PrefabGroupAssetHandler()
    {
        auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
        if (assetCatalog)
        {
            assetCatalog->EnableCatalogForAsset(azrtti_typeid<ProceduralPrefabAsset>());
            assetCatalog->AddExtension(s_Extension.data());
        }
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(this, azrtti_typeid<ProceduralPrefabAsset>());
        }
        m_assetTypeInfoHandler = AZStd::make_shared<AssetTypeInfoHandler>();
    }

    PrefabGroupAssetHandler::~PrefabGroupAssetHandler()
    {
        m_assetTypeInfoHandler.reset();
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetData* PrefabGroupAssetHandler::CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type != azrtti_typeid<ProceduralPrefabAsset>())
        {
            AZ_Error("prefab", false, "Invalid asset type! Only handle 'ProceduralPrefabAsset'");
            return nullptr;
        }
        return aznew ProceduralPrefabAsset{};
    }

    void PrefabGroupAssetHandler::DestroyAsset(AZ::Data::AssetData* ptr)
    {
        // Note: the PrefabLoaderInterface will handle the lifetime of the Prefab Template
        delete ptr;
    }

    void PrefabGroupAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(azrtti_typeid<ProceduralPrefabAsset>());
    }

    AZ::Data::AssetHandler::LoadResult PrefabGroupAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        using namespace AzToolsFramework::Prefab;

        auto* proceduralPrefabAsset = asset.GetAs<ProceduralPrefabAsset>();
        if (!proceduralPrefabAsset)
        {
            AZ_Error("prefab", false, "This should be a ProceduralPrefabAsset type, as this is the only type we process!");
            return LoadResult::Error;
        }

        AZStd::string buffer;
        buffer.resize(stream->GetLoadedSize());
        stream->Read(stream->GetLoadedSize(), buffer.data());

        auto jsonOutcome = AZ::JsonSerializationUtils::ReadJsonString(buffer);
        if (jsonOutcome.IsSuccess() == false)
        {
            AZ_Error("prefab", false, "Asset JSON failed to compile %s", jsonOutcome.GetError().c_str());
            return LoadResult::Error;
        }
        const auto& jsonDoc = jsonOutcome.GetValue();

        if (jsonDoc.IsObject() == false)
        {
            return LoadResult::Error;
        }

        if (jsonDoc.FindMember("Source") == jsonDoc.MemberEnd())
        {
            return LoadResult::Error;
        }
        const auto& templateName = jsonDoc["Source"];

        AZStd::string stringJson;
        auto stringOutcome = AZ::JsonSerializationUtils::WriteJsonString(jsonDoc, stringJson);
        if (stringOutcome.IsSuccess() == false)
        {
            AZ_Error("prefab", false, "Could not write to JSON string %s", stringOutcome.GetError().c_str());
            return LoadResult::Error;
        }

        // prepare the template
        auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
        if (!prefabLoaderInterface)
        {
            return LoadResult::Error;
        }

        auto templateId = prefabLoaderInterface->LoadTemplateFromString(stringJson.data(), templateName.GetString());
        if (templateId == InvalidTemplateId)
        {
            return LoadResult::Error;
        }

        proceduralPrefabAsset->SetTemplateId(templateId);
        proceduralPrefabAsset->SetTemplateName(templateName.GetString());
        return LoadResult::LoadComplete;
    }

    AZStd::unique_ptr<PrefabGroupAssetHandler> s_PrefabGroupAssetHandler;
}


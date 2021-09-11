/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>

#include <AzFramework/FileFunc/FileFunc.h>

#include <Translation/TranslationAsset.h>
#include <Translation/TranslationBus.h>
#include <Translation/TranslationSerializer.h>

namespace GraphCanvas
{
    void TranslationAsset::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<TranslationFormatSerializer>()->HandlesType<TranslationFormat>();
        }


        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TranslationFormat>()
                ->Version(1)
                ->Field("m_database", &TranslationFormat::m_database)
                ;

            serializeContext->Class<TranslationAsset>()
                ->Version(0)
                ;
        }
    }

    TranslationAsset::TranslationAsset(const AZ::Data::AssetId& assetId /*= AZ::Data::AssetId()*/, AZ::Data::AssetData::AssetStatus status /*= AZ::Data::AssetData::AssetStatus::NotLoaded*/)
        : AZ::Data::AssetData(assetId, status)
    {
    }

    TranslationAssetHandler::TranslationAssetHandler()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ::JsonRegistrationContext* jsonRegistrationContext;
        AZ::ComponentApplicationBus::BroadcastResult(jsonRegistrationContext, &AZ::ComponentApplicationBus::Events::GetJsonRegistrationContext);

        auto reportCallback = [](AZStd::string_view /*message*/, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view /*target*/)->
            AZ::JsonSerializationResult::ResultCode
        {
            return result;
        };

        m_serializationSettings = AZStd::make_unique<AZ::JsonSerializerSettings>();
        m_serializationSettings->m_serializeContext = serializeContext;
        m_serializationSettings->m_registrationContext = jsonRegistrationContext;
        m_serializationSettings->m_reporting = reportCallback;

        m_deserializationSettings = AZStd::make_unique<AZ::JsonDeserializerSettings>();
        m_deserializationSettings->m_serializeContext = serializeContext;
        m_deserializationSettings->m_registrationContext = jsonRegistrationContext;
        m_deserializationSettings->m_reporting = reportCallback;

        m_jsonSerializationContext = AZStd::make_unique<AZ::JsonSerializerContext>(*m_serializationSettings, m_jsonAllocator);
        m_jsonDeserializationContext = AZStd::make_unique<AZ::JsonDeserializerContext>(*m_deserializationSettings);

        m_serializer = AZStd::make_unique<TranslationFormatSerializer>();
    }

    TranslationAssetHandler::~TranslationAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr TranslationAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        AZ_UNUSED(type);
        AZ_Assert(type == GetAssetType(), "Invalid asset type! TranslationAssetHandler will handle only 'TranslationAsset'");

        // Look up the asset path to ensure it's actually a behavior tree library.
        if (!CanHandleAsset(id))
        {
            return nullptr;
        }

        return aznew TranslationAsset;
    }

    AZ::Data::AssetType TranslationAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<TranslationAsset>::Uuid();
    }

    const char* TranslationAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Names";
    }

    const char* TranslationAssetHandler::GetGroup() const
    {
        return "Names";
    }

    const char* TranslationAssetHandler::GetBrowserIcon() const
    {
        return "Icons/Components/Names.svg";
    }

    AZ::Uuid TranslationAssetHandler::GetComponentTypeId() const
    {
        return AZ::Uuid::CreateNull();
    }

    void TranslationAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("names");
    }

    AZ::Data::AssetHandler::LoadResult TranslationAssetHandler::LoadAssetData(
        [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB&)
    {
        AZ::Data::AssetHandler::LoadResult result = AZ::Data::AssetHandler::LoadResult::LoadComplete;

        auto sizeBytes = stream->GetLoadedSize();

        AZStd::unique_ptr<char[]> buffer = AZStd::make_unique<char[]>(sizeBytes + 1);
        if (stream->Read(sizeBytes, buffer.get()))
        {
            buffer[sizeBytes] = '\0';

            rapidjson::Document document;

            bool failure = false;
            if (document.Parse(buffer.get()).HasParseError())
            {
                failure = true;

                AZStd::string errorStr = AZStd::string::format(
                    "Error parsing JSON contents (offset %zu): %s", document.GetErrorOffset(),
                    rapidjson::GetParseError_En(document.GetParseError()));

                AZ_Error("TranslationDatabase", false, "Parse error in %s (%s)", asset.GetHint().c_str(), errorStr.c_str());

                result = AssetHandler::LoadResult::Error;
            }
            else
            {
                const rapidjson::Value& rootValue = document.GetObject();
                if (!document.IsObject())
                {
                    failure = true;
                }

                if (!failure)
                {
                    using namespace AZ::JsonSerializationResult;

                    TranslationFormat translationFormat;

                    ResultCode serializerResult = m_serializer->Load(
                        &translationFormat, azrtti_typeid<TranslationFormat>(), rootValue, *m_jsonDeserializationContext);
                    if (serializerResult.GetOutcome() == Outcomes::Success)
                    {
                        // Add the newly loaded translation data into the global database
                        bool warnings = false;
                        TranslationRequestBus::BroadcastResult(warnings, &TranslationRequests::Add, translationFormat);
                        if (warnings)
                        {
                            AZ_Warning("TranslationAsset", false, "Unable to add translation data to database for file: %s", asset.GetHint().c_str());
                        }
                    }
                    else
                    {
                        AZ_Error("TranslationAsset", false, "Serialization of the TranslationFormat failed for: %s", asset.GetHint().c_str());
                    }
                }
            }
        }

        return result;
    }

    void TranslationAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void TranslationAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(GetAssetType());
    }

    void TranslationAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");

        auto assetType = GetAssetType();
        AZ::Data::AssetManager::Instance().RegisterHandler(this, assetType);

        // Use AssetCatalog service to register ScriptEvent asset type and extension
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, TranslationAsset::GetFileFilter());

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(assetType);
    }

    void TranslationAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusDisconnect();

        m_serializer.reset();

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    bool TranslationAssetHandler::CanHandleAsset(const AZ::Data::AssetId&) const
    {
        return true;
    }
}

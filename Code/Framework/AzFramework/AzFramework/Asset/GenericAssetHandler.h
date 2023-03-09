/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZFRAMEWORK_ASSET_GENERICASSETHANDLER_H
#define AZFRAMEWORK_ASSET_GENERICASSETHANDLER_H

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework
{
    /**
     * Generic implementation of an asset handler for any arbitrary type that is reflected for serialization.
     *
     * Simple or game specific assets that wish to make use of automated loading and editing facilities
     * can use this handler with any asset type that's reflected for editing.
     *
     * Example:
     *
     * class MyAsset : public AZ::Data::AssetData
     * {
     * public:
     *  AZ_CLASS_ALLOCATOR(MyAsset, AZ::SystemAllocator);
     *  AZ_RTTI(MyAsset, "{AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE}", AZ::Data::AssetData);
     *
     *  static void Reflect(AZ::ReflectContext* context)
     *  {
     *      AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
     *      if (serialize)
     *      {
     *          serialize->Class<MyAsset>()
     *              ->Field("SomeField", &MyAsset::m_someField)
     *              ;
     *
     *          AZ::EditContext* edit = serialize->GetEditContext();
     *          if (edit)
     *          {
     *              edit->Class<MyAsset>("My Asset", "Asset for representing X, Y, and Z")
     *                  ->DataElement(0, &MyAsset::m_someField, "Some data field", "It's a float")
     *                  ;
     *          }
     *      }
     *  }
     *
     *  float m_someField;
     * };
     *
     *
     * using MyAssetHandler = GenericAssetHandler<MyAsset>;
     *
     */

    /** 
    * Just a base class to assign concrete RTTI to these classes - Don't derive from this - use GenericAssetHandler<T> instead.
    * This being in the heirarchy allows you to easily ask whether a particular handler derives from this type and thus is a
    * GenericAssetHandler.
    */
    class GenericAssetHandlerBase : public AZ::Data::AssetHandler
    {
    public:
        AZ_RTTI(GenericAssetHandlerBase, "{B153B8B5-25CC-4BB7-A2BD-9A47ECF4123C}", AZ::Data::AssetHandler);
        virtual ~GenericAssetHandlerBase() = default;
    };

    template <typename AssetType>
    class GenericAssetHandler
        : public GenericAssetHandlerBase
        , private AZ::AssetTypeInfoBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GenericAssetHandler<AssetType>, AZ::SystemAllocator);
        AZ_RTTI(GenericAssetHandler<AssetType>, "{8B36B3E8-8C0B-4297-BDA2-1648C155C78E}", GenericAssetHandlerBase);

        GenericAssetHandler(const char* displayName,
            const char* group,
            const char* extension,
            const AZ::Uuid& componentTypeId = AZ::Uuid::CreateNull(),
            AZ::SerializeContext* serializeContext = nullptr)
            : m_displayName(displayName)
            , m_group(group)
            , m_extension(extension)
            , m_componentTypeId(componentTypeId)
            , m_serializeContext(serializeContext)
        {
            AZ_Assert(extension, "Extension is required.");
            if (extension[0] == '.')
            {
                ++extension;
            }

            m_extension = extension;
            AZ_Assert(!m_extension.empty(), "Invalid extension provided.");

            if (!m_serializeContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<AssetType>::Uuid());
        }

        ~GenericAssetHandler()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();
        }

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& /*id*/, const AZ::Data::AssetType& /*type*/) override
        {
            return aznew AssetType();
        }

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            AssetType* assetData = asset.GetAs<AssetType>();
            AZ_Assert(assetData, "Asset is of the wrong type.");
            AZ_Assert(m_serializeContext, "Unable to retrieve serialize context.");
            if (assetData)
            {
                return AZ::Utils::LoadObjectFromStreamInPlace<AssetType>(*stream, *assetData, m_serializeContext,
                                                                         AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB)) ?
                    AZ::Data::AssetHandler::LoadResult::LoadComplete :
                    AZ::Data::AssetHandler::LoadResult::Error;
            }

            return AZ::Data::AssetHandler::LoadResult::Error;
        }

        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override
        {
            AssetType* assetData = asset.GetAs<AssetType>();
            AZ_Assert(assetData, "Asset is of the wrong type.");
            if (assetData && m_serializeContext)
            {
                return AZ::Utils::SaveObjectToStream<AssetType>(*stream,
                    AZ::ObjectStream::ST_XML,
                    assetData,
                    m_serializeContext);
            }

            return false;
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(AZ::AzTypeInfo<AssetType>::Uuid());
        }

        void Register()
        {
            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::EnableCatalogForAsset, AZ::AzTypeInfo<AssetType>::Uuid());
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddExtension, m_extension.c_str());

            AZ_Assert(AZ::Data::AssetManager::IsReady(), "AssetManager isn't ready!");
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<AssetType>::Uuid());
        }

        void Unregister()
        {
            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        bool CanHandleAsset(const AZ::Data::AssetId& id) const override
        {
            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, id);
            if (!assetPath.empty())
            {
                AZStd::string assetExtension;
                if (AzFramework::StringFunc::Path::GetExtension(assetPath.c_str(), assetExtension, false))
                {
                    return assetExtension == m_extension;
                }
            }

            return false;
        }

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override
        {
            return AZ::AzTypeInfo<AssetType>::Uuid();
        }

        const char* GetAssetTypeDisplayName() const override
        {
            return m_displayName.c_str();
        }

        const char* GetGroup() const override
        {
            return m_group.c_str();
        }

        AZ::Uuid GetComponentTypeId() const override
        {
            return m_componentTypeId;
        }

        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override
        {
            extensions.push_back(m_extension);
        }
        //////////////////////////////////////////////////////////////////////////////////////////////

        AZStd::string m_displayName;
        AZStd::string m_group;
        AZStd::string m_extension;
        AZ::Uuid m_componentTypeId = AZ::Uuid::CreateNull();
        AZ::SerializeContext* m_serializeContext;
        GenericAssetHandler(const GenericAssetHandler&) = delete;
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_ASSET_GENERICASSETHANDLER_H

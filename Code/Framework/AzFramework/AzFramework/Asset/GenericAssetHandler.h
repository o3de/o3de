/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
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
     *              ->Version(0)
     *              ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true) // optional:  automatically show in "create new" menu in Asset Editor
     *              ->Field("SomeField", &MyAsset::m_someField) // a plain old data field (example)
     *              ->Field("myAsset", &MyAsset::m_myAsset)     // assignment of some other asset we depend on (example)
     *              
     *              ;
     *
     *          AZ::EditContext* edit = serialize->GetEditContext();
     *          if (edit)
     *          {
     *              edit->Class<MyAsset>("My Asset", "Asset for representing X, Y, and Z")
     *                  ->DataElement(0, &MyAsset::m_someField, "Some data field", "It's a float")
     *                  ->DataElement(0, &MyAsset::m_myAsset, "Some Asset To Use", "It's a reference to some other asset")
     *                  ;
     *          }
     *      }
     *  }
     *
     *  float m_someField;
     *  Asset<SomeType> m_myAsset { AZ::Data::AssetLoadBehavior::PreLoad }; // example, can choose a different auto load behavior here if you want
     * };
     *
     *
     * using MyAssetHandler = GenericAssetHandler<MyAsset>;
     *
     * // Note that you must still register the actual asset handler instance on startup and tear it down on shutdown, using a system component or other
     * // singleton.
     * // Alternatively you can set autoprocess later:
     * s_myAssetHandler = new MyAssetHandler("My Asset Friendly Display Name", "My Asset Group (ie, Graphics, Audio ...)", "my_file_extension_without_dot");
     * s_myAssetHandler->SetAutoProcessToCache(true); // if you want the system to register, build and copy it to cache and extract deps for you.
     * s_myAssetHandler->Register();
     *
     * // Registration timing is important.  Your component must activate and register its type before the system component that auto builds
     * // asset checks for registration.  You do this by making sure your singleton / system component providing the above registration emits
     * // AzFramework::s_GenericAssetRegistrar as one of its Provided Services, which will cause it to activate before the system
     * // that depends on that service.
     *
     * Example:
     * 
     * void MySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided) override
     * {
     *     provided.push_back(AzFramework::s_GenericAssetRegistrar); // Activate me before things that need these registrations.
     * }
     */


    /** 
    * Just a base class to assign concrete RTTI to these classes - Don't derive from this - use GenericAssetHandler<T> instead.
    * This being in the heirarchy allows you to easily ask whether a particular handler derives from this type and thus is a
    * GenericAssetHandler.
    */
    class AZF_API GenericAssetHandlerBase : public AZ::Data::AssetHandler
    {
    public:
        AZ_RTTI(GenericAssetHandlerBase, "{B153B8B5-25CC-4BB7-A2BD-9A47ECF4123C}", AZ::Data::AssetHandler);
        virtual ~GenericAssetHandlerBase() = default;

        // This function is called when an asset is requested but not found in the catalog.
        // This default implementation will block until the asset is compiled by the asset system (This has no
        // effect in release builds or if the asset is already compiled).
        // Overriding this function will allow you to do more complex things like returning a fallback or placeholder
        // asset, or a different asset to indicate different kinds of problems with the data, or to allow async background
        // compilation.  See the body of this function for more information.
        AZ::Data::AssetId AssetMissingInCatalog(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;

        //! Overridden in GenericAssetHandler to return whether or not the asset should be auto processed into the cache.
        //! This is based on what the constructor sets.
        virtual bool AutoBuildAssetToCache() = 0;
    };

    // Important:  If you want your generic asset to automatically be processed by asset processor, and dependencies auto extracted
    // you must make the system component which registers your generic asset handler emit the following in its GetProvidedServices
    // that it is is activated before the system component that checks all existing handlers and registers the auto builder for them:
    constexpr const AZ::Crc32 s_GenericAssetRegistrar = AZ_CRC_CE("GenericAssetRegistrar");
    // Remember to also add ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true) // as an attribute in your asset's
    // reflection if you want it to show up in the asset editor "create new" / "open existing" dialogs automatically.

    template <typename AssetType>
    class GenericAssetHandler
        : public GenericAssetHandlerBase
        , private AZ::AssetTypeInfoBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GenericAssetHandler<AssetType>, AZ::SystemAllocator);
        AZ_RTTI(GenericAssetHandler<AssetType>, "{8B36B3E8-8C0B-4297-BDA2-1648C155C78E}", GenericAssetHandlerBase);

        //! Construct your generic asset handler.
        //! @param displayName The name of the asset type to show in the user-facing guis and logs
        //! @param extension The file extension to associate with this asset type.  JUST the extension without a dot.
        //! @param componentTypeId If this asset type is meant to be automatically added as a component to entities
        //!                        when dragged and dropped into the level, specify the component typeid here.
        //!                        This should be the typeid of the component that should be added.
        //! @param streamType The type of stream to use when saving this asset. (Default is ObjectStream XML)
        //! @param autoProcessToCache If true, the asset system will automatically process this asset into the cache
        //!                           using the asset processor, and automatically extract dependencies for it.
        //!                           if False, you're expected to do that work yourself, by either making a RC COPY rule in the
        //!                           asset processor setreg, or make your own Asset Builder (Custom) to handle extreme custom cases.
        GenericAssetHandler(const char* displayName,
            const char* group,
            const char* extension,
            const AZ::Uuid& componentTypeId = AZ::Uuid::CreateNull(),
            AZ::SerializeContext* serializeContext = nullptr,
            const AZ::DataStream::StreamType streamType = AZ::ObjectStream::ST_XML,
            const bool autoProcessToCache = false) 
            : m_displayName(displayName)
            , m_group(group)
            , m_extension(extension)
            , m_componentTypeId(componentTypeId)
            , m_serializeContext(serializeContext)
            , m_streamType(streamType)
            , m_autoProcessAsset(autoProcessToCache)
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

        virtual ~GenericAssetHandler()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();
        }

        bool AutoBuildAssetToCache() override
        {
            return m_autoProcessAsset;
        }

        void SetAutoBuildAssetToCache(bool autoBuild)
        {
            m_autoProcessAsset = autoBuild;
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
                    m_streamType,
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
        bool m_autoProcessAsset = false;
        AZ::Uuid m_componentTypeId = AZ::Uuid::CreateNull();
        AZ::SerializeContext* m_serializeContext;
        AZ::DataStream::StreamType m_streamType;
        GenericAssetHandler(const GenericAssetHandler&) = delete;
    };
} // namespace AzFramework


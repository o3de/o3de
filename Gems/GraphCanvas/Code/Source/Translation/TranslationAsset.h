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
#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include "TranslationSerializer.h"

namespace GraphCanvas
{
    // Stores a key/value database of strings that users can query translated
    // names from
    class TranslationFormat
    {
    public:

        AZ_RTTI(TranslationFormat, "{F51F816E-AEFB-40D4-B3DC-8478364AEB82}");

        virtual ~TranslationFormat() = default;

        AZStd::unordered_map<AZStd::string, AZStd::string> m_database;
    };

    // These are individual assets that store portions of the TranslationDatabase
    // during load time, the TranslationDatabase will enumerate through all of these
    // and build itself up
    class TranslationAsset
        : public AZ::Data::AssetData
    {
    public:

        AZ_RTTI(TranslationAsset, "{6A1A3B00-3DF2-4297-96BB-3BA067A978E6}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(TranslationAsset, AZ::SystemAllocator);

        TranslationAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded);

        ~TranslationAsset() = default;

        static const char* GetDisplayName() { return "Graph Canvas Translation"; }
        static const char* GetGroup() { return "GraphCanvas"; }
        static const char* GetFileFilter() { return ".names"; }

        static void Reflect(AZ::ReflectContext* context);

        TranslationFormat m_translationData;

    };

    //! TranslationAssetHandler will process JSON based files that provide a mapping from string, string
    //! the key will be generated using the JSON file structure, but does have some requirements.
    //!
    //! Requirements:
    //!     - Must have a top level array called "entries"
    //!     - Must provide a "base" element for any entry added
    //!
    //! Example:
    //! 
    //! {
    //! "entries": [
    //!     {
    //!         "base": "Globals",
    //!         "details": {
    //!             "name": "My Name",
    //!             "tooltip": "My Tooltip"
    //!     }
    //!   ]
    //! }
    //!
    //! The example JSON will produce the following database:
    //!
    //! [Globals.details.name, "My Name"]
    //! [Globals.details.tooltip", "My Tooltip"]
    //!
    //! Arrays
    //! Arrays are supported and will contain an index value encoded into the key, for an array called "somearray" these may look like this:
    //! "somearray": [ {
    //!         "name": "First one"
    //!      }, {
    //!         "name": "Second one"
    //!      } ]
    //!
    //! Globals.details.somearray.0.name
    //! Globals.details.somearray.1.name
    //!
    //! There is one important aspect however, if an element in an array has a "base" value, the value of this key
    //! will replace the index. This is useful when the index and/or ordering of an entry is not relevant or may
    //! change.
    //!
    //! "somearray": [ {
    //!         "name": "First one"
    //!         "base": "a_key"
    //!      }, {
    //!         "name": "Second one",
    //!          "base": "b_key"
    //!      } ]
    //!
    //! Globals.details.somearray.0.base   == "a_key"
    //! Globals.details.somearray.0.name   == "First one"
    //! Globals.details.somearray.1.base   == "b_key"
    //! Globals.details.somearray.1.name   == "Second one"
    //!
    class TranslationAssetHandler
        : public AZ::Data::AssetHandler
        , protected AZ::AssetTypeInfoBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(TranslationAssetHandler, AZ::SystemAllocator);
        AZ_RTTI(TranslationAssetHandler, "{C161AB3B-86F6-4CB1-9DAE-83F2DE084CF4}", AZ::Data::AssetHandler);

        TranslationAssetHandler();
        ~TranslationAssetHandler() override;

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        AZ::Data::AssetHandler::LoadResult LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;
        bool CanHandleAsset(const AZ::Data::AssetId& id) const override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        AZ::Uuid GetComponentTypeId() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        void Register();
        void Unregister();

    private:

        AZStd::unique_ptr<TranslationFormatSerializer> m_serializer;

        AZStd::unique_ptr<AZ::JsonSerializerSettings> m_serializationSettings;
        AZStd::unique_ptr<AZ::JsonDeserializerSettings> m_deserializationSettings;
        AZStd::unique_ptr<AZ::JsonSerializerContext> m_jsonSerializationContext;
        AZStd::unique_ptr<AZ::JsonDeserializerContext> m_jsonDeserializationContext;

        rapidjson::Document::AllocatorType m_jsonAllocator;

    };

}

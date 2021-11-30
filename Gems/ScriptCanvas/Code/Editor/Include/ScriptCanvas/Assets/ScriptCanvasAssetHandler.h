/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvasEditor
{
    AZ::Outcome<void, AZStd::string> LoadScriptCanvasDataFromJson
        ( ScriptCanvas::ScriptCanvasData& dataTarget
        , AZStd::string_view source
        , AZ::SerializeContext& serializeContext);

    /**
    * Manages editor Script Canvas graph assets.
    */
    class ScriptCanvasAssetHandler
        : public AZ::Data::AssetHandler
        , protected AZ::AssetTypeInfoBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(ScriptCanvasAssetHandler, "{098B86B2-2527-4155-84C9-A698A0D20068}", AZ::Data::AssetHandler);

        ScriptCanvasAssetHandler(AZ::SerializeContext* context = nullptr);
        ~ScriptCanvasAssetHandler();

        // Called by the asset database to create a new asset.
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;

        // Override the stream info to force source assets to load into the Editor instead of cached, processed assets.
        void GetCustomAssetStreamInfoForLoad(AZ::Data::AssetStreamInfo& streamInfo) override;

        // Called by the asset database to perform actual asset load.
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;

        // Called by the asset database to perform actual asset save. Returns true if successful otherwise false (default - as we don't require support save).
        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;
        bool SaveAssetData(const ScriptCanvasAsset* assetData, AZ::IO::GenericStream* stream);
        bool SaveAssetData(const ScriptCanvasAsset* assetData, AZ::IO::GenericStream* stream, AZ::DataStream::StreamType streamType);

        // Called by the asset database when an asset should be deleted.
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;

        // Called by asset database on registration.
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

        // Provides editor with information about script canvas graph assets.
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;

        AZ::Uuid GetComponentTypeId() const override;

        AZ::SerializeContext* GetSerializeContext() const;

        void SetSerializeContext(AZ::SerializeContext* context);

        static AZ::Data::AssetType GetAssetTypeStatic();

        // protected AZ::AssetTypeInfoBus::MultiHandler...
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;


    protected:
        AZ::SerializeContext* m_serializeContext;
    };
}

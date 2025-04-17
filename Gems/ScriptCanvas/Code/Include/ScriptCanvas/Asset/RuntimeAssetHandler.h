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

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    /**
    * Manages Script Canvas graph assets.
    */
    class RuntimeAssetHandler
        : public AZ::Data::AssetHandler
        , protected AZ::AssetTypeInfoBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(RuntimeAssetHandler, AZ::SystemAllocator);
        AZ_RTTI(RuntimeAssetHandler, "{560A330A-2905-4A43-952D-70E21F8CE16C}", AZ::Data::AssetHandler);

        RuntimeAssetHandler(AZ::SerializeContext* context = nullptr);
        ~RuntimeAssetHandler() override;
        
        // AZ::AssetTypeInfoBus
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;

        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        AZ::Uuid GetComponentTypeId() const override;

        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;

        bool CanCreateComponent(const AZ::Data::AssetId& /*assetId*/) const override;
        ////

        // Called by the asset database to create a new asset. No loading should during this call
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;

        void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override;

        // Called by the asset database to perform actual asset load.
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        
        // Called by the asset database to perform actual asset save. Returns true if successful otherwise false (default - as we don't require support save).
        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;

        // Called by the asset database when an asset should be deleted.
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;

        // Called by asset database on registration.
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

        AZ::SerializeContext* GetSerializeContext() const;
        void SetSerializeContext(AZ::SerializeContext* context);

    protected:

        AZ::SerializeContext* m_serializeContext;
    };
}

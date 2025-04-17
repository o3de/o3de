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
#include <AzCore/Memory/SystemAllocator.h>

namespace AzFramework
{
    class SpawnableAssetHandler final
        : public AZ::Data::AssetHandler
        , public AZ::AssetTypeInfoBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(SpawnableAssetHandler, AZ::SystemAllocator);
        AZ_RTTI(SpawnableAssetHandler, "{BF6E2D17-87C9-4BB1-A205-3656CF6D551D}", AZ::Data::AssetHandler);

        SpawnableAssetHandler();
        ~SpawnableAssetHandler() override;

        //
        // AssetHandler
        //

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;


        //
        // AssetTypeInfoBus
        //

        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
        static uint32_t BuildSubId(AZStd::string_view id);
        
    protected:
        LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;

    private:
    };
} // namespace AzFramework

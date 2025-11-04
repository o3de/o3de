/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Asset/AssetManager.h> // this is where AssetHandler is defined

namespace MiniAudio
{
    class SoundAssetHandler
        : public AZ::Data::AssetHandler
        , public AZ::AssetTypeInfoBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SoundAssetHandler, AZ::SystemAllocator, 0);

        SoundAssetHandler();
        ~SoundAssetHandler();

        void Register();
        void Unregister();

        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

        // AZ::AssetTypeInfoBus
        AZ::Data::AssetType GetAssetType() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetBrowserIcon() const override;
        const char* GetGroup() const override;
        AZ::Uuid GetComponentTypeId() const override;
        bool CanCreateComponent(const AZ::Data::AssetId& assetId) const override;
    };
} // namespace MiniAudio

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

namespace WhiteBox
{
    namespace Pipeline
    {
        //! Asset handler for loading and initializing WhiteBoxMeshAsset assets.
        class WhiteBoxMeshAssetHandler
            : public AZ::Data::AssetHandler
            , private AZ::AssetTypeInfoBus::Handler
        {
        public:
            inline static constexpr char AssetFileExtension[] = "wbm";

            AZ_CLASS_ALLOCATOR(WhiteBoxMeshAssetHandler, AZ::SystemAllocator)

            WhiteBoxMeshAssetHandler();
            ~WhiteBoxMeshAssetHandler();

            void Register();
            void Unregister();

            // AZ::Data::AssetHandler ...
            AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            bool SaveAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;
            void DestroyAsset(AZ::Data::AssetPtr ptr) override;
            void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

            // AZ::AssetTypeInfoBus ...
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            const char* GetAssetTypeDisplayName() const override;
            const char* GetBrowserIcon() const override;
            const char* GetGroup() const override;
            AZ::Uuid GetComponentTypeId() const override;
        };
    } // namespace Pipeline
} // namespace WhiteBox

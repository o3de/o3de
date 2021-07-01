/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Integration/System/SystemCommon.h>
#include <Integration/Assets/AssetCommon.h>
#include <Integration/Assets/MotionAsset.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }
}

namespace EMotionFX
{
    class MotionSet;

    namespace Integration
    {
        class CustomMotionSetCallback;

        /**
         * Represents a shared motion set asset in-memory, and is registered with the AZ::Data::AssetDatabase.
         */
        class MotionSetAsset
            : public EMotionFXAsset
            , public AZ::Data::AssetBus::MultiHandler
        {
        public:
            AZ_RTTI(MotionSetAsset, "{1DA936A0-F766-4B2F-B89C-9F4C8E1310F9}", EMotionFXAsset)
            AZ_CLASS_ALLOCATOR_DECL

            MotionSetAsset(AZ::Data::AssetId id = AZ::Data::AssetId());
            ~MotionSetAsset() override;

            // AZ::Data::AssetBus::MultiHandler
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            static void NotifyMotionSetModified(const AZ::Data::Asset<MotionSetAsset>& asset);

            void SetData(EMotionFX::MotionSet* motionSet);

            AZStd::unique_ptr<EMotionFX::MotionSet>     m_emfxMotionSet;            ///< EMotionFX motion set
            AZStd::vector<AZ::Data::Asset<MotionAsset>> m_motionAssets;             ///< Handles to all contained motions
            bool                                        m_isReloadPending = false;  ///< True if a dependent motion was reloaded and we're pending our own reload notification.
        };

        /**
         * Handler responsible for creating, loading, and initializing shared motion set assets.
         */
        class MotionSetAssetHandler : public EMotionFXAssetHandler<MotionSetAsset>
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            const char* GetAssetTypeDisplayName() const override;
            const char* GetBrowserIcon() const override;
        };

        class MotionSetAssetBuilderHandler : public MotionSetAssetHandler
        {
        public:
            void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override;
            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        };
    } // namespace Integration
} // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::Integration::MotionSetAsset>, "{5A306008-884B-486C-BEBB-186E28E3B63D}");
}

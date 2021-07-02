/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <Integration/Assets/AssetCommon.h>


namespace EMotionFX
{
    class Motion;
    class MotionInstance;

    namespace Integration
    {
        class MotionAsset : public EMotionFXAsset
        {
        public:
            AZ_RTTI(MotionAsset, "{00494B8E-7578-4BA2-8B28-272E90680787}", EMotionFXAsset)
            AZ_CLASS_ALLOCATOR_DECL

            MotionAsset(AZ::Data::AssetId id = AZ::Data::AssetId());

            void SetData(EMotionFX::Motion* motion);  // Only Used for testing
            EMotionFXPtr<EMotionFX::Motion> m_emfxMotion;
        };

        class MotionAssetHandler : public EMotionFXAssetHandler<MotionAsset>
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            const char* GetAssetTypeDisplayName() const override;
            const char* GetBrowserIcon() const override;
        };

    } // namespace Integration
} // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::Integration::MotionAsset>, "{B51E66B5-B576-432A-9D01-9C8DA4757CE9}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::MotionInstance>, "{491DEAEE-A540-4187-A25F-743BEB74E01C}");
}

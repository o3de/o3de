/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/containers/vector.h>

namespace MiniAudio
{
    class SoundAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(SoundAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(SoundAsset, "{7fef8671-760f-4ebd-91e7-57aaf3eef1ca}", AZ::Data::AssetData);

        static constexpr const char* FileExtension = "miniaudio";
        static constexpr const char* AssetGroup = "Sound";
        static constexpr AZ::u32 AssetSubId = 1;

        static void Reflect(AZ::ReflectContext* context);

        SoundAsset() = default;
        ~SoundAsset() override = default;

        AZStd::vector<AZ::u8> m_data;
    };

    using SoundDataAsset = AZ::Data::Asset<SoundAsset>;
    using SoundDataAssetVector = AZStd::vector<AZ::Data::Asset<SoundAsset>>;
} // namespace MiniAudio

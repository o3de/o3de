/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    namespace Data
    {
        //! JSON serializer for Asset<T>.
        class AssetJsonSerializer
            : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(AssetJsonSerializer, "{9674F4F5-7989-44D7-9CAC-DBD494A0A922}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            //! Note that the information for the Asset<T> will be loaded, but the asset data won't be loaded. After deserialization has
            //! completed it's up to the caller to queue the Asset<T> for loading with the AssetManager.
            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;
            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
                const Uuid& valueTypeId, JsonSerializerContext& context) override;

        private:
            JsonSerializationResult::Result LoadAsset(void* outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);
        };

        class SerializedAssetTracker final
        {
        public:
            AZ_RTTI(SerializedAssetTracker, "{1E067091-8C0A-44B1-A455-6E97663F6963}");
            using AssetFixUp = AZStd::function<void(Asset<AssetData>& asset)>;

            void SetAssetFixUp(AssetFixUp assetFixUpCallback);
            void FixUpAsset(Asset<AssetData>& asset);

            void AddAsset(Asset<AssetData> asset);
            AZStd::vector<Asset<AssetData>>& GetTrackedAssets();
            const AZStd::vector<Asset<AssetData>>& GetTrackedAssets() const;

        private:
            AZStd::vector<Asset<AssetData>> m_serializedAssets;
            AssetFixUp m_assetFixUpCallback;
        };
    } // namespace Data
} // namespace AZ

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Memory/Memory.h>
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
    } // namespace Data
} // namespace AZ

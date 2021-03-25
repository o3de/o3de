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
    class JsonEntityIdSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonEntityIdSerializer, "{AEA75997-087C-4E23-8E4F-465A4142EC77}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        class JsonEntityIdMapper
        {
        public:
            AZ_RTTI(JsonEntityIdMapper, "{8E139C95-827F-45B1-BCF0-F54F2D02C594}");

            virtual JsonSerializationResult::Result MapJsonToId(EntityId& outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context) = 0;
            virtual JsonSerializationResult::Result MapIdToJson(rapidjson::Value& outputValue, const EntityId& inputValue, JsonSerializerContext& context) = 0;

            inline void SetIsEntityReference(bool isEntityReference) { m_isEntityReference = isEntityReference; };

        protected:
            bool m_isEntityReference = true;
        };

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
}

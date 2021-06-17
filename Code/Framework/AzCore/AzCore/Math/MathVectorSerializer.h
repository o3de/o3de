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

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class BaseJsonVectorSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(BaseJsonVectorSerializer, "{C188D355-E6DF-4590-8B31-F40591F48A8E}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        OperationFlags GetOperationsFlags() const override;
    };

    class JsonVector2Serializer : public BaseJsonVectorSerializer
    {
    public:
        AZ_RTTI(JsonVector2Serializer, "{E1EAA209-9682-4120-B26B-3EDD9AD56D6F}", BaseJsonVectorSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonVector3Serializer : public BaseJsonVectorSerializer
    {
    public:
        AZ_RTTI(JsonVector3Serializer, "{BF82BBF3-3CD9-48DA-97CC-E4DF2EF01552}", BaseJsonVectorSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonVector4Serializer : public BaseJsonVectorSerializer
    {
    public:
        AZ_RTTI(JsonVector4Serializer, "{05B45EA7-7102-4281-8AA0-2AC72D74AAFD}", BaseJsonVectorSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonQuaternionSerializer : public BaseJsonVectorSerializer
    {
    public:
        AZ_RTTI(JsonQuaternionSerializer, "{18604375-3606-49AC-B366-0F6DF9149FF3}", BaseJsonVectorSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
}

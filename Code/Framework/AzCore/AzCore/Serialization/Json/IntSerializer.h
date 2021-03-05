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
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class JsonCharSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonCharSerializer, "{CA2A4AAC-3068-40B2-94F8-A537FBA8236E}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonShortSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonShortSerializer, "{3D6789BD-231B-4E5D-B81D-609E71A2BCB5}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonIntSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonIntSerializer, "{29E26946-0F1F-44B0-A098-1171B7B0C8FA}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonLongSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonLongSerializer, "{0EB432D0-A0C8-43B2-9D65-A73A4D6DFE3E}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonLongLongSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonLongLongSerializer, "{5E7967DE-A4DC-40E1-81A1-2896A054BB8A}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
    
    class JsonUnsignedCharSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUnsignedCharSerializer, "{1E6D606F-8490-4736-AAFF-91046FDEA2BB}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonUnsignedShortSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUnsignedShortSerializer, "{3C92D2CC-CB13-4A40-B779-47562EE36451}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonUnsignedIntSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUnsignedIntSerializer, "{70C0714A-690D-4F30-8986-ABC9DEFE9D62}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonUnsignedLongSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUnsignedLongSerializer, "{28E5499F-6AF4-4778-AE14-66BA40B56247}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
    
    class JsonUnsignedLongLongSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUnsignedLongLongSerializer, "{AB048BB3-C280-4166-9E2E-54CE2C3413CA}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
} // namespace AZ

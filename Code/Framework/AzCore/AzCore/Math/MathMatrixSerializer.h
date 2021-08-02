/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class BaseJsonMatrixSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(BaseJsonMatrixSerializer, "{18CA4637-C9B7-454B-9126-107E18A8C096}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        OperationFlags GetOperationsFlags() const override;
    };

    class JsonMatrix3x3Serializer : public BaseJsonMatrixSerializer
    {
    public:
        AZ_RTTI(JsonMatrix3x3Serializer, "{8C76CD6A-8576-4604-A746-CF7A7F20F366}", BaseJsonMatrixSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonMatrix3x4Serializer : public BaseJsonMatrixSerializer
    {
    public:
        AZ_RTTI(JsonMatrix3x4Serializer, "{E801333B-4AF1-4F43-976C-579670B02DC5}", BaseJsonMatrixSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonMatrix4x4Serializer : public BaseJsonMatrixSerializer
    {
    public:
        AZ_RTTI(JsonMatrix4x4Serializer, "{46E888FC-248A-4910-9221-4E101A10AEA1}", BaseJsonMatrixSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
}

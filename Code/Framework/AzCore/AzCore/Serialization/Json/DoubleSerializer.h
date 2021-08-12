/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class JsonDoubleSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonDoubleSerializer, "{1B504DA9-733C-45C8-BB08-7026430FD57F}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue,  const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
        OperationFlags GetOperationsFlags() const override;
    };

    class JsonFloatSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonFloatSerializer, "{A6AB9CE5-B613-49F1-AC58-C16A6850731E}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
        OperationFlags GetOperationsFlags() const override;
    };
} // namespace AZ

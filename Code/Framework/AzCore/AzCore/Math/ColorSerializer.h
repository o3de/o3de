/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class JsonColorSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonColorSerializer, "{4C0CF19D-7F92-459A-88C3-47C07BE8BD45}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

        OperationFlags GetOperationsFlags() const override;

    private:
        enum class LoadAlpha
        {
            Yes,
            No,
            IfAvailable
        };

        JsonSerializationResult::Result LoadObject(Color& output, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context);
        JsonSerializationResult::Result LoadFloatArray(Color& output, const rapidjson::Value& inputValue, LoadAlpha loadAlpha,
            JsonDeserializerContext& context);
        JsonSerializationResult::Result LoadByteArray(Color& output, const rapidjson::Value& inputValue, bool loadAlpha,
            JsonDeserializerContext& context);
        JsonSerializationResult::Result LoadHexString(Color& output, const rapidjson::Value& inputValue, bool loadAlpha,
            JsonDeserializerContext& context);
        uint32_t ClampToByteColorRange(int64_t value) const;
    };
}

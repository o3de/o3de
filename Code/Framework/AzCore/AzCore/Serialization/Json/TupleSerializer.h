/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class JsonTupleSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonTupleSerializer, "{1AA0ADC1-395A-4223-8A73-304ACDEE7793}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

        OperationFlags GetOperationsFlags() const override;

    private:
        JsonSerializationResult::Result LoadContainer(
            void* outputValue,
            const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            bool isNewInstance,
            JsonDeserializerContext& context);
    };
} // namespace AZ

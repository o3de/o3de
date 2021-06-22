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

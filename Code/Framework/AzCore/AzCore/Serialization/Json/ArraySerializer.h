/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class JsonArraySerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonArraySerializer, "{4AD184DD-A068-46D4-B364-2DDBCAB697B1}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

        OperationFlags GetOperationsFlags() const override;

    protected:
        JsonSerializationResult::Result LoadContainer(
            void* outputValue,
            const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            bool isNewInstance,
            JsonDeserializerContext& context);
    };
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>

namespace AZ
{
    class JsonUnorderedSetContainerSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUnorderedSetContainerSerializer, "{4229622F-0F18-4A52-BB22-CEE3FD13A21D}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

    private:
        JsonBasicContainerSerializer m_baseSerializer;
    };
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/UnorderedSetSerializer.h>
#include <AzCore/std/sort.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnorderedSetContainerSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonUnorderedSetContainerSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        return m_baseSerializer.Load(outputValue, outputValueTypeId, inputValue, context);
    }

    JsonSerializationResult::Result JsonUnorderedSetContainerSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;
        
        JSR::Result result = m_baseSerializer.Store(outputValue, inputValue, defaultValue, valueTypeId, context);
        
        // Only sort if there's an array. If something went wrong or the set has been fully defaulted then the output won't be an array.
        if (outputValue.IsArray())
        {
            auto less = [](const rapidjson::Value& lhs, const rapidjson::Value& rhs) -> bool
            {
                return JsonSerialization::Compare(lhs, rhs) == JsonSerializerCompareResult::Less;
            };
            AZStd::sort(outputValue.Begin(), outputValue.End(), less);
        }

        return result;
    }
} // namespace AZ

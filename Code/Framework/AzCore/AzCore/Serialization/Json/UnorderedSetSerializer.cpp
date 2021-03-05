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

#include <algorithm>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/UnorderedSetSerializer.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnorderedSetContainerSerializer, SystemAllocator, 0);

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
            // Using std::sort because AZStd::sort isn't implemented in terms of move operations.
            auto less = [](const rapidjson::Value& lhs, const rapidjson::Value& rhs) -> bool
            {
                return JsonSerialization::Compare(lhs, rhs) == JsonSerializerCompareResult::Less;
            };
            std::sort(outputValue.Begin(), outputValue.End(), less);
        }

        return result;
    }
} // namespace AZ

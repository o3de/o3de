/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Serialization/Json/PathSerializer.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::JsonPathSerializerInternal
{
    template<typename PathType>
    static JsonSerializationResult::Result Load(PathType* pathValue, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(pathValue, "Expected a valid pointer to load from json value.");

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
        case rapidjson::kObjectType:
        case rapidjson::kFalseType:
        case rapidjson::kTrueType:
        case rapidjson::kNumberType:
            [[fallthrough]];
        case rapidjson::kNullType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. String values can't be read from arrays, objects or null.");
        case rapidjson::kStringType:
        {
            size_t pathLength = inputValue.GetStringLength();
            if (pathLength <= pathValue->Native().max_size())
            {
                *pathValue = PathType(AZStd::string_view(inputValue.GetString(), pathLength)).LexicallyNormal();
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read path.");
            }
            using ErrorString = AZStd::fixed_string<256>;
            return context.Report(JsonSerializationResult::Tasks::ReadField, JSR::Outcomes::Invalid,
                ErrorString::format("Json string value is too large to fit within path type %s. It needs to be less than %zu code points",
                    azrtti_typeid<PathType>().ToFixedString().c_str(), pathValue->Native().max_size()));
        }
        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for string value.");
        }
    }
    template<typename PathType>
    static JsonSerializationResult::Result StoreWithDefault(rapidjson::Value& outputValue, const PathType* pathValue,
        const PathType* defaultPathValue, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Removes name conflicts in AzCore in uber builds.

        if (context.ShouldKeepDefaults() || defaultPathValue == nullptr || *pathValue != *defaultPathValue)
        {
            auto posixPathString = pathValue->AsPosix();
            outputValue.SetString(posixPathString.c_str(), aznumeric_caster(posixPathString.size()), context.GetJsonAllocator());
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Path successfully stored.");
        }

        return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default Path used.");
    }
}

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonPathSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonPathSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        if (outputValueTypeId == azrtti_typeid<AZ::IO::Path>())
        {
            return JsonPathSerializerInternal::Load(reinterpret_cast<AZ::IO::Path*>(outputValue), inputValue,
                context);
        }
        else if (outputValueTypeId == azrtti_typeid<AZ::IO::FixedMaxPath>())
        {
            return JsonPathSerializerInternal::Load(reinterpret_cast<AZ::IO::FixedMaxPath*>(outputValue), inputValue,
                context);
        }

        auto errorTypeIdString = outputValueTypeId.ToFixedString();
        AZ_Assert(false, "Unable to serialize json string"
            " to a path of type %s", errorTypeIdString.c_str());

        using ErrorString = AZStd::fixed_string<256>;
        return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::TypeMismatch,
            ErrorString::format("Output value type ID %s is not a valid Path type", errorTypeIdString.c_str()));
    }

    JsonSerializationResult::Result JsonPathSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        if (valueTypeId == azrtti_typeid<AZ::IO::Path>())
        {
            return JsonPathSerializerInternal::StoreWithDefault(outputValue,
                reinterpret_cast<const AZ::IO::Path*>(inputValue),
                reinterpret_cast<const AZ::IO::Path*>(defaultValue), context);
        }
        else if (valueTypeId == azrtti_typeid<AZ::IO::FixedMaxPath>())
        {
            return JsonPathSerializerInternal::StoreWithDefault(outputValue,
                reinterpret_cast<const AZ::IO::FixedMaxPath*>(inputValue),
                reinterpret_cast<const AZ::IO::FixedMaxPath*>(defaultValue), context);
        }

        AZ_Assert(false, "Unable to serialize path type %s to a json string", valueTypeId.ToFixedString().c_str());
        using ErrorString = AZStd::fixed_string<256>;
        return context.Report(JsonSerializationResult::Tasks::WriteValue, JsonSerializationResult::Outcomes::TypeMismatch,
            ErrorString::format("Input value type ID %s is not a valid Path type", valueTypeId.ToFixedString().c_str()));
    }

} // namespace AZ

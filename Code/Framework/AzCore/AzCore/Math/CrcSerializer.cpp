/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/CrcSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonCrcSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonCrcSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Crc32>() == outputValueTypeId,
            "Unable to deserialize Crc to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        Crc32* Crc = reinterpret_cast<Crc32*>(outputValue);
        AZ_Assert(Crc, "Output value for JsonCrcSerializer can't be null.");

        if (IsExplicitDefault(inputValue))
        {
            *Crc = Crc32();
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Crc32 value set to default of zero.");
        }

        switch (inputValue.GetType())
        {
        case rapidjson::kObjectType:
            return LoadObject(*Crc, inputValue, context);
        case rapidjson::kNumberType:
            return LoadNumber(*Crc, inputValue, context);
        case rapidjson::kNullType:
            [[fallthrough]];
        case rapidjson::kStringType:
            [[fallthrough]];
        case rapidjson::kFalseType:
            [[fallthrough]];
        case rapidjson::kArrayType:
            [[fallthrough]];
        case rapidjson::kTrueType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. Crc32s can only be read from numbers, or objects");
        
        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                "Unknown json type encountered in Crc.");
        }
    }

    JsonSerializationResult::Result JsonCrcSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Crc32>() == valueTypeId, "Unable to serialize Crc to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const Crc32* Crc = reinterpret_cast<const Crc32*>(inputValue);
        AZ_Assert(Crc, "Input value for JsonCrcSerializer can't be null.");
        const Crc32* defaultCrc = reinterpret_cast<const Crc32*>(defaultValue);

        if (!context.ShouldKeepDefaults() && defaultCrc && *Crc == *defaultCrc)
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default Crc32 used.");
        }

        outputValue.SetUint(Crc->GetValue());
        return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Crc32 successfully stored.");
    }

    auto JsonCrcSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    JsonSerializationResult::Result JsonCrcSerializer::LoadObject(Crc32& output, const rapidjson::Value& inputValue, 
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        if (IsExplicitDefault(inputValue))
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                "Default value for Crc provided so no change was made.");
        }

        JSR::ResultCode result = JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported);
        auto idMember = inputValue.FindMember("Value");
        if (idMember != inputValue.MemberEnd())
        {
            if (idMember->value.IsNumber())
            {
                result = LoadNumber(output, idMember->value, context);
            }
        }
        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted ?
            "Successfully read Crc." : "Problem encountered while reading Crc.");
    }

    JsonSerializationResult::Result JsonCrcSerializer::LoadNumber(Crc32& output, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.
        
        JSR::ResultCode result(JSR::Tasks::ReadField);
        AZ::u32 crcValue = 0;
        result.Combine(ContinueLoading(&crcValue, azrtti_typeid<AZ::u32>(), inputValue, context));
        output = AZ::Crc32(crcValue);

        return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully loaded data AZ::Crc32."
                       : "Failed to load data into AZ::Crc32.");
    }
}

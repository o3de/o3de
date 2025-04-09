/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Name/NameJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(NameJsonSerializer, SystemAllocator);

    JsonSerializationResult::Result NameJsonSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Name>() == outputValueTypeId,
            "Unable to deserialize AZ::Name to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        Name* name = reinterpret_cast<Name*>(outputValue);
        AZ_Assert(name, "Output value for NameJsonSerializer can't be null.");

        AZStd::string value;
        JSR::ResultCode resultCode = ContinueLoading(&value, azrtti_typeid<AZStd::string>(), inputValue, context);
        *name = value; // If loading fails this will be an empty string.

        return context.Report(resultCode,
            resultCode.GetOutcome() == JSR::Outcomes::Success ? "Successfully loaded AZ::Name" : "Failed to load AZ::Name");
    }

    JsonSerializationResult::Result NameJsonSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Name>() == valueTypeId, "Unable to serialize AZ::Name to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const Name* name = reinterpret_cast<const Name*>(inputValue);
        AZ_Assert(name, "Input value for NameJsonSerializer can't be null.");
        const Name* defaultName = reinterpret_cast<const Name*>(defaultValue);

        if (!context.ShouldKeepDefaults() && defaultName && *name == *defaultName)
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default AZ::Name used.");
        }

        outputValue.SetString(name->GetStringView().data(), context.GetJsonAllocator());
        return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "AZ::Name successfully stored.");
    }
} // namespace AZ


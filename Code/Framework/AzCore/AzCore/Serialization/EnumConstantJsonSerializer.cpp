/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EnumConstantJsonSerializer.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ::EnumConstantJsonSerializerInternal
{
    constexpr const char* ValueField = "value";
    constexpr const char* DescriptionField = "description";
} // namespace AZ::EnumConstantSerializerInternal

namespace AZ
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(EnumConstantJsonSerializer, "EnumConstantJsonSerializer", "{A231A314-A4EB-4485-835F-A58A9C3C5F08}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(EnumConstantJsonSerializer, BaseJsonSerializer);
    AZ_CLASS_ALLOCATOR_IMPL(EnumConstantJsonSerializer, SystemAllocator);

    JsonSerializationResult::Result EnumConstantJsonSerializer::Load(
        void* outputValue, const Uuid&, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        JSR::ResultCode result(JSR::Tasks::ReadField);

        switch (inputValue.GetType())
        {
        case rapidjson::kObjectType:
        {
            auto enumConstant = reinterpret_cast<Edit::EnumConstantBase*>(outputValue);
            if (auto valueIt = inputValue.FindMember(EnumConstantJsonSerializerInternal::ValueField); valueIt != inputValue.MemberEnd())
            {
                using ValueType = decltype(enumConstant->m_value);
                result.Combine(ContinueLoading(&enumConstant->m_value, azrtti_typeid<ValueType>(), valueIt->value, context));

                if (result.GetProcessing() != JSR::Processing::Completed)
                {
                    result.Combine(context.Report(result, "Failed to read enum value for EnumConstant<EnumType>."));
                }
            }
            if (auto descIt = inputValue.FindMember(EnumConstantJsonSerializerInternal::DescriptionField); descIt != inputValue.MemberEnd())
            {
                using DescriptionType = decltype(enumConstant->m_description);
                result.Combine(ContinueLoading(&enumConstant->m_description, azrtti_typeid<DescriptionType>(), descIt->value, context));

                if (result.GetProcessing() != JSR::Processing::Completed)
                {
                    result.Combine(context.Report(result, "Failed to read enum description for EnumConstant<EnumType>."));
                }
            }

            return context.Report(
                result,
                result.GetOutcome() <= JSR::Outcomes::PartialSkip ? "Successfully loaded data into EnumConstant<EnumType>."
                                                                     : "Failed to load data into EnumConstant<EnumType>.");
        }
        case rapidjson::kArrayType: // fall through
        case rapidjson::kNullType: // fall through
        case rapidjson::kStringType: // fall through
        case rapidjson::kFalseType: // fall through
        case rapidjson::kTrueType: // fall through
        case rapidjson::kNumberType:
            return context.Report(
                JSR::Tasks::ReadField,
                JSR::Outcomes::Unsupported,
                "Unsupported type. EnumConstant<EnumType> can only be read from an object.");

        default:
            return context.Report(
                JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for EnumConstant<EnumType>.");
        }
    }

    JsonSerializationResult::Result EnumConstantJsonSerializer::Store(
        rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid&, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        JSR::ResultCode result(JSR::Tasks::WriteValue);

        auto enumConstant = reinterpret_cast<const Edit::EnumConstantBase*>(inputValue);
        auto defaultEnumConstant = reinterpret_cast<const Edit::EnumConstantBase*>(defaultValue);

        if (!outputValue.IsObject())
        {
            outputValue.SetObject();
        }

        using ValueType = decltype(enumConstant->m_value);
        using DescriptionType = decltype(enumConstant->m_description);

        auto* enumValue = &enumConstant->m_value;
        auto* defaultEnumValue = defaultEnumConstant != nullptr ? &defaultEnumConstant->m_value : nullptr;
        result.Combine(ContinueStoringToJsonObjectField(
            outputValue, rapidjson::StringRef(EnumConstantJsonSerializerInternal::ValueField), enumValue, defaultEnumValue, azrtti_typeid<ValueType>(), context));

        auto* enumDescription = &enumConstant->m_description;
        auto* defaultEnumDescription = defaultEnumConstant != nullptr ? &defaultEnumConstant->m_description : nullptr;
        result.Combine(ContinueStoringToJsonObjectField(
            outputValue,
            rapidjson::StringRef(EnumConstantJsonSerializerInternal::DescriptionField),
            enumDescription,
            defaultEnumDescription,
            azrtti_typeid<DescriptionType>(),
            context));

        return context.Report(
            result,
            result.GetProcessing() == JSR::Processing::Completed ? "EnumConstant<EnumType> stored successfully."
            : "Failed to store EnumConstant<EnumType>.");
    }
} // namespace AZ

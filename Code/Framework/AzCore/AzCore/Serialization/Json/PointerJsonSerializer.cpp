/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/PointerJsonSerializer.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/PointerObject.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/utility/charconv.h>

namespace AZ::PointerJsonSerializerInternal
{
    constexpr const char* AddressField = "$address";
    constexpr const char* TypeField = "$type";
} // namespace AZ::PointerJsonSerializerInternal

namespace AZ
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(PointerJsonSerializer, "PointerJsonSerializer", "{4DC82ED8-1963-4408-980A-E0A512A12EC6}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(PointerJsonSerializer, BaseJsonSerializer);
    AZ_CLASS_ALLOCATOR_IMPL(PointerJsonSerializer, SystemAllocator);

    JsonSerializationResult::Result PointerJsonSerializer::Load(
        void* outputValue, const Uuid&, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ::PointerObject localInstance;

        JSR::ResultCode result(JSR::Tasks::ReadField);

        const char* PointerObjectTypeName = AZ::AzTypeInfo<PointerObject>::Name();

        switch (inputValue.GetType())
        {
        case rapidjson::kObjectType:
            {
                auto pointerObject = reinterpret_cast<PointerObject*>(outputValue);
                if (auto addressIt = inputValue.FindMember(PointerJsonSerializerInternal::AddressField);
                    addressIt != inputValue.MemberEnd())
                {
                    // Try to read the pointer address as an integer from the Json field
                    uintptr_t pointerAddressIntPtr{};
                    JSR::ResultCode addressResult =
                        ContinueLoading(&pointerAddressIntPtr, azrtti_typeid<decltype(pointerAddressIntPtr)>(), addressIt->value, context);

                    if (addressResult.GetProcessing() != JSR::Processing::Completed)
                    {
                        // Fall back to trying to read the pointer address as a string and convert to a integer after reading
                        AZStd::string pointerAddressString;
                        addressResult = ContinueLoading(
                            &pointerAddressString, azrtti_typeid<decltype(pointerAddressString)>(), addressIt->value, context);
                        if (addressResult.GetProcessing() == JSR::Processing::Completed)
                        {
                            AZStd::from_chars_result stringToIntResult{};
                            AZStd::string_view pointerAddressView = pointerAddressString;
                            // Now try to convert the string into a uintptr_t
                            // std::does not deal with a leading "0x" in a hex string so an explicit check for is performed
                            // https://en.cppreference.com/w/cpp/utility/from_chars
                            constexpr AZStd::string_view HexPrefix = "0x";
                            // Using StringFunc::StartsWith to perform a case-insensitive search for the hex prefix. Both "0x" and "0X" are
                            // supported
                            if (AZ::StringFunc::StartsWith(pointerAddressView, HexPrefix))
                            {
                                // Skip pass the hex prefix from the pointer string
                                pointerAddressView = pointerAddressView.substr(HexPrefix.size());
                                constexpr int HexBase = 16;
                                stringToIntResult =
                                    AZStd::from_chars(pointerAddressView.begin(), pointerAddressView.end(), pointerAddressIntPtr, HexBase);
                            }
                            else
                            {
                                stringToIntResult =
                                    AZStd::from_chars(pointerAddressView.begin(), pointerAddressView.end(), pointerAddressIntPtr);
                            }

                            if (stringToIntResult.ec != AZStd::errc{})
                            {
                                addressResult.Combine(context.Report(
                                    JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Invalid),
                                    ReporterString::format("Cannot convert string %s to a memory address", pointerAddressString.c_str())));
                            }
                        }
                    }

                    result.Combine(addressResult);
                    if (result.GetProcessing() == JSR::Processing::Completed)
                    {
                        // If the value pointer address was successfully read, store it in the address member
                        localInstance.m_address = reinterpret_cast<void*>(pointerAddressIntPtr);
                    }
                    else
                    {
                        result.Combine(context.Report(
                            result, ReporterString::format("Failed to read pointer address for %s.", PointerObjectTypeName)));
                    }
                }
                else
                {
                    result.Combine(context.Report(
                        JSR::Tasks::ReadField,
                        JSR::Outcomes::Missing,
                        ReporterString::format(
                            R"(Field "%s" is required for reading a %s.)",
                            PointerJsonSerializerInternal::AddressField,
                            PointerObjectTypeName)));
                }
                if (auto typeIt = inputValue.FindMember(PointerJsonSerializerInternal::TypeField); typeIt != inputValue.MemberEnd())
                {
                    result.Combine(ContinueLoading(&localInstance.m_typeId, azrtti_typeid<AZ::TypeId>(), typeIt->value, context));

                    if (result.GetProcessing() != JSR::Processing::Completed)
                    {
                        result.Combine(
                            context.Report(result, ReporterString::format("Failed to read type id for %s.", PointerObjectTypeName)));
                    }
                }
                else
                {
                    result.Combine(context.Report(
                        JSR::Tasks::ReadField,
                        JSR::Outcomes::Missing,
                        ReporterString::format(
                            R"(Field "%s" is required for reading a %s.)",
                            PointerJsonSerializerInternal::TypeField,
                            PointerObjectTypeName)));
                }

                // All fields have been succesfully loaded into the local instance
                // so copy it over to the outputValue address
                if (result.GetProcessing() == JSR::Processing::Completed)
                {
                    *pointerObject = AZStd::move(localInstance);
                }

                return context.Report(
                    result,
                    result.GetOutcome() <= JSR::Outcomes::PartialSkip
                        ? ReporterString::format("Successfully loaded data into %s.", PointerObjectTypeName)
                        : ReporterString::format("Failed to load data into %s.", PointerObjectTypeName));
            }
        case rapidjson::kArrayType:
        case rapidjson::kNullType:
        case rapidjson::kStringType:
        case rapidjson::kFalseType:
        case rapidjson::kTrueType:
        case rapidjson::kNumberType:
            return context.Report(
                JSR::Tasks::ReadField,
                JSR::Outcomes::Invalid,
                ReporterString::format("Unsupported type. %s can only be read from an object.", PointerObjectTypeName));

        default:
            return context.Report(
                JSR::Tasks::ReadField,
                JSR::Outcomes::Unknown,
                ReporterString::format("Unknown json type encountered for %s.", PointerObjectTypeName));
        }
    }

    JsonSerializationResult::Result PointerJsonSerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        [[maybe_unused]] const void* defaultValue,
        const Uuid&,
        JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        JSR::ResultCode result(JSR::Tasks::WriteValue);

        const char* PointerObjectTypeName = AZ::AzTypeInfo<PointerObject>::Name();

        auto pointerObject = reinterpret_cast<const PointerObject*>(inputValue);

        if (!outputValue.IsObject())
        {
            outputValue.SetObject();
        }

        auto pointerAddressAsIntPtr = reinterpret_cast<uintptr_t>(pointerObject->m_address);
        result.Combine(ContinueStoringToJsonObjectField(
            outputValue,
            rapidjson::StringRef(PointerJsonSerializerInternal::AddressField),
            &pointerAddressAsIntPtr,
            nullptr,
            azrtti_typeid<uintptr_t>(),
            context));

        result.Combine(ContinueStoringToJsonObjectField(
            outputValue,
            rapidjson::StringRef(PointerJsonSerializerInternal::TypeField),
            &pointerObject->m_typeId,
            nullptr,
            azrtti_typeid<AZ::TypeId>(),
            context));

        return context.Report(
            result,
            result.GetProcessing() == JSR::Processing::Completed ? ReporterString::format("%s stored successfully.", PointerObjectTypeName)
                                                                 : ReporterString::format("Failed to store %s.", PointerObjectTypeName));
    }

    auto PointerJsonSerializer::GetOperationsFlags() const -> BaseJsonSerializer::OperationFlags
    {
        return BaseJsonSerializer::OperationFlags::ManualDefault;
    }
} // namespace AZ

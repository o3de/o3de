/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EntityPresets/EntityPresetsSerializer.h>

#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/string_view.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        namespace
        {
            //! Spelled in lower case, and short, because these appear in a file people edit.
            constexpr AZStd::string_view BoolTypeName = "bool";
            constexpr AZStd::string_view IntTypeName = "int";
            constexpr AZStd::string_view DoubleTypeName = "double";
            constexpr AZStd::string_view StringTypeName = "string";
            constexpr AZStd::string_view AssetTypeName = "asset";

            PropertyValue::Type TypeFromName(const AZStd::string_view name)
            {
                if (name == BoolTypeName)
                {
                    return PropertyValue::Type::Bool;
                }
                if (name == DoubleTypeName)
                {
                    return PropertyValue::Type::Double;
                }
                if (name == StringTypeName)
                {
                    return PropertyValue::Type::String;
                }
                if (name == AssetTypeName)
                {
                    return PropertyValue::Type::AssetPath;
                }

                // Int is the default for anything unrecognised, matching the member's own default.
                // A preset with a misspelled type is better read as an integer than dropped.
                return PropertyValue::Type::Int;
            }

            AZStd::string_view NameFromType(const PropertyValue::Type type)
            {
                switch (type)
                {
                case PropertyValue::Type::Bool:
                    return BoolTypeName;
                case PropertyValue::Type::Double:
                    return DoubleTypeName;
                case PropertyValue::Type::String:
                    return StringTypeName;
                case PropertyValue::Type::AssetPath:
                    return AssetTypeName;
                case PropertyValue::Type::Int:
                default:
                    return IntTypeName;
                }
            }

            //! Add a string member, copying the text - rapidjson does not own string literals passed
            //! to it by reference, and these outlive the call.
            void AddStringMember(
                rapidjson::Value& object,
                const AZStd::string_view name,
                const AZStd::string_view text,
                rapidjson::Document::AllocatorType& allocator)
            {
                object.AddMember(
                    rapidjson::Value(name.data(), aznumeric_caster(name.size()), allocator),
                    rapidjson::Value(text.data(), aznumeric_caster(text.size()), allocator),
                    allocator);
            }
        } // namespace

        AZ_CLASS_ALLOCATOR_IMPL(JsonPropertyAssignmentSerializer, AZ::SystemAllocator);

        AZ::JsonSerializationResult::Result JsonPropertyAssignmentSerializer::Load(
            void* outputValue,
            const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                outputValueTypeId == azrtti_typeid<PropertyAssignment>(),
                "JsonPropertyAssignmentSerializer was handed type %s.", outputValueTypeId.ToFixedString().c_str());
            AZ_UNUSED(outputValueTypeId);
            AZ_Assert(outputValue, "Expected a valid pointer to load into.");

            if (!inputValue.IsObject())
            {
                return context.Report(
                    JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "A property assignment has to be a JSON object with 'path', 'type' and 'value'.");
            }

            auto* assignment = reinterpret_cast<PropertyAssignment*>(outputValue);

            // Read leniently. A hand-edited file with a member missing should lose that member, not
            // the whole preset, and the caller drops assignments that end up with no path.
            if (const auto path = inputValue.FindMember("path");
                path != inputValue.MemberEnd() && path->value.IsString())
            {
                assignment->m_path =
                    AZStd::string(path->value.GetString(), path->value.GetStringLength());
            }

            PropertyValue& value = assignment->m_value;

            if (const auto type = inputValue.FindMember("type");
                type != inputValue.MemberEnd() && type->value.IsString())
            {
                value.m_type =
                    TypeFromName(AZStd::string_view(type->value.GetString(), type->value.GetStringLength()));
            }

            const auto member = inputValue.FindMember("value");
            if (member == inputValue.MemberEnd())
            {
                return context.Report(
                    JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                    "Property assignment has no 'value'; the property's default will be used.");
            }

            const rapidjson::Value& raw = member->value;

            switch (value.m_type)
            {
            case PropertyValue::Type::Bool:
                if (!raw.IsBool())
                {
                    return context.Report(
                        JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Expected a boolean 'value'.");
                }
                value.m_bool = raw.GetBool();
                break;

            case PropertyValue::Type::Double:
                if (!raw.IsNumber())
                {
                    return context.Report(
                        JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Expected a numeric 'value'.");
                }
                value.m_double = raw.GetDouble();
                break;

            case PropertyValue::Type::String:
            case PropertyValue::Type::AssetPath:
                if (!raw.IsString())
                {
                    return context.Report(
                        JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Expected a string 'value'.");
                }
                value.m_string = AZStd::string(raw.GetString(), raw.GetStringLength());
                break;

            case PropertyValue::Type::Int:
            default:
                if (!raw.IsNumber())
                {
                    return context.Report(
                        JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Expected a numeric 'value'.");
                }

                // Through double when the number is not stored as an integer, so a value written as
                // 5.0 by a hand-edit or another tool still reads as 5 rather than as zero.
                value.m_int = raw.IsInt64() ? raw.GetInt64() : aznumeric_cast<AZ::s64>(raw.GetDouble());
                break;
            }

            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Read property assignment.");
        }

        AZ::JsonSerializationResult::Result JsonPropertyAssignmentSerializer::Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* /*defaultValue*/,
            const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                valueTypeId == azrtti_typeid<PropertyAssignment>(),
                "JsonPropertyAssignmentSerializer was handed type %s.", valueTypeId.ToFixedString().c_str());
            AZ_UNUSED(valueTypeId);
            AZ_Assert(inputValue, "Expected a valid pointer to store from.");

            const auto* assignment = reinterpret_cast<const PropertyAssignment*>(inputValue);
            const PropertyValue& value = assignment->m_value;

            auto& allocator = context.GetJsonAllocator();

            // Every member is written unconditionally, defaults included. Skipping defaults is the
            // usual convention, but this file is read by people: an assignment missing its type
            // because the type happened to be the default one would be confusing to edit.
            outputValue.SetObject();

            AddStringMember(outputValue, "path", assignment->m_path, allocator);
            AddStringMember(outputValue, "type", NameFromType(value.m_type), allocator);

            switch (value.m_type)
            {
            case PropertyValue::Type::Bool:
                outputValue.AddMember(rapidjson::StringRef("value"), value.m_bool, allocator);
                break;

            case PropertyValue::Type::Double:
                outputValue.AddMember(rapidjson::StringRef("value"), value.m_double, allocator);
                break;

            case PropertyValue::Type::String:
            case PropertyValue::Type::AssetPath:
                AddStringMember(outputValue, "value", value.m_string, allocator);
                break;

            case PropertyValue::Type::Int:
            default:
                outputValue.AddMember(rapidjson::StringRef("value"), static_cast<int64_t>(value.m_int), allocator);
                break;
            }

            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Wrote property assignment.");
        }
    } // namespace EntityPresets
} // namespace AzToolsFramework

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/ConfigurableStack.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/tuple.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonConfigurableStackSerializer, AZ::SystemAllocator);

    JsonSerializationResult::Result JsonConfigurableStackSerializer::Load(
        void* outputValue,
        [[maybe_unused]] const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadFromArray(outputValue, inputValue, context);
        case rapidjson::kObjectType:
            return LoadFromObject(outputValue, inputValue, context);

        case rapidjson::kNullType:
            [[fallthrough]];
        case rapidjson::kFalseType:
            [[fallthrough]];
        case rapidjson::kTrueType:
            [[fallthrough]];
        case rapidjson::kStringType:
            [[fallthrough]];
        case rapidjson::kNumberType:
            return context.Report(
                JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. Configurable stack values can only be read from arrays or objects.");
        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for string value.");
        }
    }

    JsonSerializationResult::Result JsonConfigurableStackSerializer::Store(
        [[maybe_unused]] rapidjson::Value& outputValue,
        [[maybe_unused]] const void* inputValue,
        [[maybe_unused]] const void* defaultValue,
        [[maybe_unused]] const Uuid& valueTypeId,
        JsonSerializerContext& context)
    {
        return context.Report(
            JsonSerializationResult::Tasks::WriteValue, JsonSerializationResult::Outcomes::Unsupported,
            "Configuration stacks can not be written out.");
    }

    void JsonConfigurableStackSerializer::Reflect(ReflectContext* context)
    {
        if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonConfigurableStackSerializer>()->HandlesType<ConfigurableStack>();
        }
    }

    JsonSerializationResult::Result JsonConfigurableStackSerializer::LoadFromArray(
        void* outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        auto stack = reinterpret_cast<ConfigurableStackInterface*>(outputValue);
        const Uuid& nodeValueType = stack->GetNodeType();

        JSR::ResultCode result(JSR::Tasks::ReadField);
        uint32_t counter = 0;
        for (auto& it : inputValue.GetArray())
        {
            ScopedContextPath subPath(context, counter);
            void* value = stack->AddNode(AZStd::to_string(counter));
            result.Combine(ContinueLoading(value, nodeValueType, it, context));
            counter++;
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Loaded configurable stack from array."
                                                              : "Failed to load configurable stack from array.");
    }

    JsonSerializationResult::Result JsonConfigurableStackSerializer::LoadFromObject(
        void* outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        auto stack = reinterpret_cast<ConfigurableStackInterface*>(outputValue);
        const Uuid& nodeValueType = stack->GetNodeType();

        AZStd::queue<AZStd::tuple<ConfigurableStackInterface::InsertPosition, AZStd::string_view, rapidjson::Value::ConstMemberIterator>>
            delayedEntries;
        JSR::ResultCode result(JSR::Tasks::ReadField);
        
        auto add = [&](ConfigurableStackInterface::InsertPosition position, rapidjson::Value::ConstMemberIterator target,
                       rapidjson::Value::ConstMemberIterator it)
        {
            if (target->value.IsString())
            {
                delayedEntries.emplace(position, AZStd::string_view(target->value.GetString(), target->value.GetStringLength()), it);
            }
            else
            {
                result.Combine(context.Report(
                    JSR::Tasks::ReadField, JSR::Outcomes::Skipped,
                    "Skipped value for the Configurable Stack because the target wasn't a string."));
            }
        };

        // Load all the regular entries into the stack and store any with a before or after binding for
        // later inserting.
        for (auto it = inputValue.MemberBegin(); it != inputValue.MemberEnd(); ++it)
        {
            AZStd::string_view name(it->name.GetString(), it->name.GetStringLength());
            ScopedContextPath subPath(context, name);
            if (it->value.IsObject())
            {
                if (auto target = it->value.FindMember(StackBefore); target != it->value.MemberEnd())
                {
                    add(ConfigurableStackInterface::InsertPosition::Before, target, it);
                    continue;
                }
                if (auto target = it->value.FindMember(StackAfter); target != it->value.MemberEnd())
                {
                    add(ConfigurableStackInterface::InsertPosition::After, target, it);
                    continue;
                }
            }

            void* value = stack->AddNode(name);
            result.Combine(ContinueLoading(value, nodeValueType, it->value, context));
        }

        // Insert the entries that have been delayed.
        while (!delayedEntries.empty())
        {
            auto&& [insertPosition, target, valueStore] = delayedEntries.front();
            AZStd::string_view name(valueStore->name.GetString(), valueStore->name.GetStringLength());
            ScopedContextPath subPath(context, name);
            void* value = stack->AddNode(name, target, insertPosition);
            if (value != nullptr)
            {
                result.Combine(ContinueLoading(value, nodeValueType, valueStore->value, context));
            }
            else
            {
                result.Combine(context.Report(
                    JSR::Tasks::ReadField, JSR::Outcomes::Skipped,
                    AZStd::string::format(
                        "Skipped value for the Configurable Stack because the target '%.*s' couldn't be found.", AZ_STRING_ARG(name))));
            }
            delayedEntries.pop();
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Loaded configurable stack from array."
                                                              : "Failed to load configurable stack from array.");
    }
} // namespace AZ

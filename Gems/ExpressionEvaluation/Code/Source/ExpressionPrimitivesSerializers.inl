/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/AZStdAnyDataContainer.inl>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ExpressionEvaluation/ExpressionEngine/ExpressionTree.h>

namespace AZ
{
    class ExpressionTreeVariableDescriptorSerializer
        : public BaseJsonSerializer
    {

    public:
        AZ_RTTI(ExpressionTreeVariableDescriptorSerializer, "{5EFF37D6-BD54-45C6-9FC6-B1E0D3A8204C}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    private:
        using VariableDescriptor = ExpressionEvaluation::ExpressionTree::VariableDescriptor;

        static constexpr AZStd::string_view EmptyAnyIdentifier = "Empty AZStd::any";

        static bool IsEmptyAny(const rapidjson::Value& typeId)
        {
            if (typeId.IsString())
            {
                AZStd::string_view typeName(typeId.GetString(), typeId.GetStringLength());
                return typeName == EmptyAnyIdentifier;
            }

            return false;
        }

        JsonSerializationResult::Result Load
            ( void* outputValue
            , [[maybe_unused]] const Uuid& outputValueTypeId
            , const rapidjson::Value& inputValue
            , JsonDeserializerContext& context) override
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(outputValueTypeId == azrtti_typeid<VariableDescriptor>(), "ExpressionTreeVariableDescriptorSerializer Load against "
                "output typeID that was not VariableDescriptor");
            AZ_Assert(outputValue, "ExpressionTreeVariableDescriptorSerializer Load against null output");

            JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
            auto outputDatum = reinterpret_cast<VariableDescriptor*>(outputValue);

            result.Combine(ContinueLoadingFromJsonObjectField
                ( &outputDatum->m_supportedTypes
                , azrtti_typeid<decltype(outputDatum->m_supportedTypes)>()
                , inputValue
                , "SupportedTypes"
                , context));

            // any storage begin
            AZ::Uuid typeId = AZ::Uuid::CreateNull();
            auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
            if (typeIdMember == inputValue.MemberEnd())
            {
                return context.Report
                    ( JSR::Tasks::ReadField
                    , JSR::Outcomes::Missing
                    , AZStd::string::format("ExpressionTreeVariableDescriptorSerializer::Load failed to load the %s member"
                        , JsonSerialization::TypeIdFieldIdentifier));
            }

            if (!IsEmptyAny(typeIdMember->value))
            {
                result.Combine(LoadTypeId(typeId, typeIdMember->value, context));
                if (typeId.IsNull())
                {
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic
                        , "ExpressionTreeVariableDescriptorSerializer::Load failed to load the AZ TypeId of the value");
                }

                AZStd::any storage = context.GetSerializeContext()->CreateAny(typeId);
                if (storage.empty() || storage.type() != typeId)
                {
                    return context.Report(result, "ExpressionTreeVariableDescriptorSerializer::Load failed to load a value matched the "
                        "reported AZ TypeId. The C++ declaration may have been deleted or changed.");
                }

                result.Combine(ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(&storage), typeId, inputValue, "Value", context));
                outputDatum->m_value = storage;
            }
            // any storage end

            return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
                ? "ExpressionTreeVariableDescriptorSerializer Load finished loading VariableDescriptor"
                : "ExpressionTreeVariableDescriptorSerializer Load failed to load VariableDescriptor");
        }

        JsonSerializationResult::Result Store
            ( rapidjson::Value& outputValue
            , const void* inputValue
            , const void* defaultValue
            , [[maybe_unused]] const Uuid& valueTypeId
            , JsonSerializerContext& context) override
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(valueTypeId == azrtti_typeid<VariableDescriptor>(), "VariableDescriptor Store against value typeID that "
                "was not VariableDescriptor");
            AZ_Assert(inputValue, "VariableDescriptor Store against null inputValue pointer ");

            auto inputScriptDataPtr = reinterpret_cast<const VariableDescriptor*>(inputValue);
            auto defaultScriptDataPtr = reinterpret_cast<const VariableDescriptor*>(defaultValue);

             if (defaultScriptDataPtr)
             {
                 if (inputScriptDataPtr->m_supportedTypes == defaultScriptDataPtr->m_supportedTypes
                     && AZ::Helpers::CompareAnyValue(inputScriptDataPtr->m_value, defaultScriptDataPtr->m_value))
                 {
                     return context.Report
                             ( JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "VariableDescriptor Store used defaults for "
                                "VariableDescriptor");
                 }
             }

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            outputValue.SetObject();

            result.Combine(ContinueStoringToJsonObjectField
                ( outputValue
                , "SupportedTypes"
                , &inputScriptDataPtr->m_supportedTypes
                , defaultScriptDataPtr ? &defaultScriptDataPtr->m_supportedTypes : nullptr
                , azrtti_typeid<decltype(inputScriptDataPtr->m_supportedTypes)>()
                , context));

            if (!inputScriptDataPtr->m_value.empty())
            {
                rapidjson::Value typeValue;
                result.Combine(StoreTypeId(typeValue, inputScriptDataPtr->m_value.type(), context));
                outputValue.AddMember
                    ( rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier)
                    , AZStd::move(typeValue)
                    , context.GetJsonAllocator());

                result.Combine(ContinueStoringToJsonObjectField
                    ( outputValue
                    , "Value"
                    , AZStd::any_cast<void>(const_cast<AZStd::any*>(&inputScriptDataPtr->m_value))
                    , defaultScriptDataPtr ? AZStd::any_cast<void>(const_cast<AZStd::any*>(&defaultScriptDataPtr->m_value)) : nullptr
                    , inputScriptDataPtr->m_value.type()
                    , context));
            }
            else
            {
                rapidjson::Value emptyAny;
                emptyAny.SetString(EmptyAnyIdentifier.data(), aznumeric_caster(EmptyAnyIdentifier.size()), context.GetJsonAllocator());
                outputValue.AddMember
                    ( rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier)
                    , AZStd::move(emptyAny)
                    , context.GetJsonAllocator());
            }

            return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
                ? "VariableDescriptor Store finished saving VariableDescriptor"
                : "VariableDescriptor Store failed to save VariableDescriptor");
        }
    };

    AZ_CLASS_ALLOCATOR_IMPL(ExpressionTreeVariableDescriptorSerializer, SystemAllocator);
}

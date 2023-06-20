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
#include <ExpressionEvaluation/ExpressionEngine/ExpressionTypes.h>

namespace AZ
{
    class ElementInformationSerializer
        : public BaseJsonSerializer
    {

    public:
        AZ_RTTI(ElementInformationSerializer, "{B33E6AA9-C700-4E3D-857C-55F362AFE57A}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    private:
        using ElementInformation = ExpressionEvaluation::ElementInformation;

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

            AZ_Assert(outputValueTypeId == azrtti_typeid<ElementInformation>(), "ElementInformationSerializer Load against "
                "output typeID that was not ElementInformation");
            AZ_Assert(outputValue, "ElementInformationSerializer Load against null output");

            JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
            auto outputDatum = reinterpret_cast<ElementInformation*>(outputValue);
            result.Combine(ContinueLoadingFromJsonObjectField
                ( &outputDatum->m_id
                , azrtti_typeid<decltype(outputDatum->m_id)>()
                , inputValue
                , "Id"
                , context));

            // any storage begin
            {
                AZ::Uuid typeId = AZ::Uuid::CreateNull();
                auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
                if (typeIdMember == inputValue.MemberEnd())
                {
                    return context.Report
                        ( JSR::Tasks::ReadField
                        , JSR::Outcomes::Missing
                        , AZStd::string::format("ElementInformationSerializer::Load failed to load the %s member"
                            , JsonSerialization::TypeIdFieldIdentifier));
                }

                if (!IsEmptyAny(typeIdMember->value))
                {
                    result.Combine(LoadTypeId(typeId, typeIdMember->value, context));
                    if (typeId.IsNull())
                    {
                        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic
                            , "ElementInformationSerializer::Load failed to load the AZ TypeId of the value");
                    }

                    AZStd::any storage = context.GetSerializeContext()->CreateAny(typeId);
                    if (storage.empty() || storage.type() != typeId)
                    {
                        return context.Report(result, "ElementInformationSerializer::Load failed to load a value matched the "
                            "reported AZ TypeId. The C++ declaration may have been deleted or changed.");
                    }

                    result.Combine
                        ( ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(&storage), typeId, inputValue, "Value", context));
                    outputDatum->m_extraStore = storage;
                }
            }
            // any storage end

            return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
                ? "ElementInformationSerializer Load finished loading ElementInformation"
                : "ElementInformationSerializer Load failed to load ElementInformation");
        }

        JsonSerializationResult::Result Store
            ( rapidjson::Value& outputValue
            , const void* inputValue
            , const void* defaultValue
            , [[maybe_unused]] const Uuid& valueTypeId
            , JsonSerializerContext& context) override
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(valueTypeId == azrtti_typeid<ElementInformation>(), "ElementInformation Store against value typeID that "
                "was not ElementInformation");
            AZ_Assert(inputValue, "ElementInformation Store against null inputValue pointer ");

            auto inputScriptDataPtr = reinterpret_cast<const ElementInformation*>(inputValue);
            auto defaultScriptDataPtr = reinterpret_cast<const ElementInformation*>(defaultValue);

             if (defaultScriptDataPtr)
             {
                 if (inputScriptDataPtr->m_id == defaultScriptDataPtr->m_id
                 && AZ::Helpers::CompareAnyValue(inputScriptDataPtr->m_extraStore, defaultScriptDataPtr->m_extraStore))
                 {
                     return context.Report
                             ( JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "ElementInformation Store used defaults for "
                                "ElementInformation");
                 }
             }

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            outputValue.SetObject();

            result.Combine(ContinueStoringToJsonObjectField
                ( outputValue
                , "Id"
                , &inputScriptDataPtr->m_id
                , defaultScriptDataPtr ? &defaultScriptDataPtr->m_id : nullptr
                , azrtti_typeid<decltype(inputScriptDataPtr->m_id)>()
                , context));

            if (!inputScriptDataPtr->m_extraStore.empty())
            {
                rapidjson::Value typeValue;
                result.Combine(StoreTypeId(typeValue, inputScriptDataPtr->m_extraStore.type(), context));
                outputValue.AddMember
                    ( rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier)
                    , AZStd::move(typeValue)
                    , context.GetJsonAllocator());

                result.Combine(ContinueStoringToJsonObjectField
                    ( outputValue
                    , "Value"
                    , AZStd::any_cast<void>(const_cast<AZStd::any*>(&inputScriptDataPtr->m_extraStore))
                    , defaultScriptDataPtr ? AZStd::any_cast<void>(const_cast<AZStd::any*>(&defaultScriptDataPtr->m_extraStore)) : nullptr
                    , inputScriptDataPtr->m_extraStore.type()
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
                ? "ElementInformation Store finished saving ElementInformation"
                : "ElementInformation Store failed to save ElementInformation");
        }
    };

    AZ_CLASS_ALLOCATOR_IMPL(ElementInformationSerializer, SystemAllocator);
}

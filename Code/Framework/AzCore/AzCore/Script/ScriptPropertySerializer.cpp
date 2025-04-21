/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptPropertySerializer.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(ScriptPropertySerializer, SystemAllocator);

    JsonSerializationResult::Result ScriptPropertySerializer::Load
        ( void* outputValue
        , [[maybe_unused]] const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(outputValueTypeId == azrtti_typeid<DynamicSerializableField>(), "ScriptPropertySerializer Load against output typeID that was not DynamicSerializableField");
        AZ_Assert(outputValue, "ScriptPropertySerializer Load against null output");

        auto outputVariable = reinterpret_cast<DynamicSerializableField*>(outputValue);
        JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
        AZ::Uuid typeId = AZ::Uuid::CreateNull();

        auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
        if (typeIdMember == inputValue.MemberEnd())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, AZStd::string::format("ScriptPropertySerializer::Load failed to load the %s member", JsonSerialization::TypeIdFieldIdentifier));
        }

        result.Combine(LoadTypeId(typeId, typeIdMember->value, context));
        if (typeId.IsNull())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "ScriptPropertySerializer::Load failed to load the AZ TypeId of the value");
        }

        AZStd::any storage = context.GetSerializeContext()->CreateAny(typeId);
        if (storage.empty() || storage.type() != typeId)
        {
            return context.Report(result, "ScriptPropertySerializer::Load failed to load a value matched the reported AZ TypeId. The C++ declaration may have been deleted or changed.");
        }

        DynamicSerializableField storageField;
        storageField.m_data = AZStd::any_cast<void>(&storage);
        storageField.m_typeId = typeId;
        outputVariable->CopyDataFrom(storageField, context.GetSerializeContext());

        result.Combine(ContinueLoadingFromJsonObjectField(outputVariable->m_data, typeId, inputValue, "value", context));
        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "ScriptPropertySerializer Load finished loading DynamicSerializableField"
            : "ScriptPropertySerializer Load failed to load DynamicSerializableField");
    }

    JsonSerializationResult::Result ScriptPropertySerializer::Store
        ( rapidjson::Value& outputValue
        , const void* inputValue
        , const void* defaultValue
        , [[maybe_unused]] const Uuid& valueTypeId
        , JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(valueTypeId == azrtti_typeid<DynamicSerializableField>(), "DynamicSerializableField Store against value typeID that was not DynamicSerializableField");
        AZ_Assert(inputValue, "DynamicSerializableField Store against null inputValue pointer ");

        auto inputScriptDataPtr = reinterpret_cast<const DynamicSerializableField*>(inputValue);
        auto inputFieldPtr = inputScriptDataPtr->m_data;
        auto defaultScriptDataPtr = reinterpret_cast<const DynamicSerializableField*>(defaultValue);
        auto defaultFieldPtr = defaultScriptDataPtr ? &defaultScriptDataPtr->m_data : nullptr;

        if (defaultScriptDataPtr && inputScriptDataPtr->IsEqualTo(*defaultScriptDataPtr, context.GetSerializeContext()))
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "ScriptPropertySerializer Store used defaults for DynamicSerializableField");
        }

        if (defaultScriptDataPtr && defaultScriptDataPtr->m_typeId != inputScriptDataPtr->m_typeId)
        {
            // Edge Case here, where the stored types don't match:
            // example:
            // inputScriptdataPtr has
            //   m_value{ m_data = 0x000012d2be18d820 m_typeId = 0xA96A5037 - 0xAD0D43B6 - 0x9948ED63 - 0x438C4A52 } AZ::DynamicSerializableField
            //
            // defaultScriptDataPtr has
            //   m_value{ m_data = 0x0000000000000000 m_typeId = 0x00000000 - 0x00000000 - 0x00000000 - 0x00000000 } AZ::DynamicSerializableField
            // 
            // if the script data and the default data hold a different type entirely from each other, you cannot pass the default
            // down into any further JSON functions, because all of those functions (Store, ContinueStoring, etc)
            // only take 3 params related to these objects - namely,
            //    *  the void* thing to store
            //    *  typeid of thing to store
            //    *  optional void* of default value, ASSUMED TO BE THE SAME TYPE
            defaultFieldPtr = nullptr;
        }

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        outputValue.SetObject();

        {
            rapidjson::Value typeValue;
            result.Combine(StoreTypeId(typeValue, inputScriptDataPtr->m_typeId, context));
            outputValue.AddMember(rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier), AZStd::move(typeValue), context.GetJsonAllocator());
        }

        result.Combine(ContinueStoringToJsonObjectField(outputValue, "value", inputFieldPtr, defaultFieldPtr, inputScriptDataPtr->m_typeId, context));

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "ScriptPropertySerializer Store finished saving DynamicSerializableField"
            : "ScriptPropertySerializer Store failed to save DynamicSerializableField");
    }

}

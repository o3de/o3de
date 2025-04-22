/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Serialization/RuntimeVariableSerializer.h>

using namespace ScriptCanvas;

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(RuntimeVariableSerializer, SystemAllocator);

    JsonSerializationResult::Result RuntimeVariableSerializer::Load
        ( void* outputValue
        , [[maybe_unused]] const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(outputValueTypeId == azrtti_typeid<RuntimeVariable>(), "RuntimeVariableSerializer Load against output typeID that was not RuntimeVariable");
        AZ_Assert(outputValue, "RuntimeVariableSerializer Load against null output");

        auto outputVariable = reinterpret_cast<RuntimeVariable*>(outputValue);
        JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
        AZ::Uuid typeId = AZ::Uuid::CreateNull();

        auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
        if (typeIdMember == inputValue.MemberEnd())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, AZStd::string::format("RuntimeVariableSerializer::Load failed to load the %s member", JsonSerialization::TypeIdFieldIdentifier));
        }

        result.Combine(LoadTypeId(typeId, typeIdMember->value, context));
        if (typeId.IsNull())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "RuntimeVariableSerializer::Load failed to load the AZ TypeId of the value");
        }

        outputVariable->value = context.GetSerializeContext()->CreateAny(typeId);
        if (outputVariable->value.empty() || outputVariable->value.type() != typeId)
        {
            return context.Report(result, "RuntimeVariableSerializer::Load failed to load a value matched the reported AZ TypeId. The C++ declaration may have been deleted or changed.");
        }

        result.Combine(ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(&outputVariable->value), typeId, inputValue, "value", context));

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "RuntimeVariableSerializer Load finished loading RuntimeVariable"
            : "RuntimeVariableSerializer Load failed to load RuntimeVariable");
    }

    JsonSerializationResult::Result RuntimeVariableSerializer::Store
        ( rapidjson::Value& outputValue
        , const void* inputValue
        , const void* defaultValue
        , [[maybe_unused]] const Uuid& valueTypeId
        , JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(valueTypeId == azrtti_typeid<RuntimeVariable>(), "RuntimeVariable Store against value typeID that was not RuntimeVariable");
        AZ_Assert(inputValue, "RuntimeVariable Store against null inputValue pointer ");

        auto inputScriptDataPtr = reinterpret_cast<const RuntimeVariable*>(inputValue);
        auto defaultScriptDataPtr = reinterpret_cast<const RuntimeVariable*>(defaultValue);
        auto inputAnyPtr = &inputScriptDataPtr->value;
        auto defaultAnyPtr = defaultScriptDataPtr ? &defaultScriptDataPtr->value : nullptr;

        if (defaultAnyPtr)
        {
            ScriptCanvas::Datum inputDatum(ScriptCanvas::Data::FromAZType(inputAnyPtr->type()), ScriptCanvas::Datum::eOriginality::Copy, AZStd::any_cast<void>(inputAnyPtr), inputAnyPtr->type());
            ScriptCanvas::Datum defaultDatum(ScriptCanvas::Data::FromAZType(defaultAnyPtr->type()), ScriptCanvas::Datum::eOriginality::Copy, AZStd::any_cast<void>(defaultAnyPtr), defaultAnyPtr->type());

            if (inputDatum == defaultDatum)
            {
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "RuntimeVariableSerializer Store used defaults for RuntimeVariable");
            }
        }
        
        JSR::ResultCode result(JSR::Tasks::WriteValue);
        outputValue.SetObject();

        {
            rapidjson::Value typeValue;
            result.Combine(StoreTypeId(typeValue, inputAnyPtr->type(), context));
            outputValue.AddMember(rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier), AZStd::move(typeValue), context.GetJsonAllocator());
        }
        
        result.Combine(ContinueStoringToJsonObjectField(outputValue, "value", AZStd::any_cast<void>(inputAnyPtr), AZStd::any_cast<void>(defaultAnyPtr), inputAnyPtr->type(), context));

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "RuntimeVariableSerializer Store finished saving RuntimeVariable"
            : "RuntimeVariableSerializer Store failed to save RuntimeVariable");
    }

}

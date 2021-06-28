/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptUserDataSerializer.h"

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>

using namespace ScriptCanvas;

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(ScriptUserDataSerializer, SystemAllocator, 0);

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // JsonMaterialAssignmentSerializer as an example!!! 
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    JsonSerializationResult::Result ScriptUserDataSerializer::Load
        ( void* outputValue
        , const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValueTypeId == azrtti_typeid<RuntimeVariable>(), "ScriptUserDataSerializer Load against output typeID that was not RuntimeVariable");
        AZ_Assert(outputValue, "ScriptUserDataSerializer Load against null output");

        auto outputVariable = reinterpret_cast<RuntimeVariable*>(outputValue);
        JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
        AZ::Uuid typeId = AZ::Uuid::CreateNull();

        result.Combine(LoadTypeId(typeId, inputValue, context));
        if (typeId.IsNull())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "ScriptUserDataSerializer::Load failed to load the AZ TypeId of the value");
        }

        outputVariable->value = context.GetSerializeContext()->CreateAny(typeId);
        if (outputVariable->value.empty() || outputVariable->value.type() != typeId)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "ScriptUserDataSerializer::Load failed to load a value matched the reported AZ TypeId. The C++ declaration may have been deleted or changed.");
        }

        result.Combine(ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(&outputVariable->value) , typeId, inputValue, "value", context));
        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "ScriptUserDataSerializer Store finished loading RuntimeVariable"
            : "ScriptUserDataSerializer Store failed to load RuntimeVariable");
    }

    JsonSerializationResult::Result ScriptUserDataSerializer::Store
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
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "ScriptUserDataSerializer Store used defaults for RuntimeVariable");
            }
        }
        
        outputValue.SetObject();
        JSR::ResultCode result(JSR::Tasks::WriteValue);

        {
            rapidjson::Value typeValue;
            result.Combine(StoreTypeId(typeValue, inputAnyPtr->type(), context));
            outputValue.AddMember("$type", typeValue, context.GetJsonAllocator());
        }

        {
            rapidjson::Value valueValue;
            valueValue.SetObject();
            result.Combine(ContinueStoringToJsonObjectField(valueValue, "value", AZStd::any_cast<void>(inputAnyPtr), AZStd::any_cast<void>(defaultAnyPtr), inputAnyPtr->type(), context));
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "ScriptUserDataSerializer Store finished saving RuntimeVariable"
            : "ScriptUserDataSerializer Store failed to save RuntimeVariable");
    }

}

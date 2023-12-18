/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            constexpr const char TypeField[] = "type";
            constexpr const char ArgsField[] = "args";
        }

        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialFunctorSourceDataSerializer, SystemAllocator);

        JsonSerializationResult::Result JsonMaterialFunctorSourceDataSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialFunctorSourceDataHolder>() == outputValueTypeId,
                "Unable to deserialize material functor to json because the provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(outputValueTypeId);

            MaterialFunctorSourceDataHolder* functorHolder = reinterpret_cast<MaterialFunctorSourceDataHolder*>(outputValue);
            AZ_Assert(functorHolder, "Output value for JsonMaterialFunctorSourceDataSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);

            if (!inputValue.IsObject())
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Material functor data must be a JSON object.");
            }

            Uuid functorTypeId;
            if (!inputValue.HasMember(TypeField))
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Functor type name is not specified.");
            }

            // Load the name first and find the type.
            AZStd::string functorName;
            result.Combine(ContinueLoadingFromJsonObjectField(&functorName, azrtti_typeid<AZStd::string>(), inputValue, TypeField, context));
            functorTypeId = MaterialFunctorSourceDataRegistration::Get()->FindMaterialFunctorTypeIdByName(functorName);
            if (functorTypeId.IsNull())
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Functor type name is not registered.");
            }

            // Create the actual source data of the functor.
            const SerializeContext::ClassData* actualClassData = context.GetSerializeContext()->FindClassData(functorTypeId);
            if (actualClassData)
            {
                void* instance = actualClassData->m_factory->Create(actualClassData->m_name);
                if (inputValue.HasMember(ArgsField))
                {
                    result.Combine(ContinueLoading(instance, functorTypeId, inputValue[ArgsField], context));
                }
                else
                {
                    result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed));
                }
                functorHolder->m_actualSourceData = reinterpret_cast<MaterialFunctorSourceData*>(instance);
            }
            else
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Class data is not registered in the SerializeContext.");
            }

            return context.Report(result, "Successfully processed MaterialFunctorSourceData.");
        }


        JsonSerializationResult::Result JsonMaterialFunctorSourceDataSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            [[maybe_unused]] const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialFunctorSourceDataHolder>() == valueTypeId,
                "Unable to serialize material functor to json because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(valueTypeId);

            JSR::ResultCode result(JSR::Tasks::WriteValue);

            outputValue.SetObject();

            const MaterialFunctorSourceDataHolder* functorHolder = reinterpret_cast<const MaterialFunctorSourceDataHolder*>(inputValue);

            if (!functorHolder->m_actualSourceData)
            {
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Unsupported, "No actual functor source data lives in this holder.");
            }

            Uuid functorTypeId = functorHolder->m_actualSourceData->RTTI_GetType();
            AZStd::string functorName = MaterialFunctorSourceDataRegistration::Get()->FindMaterialFunctorNameByTypeId(functorTypeId);
            if (functorName.empty())
            {
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Unsupported, "Functor name is not registered.");
            }

            const AZStd::string emptyString;
            result.Combine(ContinueStoringToJsonObjectField(outputValue, TypeField, &functorName, &emptyString, azrtti_typeid<AZStd::string>(), context));
            result.Combine(ContinueStoringToJsonObjectField(outputValue, ArgsField, functorHolder->m_actualSourceData.get(), nullptr, functorTypeId, context));

            return context.Report(result, "Successfully processed MaterialFunctorSourceData.");
        }

        BaseJsonSerializer::OperationFlags JsonMaterialFunctorSourceDataSerializer::GetOperationsFlags() const
        {
            return OperationFlags::ManualDefault;
        }
    } // namespace RPI
} // namespace AZ

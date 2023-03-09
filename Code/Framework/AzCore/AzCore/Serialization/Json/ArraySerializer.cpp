/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/ArraySerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonArraySerializer, SystemAllocator);

    JsonSerializationResult::Result JsonArraySerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadContainer(outputValue, outputValueTypeId, inputValue, false, context);

        case rapidjson::kObjectType:
            if (IsExplicitDefault(inputValue))
            {
                // Because this serializer has only the operation flag "InitializeNewInstance" set, the only time this will be called with
                // an explicit default is when a new instance has been created.
                return LoadContainer(outputValue, outputValueTypeId, inputValue, true, context);
            }
            [[fallthrough]];
        case rapidjson::kNullType:
            [[fallthrough]];
        case rapidjson::kStringType:
            [[fallthrough]];
        case rapidjson::kFalseType:
            [[fallthrough]];
        case rapidjson::kTrueType:
            [[fallthrough]];
        case rapidjson::kNumberType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. AZStd::array entries can only be read from an array.");

        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                "Unknown json type encountered for deserialization to AZStd::array.");
        }
    }

    JsonSerializationResult::Result JsonArraySerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(valueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the AZStd::array instance.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for representation of the AZStd::array instance.");
        }

        if (!container->IsFixedSize() || !container->IsFixedCapacity())
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve the correct container information for AZStd::array instance.");
        }

        ContinuationFlags flags = ContinuationFlags::None;
        Uuid elementTypeId = Uuid::CreateNull();
        auto typeEnumCallback = [&elementTypeId, &flags](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            AZ_Assert(genericClassElement, "No generic class element found during storing of a AZStd::array to json.");
            elementTypeId = genericClassElement->m_typeId;
            if (genericClassElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
            {
                flags = ContinuationFlags::ResolvePointer;
            }
            return false;
        };
        container->EnumTypes(typeEnumCallback);
    
        JSR::ResultCode retVal(JSR::Tasks::WriteValue);
        rapidjson::Value array;
        array.SetArray();

        void* nonConstInputValue = const_cast<void*>(inputValue);
        void* nonConstDefaultValue = const_cast<void*>(defaultValue);
        const size_t size = container->Size(nonConstInputValue);
        for (size_t i = 0; i < size; ++i)
        {
            ScopedContextPath subPath(context, i);

            void* element = container->GetElementByIndex(nonConstInputValue, nullptr, i);
            void* defaultElement = nullptr;
            if (!context.ShouldKeepDefaults() && defaultValue)
            {
                defaultElement = container->GetElementByIndex(nonConstDefaultValue, nullptr, i);
            }

            rapidjson::Value storedValue = GetExplicitDefault();
            JSR::ResultCode result = ContinueStoring(storedValue, element, defaultElement, elementTypeId, context, flags);
            if (result.GetProcessing() == JSR::Processing::Halted)
            {
                return context.Report(result, "Failed to store data for element from AZStd::array.");
            }

            array.PushBack(AZStd::move(storedValue), context.GetJsonAllocator());
            retVal.Combine(result);
        }

        if (retVal.GetOutcome() != JSR::Outcomes::DefaultsUsed)
        {
            outputValue = AZStd::move(array);
            return context.Report(retVal, "Content written from AZStd::array.");
        }
        else
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed,
                "No content written from AZStd::array because only defaults were found.");
        }
    }

    auto JsonArraySerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    JsonSerializationResult::Result JsonArraySerializer::LoadContainer(
        void* outputValue,
        const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        bool isNewInstance,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(outputValueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the AZStd::array instance.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for representation of the AZStd::array instance.");
        }

        if (!container->IsFixedSize() || !container->IsFixedCapacity())
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve the correct container information for AZStd::array instance.");
        }

        ContinuationFlags flags = isNewInstance ? ContinuationFlags::LoadAsNewInstance : ContinuationFlags::None;
        Uuid elementTypeId = Uuid::CreateNull();
        auto typeEnumCallback = [&elementTypeId, &flags](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            AZ_Assert(genericClassElement, "No generic class element found during storing of a AZStd::array to json.");
            elementTypeId = genericClassElement->m_typeId;
            if (genericClassElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
            {
                flags = ContinuationFlags::ResolvePointer;
            }
            return false;
        };
        container->EnumTypes(typeEnumCallback);

        const size_t size = container->Size(outputValue);
        if (!isNewInstance && inputValue.Size() < size)
        {
            return context.Report(
                JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Not enough entries in JSON array to load an AZStd::array from.");
        }

        rapidjson::Value explicitDefaultValue = GetExplicitDefault();
        
        JSR::ResultCode retVal(JSR::Tasks::ReadField);
        for (size_t i = 0; i < size; ++i)
        {
            ScopedContextPath subPath(context, i);

            void* element = container->GetElementByIndex(outputValue, nullptr, i);
            JSR::ResultCode result = ContinueLoading(
                element, elementTypeId, isNewInstance ? explicitDefaultValue : inputValue[aznumeric_caster(i)], context, flags);
            if (result.GetProcessing() == JSR::Processing::Halted)
            {
                return context.Report(result, "Failed to load data to element in AZStd::array.");
            }
            retVal.Combine(result);
        }

        if (isNewInstance)
        {
            return context.Report(retVal, "Filled new instance of AZStd::array with defaults.");
        }
        else if (container->Size(outputValue) == inputValue.Size())
        {
            return context.Report(retVal, "Successfully read entries into AZStd::array.");
        }
        else
        {
            retVal.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Skipped));
            return context.Report(
                retVal, "Successfully read available entries into AZStd::array, but there were still values left in the JSON array.");
        }
    }
} // namespace AZ

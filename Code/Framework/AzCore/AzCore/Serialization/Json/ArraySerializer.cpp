/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    AZ_CLASS_ALLOCATOR_IMPL(JsonArraySerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonArraySerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadContainer(outputValue, outputValueTypeId, inputValue, context);

        case rapidjson::kObjectType: // fall through
        case rapidjson::kNullType: // fall through
        case rapidjson::kStringType: // fall through
        case rapidjson::kFalseType: // fall through
        case rapidjson::kTrueType: // fall through
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

        Flags flags = Flags::None;
        Uuid elementTypeId = Uuid::CreateNull();
        auto typeEnumCallback = [&elementTypeId, &flags](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            AZ_Assert(genericClassElement, "No generic class element found during storing of a AZStd::array to json.");
            elementTypeId = genericClassElement->m_typeId;
            if (genericClassElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
            {
                flags = Flags::ResolvePointer;
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

    JsonSerializationResult::Result JsonArraySerializer::LoadContainer(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
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

        const size_t size = container->Size(outputValue);
        if (inputValue.Size() < size)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Not enough entries in JSON array to load an AZStd::array from.");
        }

        Flags flags = Flags::None;
        Uuid elementTypeId = Uuid::CreateNull();
        auto typeEnumCallback = [&elementTypeId, &flags](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            AZ_Assert(genericClassElement, "No generic class element found during storing of a AZStd::array to json.");
            elementTypeId = genericClassElement->m_typeId;
            if (genericClassElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
            {
                flags = Flags::ResolvePointer;
            }
            return false;
        };
        container->EnumTypes(typeEnumCallback);

        JSR::ResultCode retVal(JSR::Tasks::ReadField);
        for (size_t i = 0; i < size; ++i)
        {
            ScopedContextPath subPath(context, i);

            void* element = container->GetElementByIndex(outputValue, nullptr, i);
            JSR::ResultCode result = ContinueLoading(element, elementTypeId, inputValue[aznumeric_caster(i)], context, flags);
            if (result.GetProcessing() == JSR::Processing::Halted)
            {
                return context.Report(result, "Failed to load data to element in AZStd::array.");
            }
            retVal.Combine(result);
        }

        if (container->Size(outputValue) == inputValue.Size())
        {
            return context.Report(retVal, "Successfully read entries into AZStd::array.");
        }
        else
        {
            retVal.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Skipped));
            return context.Report(retVal,
                "Successfully read available entries into AZStd::array, but there were still values left in the JSON array.");
        }
    }
} // namespace AZ

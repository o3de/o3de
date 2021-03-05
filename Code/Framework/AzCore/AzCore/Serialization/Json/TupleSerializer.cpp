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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/TupleSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/utils.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonTupleSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonTupleSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
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
                "Unsupported type. AZStd::pair or AZStd::tuple can only be read from an array.");

        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                "Unknown json type encountered for AZStd::pair or AZStd::tuple.");
        }
    }

    JsonSerializationResult::Result JsonTupleSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(valueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the AZStd::pair or AZStd::tuple instance.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for container representation of the AZStd::pair or AZStd::tuple instance.");
        }

        size_t typeCount = 0;
        auto typeCountCallback = [&typeCount](const Uuid&, const SerializeContext::ClassElement*)
        {
            typeCount++;
            return true;
        };
        container->EnumTypes(typeCountCallback);

        AZStd::vector<const SerializeContext::ClassElement*> classElements;
        classElements.reserve(typeCount);
        auto typeEnumCallback = [&classElements](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            classElements.push_back(genericClassElement);
            return true;
        };
        container->EnumTypes(typeEnumCallback);

        JSR::ResultCode retVal(JSR::Tasks::WriteValue);
        AZStd::vector<rapidjson::Value> elementValues;
        elementValues.resize(typeCount);
        for (size_t i = 0; i < typeCount; ++i)
        {
            const void* elementAddress = container->GetElementByIndex(const_cast<void*>(inputValue), nullptr, i);
            AZ_Assert(elementAddress, "Address of AZStd::pair or AZStd::tuple element %zu could not be retrieved.", i);
            const void* defaultElementAddress =
                defaultValue ? container->GetElementByIndex(const_cast<void*>(defaultValue), nullptr, i) : nullptr;

            ScopedContextPath subPath(context, i);

            Flags flags = classElements[i]->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER ?
                Flags::ResolvePointer : Flags::None;

            JSR::ResultCode result = ContinueStoring(elementValues[i], elementAddress, defaultElementAddress,
                classElements[i]->m_typeId, context, flags);
            if (result.GetProcessing() == JSR::Processing::Halted)
            {
                return context.Report(result, "Failed to store data for AZStd::pair or AZStd::tuple element.");
            }
            retVal.Combine(result);
        }

        if (context.ShouldKeepDefaults() || retVal.GetOutcome() != JSR::Outcomes::DefaultsUsed)
        {
            outputValue.SetArray();
            for (size_t i = 0; i < typeCount; ++i)
            {
                outputValue.PushBack(AZStd::move(elementValues[i]), context.GetJsonAllocator());
            }
            return context.Report(retVal, "AZStd::pair or AZStd::tuple written.");
        }
        else
        {
            return context.Report(retVal, "AZStd::pair or AZStd::tuple not written because only defaults were found.");
        }
    }

    JsonSerializationResult::Result JsonTupleSerializer::LoadContainer(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(outputValueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the AZStd::pair instance.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for container representation of the AZStd::pair instance.");
        }

        size_t typeCount = 0;
        auto typeCountCallback = [&typeCount](const Uuid&, const SerializeContext::ClassElement*)
        {
            typeCount++;
            return true;
        };
        container->EnumTypes(typeCountCallback);

        rapidjson::SizeType arraySize = inputValue.Size();
        if (arraySize < typeCount)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Not enough entries in array to load an AZStd::pair or AZStd::tuple from.");
        }

        AZStd::vector<const SerializeContext::ClassElement*> classElements;
        classElements.reserve(typeCount);
        auto typeEnumCallback = [&classElements](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            classElements.push_back(genericClassElement);
            return true;
        };
        container->EnumTypes(typeEnumCallback);

        JSR::ResultCode retVal(JSR::Tasks::ReadField);
        rapidjson::SizeType arrayIndex = 0;
        size_t numElementsWritten = 0;
        for (size_t i = 0; i < typeCount; ++i)
        {
            ScopedContextPath subPath(context, i);
            
            void* elementAddress = container->GetElementByIndex(outputValue, nullptr, i);
            AZ_Assert(elementAddress, "Address of AZStd::pair or AZStd::tuple element %zu could not be retrieved.", i);

            Flags flags = classElements[i]->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER ?
                Flags::ResolvePointer : Flags::None;

            while (arrayIndex < inputValue.Size())
            {
                JSR::ResultCode result = ContinueLoading(elementAddress, classElements[i]->m_typeId, inputValue[arrayIndex], context, flags);
                retVal.Combine(result);
                arrayIndex++;
                if (result.GetProcessing() == JSR::Processing::Halted)
                {
                    return context.Report(retVal, "Failed to read element for AZStd::pair or AZStd::tuple.");
                }
                else if (result.GetProcessing() != JSR::Processing::Altered)
                {
                    numElementsWritten++;
                    break;
                }
            }
        }

        if (numElementsWritten < typeCount)
        {
            AZStd::string_view message = numElementsWritten == 0 ?
                "Unable to read data for AZStd::pair or AZStd::tuple." :
                "Partially read data for AZStd::pair or AZStd::tuple.";
            return context.Report(retVal, message);
        }
        else
        {
            return context.Report(retVal, "Successfully read AZStd::pair or AZStd::tuple.");
        }
    }
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    AZ_CLASS_ALLOCATOR_IMPL(JsonTupleSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonTupleSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
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

            ContinuationFlags flags = classElements[i]->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER
                ? ContinuationFlags::ResolvePointer
                : ContinuationFlags::None;

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

    auto JsonTupleSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    JsonSerializationResult::Result JsonTupleSerializer::LoadContainer(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, bool isNewInstance, JsonDeserializerContext& context)
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

        AZStd::vector<const SerializeContext::ClassElement*> classElements;
        classElements.reserve(typeCount);
        auto typeEnumCallback = [&classElements](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            classElements.push_back(genericClassElement);
            return true;
        };
        container->EnumTypes(typeEnumCallback);

        JSR::ResultCode retVal(JSR::Tasks::ReadField);
        if (isNewInstance)
        {
            rapidjson::Value explicitDefaultValue = GetExplicitDefault();

            for (size_t i = 0; i < typeCount; ++i)
            {
                ScopedContextPath subPath(context, i);

                void* elementAddress = container->GetElementByIndex(outputValue, nullptr, i);
                AZ_Assert(elementAddress, "Address of AZStd::pair or AZStd::tuple element %zu could not be retrieved.", i);

                ContinuationFlags flags = ContinuationFlags::LoadAsNewInstance;
                flags |=
                    (classElements[i]->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER ? ContinuationFlags::ResolvePointer
                                                                                                    : ContinuationFlags::None);
                JSR::ResultCode result = ContinueLoading(elementAddress, classElements[i]->m_typeId, explicitDefaultValue, context, flags);
                retVal.Combine(result);
                if (result.GetProcessing() == JSR::Processing::Halted)
                {
                    return context.Report(retVal, "Failed to read element for AZStd::pair or AZStd::tuple.");
                }
            }

            return context.Report(retVal, "Initialized AZStd::pair or AZStd::tuple to defaults.");
        }
        else
        {
            if (inputValue.Size() < typeCount)
            {
                return context.Report(
                    JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Not enough entries in array to load an AZStd::pair or AZStd::tuple from.");
            }

            rapidjson::SizeType arrayIndex = 0;
            size_t numElementsWritten = 0;
            for (size_t i = 0; i < typeCount; ++i)
            {
                ScopedContextPath subPath(context, i);

                void* elementAddress = container->GetElementByIndex(outputValue, nullptr, i);
                AZ_Assert(elementAddress, "Address of AZStd::pair or AZStd::tuple element %zu could not be retrieved.", i);

                ContinuationFlags flags =
                    (classElements[i]->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER ? ContinuationFlags::ResolvePointer
                                                                                                    : ContinuationFlags::None);
                while (arrayIndex < inputValue.Size())
                {
                    JSR::ResultCode result =
                        ContinueLoading(elementAddress, classElements[i]->m_typeId, inputValue[arrayIndex], context, flags);
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
                AZStd::string_view message = numElementsWritten == 0 ? "Unable to read data for AZStd::pair or AZStd::tuple."
                                                                     : "Partially read data for AZStd::pair or AZStd::tuple.";
                return context.Report(retVal, message);
            }
            else
            {
                return context.Report(retVal, "Successfully read AZStd::pair or AZStd::tuple.");
            }
        }
    }
} // namespace AZ

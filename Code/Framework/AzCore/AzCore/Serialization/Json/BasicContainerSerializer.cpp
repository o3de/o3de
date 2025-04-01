/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonBasicContainerSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonBasicContainerSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadContainer(outputValue, outputValueTypeId, inputValue, context);

        case rapidjson::kObjectType:
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
                "Unsupported type. Basic containers can only be read from an array.");

        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                "Unknown json type encountered for deserialization from a basic container.");
        }
    }

    bool JsonBasicContainerSerializer::ShouldClearContainer(const JsonDeserializerContext& context) const
    {
        return context.ShouldClearContainers();
    }

    JsonSerializationResult::Result JsonBasicContainerSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* /*defaultValue*/, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(valueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the basic container instance.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for representation of the basic container instance.");
        }

        rapidjson::Value array;
        array.SetArray();
        size_t index = 0;
        JSR::ResultCode retVal(JSR::Tasks::WriteValue);
        auto elementCallback = [this, &array, &retVal, &index, &context]
            (void* elementPtr, const Uuid& elementId, const SerializeContext::ClassData*, const SerializeContext::ClassElement* classElement)
        {
            ContinuationFlags flags = classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER
                ? ContinuationFlags::ResolvePointer
                : ContinuationFlags::None;
            flags |= ContinuationFlags::ReplaceDefault;
            
            ScopedContextPath subPath(context, index);
            index++;

            rapidjson::Value storedValue;
            JSR::ResultCode result = ContinueStoring(storedValue, elementPtr, nullptr, elementId, context, flags);
            if (result.GetProcessing() == JSR::Processing::Halted)
            {
                retVal = context.Report(result, "Failed to store data for element in basic container.");
                return false;
            }

            array.PushBack(AZStd::move(storedValue), context.GetJsonAllocator());
            retVal.Combine(result);    
            return true;
        };
        container->EnumElements(const_cast<void*>(inputValue), elementCallback);

        if (retVal.GetProcessing() == JSR::Processing::Halted)
        {
            return context.Report(retVal, "Processing of basic container was halted.");
        }

        if (context.ShouldKeepDefaults())
        {
            outputValue = AZStd::move(array);
            if (retVal.HasDoneWork())
            {
                AZ_Assert(retVal.GetOutcome() != JSR::Outcomes::DefaultsUsed,
                    "Basic container serialized with 'keep defaults' but still got default values.");
                AZ_Assert(retVal.GetOutcome() != JSR::Outcomes::PartialDefaults,
                    "Basic container serialized with 'keep defaults' but still got partial default values.");
                return context.Report(retVal , "Content written to basic container.");
            }
            else
            {
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success,
                    "Empty array written because the provided basic container is empty.");
            }
        }
        else
        {
            if (retVal.HasDoneWork())
            {
                // If at least one value was written, even if it has all defaults, then the array has
                // a value written to it and is therefore not in a default state anymore.
                retVal.Combine(JSR::ResultCode(JSR::Tasks::WriteValue, JSR::Outcomes::Success));

                outputValue = AZStd::move(array);
                return context.Report(retVal, "Content written to basic container.");
            }
            else
            {
                return context.Report(retVal, "No values written because the basic container was empty.");
            }
        }
    }

    JsonSerializationResult::Result JsonBasicContainerSerializer::LoadContainer(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(outputValueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the basic container.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve container meta information for the basic container.");
        }

        const SerializeContext::ClassElement* classElement = nullptr;
        auto typeEnumCallback = [&classElement](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            AZ_Assert(!classElement, "There are multiple class elements registered for a basic container where only one was expected.");
            classElement = genericClassElement;
            return true;
        };
        container->EnumTypes(typeEnumCallback);
        AZ_Assert(classElement, "No class element found for the type in the basic container.");

        ContinuationFlags flags = classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER
            ? ContinuationFlags::ResolvePointer
            : ContinuationFlags::None;
        flags |= ContinuationFlags::LoadAsNewInstance;

        const size_t capacity = container->IsFixedCapacity() ? container->Capacity(outputValue) : std::numeric_limits<size_t>::max();

        JSR::ResultCode retVal(JSR::Tasks::ReadField);
        size_t containerSize = container->Size(outputValue);
        if (containerSize > 0 && ShouldClearContainer(context))
        {
            JSR::Result result = context.Report(JSR::Tasks::Clear, JSR::Outcomes::Success, "Clearing basic container.");
            if (result.GetResultCode().GetOutcome() == JSR::Outcomes::Success)
            {
                container->ClearElements(outputValue, context.GetSerializeContext());
                containerSize = container->Size(outputValue);
                result = context.Report(JSR::Tasks::Clear, containerSize == 0 ? JSR::Outcomes::Success : JSR::Outcomes::Unsupported,
                    containerSize == 0 ? "Cleared basic container." : "Failed to clear basic container.");
            }
            if (result.GetResultCode().GetProcessing() != JSR::Processing::Completed)
            {
                return result;
            }
            retVal.Combine(result);
        }
        rapidjson::SizeType arraySize = inputValue.Size();
        for (rapidjson::SizeType i = 0; i < arraySize; ++i)
        {
            ScopedContextPath subPath(context, i);

            size_t expectedSize = container->Size(outputValue) + 1;

            if (expectedSize > capacity)
            {
                retVal.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Skipped,
                    "Unable to load more entries in basic container because it's full."));
                break;
            }

            void* elementAddress = container->ReserveElement(outputValue, classElement);
            if (!elementAddress)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic,
                    "Failed to allocate an item in the basic container.");
            }
            if (classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
            {
                *reinterpret_cast<void**>(elementAddress) = nullptr;
            }
            
            JSR::ResultCode result = ContinueLoading(elementAddress, classElement->m_typeId, inputValue[i], context, flags);
            if (result.GetProcessing() == JSR::Processing::Halted)
            {
                container->FreeReservedElement(outputValue, elementAddress, context.GetSerializeContext());
                return context.Report(retVal, "Failed to read element for basic container.");
            }
            else if (result.GetProcessing() == JSR::Processing::Altered)
            {
                container->FreeReservedElement(outputValue, elementAddress, context.GetSerializeContext());
                retVal.Combine(result);
            }
            else
            {
                container->StoreElement(outputValue, elementAddress);
                if (container->Size(outputValue) != expectedSize)
                {
                    retVal.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unavailable,
                        "Unable to store element to basic container."));
                }
                else
                {
                    retVal.Combine(result);
                }
            } 
        }

        if (!retVal.HasDoneWork() && inputValue.Empty())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "No values provided for basic container.");
        }

        size_t addedCount = container->Size(outputValue) - containerSize;
        if (addedCount > 0)
        {
            // Values were added which means the container is no longer in its default state of being empty.
            retVal.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success));
        }
        AZStd::string_view message =
            addedCount >= arraySize ? "Successfully read basic container.":
            addedCount == 0 ? "Unable to read data for basic container." :
            "Partially read data for basic container.";
        return context.Report(retVal, message);
    }
} // namespace AZ

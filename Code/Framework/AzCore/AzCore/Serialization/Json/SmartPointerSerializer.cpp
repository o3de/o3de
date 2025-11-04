/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/SmartPointerSerializer.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonSmartPointerSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonSmartPointerSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(outputValueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the smart pointer instance. Only registered classes can be serialized.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for representation of the smart pointer instance. "
                "Only smart pointers that have a registered container can be serialized.");
        }

        if (!container->IsSmartPointer())
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "The provided type to the JsonSmartPointer isn't a smart pointer.");
        }

        if (inputValue.IsNull())
        {
            // Do this simple case explicitly here in order to avoid complex logic to deal with reference counting for null pointers.
            container->ClearElements(outputValue, nullptr);
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "The smart pointer was explicitly set to null.");
        }

        JSR::ResultCode result(JSR::Tasks::ReadField);
        auto elementCallback = [this, &inputValue, &context, container, &result](void* instance, const Uuid& elementClassId,
            const SerializeContext::ClassData*, const SerializeContext::ClassElement* classElement)
        {
            if (*reinterpret_cast<void**>(instance))
            {
                bool isTypeExplicit = false;
                Uuid actualType = classElement->m_azRtti ?
                    classElement->m_azRtti->GetActualUuid(*reinterpret_cast<void**>(instance)) : elementClassId;
                Uuid targetTypeId;
                if (inputValue.IsObject())
                {
                    // Only json objects can hold a member field with the type id.
                    result = LoadTypeId(targetTypeId, inputValue, context, &elementClassId, &isTypeExplicit);
                    if (result.GetProcessing() == JSR::Processing::Halted)
                    {
                        result = context.Report(result, "Failed to retrieve a target type for the data in the smart pointer.");
                        return false;
                    }
                }
                else
                {
                    // In all other cases assume that the stored type is the same as the currently assigned type
                    // as only classes (represented by json objects) can have inheritance.
                    targetTypeId = elementClassId;
                }

                if (!isTypeExplicit || actualType == targetTypeId)
                {
                    // If the target type is the same as the type already stored in the smart pointer than no new
                    // instance is created and the existing instance will be updated with the data in the json document.
                    result = ContinueLoading(instance, elementClassId, inputValue, context, ContinuationFlags::ResolvePointer);
                    return false;
                }
            }

            // If there's no element in the smart pointer or the type doesn't match the target type a new instance
            // will be created, but this can't be loaded directly into the smart pointer as this would cause the
            // reference counter to be incorrect or the meta data of the shared_pointer to potentially point at.
            // the wrong address. In these cases explicitly reset the smart pointer. This will erase the existing
            // data but that's fine as it's not being used.
            void* element = nullptr;
            result = ContinueLoading(&element, elementClassId, inputValue, context, ContinuationFlags::ResolvePointer);
            if (result.GetProcessing() != JSR::Processing::Halted && result.GetProcessing() != JSR::Processing::Altered)
            {
                void* elementPtr = container->ReserveElement(instance, nullptr);
                *reinterpret_cast<void**>(elementPtr) = element;
                container->StoreElement(instance, nullptr);
            }
            return false;
        };
        container->EnumElements(outputValue, elementCallback);

        AZStd::string_view message =
            result.GetProcessing() == JSR::Processing::Completed ? "Successfully read data into smart pointer." :
            result.GetProcessing() != JSR::Processing::Halted ? "Partially read data into smart pointer." :
            "Failed to read data for smart pointer.";
        return context.Report(result, message);
    }

    JsonSerializationResult::Result JsonSmartPointerSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(valueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the smart pointer instance. Only registered classes can be serialized.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for representation of the smart pointer instance. "
                "Only smart pointers that have a registered container can be serialized.");
        }

        if (!container->IsSmartPointer())
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "The provided type to the JsonSmartPointer isn't a smart pointer.");
        }

        Uuid inputPtrType;
        auto inputCallback = [&inputValue, &inputPtrType]
            (void* elementPtr, const Uuid& elementId, const SerializeContext::ClassData*, const SerializeContext::ClassElement*)
            {
                inputValue = elementPtr;
                inputPtrType = elementId;
                return false;
            };
        container->EnumElements(const_cast<void*>(inputValue), inputCallback);

        if (defaultValue)
        {
            auto defaultInputCallback = [&defaultValue]
                (void* elementPtr, const Uuid&, const SerializeContext::ClassData*, const SerializeContext::ClassElement*)
                {
                    defaultValue = elementPtr;
                    return false;
                };
            container->EnumElements(const_cast<void*>(defaultValue), defaultInputCallback);
        }

        JSR::ResultCode result =
            ContinueStoring(outputValue, inputValue, defaultValue, inputPtrType, context, ContinuationFlags::ResolvePointer);
        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted ?
            "Successfully processed smart pointer." : "A problem occurred while processing a smart pointer.");
    }

    BaseJsonSerializer::OperationFlags JsonSmartPointerSerializer::GetOperationsFlags() const
    {
        return OperationFlags::ManualDefault;
    }
} // namespace AZ

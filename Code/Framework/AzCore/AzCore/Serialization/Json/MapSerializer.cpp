/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <algorithm>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/MapSerializer.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonMapSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnorderedMapSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnorderedMultiMapSerializer, SystemAllocator);
    
    // JsonMapSerializer

    JsonSerializationResult::Result JsonMapSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType: // fall through
        case rapidjson::kObjectType: 
            return LoadContainer(outputValue, outputValueTypeId, inputValue, context);

        case rapidjson::kNullType: // fall through
        case rapidjson::kStringType: // fall through
        case rapidjson::kFalseType: // fall through
        case rapidjson::kTrueType: // fall through
        case rapidjson::kNumberType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. Associative containers can only be read from an array or object.");

        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                "Unknown json type encountered for deserialization from an associative container.");
        }
    }

    JsonSerializationResult::Result JsonMapSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        return Store(outputValue, inputValue, defaultValue, valueTypeId, context, false);
    }

    JsonSerializationResult::Result JsonMapSerializer::LoadContainer(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(outputValueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                "Unable to retrieve information for definition of the associative container instance.");
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        AZ_Assert(container, "Unable to retrieve information for representation of the associative container instance.");
        
        const SerializeContext::ClassElement* pairElement = nullptr;
        auto pairTypeEnumCallback = [&pairElement](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            AZ_Assert(!pairElement, "A map is expected to only have one element.");
            pairElement = genericClassElement;
            return true;
        };
        container->EnumTypes(pairTypeEnumCallback);
        AZ_Assert(pairElement, "A map is expected to have exactly one pair element.");

        const SerializeContext::ClassData* pairClass = context.GetSerializeContext()->FindClassData(pairElement->m_typeId);
        AZ_Assert(pairClass, "Associative container was registered but not the pair that's used for storage.");
        SerializeContext::IDataContainer* pairContainer = pairClass->m_container;
        AZ_Assert(pairClass, "Associative container is missing the interface to the storage container.");
        const SerializeContext::ClassElement* keyElement = nullptr;
        const SerializeContext::ClassElement* valueElement = nullptr;
        auto keyValueTypeEnumCallback = [&keyElement, &valueElement](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            if (keyElement)
            {
                AZ_Assert(!valueElement, "The pair element in a container can't have more than 2 elements.");
                valueElement = genericClassElement;
            }
            else
            {
                keyElement = genericClassElement;
            }
            return true;
        };
        pairContainer->EnumTypes(keyValueTypeEnumCallback);
        AZ_Assert(keyElement && valueElement, "Expected the pair element in a container to have exactly 2 elements.");

        size_t containerSize = container->Size(outputValue);
        rapidjson::SizeType maximumSize = 0;
        const rapidjson::Value defaultValue(rapidjson::kObjectType);
        JSR::ResultCode retVal(JSR::Tasks::ReadField);
        if (containerSize > 0 && ShouldClearContainer(context))
        {
            JSR::Result result = context.Report(JSR::Tasks::Clear, JSR::Outcomes::Success, "Clearing associative container.");
            if (result.GetResultCode().GetOutcome() == JSR::Outcomes::Success)
            {
                container->ClearElements(outputValue, context.GetSerializeContext());
                containerSize = container->Size(outputValue);
                result = context.Report(JSR::Tasks::Clear, containerSize == 0 ? JSR::Outcomes::Success : JSR::Outcomes::Unsupported,
                    containerSize == 0 ? "Cleared associative container." : "Failed to clear associative container.");
            }
            if (result.GetResultCode().GetProcessing() != JSR::Processing::Completed)
            {
                return result;
            }
            retVal.Combine(result);
        }
        if (inputValue.IsObject())
        {
            maximumSize = inputValue.MemberCount();
            // Don't early out here because an empty object is also considered a default object.
            for (auto& entry : inputValue.GetObject())
            {
                AZStd::string_view keyName(entry.name.GetString(), entry.name.GetStringLength());
                ScopedContextPath subPath(context, keyName);

                const rapidjson::Value& key = (keyName == JsonSerialization::DefaultStringIdentifier) ? defaultValue : entry.name;

                JSR::Result elementResult = LoadElement(outputValue, container, pairElement, pairContainer, 
                    keyElement, valueElement, key, entry.value, context);
                if (elementResult.GetResultCode().GetProcessing() != JSR::Processing::Halted)
                {
                    retVal.Combine(elementResult.GetResultCode());
                }
                else
                {
                    return elementResult;
                }
            }
        }
        else
        {
            AZ_Assert(inputValue.IsArray(), "Maps can only be loaded from an object or an array.");
            maximumSize = inputValue.Size();
            if (maximumSize == 0)
            {
                return context.Report(retVal.HasDoneWork() ? retVal : JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success),
                    "No values provided for map.");
            }
            for (rapidjson::SizeType i = 0; i < maximumSize; ++i)
            {
                ScopedContextPath subPath(context, i);

                if (!inputValue[i].IsObject())
                {
                    retVal.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, AZStd::string::format(
                            R"(Unsupported type for elements in an associative container. If the array for is used, an object with "%s" and "%s" is expected)",
                            JsonSerialization::KeyFieldIdentifier, JsonSerialization::ValueFieldIdentifier)));
                    continue;
                }

                const rapidjson::Value::ConstMemberIterator keyMember = inputValue[i].FindMember(JsonSerialization::KeyFieldIdentifier);
                const rapidjson::Value::ConstMemberIterator valueMember = inputValue[i].FindMember(JsonSerialization::ValueFieldIdentifier);
                const rapidjson::Value& key = (keyMember != inputValue[i].MemberEnd()) ? keyMember->value : defaultValue;
                const rapidjson::Value& value = (valueMember != inputValue[i].MemberEnd()) ? valueMember->value : defaultValue;

                JSR::Result elementResult = LoadElement(outputValue, container, pairElement, pairContainer,
                    keyElement, valueElement, key, value, context);
                if (elementResult.GetResultCode().GetProcessing() != JSR::Processing::Halted)
                {
                    retVal.Combine(elementResult.GetResultCode());
                }
                else
                {
                    return elementResult;
                }
            }
        }

        size_t addedCount = container->Size(outputValue) - containerSize;
        if (addedCount > 0)
        {
            // If at least one entry was added then the map is no longer in it's default state so
            // mark is with success so the result can at best be partial defaults.
            retVal.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success));
        }
        AZStd::string_view message;
        if (addedCount >= maximumSize)
        {
            message =
                retVal.GetProcessing() == JSR::Processing::Completed ? "Successfully read associative container." :
                "Partially read element data for the associative container.";
        }
        else
        {
            message =
                addedCount == 0 ? "Unable to read data for the associative container." : 
                "Partially read data for the associative container.";
        }
        return context.Report(retVal, message);
    }

    JsonSerializationResult::Result JsonMapSerializer::LoadElement(void* outputValue, SerializeContext::IDataContainer* container,
        const SerializeContext::ClassElement* pairElement, SerializeContext::IDataContainer* pairContainer,
        const SerializeContext::ClassElement* keyElement, const SerializeContext::ClassElement* valueElement,
        const rapidjson::Value& key, const rapidjson::Value& value, JsonDeserializerContext& context, bool isMultiMap)
    {
        namespace JSR = JsonSerializationResult;

        size_t expectedSize = container->Size(outputValue) + 1;
        void* address = container->ReserveElement(outputValue, pairElement);
        if (!address)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic,
                "Failed to allocate an item for an associative container.");
        }

        // Load key
        void* keyAddress = pairContainer->GetElementByIndex(address, pairElement, 0);
        AZ_Assert(keyAddress, "Element reserved for associative container, but unable to retrieve address of the key.");
        ContinuationFlags keyLoadFlags = ContinuationFlags::LoadAsNewInstance;
        if (keyElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
        {
            keyLoadFlags |= ContinuationFlags::ResolvePointer;
            *reinterpret_cast<void**>(keyAddress) = nullptr;
        }
        JSR::ResultCode keyResult = ContinueLoading(keyAddress, keyElement->m_typeId, key, context, keyLoadFlags);
        if (keyResult.GetProcessing() == JSR::Processing::Halted)
        {
            container->FreeReservedElement(outputValue, address, context.GetSerializeContext());
            return context.Report(keyResult, "Failed to read key for associative container.");
        }

        void* valueAddress = nullptr;
        bool keyExists = false;

        // For multimaps, we append values to keys instead updating them.
        // This is to ensure legacy multimap serialization support.
        if (!isMultiMap)
        {
            auto associativeContainer = container->GetAssociativeContainerInterface();
            void* existingKeyValuePair = associativeContainer->GetElementByKey(outputValue, keyElement, keyAddress);
            if (existingKeyValuePair)
            {
                valueAddress = pairContainer->GetElementByIndex(existingKeyValuePair, pairElement, 1);
                expectedSize--;
                keyExists = true;
            }
        }

        // If the key doesn't exist or it's a multimap, we're adding the new element we reserved above.
        if (!keyExists)
        {
            valueAddress = pairContainer->GetElementByIndex(address, pairElement, 1);
        }

        // Load value
        AZ_Assert(valueAddress, "Element reserved for associative container, but unable to retrieve address of the value.");
        ContinuationFlags valueLoadFlags = ContinuationFlags::LoadAsNewInstance; 
        if (valueElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
        {
            valueLoadFlags |= ContinuationFlags::ResolvePointer;
            *reinterpret_cast<void**>(valueAddress) = nullptr;
        }
        JSR::ResultCode valueResult = ContinueLoading(valueAddress, valueElement->m_typeId, value, context, valueLoadFlags);
        if (valueResult.GetProcessing() == JSR::Processing::Halted)
        {
            container->FreeReservedElement(outputValue, address, context.GetSerializeContext());
            return context.Report(valueResult, "Failed to read value for associative container.");
        }

        // Finalize loading
        if (keyResult.GetProcessing() == JSR::Processing::Altered ||
            valueResult.GetProcessing() == JSR::Processing::Altered)
        {
            container->FreeReservedElement(outputValue, address, context.GetSerializeContext());
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unavailable,
                "Unable to fully process an element for the associative container.");
        }
        else
        {
            // Even if the key exists, calling StoreElement will not replace the existing key
            // and will free the temporary address as expected. Checking if the key already 
            // exists and skipping the call to StoreElement if it does, makes the intent more
            // clear. The end result is the same either way.
            if (!keyExists)
            {
                container->StoreElement(outputValue, address);
            }
            else
            {
                container->FreeReservedElement(outputValue, address, context.GetSerializeContext());
            }
            if (container->Size(outputValue) != expectedSize)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unavailable,
                    "Unable to store the element that was read to the associative container.");
            }
        }

        JSR::ResultCode result = JSR::ResultCode::Combine(keyResult, valueResult);

        AZStd::string_view message = result.GetProcessing() == JSR::Processing::Completed
            ? "Successfully loaded an entry into the associative container."
            : "Partially loaded an entry into the associative container.";
        return context.Report(result, message);
    }

    JsonSerializationResult::Result JsonMapSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context, bool sortResult)
    {
        namespace JSR = JsonSerializationResult;

        JsonBasicContainerSerializer serializer;

        // This will fill up keyValues with an array of pairs.
        rapidjson::Value keyValues(rapidjson::kObjectType);
        JSR::Result result = serializer.Store(keyValues, inputValue, defaultValue, valueTypeId, context);
        JSR::ResultCode code = result.GetResultCode();
        if (code.GetProcessing() == JSR::Processing::Halted)
        {
            return context.Report(code, "Associative container failed to extract key/value pairs from the provided container.");
        }

        if (keyValues.IsArray())
        {
            if (context.ShouldKeepDefaults() && keyValues.Empty())
            {
                outputValue.SetArray();
                return context.Report(code, "Successfully stored associative container.");
            }

            AZ_Assert(!keyValues.Empty(), "Intermediate array for associative container can't be empty "
                "because an empty array would be stored as an empty default object.");
            
            if (CanBeConvertedToObject(keyValues))
            {
                // Convert the array to a Object with the member being the key.
                outputValue.SetObject();
                for (auto& entry : keyValues.GetArray())
                {
                    if (entry.IsArray() && entry.Size() >= 2)
                    {
                        AZ_Assert(entry.Size() == 2,
                            "Associative container expected the entry to be a key/value pair but the array contained (%u) entries.", entry.Size());
                        if (IsExplicitDefault(entry[0]))
                        {
                            outputValue.AddMember(rapidjson::StringRef(JsonSerialization::DefaultStringIdentifier), AZStd::move(entry[1]),
                                context.GetJsonAllocator());
                        }
                        else
                        {
                            outputValue.AddMember(AZStd::move(entry[0]), AZStd::move(entry[1]), context.GetJsonAllocator());
                        }
                    }
                    else
                    {
                        AZ_Assert(IsExplicitDefault(entry), "Associative container expected the entry to be an explicit default object.");
                        outputValue.AddMember(rapidjson::StringRef(JsonSerialization::DefaultStringIdentifier),
                            rapidjson::Value(rapidjson::kObjectType), context.GetJsonAllocator());
                    }
                }

                if (sortResult)
                {
                    auto less = [](
                        const rapidjson::Value::MemberIterator::value_type& lhs,
                        const rapidjson::Value::MemberIterator::value_type& rhs) -> bool
                    {
                        JsonSerializerCompareResult result = JsonSerialization::Compare(lhs.name, rhs.name);
                        if (result == JsonSerializerCompareResult::Equal)
                        {
                            return JsonSerialization::Compare(lhs.value, rhs.value) == JsonSerializerCompareResult::Less;
                        }
                        else
                        {
                            return result == JsonSerializerCompareResult::Less;
                        }
                    };
                    // Using std::sort because AZStd::sort isn't implemented in terms of move operations.
                    std::sort(outputValue.MemberBegin(), outputValue.MemberEnd(), less);
                }
            }
            else
            {
                // The key isn't a string so store the map as an array of key/value pairs.
                outputValue.SetArray();
                for (auto& entry : keyValues.GetArray())
                {
                    rapidjson::Value outputEntry(rapidjson::kObjectType);
                    if (entry.IsArray() && entry.Size() >= 2)
                    {
                        AZ_Assert(entry.Size() == 2, "Associative container expected the entry to be a key/value pair "
                            "but the array contained (%u) entries.", entry.Size());
                        outputEntry.AddMember(rapidjson::StringRef(JsonSerialization::KeyFieldIdentifier),
                            AZStd::move(entry[0]), context.GetJsonAllocator());
                        outputEntry.AddMember(rapidjson::StringRef(JsonSerialization::ValueFieldIdentifier),
                            AZStd::move(entry[1]), context.GetJsonAllocator());
                    }
                    else
                    {
                        AZ_Assert(!entry.IsArray(), "Associative container expected the entry to be a key/value pair "
                            "but the array contained (%u) entries.", entry.Size());
                        AZ_Assert(IsExplicitDefault(entry), "Associative container expected the entry to be an explicit default object.");
                        outputEntry.AddMember(rapidjson::StringRef(JsonSerialization::KeyFieldIdentifier),
                            rapidjson::Value(rapidjson::kObjectType), context.GetJsonAllocator());
                        outputEntry.AddMember(rapidjson::StringRef(JsonSerialization::ValueFieldIdentifier),
                            rapidjson::Value(rapidjson::kObjectType), context.GetJsonAllocator());
                    }
                    outputValue.PushBack(AZStd::move(outputEntry), context.GetJsonAllocator());
                }

                if (sortResult)
                {
                    auto less = [](const rapidjson::Value& lhs, const rapidjson::Value& rhs) -> bool
                    {
                        return JsonSerialization::Compare(lhs, rhs) == JsonSerializerCompareResult::Less;
                    };
                    // Using std::sort because AZStd::sort isn't implemented in terms of move operations.
                    std::sort(outputValue.Begin(), outputValue.End(), less);
                }
            }
        }
        else
        {
            outputValue = AZStd::move(keyValues);
        }

        return context.Report(code, "Successfully stored associative container.");
    }

    bool JsonMapSerializer::CanBeConvertedToObject(const rapidjson::Value& keyValues)
    {
        // Check if all keys are either a default or a string.
        for (const auto& entry : keyValues.GetArray())
        {
            if (entry.IsArray() && !entry.Empty())
            {
                const rapidjson::Value& key = entry[0];
                if (!key.IsString() && !IsExplicitDefault(key))
                {
                    return false;
                }
            }
            else if (!IsExplicitDefault(entry))
            {
                return false;
            }
        }
        return true;
    }

    bool JsonMapSerializer::ShouldClearContainer(const JsonDeserializerContext& context) const
    {
        return context.ShouldClearContainers();
    }

    
    // JsonUnorderedMapSerializer

    JsonSerializationResult::Result JsonUnorderedMapSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        return JsonMapSerializer::Store(outputValue, inputValue, defaultValue, valueTypeId, context, true);
    }



    // JsonUnorderedMultiMapSerializer

    JsonSerializationResult::Result JsonUnorderedMultiMapSerializer::LoadElement(void* outputValue, SerializeContext::IDataContainer* container,
        const SerializeContext::ClassElement* pairElement, SerializeContext::IDataContainer* pairContainer,
        const SerializeContext::ClassElement* keyElement, const SerializeContext::ClassElement* valueElement,
        const rapidjson::Value& key, const rapidjson::Value& value, JsonDeserializerContext& context, [[maybe_unused]] bool isMultiMap)
    {
        namespace JSR = JsonSerializationResult;

        if (value.IsArray())
        {
            JSR::ResultCode result(JSR::Tasks::ReadField);
            for (auto& entry : value.GetArray())
            {
                result.Combine(JsonMapSerializer::LoadElement(outputValue, container, pairElement, pairContainer,
                    keyElement, valueElement, key, entry, context, true));
                if (result.GetProcessing() == JSR::Processing::Halted)
                {
                    return context.Report(result, "Unable to process the key or all values in multi-map.");
                }
            }
            return context.Report(result, "Successfully loaded values for key in multi-map");
        }
        else if (IsExplicitDefault(value))
        {
            return JsonMapSerializer::LoadElement(outputValue, container, pairElement, pairContainer,
                keyElement, valueElement, key, value, context, true);
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Values for multi-maps need to be stored in an array.");
        }
    }

    JsonSerializationResult::Result JsonUnorderedMultiMapSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        JSR::Result result = JsonMapSerializer::Store(outputValue, inputValue, defaultValue, valueTypeId, context, true);
        if (result.GetResultCode().GetProcessing() == JSR::Processing::Halted)
        {
            return result;
        }

        // RapidJSON allows objects with duplicate names, but this is technically not valid json. Map and unordered_map can't
        // run into this problem because keys are guaranteed to be unique, but multimap can have duplicate keys. To address this
        // an additional pass is done to combine duplicate into one entry and store the values in an array. This however also has a
        // problem because now it's not possible to store values that are arrays, so a general rule is applied that multimaps always
        // have their values stored in arrays. This results in the following formats:
        //      { "SomeStringKey": [ 1234 ] }
        // and
        //      [ { "Key": 1234, "Value": [ 5678 ] } ]

        // Note that the output is sorted by key and then value
        rapidjson::Value currentKey(rapidjson::kNullType);
        rapidjson::Value currentValues(rapidjson::kArrayType);

        if (outputValue.IsObject())
        {
            if (outputValue.MemberCount() == 0)
            {
                return result;
            }

            if (outputValue.MemberCount() == 1)
            {
                if (IsExplicitDefault(outputValue.MemberBegin()->value))
                {
                    return result;
                }
            }

            rapidjson::Value newOutput(rapidjson::kObjectType);

            for (auto& entry : outputValue.GetObject())
            {
                if (JsonSerialization::Compare(currentKey, entry.name) != JsonSerializerCompareResult::Equal)
                {
                    if (!currentKey.IsNull())
                    {
                        newOutput.AddMember(AZStd::move(currentKey), AZStd::move(currentValues), context.GetJsonAllocator());
                        currentValues = rapidjson::Value(rapidjson::kArrayType);
                    }
                    currentKey = AZStd::move(entry.name);
                }
                currentValues.PushBack(AZStd::move(entry.value), context.GetJsonAllocator());
            }
            newOutput.AddMember(AZStd::move(currentKey), AZStd::move(currentValues), context.GetJsonAllocator());

            outputValue = AZStd::move(newOutput);
        }
        else
        {
            rapidjson::Value newOutput(rapidjson::kArrayType);

            AZ_Assert(outputValue.IsArray(),
                "JsonUnorderedMultiMapSerializer was expecting an object or array after storing and got neither.");
            if (outputValue.Empty())
            {
                return result;
            }

            if (outputValue.Size() == 1)
            {
                auto valueIt = outputValue[0].FindMember(JsonSerialization::ValueFieldIdentifier);
                AZ_Assert(valueIt != outputValue[0].MemberEnd(),
                    "There should always be a value in the array produced by JsonMapSerializer.");
                if (IsExplicitDefault(valueIt->value))
                {
                    return result;
                }
            }

            for (auto& entry : outputValue.GetArray())
            {
                auto keyIt = entry.FindMember(JsonSerialization::KeyFieldIdentifier);
                AZ_Assert(keyIt != entry.MemberEnd(), "There should always be a key in the array produced by JsonMapSerializer.");
                auto valueIt = entry.FindMember(JsonSerialization::ValueFieldIdentifier);
                AZ_Assert(valueIt != entry.MemberEnd(), "There should always be a value in the array produced by JsonMapSerializer.");

                if (JsonSerialization::Compare(currentKey, keyIt->value) != JsonSerializerCompareResult::Equal)
                {
                    if (!currentKey.IsNull())
                    {
                        rapidjson::Value newEntry(rapidjson::kObjectType);
                        newEntry.AddMember(rapidjson::StringRef(JsonSerialization::KeyFieldIdentifier),
                            AZStd::move(currentKey), context.GetJsonAllocator());
                        newEntry.AddMember(rapidjson::StringRef(JsonSerialization::ValueFieldIdentifier),
                            AZStd::move(currentValues), context.GetJsonAllocator());
                        newOutput.PushBack(AZStd::move(newEntry), context.GetJsonAllocator());
                        currentValues = rapidjson::Value(rapidjson::kArrayType);
                    }
                    currentKey = AZStd::move(keyIt->value);
                }
                currentValues.PushBack(AZStd::move(valueIt->value), context.GetJsonAllocator());
            }
            rapidjson::Value newEntry(rapidjson::kObjectType);
            newEntry.AddMember(rapidjson::StringRef(JsonSerialization::KeyFieldIdentifier),
                AZStd::move(currentKey), context.GetJsonAllocator());
            newEntry.AddMember(rapidjson::StringRef(JsonSerialization::ValueFieldIdentifier),
                AZStd::move(currentValues), context.GetJsonAllocator());
            newOutput.PushBack(AZStd::move(newEntry), context.GetJsonAllocator());

            outputValue = AZStd::move(newOutput);
        }
        
        return result;
    }
}

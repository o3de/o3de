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

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    /*
        A dependant pair is an object whose 2nd component type depends on the value of the first component.
        This type uses CRTP. The derived class should implement:

        // The key name of the first component of the pair (when serialized as an object)
        const char* IndexMemberName() const;

        // The key name of the second component of the pair (when serialized as an object)
        const char* ValueMemberName() const;

        // A human readable type name for the type being handled
        const char* PrettyTypeName() const;

        // Defines the type of the first component
        using IndexMemberType = ...;

        // Given a value for the index member, retrieves the corresponding data type
        // The serialized representation may not always have an explicit value for the index, so index type is optional
        AZStd::optional<TypeOption> GetIndexTypeFromIndex(IDataContainer&, const AZStd::optional<IndexMemberType>&) const;

        // Given the type of the stored second component, retrieves the value of the correspond index
        AZStd::optional<IndexMemberType> GetIndexFromIndexType(IDataContainer&, const TypeOption&) const;

    */
    template<class Derived>
    class GenericDependantPairSerializer
        : public BaseJsonSerializer
    {
    public:
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

    protected:
        using TypeOption = AZStd::pair<AZ::Uuid, const AZ::SerializeContext::ClassElement*>;
        using TypeOptions = AZStd::vector<TypeOption>;
        using IDataContainer = AZ::SerializeContext::IDataContainer;

        struct IndexValueView
        {
            // Both index and value may or may not be present
            const rapidjson::Value* index;
            const rapidjson::Value* value;
        };

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId,
            IndexValueView input, JsonDeserializerContext& context);

        using BaseJsonSerializer::ContinueLoading;
        using BaseJsonSerializer::ContinueStoring;

        template<class T>
        inline JsonSerializationResult::ResultCode ContinueLoading(T& valueOut, const rapidjson::Value& valueIn,
            JsonDeserializerContext& context, ContinuationFlags flags = ContinuationFlags::None)
        {
            static_assert(AZStd::is_same_v<AZStd::decay_t<T>, T>, "value types only");
            return ContinueLoading(&valueOut, azrtti_typeid<T>(), valueIn, context, flags);
        }

        template<class T>
        inline JsonSerializationResult::ResultCode ContinueStoring(rapidjson::Value& output, const T& object, const T* defaultObject,
            JsonSerializerContext& context, ContinuationFlags flags = ContinuationFlags::None)
        {
            static_assert(AZStd::is_same_v<AZStd::decay_t<T>, T>, "value types only");
            return ContinueStoring(output, &object, defaultObject, azrtti_typeid<T>(), context, flags);
        }

        template<class T>
        static auto FindMemberOrNull(const T& obj, const char* member)
            -> decltype(&(obj.FindMember(member)->value))
        {
            auto it = obj.FindMember(member);
            if (it != obj.MemberEnd())
            {
                return &(it->value);
            }
            else
            {
                return nullptr;
            }
        }

        struct ReservedElementDeleter
        {
            AZ::SerializeContext::IDataContainer& container;
            void* instance;
            AZ::SerializeContext* sc;
            void operator()(void* ptr) const
            {
                if (ptr)
                {
                    container.FreeReservedElement(instance, ptr, sc);
                }
            }
        };

        AZStd::unique_ptr<void, ReservedElementDeleter> ReserveElementPtr(AZ::SerializeContext::IDataContainer& container, void* instance, const SerializeContext::ClassElement* ce, AZ::SerializeContext* sc)
        {
            return AZStd::unique_ptr<void, ReservedElementDeleter>(
                container.ReserveElement(instance, ce),
                ReservedElementDeleter{ container, instance, sc }
            );
        }

    private:

        Derived* GetDerived() { return static_cast<Derived*>(this); }
        const Derived* GetDerived() const { return static_cast<const Derived*>(this); }
    };


    template<class Derived>
    JsonSerializationResult::Result GenericDependantPairSerializer<Derived>::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        switch (inputValue.GetType())
        {
        case rapidjson::kObjectType:
        {
            const auto& inputObject = inputValue.GetObject();
            IndexValueView view;
            view.index = FindMemberOrNull(inputObject, GetDerived()->IndexMemberName());
            view.value = FindMemberOrNull(inputObject, GetDerived()->ValueMemberName());
            return Load(outputValue, outputValueTypeId, view, context);
        }

        case rapidjson::kArrayType: // fall through
        case rapidjson::kNullType: // fall through
        case rapidjson::kStringType: // fall through
        case rapidjson::kFalseType: // fall through
        case rapidjson::kTrueType: // fall through
        case rapidjson::kNumberType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                AZStd::string::format("Unsupported type. %s can only be read from an object.", GetDerived()->PrettyTypeName())
            );

        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                AZStd::string::format("Unknown json type encountered for deserialization from %s.", GetDerived()->PrettyTypeName())
            );
        }
    }

    template<class Derived>
    JsonSerializationResult::Result GenericDependantPairSerializer<Derived>::Load(void* outputValue, const Uuid& outputValueTypeId, IndexValueView input, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        // Read the index
        AZStd::optional<typename Derived::IndexMemberType> index;
        if (input.index)
        {
            auto indexResult = ContinueLoading(index.emplace(), *input.index, context);
            if (indexResult.GetProcessing() == JSR::Processing::Halted)
            {
                return context.Report(indexResult,
                    AZStd::string::format("Failed to read first component of %s", GetDerived()->PrettyTypeName())
                );
            }
        }
        else
        {
            // TODO: report missing field without failure?
        }

        // Get IDataContainer from serialize context
        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(outputValueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                AZStd::string::format("Unable to retrieve information for definition of the %s type instance.", GetDerived()->PrettyTypeName())
            );
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                AZStd::string::format("Unable to retrieve IDataContainer for %s.", GetDerived()->PrettyTypeName())
            );
        }

        // The object may be empty only if it is not fixed size
        const bool allowsEmptyValues = !container->IsFixedSize();

        // Get the type which this index should store
        const AZStd::optional<TypeOption> mbTypeAtIndex = GetDerived()->GetIndexTypeFromIndex(*container, index);

        if (!mbTypeAtIndex)
        {
            if (allowsEmptyValues)
            {
                // Do nothing: missing field is explicit default
            }
            else
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Invalid,
                    AZStd::string::format("%s type does not have type option for it second component matching the given first component", GetDerived()->PrettyTypeName())
                );
            }
        }
        else
        {
            auto& typeAtIndex = *mbTypeAtIndex;

            // If there was no class element returned, synthesize a minimal one using the type ID
            AZStd::optional<SerializeContext::ClassElement> classElementStorage;
            const SerializeContext::ClassElement* classElement = typeAtIndex.second;
            if (!classElement)
            {
                auto* ce = &classElementStorage.emplace();
                ce->m_typeId = typeAtIndex.first;
                classElement = ce;
            }

            // Reserve space for the value
            auto valueStorage = ReserveElementPtr(*container, outputValue, classElement, context.GetSerializeContext());
            if (!valueStorage)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic,
                    AZStd::string::format("Failed to allocate an item for a %s", GetDerived()->PrettyTypeName())
                );
            }

            // Load data into the element
            if (input.value)
            {
                auto valueResult = ContinueLoading(valueStorage.get(), typeAtIndex.first, *input.value, context);
                if (valueResult.GetProcessing() == JSR::Processing::Halted)
                {
                    return context.Report(valueResult, AZStd::string::format("Failed to read second component of %s", GetDerived()->PrettyTypeName()));
                }
            }
            else
            {
                // Do nothing: lack of a `value' means use the default constructed value, which is handled by ReserveElement
            }

            // Release the reserved element ptr, which keeps this element in the container when the unique ptr is destroyed
            valueStorage.release();
        }

        // NB: is this the correct way of returning a successful result?
        JSR::ResultCode result(JSR::Tasks::ReadField);
        return context.Report(result, AZStd::string::format("Successfully loaded %s", GetDerived()->PrettyTypeName()));
    }


    template<class Derived>
    JsonSerializationResult::Result GenericDependantPairSerializer<Derived>::Store(rapidjson::Value& outputValue, const void* inputValue, [[maybe_unused]] const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        // Get IDataContainer from serialize context
        const SerializeContext::ClassData* containerClass = context.GetSerializeContext()->FindClassData(valueTypeId);
        if (!containerClass)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                AZStd::string::format("Unable to retrieve information for definition of %s type instance.", GetDerived()->PrettyTypeName())
            );
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        if (!container)
        {
            return context.Report(JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported,
                AZStd::string::format("Unable to retrieve IDataContainer for %s.", GetDerived()->PrettyTypeName())
            );
        }

        JSR::ResultCode retVal(JSR::Tasks::WriteValue);
        rapidjson::Value outObject;
        outObject.SetObject();

        const auto ContinueStoringMember = [this, &context, &retVal, &outObject]
        (const char* name, const void* elementPtr, const Uuid& elementId, ContinuationFlags flags)
        {
            rapidjson::Value storedValue;
            JSR::ResultCode result = ContinueStoring(storedValue, elementPtr, nullptr, elementId, context, flags);
            if (result.GetProcessing() == JSR::Processing::Halted)
            {
                retVal = context.Report(result, AZStd::string::format("Failed to store data for '%s' in %s", name, GetDerived()->PrettyTypeName()));
                return false;
            }

            outObject.AddMember(rapidjson::GenericStringRef<char>(name), AZStd::move(storedValue), context.GetJsonAllocator());
            retVal.Combine(result);
            return true;
        };

        // Enum elements, which should give us zero or one elements
        size_t elements = 0;
        AZStd::optional<TypeOption> typeOption;

        container->EnumElements(const_cast<void*>(inputValue), [&]
        (void* elementPtr, const Uuid& elementId, const SerializeContext::ClassData*, const SerializeContext::ClassElement* classElement)
        {
            elements++;
            if (elements != 1)
            {
                return false;
            }

            // Store the class element for later use
            typeOption.emplace(elementId, classElement);

            ContinuationFlags flags = classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER ?
                ContinuationFlags::ResolvePointer : ContinuationFlags::None;
            flags |= ContinuationFlags::ReplaceDefault;

            ScopedContextPath subPath(context, 0);
            if (!ContinueStoringMember(GetDerived()->ValueMemberName(), elementPtr, elementId, flags))
            {
                return false;
            }

            // Return true to continue here, even though we expect variant to have at most 1 element
            // In case this is used with a wrong type, we continue to detect that case
            return true;
        });

        // The object may be empty only if it is not fixed size
        const bool allowsEmptyValues = !container->IsFixedSize();

        // Handle error cases
        if (elements == 0)
        {
            if (!allowsEmptyValues)
            {
                // This message is based on the implemention of IDataContainer for variant. It claims that variant always has 1 element (size=capacity=1)
                // but somethings EnumElements will invoke the callback 0 times - if the variant is valueless by exception.
                // Since this appears to be a very remote edge case, we don't attempt to support it
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Catastrophic,
                    AZStd::string::format("Could not write value for %s because it has no value, but is required to have a value (or the object is not a %s)",
                        GetDerived()->PrettyTypeName(), GetDerived()->PrettyTypeName()
                    ));
            }
        }
        else if (elements != 1)
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Catastrophic,
                AZStd::string::format("Could not write value for %s because it does not appear to be a %s",
                    GetDerived()->PrettyTypeName(), GetDerived()->PrettyTypeName()
                ));
        }

        // class element should be present here
        if (typeOption)
        {
            // Get the index
            AZStd::optional<typename Derived::IndexMemberType> index = GetDerived()->GetIndexFromIndexType(*container, *typeOption);
            if (!index)
            {
                // The element type is not one of the types returned by EnumType - Should be impossible
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Catastrophic,
                    AZStd::string::format("Internal logic error in GenericDependantPairSerializer::Store line %d - report this as a bug", __LINE__));
            }

            // Write out the index
            ContinueStoringMember(GetDerived()->IndexMemberName(), &*index, azrtti_typeid<typename Derived::IndexMemberType>(), ContinuationFlags::ReplaceDefault);
        }
        else
        {
            if (allowsEmptyValues)
            {
                // Do nothing: missing field is explicit default
            }
            else
            {
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Catastrophic,
                    AZStd::string::format("Internal logic error in GenericDependantPairSerializer::Store line %d - report this as a bug", __LINE__));
            }
        }

        // Taken from BasicContainerSerializer
        if (retVal.GetProcessing() == JSR::Processing::Halted)
        {
            return context.Report(retVal, "Processing was halted.");
        }

        if (context.ShouldKeepDefaults())
        {
            outputValue = AZStd::move(outObject);
            if (retVal.HasDoneWork())
            {
                AZ_Assert(retVal.GetOutcome() != JSR::Outcomes::DefaultsUsed,
                    "serialized with 'keep defaults' but still got default values.");
                AZ_Assert(retVal.GetOutcome() != JSR::Outcomes::PartialDefaults,
                    "serialized with 'keep defaults' but still got partial default values.");
                return context.Report(retVal, "Content written to container.");
            }
            else
            {
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success,
                    "Empty object written because the provided container is empty.");
            }
        }
        else
        {
            if (retVal.HasDoneWork())
            {
                outputValue = AZStd::move(outObject);
                return context.Report(retVal, "Content written to container.");
            }
            else
            {
                return context.Report(retVal, "No values written because the container was empty.");
            }
        }
    }
}


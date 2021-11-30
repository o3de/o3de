/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerializer.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/any.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ
{
    JsonSerializationResult::ResultCode JsonSerializer::Store(rapidjson::Value& output, const void* object, const void* defaultObject,
        const Uuid& typeId, UseTypeSerializer custom, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        if (!object)
        {
            return context.Report(Tasks::ReadField, Outcomes::Catastrophic,
                "Target object for Json Serialization is pointing to nothing during storing.");
        }

        // First check if there's a generic serializer registered for this. This makes it possible to use serializers that
        // are not (directly) registered with the Serialize Context.
        if (BaseJsonSerializer* serializer
            = (custom == UseTypeSerializer::Yes ? context.GetRegistrationContext()->GetSerializerForType(typeId) : nullptr))
        {
            // Start by setting the object to be an explicit default.
            output.SetObject();
            return serializer->Store(output, object, defaultObject, typeId, context);
        }

        const SerializeContext::ClassData* classData = context.GetSerializeContext()->FindClassData(typeId);
        if (!classData)
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                AZStd::string::format("Failed to retrieve serialization information for %s.", typeId.ToString<AZStd::string>().c_str()));
        }

        if (!defaultObject && !context.ShouldKeepDefaults())
        {
            ResultCode result(Tasks::WriteValue);
            AZStd::any defaultObjectInstance = context.GetSerializeContext()->CreateAny(typeId);
            if (defaultObjectInstance.empty())
            {
                result = context.Report(Tasks::CreateDefault, Outcomes::Unsupported,
                    "No factory available to create a default object for comparison.");
            }
            void* defaultObjectPtr = AZStd::any_cast<void>(&defaultObjectInstance);
            ResultCode conversionResult = StoreWithClassData(output, object, defaultObjectPtr, *classData, StoreTypeId::No
                , UseTypeSerializer::Yes, context);
            return ResultCode::Combine(result, conversionResult);
        }
        else
        {
            return StoreWithClassData(output, object, defaultObject, *classData, StoreTypeId::No, custom, context);
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreFromPointer(rapidjson::Value& output, const void* object,
        const void* defaultObject, const Uuid& typeId, UseTypeSerializer custom, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        const SerializeContext::ClassData* classData = context.GetSerializeContext()->FindClassData(typeId);
        if (!classData)
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown, 
                AZStd::string::format("Failed to retrieve serialization information for %s.", typeId.ToString<AZStd::string>().c_str()));
        }
        if (!classData->m_azRtti)
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                AZStd::string::format("Failed to retrieve rtti information for %s.", classData->m_name));
        }
        AZ_Assert(classData->m_azRtti->GetTypeId() == typeId, "Type id mismatch in '%s' during serialization to a json file. (%s vs %s)",
            classData->m_name, classData->m_azRtti->GetTypeId().ToString<AZStd::string>().c_str(), typeId.ToString<AZStd::string>().c_str());

        return StoreWithClassDataFromPointer(output, object, defaultObject, *classData, custom, context);
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreWithClassData(rapidjson::Value& node, const void* object,
        const void* defaultObject, const SerializeContext::ClassData& classData, StoreTypeId storeTypeId,
        UseTypeSerializer custom, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        // Start by setting the object to be an explicit default.
        node.SetObject();

        auto serializer = custom == UseTypeSerializer::Yes
            ? context.GetRegistrationContext()->GetSerializerForType(classData.m_typeId) : nullptr;

        if (serializer)
        {
            ResultCode result = serializer->Store(node, object, defaultObject, classData.m_typeId, context);
            if (storeTypeId == StoreTypeId::Yes && result.GetProcessing() != Processing::Halted)
            {
                result.Combine(InsertTypeId(node, classData, context));
            }
            return result;
        }

        if (classData.m_azRtti && (classData.m_azRtti->GetTypeTraits() & AZ::TypeTraits::is_enum) == AZ::TypeTraits::is_enum)
        {
            if (storeTypeId == StoreTypeId::Yes)
            {
                return context.Report(Tasks::WriteValue, Outcomes::Catastrophic,
                    "Unable to store type information in a JSON Serializer enum.");
            }
            return StoreEnum(node, object, defaultObject, classData, context);
        }

        if (classData.m_azRtti && classData.m_azRtti->GetGenericTypeId() != classData.m_typeId)
        {
            serializer = context.GetRegistrationContext()->GetSerializerForType(classData.m_azRtti->GetGenericTypeId());
            if (serializer)
            {
                ResultCode result = serializer->Store(node, object, defaultObject, classData.m_typeId, context);
                if (storeTypeId == StoreTypeId::Yes && result.GetProcessing() != Processing::Halted)
                {
                    result.Combine(InsertTypeId(node, classData, context));
                }
                return result;
            }
        }
        
        if (classData.m_container)
        {
            return context.Report(Tasks::ReadField, Outcomes::Unsupported,
                "The Json Serializer uses custom serializers to store containers. If this message is encountered "
                "then a serializer for the target containers is missing, isn't registered or doesn't exist.");
        }
        else
        {
            ResultCode result(Tasks::WriteValue);
            if (storeTypeId == StoreTypeId::Yes)
            {
                // Not using InsertTypeId here to avoid needing to create the temporary value and swap it in that call.
                node.AddMember(rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier),
                    StoreTypeName(classData, context), context.GetJsonAllocator());
                result = ResultCode(Tasks::WriteValue, Outcomes::Success);
            }
            return result.Combine(StoreClass(node, object, defaultObject, classData, context));
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreWithClassDataFromPointer(rapidjson::Value& output, const void* object,
        const void* defaultObject, const SerializeContext::ClassData& classData, UseTypeSerializer custom, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        StoreTypeId storeTypeId = StoreTypeId::No;
        const SerializeContext::ClassData* resolvedClassData = &classData;
        AZStd::any defaultPointerObject;

        ResultCode result(Tasks::RetrieveInfo);
        ResolvePointerResult pointerResolution = ResolvePointer(result, storeTypeId, object, defaultObject, defaultPointerObject,
            resolvedClassData, *classData.m_azRtti, context);
        if (pointerResolution == ResolvePointerResult::FullyProcessed)
        {
            return result;
        }
        else if (pointerResolution == ResolvePointerResult::WriteNull)
        {
            output = rapidjson::Value(rapidjson::kNullType);
            return result;
        }
        else
        {
            return StoreWithClassData(output, object, defaultObject, *resolvedClassData, storeTypeId, custom, context);
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreWithClassElement(rapidjson::Value& parentNode, const void* object,
        const void* defaultObject, const SerializeContext::ClassElement& classElement, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        ScopedContextPath elementPath(context, classElement.m_name);

        const SerializeContext::ClassData* elementClassData =
            context.GetSerializeContext()->FindClassData(classElement.m_typeId);
        if (!elementClassData)
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                AZStd::string::format("Failed to retrieve serialization information for type %s.",
                    classElement.m_typeId.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str()));
        }
        if (!elementClassData->m_azRtti)
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                AZStd::string::format("Failed to retrieve rtti information for %s.", elementClassData->m_name));
        }
        // The SerializeGenericTypeInfo<Data::Asset<T>>::GenericClassGenericAsset is a special case
        // The ClassData typeId is set to the GetAssetClassId(), while the RTTI is set using azrtti_typid<Data::Asset<T>>
        AZ_Assert(elementClassData->m_azRtti->GetTypeId() == elementClassData->m_typeId
            || elementClassData->m_typeId == GetAssetClassId(), "Type id mismatch in '%s' during serialization to a json file. (%s vs %s)",
            elementClassData->m_name, elementClassData->m_azRtti->GetTypeId().ToString<AZStd::string>().c_str(), elementClassData->m_typeId.ToString<AZStd::string>().c_str());

        if (classElement.m_flags & SerializeContext::ClassElement::FLG_NO_DEFAULT_VALUE)
        {
            defaultObject = nullptr;
        }

        if (classElement.m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS)
        {
            // Base class information can be reconstructed so doesn't need to be written to the final json. StoreClass
            // will simply pick up where this left off and write to the same element.
            return StoreClass(parentNode, object, defaultObject, *elementClassData, context);
        }
        else
        {
            rapidjson::Value value;
            ResultCode result = classElement.m_flags & SerializeContext::ClassElement::FLG_POINTER ?
                StoreWithClassDataFromPointer(value, object, defaultObject, *elementClassData, UseTypeSerializer::Yes, context):
                StoreWithClassData(value, object, defaultObject, *elementClassData, StoreTypeId::No, UseTypeSerializer::Yes, context);
            if (result.GetProcessing() != Processing::Halted)
            {
                if (parentNode.IsObject())
                {
                    if (context.ShouldKeepDefaults() || result.GetOutcome() != Outcomes::DefaultsUsed)
                    {
                        parentNode.AddMember(rapidjson::StringRef(classElement.m_name), AZStd::move(value), context.GetJsonAllocator());
                    }
                }
                else
                {
                    result = context.Report(Tasks::WriteValue, Outcomes::Catastrophic,
                        "StoreWithClassElement can only write to objects. Unexpected type encountered.");
                }
            }
            return result;
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreClass(rapidjson::Value& output, const void* object, const void* defaultObject,
        const SerializeContext::ClassData& classData, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        AZ_Assert(output.IsObject(), "Unable to write class to the json node as it's not an object.");
        if (!classData.m_elements.empty())
        {
            ResultCode result(Tasks::WriteValue);
            for (const SerializeContext::ClassElement& element : classData.m_elements)
            {
                const void* elementPtr = reinterpret_cast<const uint8_t*>(object) + element.m_offset;
                const void* elementDefaultPtr = defaultObject ?
                    (reinterpret_cast<const uint8_t*>(defaultObject) + element.m_offset) : nullptr;

                result.Combine(StoreWithClassElement(output, elementPtr, elementDefaultPtr, element, context));
            }
            return result;
        }
        else
        {
            return context.Report(Tasks::WriteValue, context.ShouldKeepDefaults() ? Outcomes::Success : Outcomes::DefaultsUsed,
                "Class didn't contain any elements to store.");
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreEnum(rapidjson::Value& output, const void* object, const void* defaultObject,
        const SerializeContext::ClassData& classData, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        AZ::AttributeReader attributeReader(nullptr, AZ::FindAttribute(AZ::Serialize::Attributes::EnumUnderlyingType, classData.m_attributes));
        AZ::TypeId underlyingTypeId = AZ::TypeId::CreateNull();
        if (!attributeReader.Read<AZ::TypeId>(underlyingTypeId))
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                "Unable to find underlying type of enum in class data.");
        }

        const SerializeContext::ClassData* underlyingClassData = context.GetSerializeContext()->FindClassData(underlyingTypeId);
        if (!underlyingClassData)
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                "Unable to retrieve information for underlying integral type of enum.");
        }

        IRttiHelper* underlyingTypeRtti = underlyingClassData->m_azRtti;
        if (!underlyingTypeRtti)
        {
            return context.Report(Tasks::Convert, Outcomes::Unknown, AZStd::string::format(
                "Rtti for underlying type %s, of enum is not registered. Enum Value can not be stored", underlyingClassData->m_name));
        }

        const size_t underlyingTypeSize = underlyingTypeRtti->GetTypeSize();
        const TypeTraits underlyingTypeTraits = underlyingTypeRtti->GetTypeTraits();
        const bool isSigned = (underlyingTypeTraits & TypeTraits::is_signed) == TypeTraits::is_signed;
        const bool isUnsigned = (underlyingTypeTraits & TypeTraits::is_unsigned) == TypeTraits::is_unsigned;
        if (isSigned)
        {
            // Cast to either an int16_t, int32_t, int64_t depending on the size of the underlying type
            // int8_t is not supported.
            switch (underlyingTypeSize)
            {
            case 2:
                return StoreUnderlyingEnumType(output, reinterpret_cast<const int16_t*>(object),
                    reinterpret_cast<const int16_t*>(defaultObject), classData, context);
            case 4:
                return StoreUnderlyingEnumType(output, reinterpret_cast<const int32_t*>(object),
                    reinterpret_cast<const int32_t*>(defaultObject), classData, context);
            case 8:
                return StoreUnderlyingEnumType(output, reinterpret_cast<const int64_t*>(object),
                    reinterpret_cast<const int64_t*>(defaultObject), classData, context);
            default:
                return context.Report(Tasks::Convert, Outcomes::Unknown,
                    AZStd::string::format("Signed Underlying type with size=%zu is not supported by Enum Serializer", underlyingTypeSize));
            }
        }
        else if (isUnsigned)
        {
            // Cast to either an uint8_t, uint16_t, uint32_t, uint64_t depending on the size of the underlying type
            switch (underlyingTypeSize)
            {
            case 1:
                return StoreUnderlyingEnumType(output, reinterpret_cast<const uint8_t*>(object),
                    reinterpret_cast<const uint8_t*>(defaultObject), classData, context);
            case 2:
                return StoreUnderlyingEnumType(output, reinterpret_cast<const uint16_t*>(object),
                    reinterpret_cast<const uint16_t*>(defaultObject), classData, context);
            case 4:
                return StoreUnderlyingEnumType(output, reinterpret_cast<const uint32_t*>(object),
                    reinterpret_cast<const uint32_t*>(defaultObject), classData, context);
            case 8:
                return StoreUnderlyingEnumType(output, reinterpret_cast<const uint64_t*>(object),
                    reinterpret_cast<const uint64_t*>(defaultObject), classData, context);
            default:
                return context.Report(Tasks::Convert, Outcomes::Unknown,
                    AZStd::string::format("Unsigned Underlying type with size=%zu is not supported by Enum Serializer", underlyingTypeSize));
            }
        }

        return context.Report(Tasks::WriteValue, Outcomes::Catastrophic, AZStd::string::format(
            "Unsupported underlying type %s for enum %s. Enum value can not be stored.", underlyingClassData->m_name, classData.m_name));
    }

    template<typename UnderlyingType>
    JsonSerializationResult::Result JsonSerializer::StoreUnderlyingEnumType(rapidjson::Value& outputValue, const UnderlyingType* value,
        const UnderlyingType* defaultValue, const SerializeContext::ClassData& classData, JsonSerializerContext& context)
    {
        /*
             * The implementation strategy for storing an enumeration with Json is to first look for an exact value match for the enumeration
             * using the values registered within the SerializeContext
             * If that is found, the enum option is stored as a Json string
             * If an exact value cannot be found, then for each value registered with the SerializeContext, the values are assumed to be bitflags.
             * The values are checked using bitwise-and against the input.
             * For each registered enum value that is found, its bit value is cleared from enum and processing continues.
             * Afterwards the enum option is added as a string to a Json array
             * Finally if there is any non-zero value that is left after processing each registered enum option,
             * it gets added as an integral value to a Json array
             */

        using namespace JsonSerializationResult;

        UnderlyingType enumInputValue{ *value };
        
        using EnumConstantBase = SerializeContextEnumInternal::EnumConstantBase;

        using EnumConstantBasePtr = AZStd::unique_ptr<SerializeContextEnumInternal::EnumConstantBase>;
        //! Comparison function which treats each enum value as unsigned for parsing purposes.
        //! The set below is for parsing each enum value as a set of bit flags, where it is configured
        //! to parse values with higher unsigned enum values first
        struct EnumUnsignedCompare
        {
            bool operator()(const AZStd::reference_wrapper<EnumConstantBase>& lhs, const AZStd::reference_wrapper<EnumConstantBase>& rhs)
            {
                return static_cast<EnumConstantBase&>(lhs).GetEnumValueAsUInt() > static_cast<EnumConstantBase&>(rhs).GetEnumValueAsUInt();
            }
        };

        AZStd::set<AZStd::reference_wrapper<EnumConstantBase>, EnumUnsignedCompare> enumConstantSet;
        for (const AZ::AttributeSharedPair& enumAttributePair : classData.m_attributes)
        {
            if (enumAttributePair.first == AZ::Serialize::Attributes::EnumValueKey)
            {
                auto enumConstantAttribute = azrtti_cast<AZ::AttributeData<EnumConstantBasePtr>*>(enumAttributePair.second.get());
                if (!enumConstantAttribute)
                {
                    context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                        "EnumConstant element was unable to be read from attribute ID 'EnumValue'");
                    continue;
                }

                const EnumConstantBasePtr& enumConstantPtr = enumConstantAttribute->Get(nullptr);
                UnderlyingType enumConstantAsInt = static_cast<UnderlyingType>(AZStd::is_signed<UnderlyingType>::value ?
                    enumConstantPtr->GetEnumValueAsInt() : enumConstantPtr->GetEnumValueAsUInt());

                if (enumInputValue == enumConstantAsInt)
                {
                    if (context.ShouldKeepDefaults() || !defaultValue || enumInputValue != *defaultValue)
                    {
                        outputValue.SetString(enumConstantPtr->GetEnumValueName().data(),
                            aznumeric_cast<rapidjson::SizeType>(enumConstantPtr->GetEnumValueName().size()), context.GetJsonAllocator());
                        return context.Report(Tasks::WriteValue, Outcomes::Success, "Successfully stored Enum value as string");
                    }
                    else
                    {
                        return context.Report(Tasks::WriteValue, Outcomes::DefaultsUsed, "Enum value defaulted as string");
                    }
                }

                //! A registered enum with a value of 0 that doesn't match the exact enum value is not output as part of the json array
                if (enumConstantAsInt == 0)
                {
                    continue;
                }

                //! Add registered enum value where all of bits set within the input enum value
                if ((enumConstantAsInt & enumInputValue) == enumConstantAsInt)
                {
                    enumConstantSet.emplace(*enumConstantPtr);
                }
            }
        }

        if (!context.ShouldKeepDefaults() && defaultValue && enumInputValue == *(defaultValue))
        {
            return context.Report(Tasks::WriteValue, Outcomes::DefaultsUsed,
                enumConstantSet.empty() ? "Enum value defaulted as int" : "Enum value defaulted as array");

        }

        // Special case: If none of the enum values share bits with the current input value, write out the value as an integral
        if (enumConstantSet.empty())
        {
            outputValue = enumInputValue;
            return context.Report(Tasks::WriteValue, Outcomes::Success, "Successfully stored Enum value as int");
        }
        else
        {
            // Mask of each enum constant bits from the input value and add that value to the JsonArray
            outputValue.SetArray();
            UnderlyingType enumInputBitset = enumInputValue;
            for (const EnumConstantBase& enumConstant : enumConstantSet)
            {
                enumInputBitset = enumInputBitset & ~enumConstant.GetEnumValueAsUInt();
                rapidjson::Value enumConstantNameValue(enumConstant.GetEnumValueName().data(),
                    aznumeric_cast<rapidjson::SizeType>(enumConstant.GetEnumValueName().size()), context.GetJsonAllocator());
                outputValue.PushBack(AZStd::move(enumConstantNameValue), context.GetJsonAllocator());
            }

            //! If after all the enum values whose bitflags were mask'ed out of the input enum
            //! the input enum value is still non-zero, that means that the remaining value is represented 
            //! is a raw integral of the left over flags
            //! Therefore it gets written to the json array has an integral value
            if (enumInputBitset != 0)
            {
                outputValue.PushBack(rapidjson::Value(enumInputBitset), context.GetJsonAllocator());
            }
        }

        return context.Report(Tasks::WriteValue, Outcomes::Success, "Enum value defaulted as array");
    }

    JsonSerializer::ResolvePointerResult JsonSerializer::ResolvePointer(
        JsonSerializationResult::ResultCode& status, StoreTypeId& storeTypeId,
        const void*& object, const void*& defaultObject, AZStd::any& defaultObjectStorage,
        const SerializeContext::ClassData*& elementClassData, const AZ::IRttiHelper& rtti,
        JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        object = *reinterpret_cast<const void* const*>(object);
        defaultObject = defaultObject ? *reinterpret_cast<const void* const*>(defaultObject) : nullptr;
        if (!object)
        {
            status = ResultCode(status.GetTask(), (context.ShouldKeepDefaults() || defaultObject) ? Outcomes::Success : Outcomes::DefaultsUsed);
            return ResolvePointerResult::WriteNull;
        }

        // The pointer is pointing to an instance, so determine if the pointer is polymorphic. If so, make to 
        // tell the caller of this function to write the type id and provide a default object, if requested, for
        // the specific polymorphic instance the pointer is pointing to.
        const AZ::Uuid& actualClassId = rtti.GetActualUuid(object);

        // Note: If it is crashing here, it might be that you're serializing a pointer and forgot to initialize it with nullptr.
        // Check the elementClassData to identify the causing element.
        const AZ::Uuid& actualDefaultClassId = rtti.GetActualUuid(defaultObject);

        if (actualClassId != rtti.GetTypeId())
        {
            elementClassData = context.GetSerializeContext()->FindClassData(actualClassId);
            if (!elementClassData)
            {
                status = context.Report(Tasks::RetrieveInfo, Outcomes::Unknown, AZStd::string::format(
                    "Failed to retrieve serialization information for because type '%s' is unknown.", actualClassId.ToString<AZStd::string>().c_str()));
                return ResolvePointerResult::FullyProcessed;
            }

            storeTypeId = (actualClassId != actualDefaultClassId || context.ShouldKeepDefaults()) ?
                StoreTypeId::Yes : StoreTypeId::No;
            object = rtti.Cast(object, actualClassId);
        }
        if (actualDefaultClassId != actualClassId)
        {
            // Defaults is pointing to a different type than is stored, so those defaults can't be used as a reference.
            defaultObject = nullptr;
        }

        if (defaultObject)
        {
            defaultObject = rtti.Cast(defaultObject, actualDefaultClassId);
        }
        else if (!context.ShouldKeepDefaults())
        {
            defaultObjectStorage = context.GetSerializeContext()->CreateAny(actualClassId);
            if (defaultObjectStorage.empty())
            {
                status = context.Report(Tasks::CreateDefault, Outcomes::Unsupported, AZStd::string::format(
                    "No factory available to create a default pointer object for comparison for type %s.", actualClassId.ToString<AZStd::string>().c_str()));
                return ResolvePointerResult::FullyProcessed;
            }
            defaultObject = AZStd::any_cast<void>(&defaultObjectStorage);
        }
        return ResolvePointerResult::ContinueProcessing;
    }

    rapidjson::Value JsonSerializer::StoreTypeName(const SerializeContext::ClassData& classData, JsonSerializerContext& context)
    {
        rapidjson::Value result;
        AZStd::vector<Uuid> ids = context.GetSerializeContext()->FindClassId(Crc32(classData.m_name));
        if (ids.size() <= 1)
        {
            result = rapidjson::StringRef(classData.m_name);
        }
        else
        {
            // Only write the Uuid for the class if there are multiple classes sharing the same name.
            // In this case it wouldn't be enough to determine which class needs to be used. The 
            // class name is still added as a comment for be friendlier for users to read.
            AZStd::string fullName = classData.m_typeId.ToString<AZStd::string>();
            fullName += ' ';
            fullName += classData.m_name;
            result.SetString(fullName.c_str(), aznumeric_caster(fullName.size()), context.GetJsonAllocator());
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreTypeName(rapidjson::Value& output,
        const Uuid& typeId, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        const SerializeContext::ClassData* data = context.GetSerializeContext()->FindClassData(typeId);
        if (data)
        {
            output = JsonSerializer::StoreTypeName(*data, context);
            return context.Report(Tasks::WriteValue, Outcomes::Success, "Type id successfully stored to json value.");
        }
        else
        {
            output = JsonSerializer::GetExplicitDefault();
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                AZStd::string::format("Unable to retrieve description for type %s.", typeId.ToString<AZStd::string>().c_str()));
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::InsertTypeId(
        rapidjson::Value& output, const SerializeContext::ClassData& classData, JsonSerializerContext& context)
    {
        using namespace JsonSerializationResult;

        if (output.IsObject())
        {
            rapidjson::Value insertedObject(rapidjson::kObjectType);
            insertedObject.AddMember(
                rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier), StoreTypeName(classData, context),
                context.GetJsonAllocator());

            for (auto& element : output.GetObject())
            {
                insertedObject.AddMember(AZStd::move(element.name), AZStd::move(element.value), context.GetJsonAllocator());
            }
            output = AZStd::move(insertedObject);
            return ResultCode(Tasks::WriteValue, Outcomes::Success);
        }
        else
        {
            return context.Report(Tasks::WriteValue, Outcomes::Catastrophic, "Only able to store type information in a JSON Object.");
        }
    }

    rapidjson::Value JsonSerializer::GetExplicitDefault()
    {
        return rapidjson::Value(rapidjson::kObjectType);
    }
} // namespace AZ

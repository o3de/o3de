/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzCore/RTTI/TypeInfo.h"
#include <AzCore/Math/UuidSerializer.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonDeserializer.h>
#include <AzCore/Serialization/Json/JsonStringConversionUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    JsonSerializationResult::ResultCode JsonDeserializer::DeserializerDefaultCheck(BaseJsonSerializer* serializer, void* object,
        const Uuid& typeId,const rapidjson::Value& value, bool isNewInstance, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        bool isExplicitDefault = IsExplicitDefault(value);
        bool manuallyDefaults = (serializer->GetOperationsFlags() & BaseJsonSerializer::OperationFlags::ManualDefault) ==
            BaseJsonSerializer::OperationFlags::ManualDefault;
        bool initializeNewInstance = (serializer->GetOperationsFlags() & BaseJsonSerializer::OperationFlags::InitializeNewInstance) ==
            BaseJsonSerializer::OperationFlags::InitializeNewInstance;

        return
            !isExplicitDefault || (isExplicitDefault && manuallyDefaults) || (isExplicitDefault && isNewInstance && initializeNewInstance)
            ? serializer->Load(object, typeId, value, context)
            : context.Report(Tasks::ReadField, Outcomes::DefaultsUsed, "Value has an explicit default.");
    }

    JsonSerializationResult::ResultCode JsonDeserializer::Load(
        void* object, const Uuid& typeId, const rapidjson::Value& value, bool isNewInstance, UseTypeDeserializer custom,
        JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        if (!object)
        {
            return context.Report(Tasks::ReadField, Outcomes::Catastrophic,
                "Target object for Json Serialization is pointing to nothing during loading.");
        }

        if (BaseJsonSerializer* serializer
            = (custom == UseTypeDeserializer::Yes ? context.GetRegistrationContext()->GetSerializerForType(typeId) : nullptr))
        {
            return DeserializerDefaultCheck(serializer, object, typeId, value, isNewInstance, context);
        }

        const SerializeContext::ClassData* classData = context.GetSerializeContext()->FindClassData(typeId);
        if (!classData)
        {
            return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                AZStd::string::format("Failed to retrieve serialization information for %s.", typeId.ToString<AZStd::string>().c_str()));
        }

        if (classData->m_azRtti && classData->m_azRtti->GetGenericTypeId() != typeId)
        {
            if (((classData->m_azRtti->GetTypeTraits() & (AZ::TypeTraits::is_signed | AZ::TypeTraits::is_unsigned)) != AZ::TypeTraits{0}) &&
                context.GetSerializeContext()->GetUnderlyingTypeId(typeId) == classData->m_typeId)
            {
                // This value is from an enum, where a field has been reflected using ClassBuilder::Field, but the enum
                // type itself has not been reflected using EnumBuilder. Treat it as an enum.
                return LoadEnum(object, *classData, value, context);
            }
            
            if (BaseJsonSerializer* serializer
                = (custom == UseTypeDeserializer::Yes)
                    ? context.GetRegistrationContext()->GetSerializerForType(classData->m_azRtti->GetGenericTypeId())
                    : nullptr)
            {
                return DeserializerDefaultCheck(serializer, object, typeId, value, isNewInstance, context);
            }
        }

        if (IsExplicitDefault(value))
        {
            return context.Report(Tasks::ReadField, Outcomes::DefaultsUsed, "Value has an explicit default.");
        }
        
        if (classData->m_azRtti && (classData->m_azRtti->GetTypeTraits() & AZ::TypeTraits::is_enum) == AZ::TypeTraits::is_enum)
        {
            return LoadEnum(object, *classData, value, context);
        }
        if (classData->m_container)
        {
            return context.Report(Tasks::ReadField, Outcomes::Unsupported,
                "The Json Serializer uses custom serializers to load containers. If this message is encountered "
                "then a serializer for the target containers is missing, isn't registered or doesn't exist.");
        }
        if (value.IsObject())
        {
            return LoadClass(object, *classData, value, context);
        }
        return context.Report(Tasks::ReadField, Outcomes::Unsupported,
            AZStd::string::format("Reading into targets of type '%s' is not supported.", classData->m_name));
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadToPointer(void* object, const Uuid& typeId,
        const rapidjson::Value& value, UseTypeDeserializer useCustom, JsonDeserializerContext& context)
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
        AZ_Assert(classData->m_azRtti->GetTypeId() == typeId, "Type id mismatch during deserialization of a json file. (%s vs %s)",
            classData->m_azRtti->GetTypeId().ToString<AZStd::string>().c_str(), typeId.ToString<AZStd::string>().c_str());

        void** objectPtr = reinterpret_cast<void**>(object);
        bool isNull = *objectPtr == nullptr;
        Uuid resolvedTypeId = typeId;
        ResultCode status(Tasks::RetrieveInfo);

        ResolvePointerResult pointerResolution = ResolvePointer(objectPtr, resolvedTypeId, status, value, *classData->m_azRtti, context);
        if (pointerResolution == ResolvePointerResult::FullyProcessed)
        {
            return status;
        }
        else
        {
            const SerializeContext::ClassData* resolvedClassData = context.GetSerializeContext()->FindClassData(resolvedTypeId);
            if (resolvedClassData)
            {
                status = JsonDeserializer::Load(*objectPtr, resolvedTypeId, value, true, useCustom, context);

                *objectPtr = resolvedClassData->m_azRtti->Cast(*objectPtr, typeId);

                if (isNull && (status.GetProcessing() == Processing::Halted || status.GetProcessing() == Processing::Altered))
                {
                    // If object was pointing to null it means an additional step was taken to create an instance to load to.
                    // If loading fails, that steps has to be undone to adhere to the no-side effects rule.
                    AZ_Assert(resolvedClassData->m_factory,
                        "Expected class data to have a factory as it was previously used in the Json Deserializer to create a new instance.");
                    resolvedClassData->m_factory->Destroy(*objectPtr);
                    *objectPtr = nullptr;
                }
                return status;
            }
            else
            {
                return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                    AZStd::string::format("Failed to retrieve serialization information for pointer type %s.", typeId.ToString<AZStd::string>().c_str()));
            }
        }
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadWithClassElement(void* object, const rapidjson::Value& value,
        const SerializeContext::ClassElement& classElement, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        if (classElement.m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
        {
            if (!classElement.m_azRtti)
            {
                return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                    AZStd::string::format("Failed to retrieve rtti information for %s.", classElement.m_name));
            }
            AZ_Assert(classElement.m_azRtti->GetTypeId() == classElement.m_typeId,
                "Type id mismatch during deserialization of a json file. (%s vs %s)");
            return LoadToPointer(object, classElement.m_typeId, value, UseTypeDeserializer::Yes, context);
        }
        else
        {
            return Load(object, classElement.m_typeId, value, false, UseTypeDeserializer::Yes, context);
        }
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadClass(void* object, const SerializeContext::ClassData& classData,
        const rapidjson::Value& value, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        AZ_Assert(context.GetRegistrationContext() && context.GetSerializeContext(), "Expected valid registration context and serialize context.");

        size_t numLoads = 0;
        ResultCode retVal(Tasks::ReadField);
        for (auto iter = value.MemberBegin(); iter != value.MemberEnd(); ++iter)
        {
            const rapidjson::Value& val = iter->value;
            AZStd::string_view name(iter->name.GetString(), iter->name.GetStringLength());
            if (name == JsonSerialization::TypeIdFieldIdentifier)
            {
                continue;
            }
            Crc32 nameCrc(name);
            ElementDataResult foundElementData = FindElementByNameCrc(*context.GetSerializeContext(), object, classData, nameCrc);

            ScopedContextPath subPath(context, name);
            if (foundElementData.m_found)
            {
                ResultCode result = LoadWithClassElement(foundElementData.m_data, val, *foundElementData.m_info, context);
                retVal.Combine(result);

                if (result.GetProcessing() == Processing::Halted)
                {
                    return context.Report(result, "Loading of element has failed.");
                }
                else if (result.GetProcessing() != Processing::Altered)
                {
                    numLoads++;
                }
            }
            else
            {
                retVal.Combine(context.Report(Tasks::ReadField, Outcomes::Skipped,
                    "Skipping field as there's no matching variable in the target."));
            }
        }

        size_t elementCount = CountElements(*context.GetSerializeContext(), classData);
        if (elementCount > numLoads)
        {
            retVal.Combine(ResultCode(Tasks::ReadField, numLoads == 0 ? Outcomes::DefaultsUsed : Outcomes::PartialDefaults));
        }

        return retVal;
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadEnum(void* object, const SerializeContext::ClassData& classData,
        const rapidjson::Value& value, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::AttributeReader attributeReader(nullptr, AZ::FindAttribute(AZ::Serialize::Attributes::EnumUnderlyingType, classData.m_attributes));
        AZ::TypeId underlyingTypeId = AZ::TypeId::CreateNull();
        if (!attributeReader.Read<AZ::TypeId>(underlyingTypeId))
        {
            // for non-reflected enums, the passed-in classData already represents the enum's underlying type
            if (context.GetSerializeContext()->GetUnderlyingTypeId(classData.m_typeId) == classData.m_typeId)
            {
                underlyingTypeId = classData.m_typeId;
            }
            else
            {
                return context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                    "Unable to find underlying type of enum in class data.");
            }
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
                return LoadUnderlyingEnumType(*reinterpret_cast<int16_t*>(object), classData, value, context);
            case 4:
                return LoadUnderlyingEnumType(*reinterpret_cast<int32_t*>(object), classData, value, context);
            case 8:
                return LoadUnderlyingEnumType(*reinterpret_cast<int64_t*>(object), classData, value, context);
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
                return LoadUnderlyingEnumType(*reinterpret_cast<uint8_t*>(object), classData, value, context);
            case 2:
                return LoadUnderlyingEnumType(*reinterpret_cast<uint16_t*>(object), classData, value, context);
            case 4:
                return LoadUnderlyingEnumType(*reinterpret_cast<uint32_t*>(object), classData, value, context);
            case 8:
                return LoadUnderlyingEnumType(*reinterpret_cast<uint64_t*>(object), classData, value, context);
            default:
                return context.Report(Tasks::Convert, Outcomes::Unknown,
                    AZStd::string::format("Unsigned Underlying type with size=%zu is not supported by Enum Serializer", underlyingTypeSize));
            }
        }

        return context.Report(Tasks::WriteValue, Outcomes::Catastrophic,
            AZStd::string::format("Unsupported underlying type %s for enum %s. Cannot load into enum output.",
                underlyingClassData->m_name, classData.m_name));
    }

    template<typename UnderlyingType>
    JsonSerializationResult::Result JsonDeserializer::LoadUnderlyingEnumType(UnderlyingType& outputValue,
        const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadEnumFromContainer(outputValue, classData, inputValue, context);
        case rapidjson::kStringType:
            return LoadEnumFromString(outputValue, classData, inputValue, context);
        case rapidjson::kNumberType:
            return LoadEnumFromNumber(outputValue, classData, inputValue, context);
        case rapidjson::kObjectType: // unsupported types
            // fall through
        case rapidjson::kNullType:
            // fall through
        case rapidjson::kFalseType:
            // fall through
        case rapidjson::kTrueType:
            return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported,
                "Unsupported type. Enums must be read from an array, string or number");
        default:
            return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unknown,
                "Unknown json type encountered for deserialization. This function should only be called from Load() which guarantees this never occurs.");
        }
    }

    template<typename UnderlyingType>
    JsonSerializationResult::Result JsonDeserializer::LoadEnumFromNumber(UnderlyingType& outputValue,
        const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        ResultCode result = ResultCode(Tasks::ReadField, Outcomes::Unsupported);
        if (inputValue.IsUint64())
        {
            result = JsonNumericCast(outputValue, inputValue.GetUint64(), context.GetPath(), context.GetReporter());
        }
        else if (inputValue.IsInt64())
        {
            result = JsonNumericCast(outputValue, inputValue.GetInt64(), context.GetPath(), context.GetReporter());
        }

        if (result.GetOutcome() == Outcomes::Success)
        {
            return context.Report(result, "Successfully read enum value from number field.");
        }
        else
        {
            return context.Report(result,
                ReportAvailableEnumOptions("Failed to read enum value from number field.",
                    classData.m_attributes, AZStd::is_signed<UnderlyingType>::value));
        }
    }

    template<typename UnderlyingType>
    JsonSerializationResult::Result JsonDeserializer::LoadEnumFromString(UnderlyingType& outputValue,
        const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view enumString(inputValue.GetString(), inputValue.GetStringLength());
        if (enumString.empty())
        {
            return context.Report(Tasks::ReadField, Outcomes::DefaultsUsed, "Input string is empty, no enumeration can be parsed from it.");
        }

        // Replace the reporting callback with an empty function as the SerializeInternal::TextToValue function is allowed to fail in this case.
        // As the enum value can be either an int or a string, there should not be an error reporting that the string was unable to be converted
        // to an int.
        {
            ScopedContextReporter reporter(context,
                [](AZStd::string_view, ResultCode resultCode, AZStd::string_view) -> ResultCode
            {
                return resultCode;
            });
            JsonSerializationResult::Result textToValueResult = SerializerInternal::TextToValue(&outputValue, enumString.data(), context);
            if (textToValueResult.GetResultCode().GetOutcome() == JsonSerializationResult::Outcomes::Success)
            {
                return textToValueResult;
            }
        }

        for (const AttributeSharedPair& enumAttributePair : classData.m_attributes)
        {
            if (enumAttributePair.first != Serialize::Attributes::EnumValueKey)
            {
                continue;
            }

            using EnumConstantPtr = AZStd::unique_ptr<SerializeContextEnumInternal::EnumConstantBase>;
            auto enumConstantAttribute = azrtti_cast<AZ::AttributeData<EnumConstantPtr>*>(enumAttributePair.second.get());
            if (!enumConstantAttribute)
            {
                context.Report(Tasks::RetrieveInfo, Outcomes::Unknown, "EnumConstant element was unable to be read from attribute ID 'EnumValue'");
                continue;
            }

            const EnumConstantPtr& enumConstantPtr = enumConstantAttribute->Get(nullptr);
            if (enumString.size() == enumConstantPtr->GetEnumValueName().size()
                && azstrnicmp(enumString.data(), enumConstantPtr->GetEnumValueName().data(), enumString.size()) == 0)
            {
                UnderlyingType enumConstantAsInt = aznumeric_cast<UnderlyingType>(AZStd::is_signed<UnderlyingType>::value ?
                    enumConstantPtr->GetEnumValueAsInt() : enumConstantPtr->GetEnumValueAsUInt());
                outputValue = enumConstantAsInt;
                return context.Report(Tasks::ReadField, Outcomes::Success, "Successfully loaded enum value from string");
            }
        }

        return context.Report(Tasks::ReadField, Outcomes::Unsupported, ReportAvailableEnumOptions(
            "Unsupported value for string. Enum value could not read.", classData.m_attributes, AZStd::is_signed<UnderlyingType>::value));
    }

    template<typename UnderlyingType>
    JsonSerializationResult::Result JsonDeserializer::LoadEnumFromContainer(UnderlyingType& outputValue,
        const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        const rapidjson::SizeType arraySize = inputValue.Size();
        if (arraySize == 0)
        {
            return context.Report(Tasks::ReadField, Outcomes::DefaultsUsed, "Input array is empty, no enumeration can be parsed from it.");
        }

        // Zero the underlying type output as any enum values will be bitwise-or'ed into the output
        UnderlyingType finalEnum = {};

        ResultCode result = ResultCode(Tasks::ReadField);

        for (rapidjson::SizeType i = 0; i < arraySize; ++i)
        {
            ScopedContextPath enumArraySubPath(context, i);

            UnderlyingType enumConstant{};
            const rapidjson::Value& jsonInputValueElement = inputValue[i];
            switch (jsonInputValueElement.GetType())
            {
            case rapidjson::kStringType:
                result.Combine(LoadEnumFromString(enumConstant, classData, jsonInputValueElement, context));
                break;
            case rapidjson::kNumberType:
                result.Combine(LoadEnumFromNumber(enumConstant, classData, jsonInputValueElement, context));
                break;
            case rapidjson::kArrayType:
                // fall through
            case rapidjson::kObjectType:
                // fall through
            case rapidjson::kNullType:
                // fall through
            case rapidjson::kFalseType:
                // fall through
            case rapidjson::kTrueType:
                result.Combine(context.Report(Tasks::ReadField, Outcomes::Unsupported,
                    "Unsupported type. Enum values within an array must either be a string or a number."));
                break;

            default:
                return context.Report(Tasks::ReadField, Outcomes::Unknown,
                    "Unknown json type encountered for deserialization from a enumeration.");
            }

            if (result.GetProcessing() == Processing::Halted)
            {
                return context.Report(result, "Enum value could not be parsed from array");
            }

            //! bitwise-or the enumConstant value to the outputValue parameter.
            //! enumConstant is zero-initialized using {}, so for non-string/non-int path this operations is a no-op
            finalEnum |= enumConstant;
        }

        outputValue = finalEnum;
        return context.Report(result, "Enum value has been parsed from array");
    }

    AZStd::string JsonDeserializer::ReportAvailableEnumOptions(AZStd::string_view message,
        const AZStd::vector<AttributeSharedPair, AZStdFunctorAllocator>& attributes, bool signedValues)
    {
        AZStd::string result = message;
        result += " Available options are: ";
        bool first = true;
        for (const AZ::AttributeSharedPair& enumAttributePair : attributes)
        {
            if (enumAttributePair.first != AZ::Serialize::Attributes::EnumValueKey)
            {
                continue;
            }

            using EnumConstantPtr = AZStd::unique_ptr<SerializeContextEnumInternal::EnumConstantBase>;
            auto enumConstantAttribute = azrtti_cast<AZ::AttributeData<EnumConstantPtr>*>(enumAttributePair.second.get());
            if (!enumConstantAttribute)
            {
                continue;
            }

            const EnumConstantPtr& enumConstantPtr = enumConstantAttribute->Get(nullptr);
            if (!first)
            {
                result += ',';
                result += ' ';
            }
            else
            {
                first = false;
            }

            result += '"';
            result += enumConstantPtr->GetEnumValueName();
            result += '"';
            result += ' ';
            result += '(';
            result += AZStd::to_string(signedValues ? enumConstantPtr->GetEnumValueAsInt() : enumConstantPtr->GetEnumValueAsUInt());
            result += ')';
        }

        return result;
    }

    JsonDeserializer::ResolvePointerResult JsonDeserializer::ResolvePointer(void** object, Uuid& objectType, JsonSerializationResult::ResultCode& status,
        const rapidjson::Value& pointerData, const AZ::IRttiHelper& rtti, JsonDeserializerContext& context)
    {
        using namespace AZ::JsonSerializationResult;

        if (pointerData.IsNull())
        {
            // Json files stores a nullptr, but there has been data assigned to the pointer, so the data needs to be freed.
            if (*object)
            {
                const AZ::Uuid& actualClassId = rtti.GetActualUuid(*object);
                const SerializeContext::ClassData* actualClassData = context.GetSerializeContext()->FindClassData(actualClassId);
                if (!actualClassData)
                {
                    status = context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                        AZStd::string::format("Unable to find serialization information for type %s.", actualClassId.ToString<AZStd::string>().c_str()));
                    return ResolvePointerResult::FullyProcessed;
                }

                if (actualClassData->m_factory)
                {
                    actualClassData->m_factory->Destroy(*object);
                    *object = nullptr;
                }
                else
                {
                    status = context.Report(Tasks::RetrieveInfo, Outcomes::Catastrophic,
                        "Unable to find the factory needed to clear out the default value.");
                    return ResolvePointerResult::FullyProcessed;
                }
            }
            status = ResultCode(Tasks::ReadField, Outcomes::Success);
            return ResolvePointerResult::FullyProcessed;
        }
        else
        {
            // There's data stored in the JSON document so try to determine the type, create an instance of it and 
            // return the new object to continue loading.
            LoadTypeIdResult loadedTypeId = LoadTypeIdFromJsonObject(pointerData, rtti, context);
            if (loadedTypeId.m_determination == TypeIdDetermination::FailedToDetermine ||
                loadedTypeId.m_determination == TypeIdDetermination::FailedDueToMultipleTypeIds)
            {
                    auto typeField = pointerData.FindMember(JsonSerialization::TypeIdFieldIdentifier);
                    if (typeField != pointerData.MemberEnd() && typeField->value.IsString())
                    {
                        const char* format = loadedTypeId.m_determination == TypeIdDetermination::FailedToDetermine ?
                            "Unable to resolve provided type: %.*s." : 
                            "Unable to resolve provided type %.*s because the same name points to multiple types.";
                        status = context.Report(Tasks::RetrieveInfo, Outcomes::Unknown, 
                            AZStd::string::format(format, typeField->value.GetStringLength(), typeField->value.GetString()));
                    }
                    else
                    {
                        const char* message = loadedTypeId.m_determination == TypeIdDetermination::FailedToDetermine ?
                            "Unable to resolve provided type." :
                            "Unable to resolve provided type because the same name points to multiple types.";
                        status = context.Report(Tasks::RetrieveInfo, Outcomes::Unknown, message);
                    }
                    return ResolvePointerResult::FullyProcessed;
            }

            if (loadedTypeId.m_typeId != objectType)
            {
                const SerializeContext::ClassData* targetClassData = context.GetSerializeContext()->FindClassData(loadedTypeId.m_typeId);
                if (targetClassData)
                {
                    if (!context.GetSerializeContext()->CanDowncast(loadedTypeId.m_typeId, objectType, targetClassData->m_azRtti, &rtti))
                    {
                        status = context.Report(Tasks::Convert, Outcomes::TypeMismatch,
                            "Requested type can't be cast to the type of the target value.");
                        return ResolvePointerResult::FullyProcessed;
                    }
                }
                else
                {
                    using ReporterString = AZStd::fixed_string<1024>;
                    status = context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                        ReporterString::format("Serialization information for target type %s not found.", loadedTypeId.m_typeId.ToString<ReporterString>().c_str()));
                    return ResolvePointerResult::FullyProcessed;
                }
                objectType = loadedTypeId.m_typeId;
            }

            if (*object)
            {
                // The pointer already has an assigned value so look if the type is the same, in which case load
                // on top of it, or clean it up and create a new instance if they're not the same.
                if (loadedTypeId.m_determination == TypeIdDetermination::ExplicitTypeId)
                {
                    const AZ::Uuid& actualClassId = rtti.GetActualUuid(*object);
                    if (actualClassId != loadedTypeId.m_typeId)
                    {
                        const SerializeContext::ClassData* actualClassData = context.GetSerializeContext()->FindClassData(actualClassId);
                        if (!actualClassData)
                        {
                            status = context.Report(Tasks::RetrieveInfo, Outcomes::Unknown, 
                                AZStd::string::format("Unable to find serialization information for type %s.", actualClassId.ToString<AZStd::string>().c_str()));
                            return ResolvePointerResult::FullyProcessed;
                        }

                        if (actualClassData->m_factory)
                        {
                            actualClassData->m_factory->Destroy(*object);
                            *object = nullptr;
                        }
                        else
                        {
                            status = context.Report(Tasks::RetrieveInfo, Outcomes::Catastrophic,
                                "Unable to find the factory needed to clear out the default value.");
                            return ResolvePointerResult::FullyProcessed;
                        }
                    }
                    else
                    {
                        *object = rtti.Cast(*object, actualClassId);
                    }
                }
                else
                {
                    // The json file doesn't explicitly specify a type so use the type that's already assigned and assume it's the
                    // default.
                    objectType = rtti.GetActualUuid(*object);
                    *object = rtti.Cast(*object, objectType);
                }
            }

            if (!*object)
            {
                // There's no object yet, so create an instance, otherwise reuse the existing object if possible.
                const SerializeContext::ClassData* actualClassData = context.GetSerializeContext()->FindClassData(loadedTypeId.m_typeId);
                if (actualClassData)
                {
                    if (actualClassData->m_factory)
                    {
                        if (actualClassData->m_azRtti)
                        {
                            if (actualClassData->m_azRtti->IsAbstract())
                            {
                                status = context.Report(Tasks::CreateDefault, Outcomes::Catastrophic,
                                    "Unable to create an instance of a base class.");
                                return ResolvePointerResult::FullyProcessed;
                            }
                        }

                        void* instance = actualClassData->m_factory->Create("Json Serializer");
                        if (!instance)
                        {
                            status = context.Report(Tasks::CreateDefault, Outcomes::Catastrophic,
                                "Unable to create a new instance.");
                            return ResolvePointerResult::FullyProcessed;
                        }
                        *object = instance;
                    }
                    else
                    {
                        status = context.Report(Tasks::RetrieveInfo, Outcomes::Catastrophic,
                            "Unable to find the factory needed to clear to create a new value.");
                        return ResolvePointerResult::FullyProcessed;
                    }
                }
                else
                {
                    status = context.Report(Tasks::RetrieveInfo, Outcomes::Unknown,
                        AZStd::string::format("Unable to find serialization information for type %s.", loadedTypeId.m_typeId.ToString<AZStd::string>().c_str()));
                    return ResolvePointerResult::FullyProcessed;
                }
            }
        }
        return ResolvePointerResult::ContinueProcessing;
    }

    JsonDeserializer::LoadTypeIdResult JsonDeserializer::LoadTypeIdFromJsonObject(const rapidjson::Value& node, const AZ::IRttiHelper& rtti,
        JsonDeserializerContext& context)
    {
        LoadTypeIdResult result;

        // Node doesn't contain an object, so there's no room to store the type id field. In this case the element can't be 
        // specialized, so return it's type.
        if (!node.IsObject())
        {
            result.m_determination = TypeIdDetermination::ImplicitTypeId;
            result.m_typeId = rtti.GetTypeId();
            return result;
        }

        auto typeField = node.FindMember(JsonSerialization::TypeIdFieldIdentifier);
        if (typeField == node.MemberEnd())
        {
            result.m_determination = TypeIdDetermination::ImplicitTypeId;
            result.m_typeId = rtti.GetTypeId();
            return result;
        }

        if (typeField->value.IsString())
        {
            return LoadTypeIdFromJsonString(typeField->value, &rtti, context);
        }

        result.m_determination = TypeIdDetermination::FailedToDetermine;
        result.m_typeId = Uuid::CreateNull();
        return result;
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadTypeId(Uuid& typeId, const rapidjson::Value& input,
        JsonDeserializerContext& context, const Uuid* baseTypeId, bool* isExplicit)
    {
        using namespace JsonSerializationResult;

        SerializeContext::IRttiHelper* baseClassRtti = nullptr;
        if (baseTypeId)
        {
            const SerializeContext::ClassData* baseClassData = context.GetSerializeContext()->FindClassData(*baseTypeId);
            baseClassRtti = baseClassData ? baseClassData->m_azRtti : nullptr;
        }

        JsonDeserializer::LoadTypeIdResult typeIdResult;
        if (input.IsObject())
        {
            if (baseClassRtti)
            {
                typeIdResult = JsonDeserializer::LoadTypeIdFromJsonObject(input, *baseClassRtti, context);
            }
            else
            {
                typeIdResult.m_determination = JsonDeserializer::TypeIdDetermination::FailedToDetermine;
            }
        }
        else if (input.IsString())
        {
            typeIdResult = JsonDeserializer::LoadTypeIdFromJsonString(input, baseClassRtti, context);
        }
        else
        {
            typeIdResult.m_determination = JsonDeserializer::TypeIdDetermination::FailedToDetermine;
        }

        switch (typeIdResult.m_determination)
        {
        case JsonDeserializer::TypeIdDetermination::ExplicitTypeId:
            if (isExplicit)
            {
                *isExplicit = true;
            }
            typeId = typeIdResult.m_typeId;
            return context.Report(Tasks::ReadField, Outcomes::Success, "Successfully read type id from json value.");
        case JsonDeserializer::TypeIdDetermination::ImplicitTypeId:
            if (isExplicit)
            {
                *isExplicit = false;
            }
            typeId = typeIdResult.m_typeId;
            return context.Report(Tasks::ReadField, Outcomes::Success, "Successfully read type id from json value.");
        case JsonDeserializer::TypeIdDetermination::FailedToDetermine:
            typeId = Uuid::CreateNull();
            return context.Report(Tasks::ReadField, Outcomes::Unknown, "Unable to find type id with the provided string.");
        case JsonDeserializer::TypeIdDetermination::FailedDueToMultipleTypeIds:
            typeId = Uuid::CreateNull();
            return context.Report(Tasks::ReadField, Outcomes::Unknown,
                "The provided string points to multiple type ids. Use the uuid of the intended class instead.");
        default:
            typeId = Uuid::CreateNull();
            return context.Report(Tasks::ReadField, Outcomes::Catastrophic, "Unknown result returned while loading type id.");
        }
    }

    JsonDeserializer::LoadTypeIdResult JsonDeserializer::LoadTypeIdFromJsonString(const rapidjson::Value& node, const AZ::IRttiHelper* baseClassRtti,
        JsonDeserializerContext& context)
    {
        using namespace JsonSerializationResult;

        LoadTypeIdResult result;

        if (node.IsString())
        {
            // First check if the string contains a Uuid and if so, use that as the type id.
            JsonUuidSerializer* uuidSerializer =
                azrtti_cast<JsonUuidSerializer*>(context.GetRegistrationContext()->GetSerializerForType(azrtti_typeid<Uuid>()));
            if (uuidSerializer)
            {
                JsonUuidSerializer::MessageResult serializerResult =
                    uuidSerializer->UnreportedLoad(&result.m_typeId, azrtti_typeid<Uuid>(), node);
                if (serializerResult.m_result.GetOutcome() == Outcomes::Success)
                {
                    result.m_determination = TypeIdDetermination::ExplicitTypeId;
                    return result;
                }
            }

            // There was no uuid, so use the name to look the info in the Serialize Context
            AZStd::string_view typeName(node.GetString(), node.GetStringLength());
            // Remove any leading and/or trailing whitespace.
            auto spaceCheck = [](char chr) -> bool { return !AZStd::is_space(chr); };
            typeName.remove_prefix(AZStd::find_if(typeName.begin(), typeName.end(), spaceCheck) - typeName.begin());
            typeName.remove_suffix(AZStd::find_if(typeName.rbegin(), typeName.rend(), spaceCheck) - typeName.rbegin());

            Crc32 nameCrc = Crc32(typeName);
            AZStd::vector<Uuid> typeIdCandidates = context.GetSerializeContext()->FindClassId(nameCrc);
            if (typeIdCandidates.empty())
            {
                result.m_determination = TypeIdDetermination::FailedToDetermine;
                result.m_typeId = Uuid::CreateNull();
                return result;
            }
            else if (typeIdCandidates.size() == 1)
            {
                result.m_determination = TypeIdDetermination::ExplicitTypeId;
                result.m_typeId = typeIdCandidates[0];
                return result;
            }
            else if (baseClassRtti)
            {
                size_t numApplicableCandidates = 0;
                const Uuid* lastApplicableCandidate = nullptr;
                for (const Uuid& typeId : typeIdCandidates)
                {
                    const SerializeContext::ClassData* classData = context.GetSerializeContext()->FindClassData(typeId);
                    if (context.GetSerializeContext()->CanDowncast(typeId, baseClassRtti->GetTypeId(), 
                        classData ? classData->m_azRtti : nullptr, baseClassRtti))
                    {
                        ++numApplicableCandidates;
                        lastApplicableCandidate = &typeId;
                    }
                }

                if (numApplicableCandidates == 1)
                {
                    result.m_determination = TypeIdDetermination::ExplicitTypeId;
                    result.m_typeId = *lastApplicableCandidate;
                    return result;
                }
            }

            // Multiple classes can fulfill this requested type so this needs to be disambiguated through an explicit
            // uuid instead of a name.
            result.m_determination = TypeIdDetermination::FailedDueToMultipleTypeIds;
            result.m_typeId = Uuid::CreateNull();
            return result;
        }
        
        result.m_determination = TypeIdDetermination::FailedToDetermine;
        result.m_typeId = Uuid::CreateNull();
        return result;
    }

    JsonDeserializer::ElementDataResult JsonDeserializer::FindElementByNameCrc(SerializeContext& serializeContext, 
        void* object, const SerializeContext::ClassData& classData, const Crc32 nameCrc)
    {
        // The class data stores base class element information first in the set of m_elements
        // We use a reverse iterator to ensure that derived class data takes precedence over base classes' data
        // for the case of naming conflicts in the serialized data between base and derived classes
        for (auto elementData = classData.m_elements.crbegin(); elementData != classData.m_elements.crend(); ++elementData)
        {
            if (elementData->m_nameCrc == nameCrc)
            {
                ElementDataResult result;
                result.m_data = reinterpret_cast<void*>(reinterpret_cast<char*>(object) + elementData->m_offset);
                result.m_info = &*elementData;
                result.m_found = true;
                return result;
            }
                
            if (elementData->m_flags & SerializeContext::ClassElement::Flags::FLG_BASE_CLASS)
            {
                const SerializeContext::ClassData* baseClassData = serializeContext.FindClassData(elementData->m_typeId);
                if (baseClassData)
                {
                    void* subclassOffset = reinterpret_cast<char*>(object) + elementData->m_offset;
                    ElementDataResult foundElementData = FindElementByNameCrc(
                        serializeContext, subclassOffset, *baseClassData, nameCrc);
                    if (foundElementData.m_found)
                    {
                        return foundElementData;
                    }
                }
            }
        }

        return ElementDataResult{};
    }

    size_t JsonDeserializer::CountElements(SerializeContext& serializeContext, const SerializeContext::ClassData& classData)
    {
        size_t count = 0;
        for (auto& element : classData.m_elements)
        {
            if (element.m_flags & SerializeContext::ClassElement::Flags::FLG_BASE_CLASS)
            {
                const SerializeContext::ClassData* baseClassData = serializeContext.FindClassData(element.m_typeId);
                if (baseClassData)
                {
                    count += CountElements(serializeContext, *baseClassData);
                }
            }
            else
            {
                count++;
            }
        }
        return count;
    }

    bool JsonDeserializer::IsExplicitDefault(const rapidjson::Value& value)
    {
        return value.IsObject() && value.MemberCount() == 0;
    }
} // namespace AZ

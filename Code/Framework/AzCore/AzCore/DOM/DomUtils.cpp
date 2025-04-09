/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Dom::Utils
{
    const AZ::Name TypeFieldName = AZ::Name::FromStringLiteral("$type", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name PointerTypeName = AZ::Name::FromStringLiteral("pointer", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name PointerValueFieldName = AZ::Name::FromStringLiteral("value", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name PointerTypeFieldName = AZ::Name::FromStringLiteral("pointerType", AZ::Interface<AZ::NameDictionary>::Get());

    Visitor::Result ReadFromString(Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime, Visitor& visitor)
    {
        return backend.ReadFromBuffer(string.data(), string.length(), lifetime, visitor);
    }

    Visitor::Result ReadFromStringInPlace(Backend& backend, AZStd::string& string, Visitor& visitor)
    {
        return backend.ReadFromBufferInPlace(string.data(), string.size(), visitor);
    }

    AZ::Outcome<Value, AZStd::string> SerializedStringToValue(
        Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime)
    {
        return WriteToValue(
            [&](Visitor& visitor)
            {
                return backend.ReadFromBuffer(string.data(), string.size(), lifetime, visitor);
            });
    }

    AZ::Outcome<void, AZStd::string> ValueToSerializedString(Backend& backend, Dom::Value value, AZStd::string& buffer)
    {
        Dom::Visitor::Result result = backend.WriteToBuffer(
            buffer,
            [&](Visitor& visitor)
            {
                return value.Accept(visitor, false);
            });
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError().FormatVisitorErrorMessage());
        }
        return AZ::Success();
    }

    AZ::Outcome<Value, AZStd::string> WriteToValue(const Backend::WriteCallback& writeCallback)
    {
        Value value;
        AZStd::unique_ptr<Visitor> writer = value.GetWriteHandler();
        Visitor::Result result = writeCallback(*writer);
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError().FormatVisitorErrorMessage());
        }
        return AZ::Success(AZStd::move(value));
    }

    Value TypeIdToDomValue(const AZ::TypeId& typeId)
    {
        // Assign a custom reporting callback to ignore unregistered types
        AZ::JsonSerializerSettings settings;
        AZStd::string scratchBuffer;
        settings.m_reporting = [&scratchBuffer](
            AZStd::string_view message,
               AZ::JsonSerializationResult::ResultCode result,
               AZStd::string_view path) -> auto
        {
            // Unregistered types are acceptable and do not require a warning
            if (result.GetTask() != AZ::JsonSerializationResult::Tasks::RetrieveInfo ||
                result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::Unknown)
            {
                // Default Json serialization issue reporting
                if (result.GetProcessing() != JsonSerializationResult::Processing::Completed)
                {
                    scratchBuffer.append(message.begin(), message.end());
                    scratchBuffer.append("\n    Reason: ");
                    result.AppendToString(scratchBuffer, path);
                    scratchBuffer.append(".");
                    AZ_Warning("JSON Serialization", false, "%s", scratchBuffer.c_str());

                    scratchBuffer.clear();
                }
            }

            return result;
        };

        rapidjson::Document buffer;
        JsonSerialization::StoreTypeId(buffer, buffer.GetAllocator(), typeId, AZStd::string_view{}, settings);
        if (!buffer.IsString())
        {
            return Value("", false);
        }
        AZ_Assert(buffer.IsString(), "TypeId should be stored as a string");
        return Value(AZStd::string_view(buffer.GetString(), buffer.GetStringLength()), true);
    }

    AZ::TypeId DomValueToTypeId(const AZ::Dom::Value& value, const AZ::TypeId* baseClassId)
    {
        if (value.IsString())
        {
            AZ::TypeId result = AZ::TypeId::CreateNull();
            rapidjson::Value buffer;
            buffer.SetString(value.GetString().data(), aznumeric_caster(value.GetStringLength()));
            JsonSerialization::LoadTypeId(result, buffer, baseClassId);
            return result;
        }
        else
        {
            return ValueToType<AZ::TypeId>(value).value_or(AZ::TypeId::CreateNull());
        }
    }

    bool CanLoadViaJsonSerialization(
        const AZ::TypeId& typeId, const Value& root, JsonDeserializerSettings settings)
    {
        // A serializeContext is required for making the temporary AZStd::any for storage
        // so if the supplied serialize context is nullptr, query the one associated with the ComponentApplication
        auto componentApplicationInterface = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
        if (settings.m_serializeContext == nullptr)
        {
            settings.m_serializeContext = componentApplicationInterface != nullptr ? componentApplicationInterface->GetSerializeContext() : nullptr;
        }
        if (settings.m_serializeContext == nullptr)
        {
            return false;
        }

        if (settings.m_registrationContext == nullptr)
        {
            settings.m_registrationContext = componentApplicationInterface != nullptr ? componentApplicationInterface->GetJsonRegistrationContext() : nullptr;
        }
        if (settings.m_registrationContext == nullptr)
        {
            return false;
        }

        JsonSerializerSettings serializerSettings;
        serializerSettings.m_serializeContext = settings.m_serializeContext;
        serializerSettings.m_registrationContext = settings.m_registrationContext;
        // Check if the type is serializable before moving on converting the Dom Value into a temporary JSON document
        if (!JsonSerialization::IsTypeSerializable(typeId, serializerSettings))
        {
            return false;
        }

        rapidjson::Document jsonViewOfDomValue;
        auto WriteDomFieldToJson = [&root](Visitor& visitor)
        {
            const bool copyStrings = false;
            return root.Accept(visitor, copyStrings);
        };
        if (auto convertToJsonResult = Json::WriteToRapidJsonValue(jsonViewOfDomValue, jsonViewOfDomValue.GetAllocator(), AZStd::move(WriteDomFieldToJson));
            !convertToJsonResult)
        {
            return false;
        }

        AZStd::any dryRunStorage = settings.m_serializeContext->CreateAny(typeId);
        // CreateAny will fail if the type is not default constructible or not reflected to the Serialize Context
        if (dryRunStorage.empty())
        {
            return false;
        }
        JsonSerializationResult::ResultCode loadResult = JsonSerialization::Load(AZStd::any_cast<void>(&dryRunStorage), typeId, jsonViewOfDomValue, settings);
        return loadResult.GetProcessing() != JsonSerializationResult::Processing::Halted;
    }

    JsonSerializationResult::ResultCode LoadViaJsonSerialization(
        void* object, const AZ::TypeId& typeId, const Value& root, const JsonDeserializerSettings& settings)
    {
        // Check if the Type is serializable before attempting to load into the object pointer
        JsonSerializerSettings serializerSettings;
        serializerSettings.m_serializeContext = settings.m_serializeContext;
        serializerSettings.m_registrationContext = settings.m_registrationContext;
        if (!JsonSerialization::IsTypeSerializable(typeId, serializerSettings))
        {
            return JsonSerializationResult::ResultCode{ JsonSerializationResult::Tasks::Convert,
                                                        JsonSerializationResult::Outcomes::Catastrophic };
        }

        rapidjson::Document buffer;
        auto convertToRapidjsonResult = Json::WriteToRapidJsonValue(buffer, buffer.GetAllocator(), [&root](Visitor& visitor)
            {
                const bool copyStrings = false;
                return root.Accept(visitor, copyStrings);
            });
        if (!convertToRapidjsonResult.IsSuccess())
        {
            return JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Catastrophic);
        }
        return JsonSerialization::Load(object, typeId, buffer, settings);
    }

    JsonSerializationResult::ResultCode StoreViaJsonSerialization(
        const void* object, const void* defaultObject, const AZ::TypeId& typeId, Value& output, const JsonSerializerSettings& settings)
    {
        // Check if the Type is serializable before attempting to store the object address into the Dom Value
        if (!JsonSerialization::IsTypeSerializable(typeId, settings))
        {
            return JsonSerializationResult::ResultCode{ JsonSerializationResult::Tasks::Convert,
                                                        JsonSerializationResult::Outcomes::Catastrophic };
        }

        rapidjson::Document buffer;
        auto result = JsonSerialization::Store(buffer, buffer.GetAllocator(), object, defaultObject, typeId, settings);
        auto outputWriter = output.GetWriteHandler();
        auto convertToAzDomResult = Json::VisitRapidJsonValue(buffer, *outputWriter, Lifetime::Temporary);
        if (!convertToAzDomResult.IsSuccess())
        {
            result.Combine(JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Catastrophic));
        }
        return result;
    }

    bool DeepCompareIsEqual(const Value& lhs, const Value& rhs, const ComparisonParameters& parameters)
    {
        const Value::ValueType& lhsValue = lhs.GetInternalValue();
        const Value::ValueType& rhsValue = rhs.GetInternalValue();

        if (lhs.IsString() && rhs.IsString())
        {
            // If we both hold the same ref counted string we don't need to do a full comparison
            if (AZStd::holds_alternative<Value::SharedStringType>(lhsValue) && lhsValue == rhsValue)
            {
                return true;
            }
            return lhs.GetString() == rhs.GetString();
        }

        return AZStd::visit(
            [&](auto&& ourValue) -> bool
            {
                using Alternative = AZStd::decay_t<decltype(ourValue)>;

                if constexpr (AZStd::is_same_v<Alternative, ObjectPtr>)
                {
                    if (!rhs.IsObject())
                    {
                        return false;
                    }
                    auto&& theirValue = AZStd::get<AZStd::remove_cvref_t<decltype(ourValue)>>(rhsValue);
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    const Object::ContainerType& ourValues = ourValue->GetValues();
                    const Object::ContainerType& theirValues = theirValue->GetValues();

                    if (ourValues.size() != theirValues.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourValues.size(); ++i)
                    {
                        const Object::EntryType& lhsChild = ourValues[i];
                        auto rhsIt = rhs.FindMember(lhsChild.first);
                        if (rhsIt == rhs.MemberEnd() || !DeepCompareIsEqual(lhsChild.second, rhsIt->second, parameters))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, ArrayPtr>)
                {
                    if (!rhs.IsArray())
                    {
                        return false;
                    }
                    auto&& theirValue = AZStd::get<AZStd::remove_cvref_t<decltype(ourValue)>>(rhsValue);
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    const Array::ContainerType& ourValues = ourValue->GetValues();
                    const Array::ContainerType& theirValues = theirValue->GetValues();

                    if (ourValues.size() != theirValues.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourValues.size(); ++i)
                    {
                        const Value& lhsChild = ourValues[i];
                        const Value& rhsChild = theirValues[i];
                        if (!DeepCompareIsEqual(lhsChild, rhsChild, parameters))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, NodePtr>)
                {
                    if (!rhs.IsNode())
                    {
                        return false;
                    }
                    auto&& theirValue = AZStd::get<AZStd::remove_cvref_t<decltype(ourValue)>>(rhsValue);
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    const Node& ourNode = *ourValue;
                    const Node& theirNode = *theirValue;

                    const Object::ContainerType& ourProperties = ourNode.GetProperties();
                    const Object::ContainerType& theirProperties = theirNode.GetProperties();

                    if (ourProperties.size() != theirProperties.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourProperties.size(); ++i)
                    {
                        const Object::EntryType& lhsChild = ourProperties[i];
                        auto rhsIt = rhs.FindMember(lhsChild.first);
                        if (rhsIt == rhs.MemberEnd() || !DeepCompareIsEqual(lhsChild.second, rhsIt->second, parameters))
                        {
                            return false;
                        }
                    }

                    const Array::ContainerType& ourChildren = ourNode.GetChildren();
                    const Array::ContainerType& theirChildren = theirNode.GetChildren();

                    if (ourChildren.size() != theirChildren.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourChildren.size(); ++i)
                    {
                        const Value& lhsChild = ourChildren[i];
                        const Value& rhsChild = theirChildren[i];
                        if (!DeepCompareIsEqual(lhsChild, rhsChild, parameters))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, Value::OpaqueStorageType>)
                {
                    if (!rhs.IsOpaqueValue())
                    {
                        return false;
                    }

                    const Value::OpaqueStorageType& theirValue = AZStd::get<Value::OpaqueStorageType>(rhsValue);
                    if (parameters.m_treatOpaqueValuesOfSameTypeAsEqual)
                    {
                        return ourValue->type() == theirValue->type();
                    }
                    else
                    {
                        return ourValue == theirValue;
                    }
                }
                else
                {
                    return lhs == rhs;
                }
            },
            lhsValue);
    }

    Value DeepCopy(const Value& value, bool copyStrings)
    {
        Value copiedValue;
        AZStd::unique_ptr<Visitor> writer = copiedValue.GetWriteHandler();
        value.Accept(*writer, copyStrings);
        return copiedValue;
    }

    void* TryMarshalValueToPointer(const AZ::Dom::Value& value, const AZ::TypeId& expectedType)
    {
        if (value.IsObject())
        {
            auto typeIdIt = value.FindMember(TypeFieldName);
            if (typeIdIt != value.MemberEnd() && typeIdIt->second.GetString() == PointerTypeName.GetStringView())
            {
                if (!expectedType.IsNull())
                {
                    auto typeFieldIt = value.FindMember(PointerTypeFieldName);
                    if (typeFieldIt == value.MemberEnd())
                    {
                        return nullptr;
                    }
                    AZ::TypeId actualTypeId = DomValueToTypeId(typeFieldIt->second);
                    if (actualTypeId != expectedType)
                    {
                        return nullptr;
                    }
                }
                return reinterpret_cast<void*>(value[PointerValueFieldName].GetUint64());
            }
        }
        else if (value.IsOpaqueValue())
        {
            const auto& opaqueAny = value.GetOpaqueValue();
            if (opaqueAny.type() == expectedType)
            {
                return AZStd::any_cast<void>(const_cast<AZStd::any*>(&opaqueAny));
            }
        }

        return nullptr;
    }

    // remove dangerous implementation that could result in dangling references
    void* TryMarshalValueToPointer(AZ::Dom::Value&& value, const AZ::TypeId& expectedType) = delete;

    Dom::Value MarshalTypedPointerToValue(const void* value, const AZ::TypeId& typeId)
    {
        Dom::Value result(Dom::Type::Object);
        result[TypeFieldName] = Dom::Value(PointerTypeName.GetStringView(), false);
        result[PointerValueFieldName] = Dom::Value(static_cast<uint64_t>(reinterpret_cast<const uintptr_t>(value)));
        Dom::Value typeName = TypeIdToDomValue(typeId);
        if (!typeName.GetString().empty())
        {
            result[PointerTypeFieldName] = AZStd::move(typeName);
        }
        return result;
    }

    AZ::TypeId GetValueTypeId(const Dom::Value& value)
    {
        switch (value.GetType())
        {
        case Type::Bool:
            return azrtti_typeid<bool>();
        case Type::Double:
            return azrtti_typeid<double>();
        case Type::Int64:
            return azrtti_typeid<int64_t>();
        case Type::Uint64:
            return azrtti_typeid<uint64_t>();
        case Type::String:
            return azrtti_typeid<AZStd::string_view>();
        // For compound types, just treat the stored type as Value
        case Type::Array:
        case Type::Object:
        case Type::Node:
            return azrtti_typeid<Value>();
        case Type::Opaque:
            return value.GetOpaqueValue().get_type_info().m_id;
        default:
            return azrtti_typeid<void>();
        }
    }

    Dom::Value MarshalOpaqueValue(const void* valueAddress, const MarshalTypeTraits& typeTraits,
        AZStd::any::action_handler_for_t actionHandler)
    {
        if (typeTraits.m_isPointer)
        {
            return MarshalTypedPointerToValue(valueAddress, typeTraits.m_typeId);
        }
        else
        {
            // For the non-pointer case source object is copied into the Dom::Value
            // First try to use Json Serialization system if available to leverage
            // the SerializeContext and JsonRegistrationContext for writing the value to a Dom::Value
            // The ideal scenario is trying to replicate the data structure into the Dom Value as if
            // it is a JSON Object.
            // For example a C++ struct such as follows
            /*
             ```c++
             struct DiceComponentConfig
             {
                int m_sides;
                AZStd::vector<double> m_probabilities;
                AZStd::string m_name;
             };
             ```
             */
            // Which contains the data ina C++ psuedo-layout of DiceComponentConfig{ 6, [1/6, 1/6, 1/6, 1/6, 1/6, 1/6], "Six-Sided Die" }
            // Could map to JSON Object as follows if JSON Serialization is available
            /*
            ```JSON
            {
                "m_sides": 6,
                "m_probabilities": [
                    0.166667,
                    0.166667
                    0.166667
                    0.166667
                    0.166667
                    0.166667
                ],
                "m_name": "Six-Sided Die"
            }
            ```
            */
            // That could then map into a Dom::Value of the following layout
            // Dom::Value
            // -> Object
            //   1. Field: "m_sides" -> Int
            //   2. Field: "m_probabilities" -> Array
            //      Indices: 0-6 -> int
            //   3. Field: "m_name": -> String
            //
            // However if JSON Serialization is not available, the data will be stored in an AZStd::any as an opaque
            // type in which the data structure is opaque to the Dom::Value
            // i.e
            // Dom::Value
            // -> Opaque = <value>
            //
            // The drawbacks of an opaque type is that the Dom::Value can only shallow compare two opaque values
            // via looking at their memory address
            // It can't actually compare the data
            // Therefore two opaque values with the same data, but that different are aprt of different objects
            // will always compare unequal. This can result in-efficient behavior such as creating more Dom Patches than necessary
            AZ::JsonSerializerSettings storeSettings;
            // Defaults should be kept in the Dom::Value to make sure a complete object is written to the Dom
            storeSettings.m_keepDefaults = true;

            // Create a pass no-op issue reporting to skip the DefaultIssueReporter logging AZ_Warnings
            storeSettings.m_reporting = [](AZStd::string_view, JsonSerializationResult::ResultCode result, AZStd::string_view)
            {
                return result;
            };
            Value newValue;
            if (auto storeViaSerializationResult = StoreViaJsonSerialization(valueAddress, nullptr, typeTraits.m_typeId, newValue, storeSettings);
                storeViaSerializationResult.GetProcessing() != JsonSerializationResult::Processing::Halted)
            {
                return newValue;
            }

            // The data will be stored in AZStd::any as a fail-safe
            AZStd::any::type_info typeInfo;
            typeInfo.m_id = typeTraits.m_typeId;
            typeInfo.m_handler = AZStd::move(actionHandler);
            typeInfo.m_isPointer = false;
            typeInfo.m_useHeap = typeTraits.m_typeSize > AZStd::Internal::ANY_SBO_BUF_SIZE;
            return Dom::Value::FromOpaqueValue(AZStd::any(valueAddress, typeInfo));
        }
    }

    Dom::Value ValueFromType(const void* valueAddress, const MarshalTypeTraits& typeTraits,
        AZStd::any::action_handler_for_t actionHandler)
    {
        if (typeTraits.m_typeId == azrtti_typeid<Dom::Value>())
        {
            // Rely on the Dom::Value copy constructor to make a copy
            return *reinterpret_cast<const Dom::Value*>(valueAddress);
        }
        else
        {
            return MarshalOpaqueValue(valueAddress, typeTraits, AZStd::move(actionHandler));
        }
    }

} // namespace AZ::Dom::Utils

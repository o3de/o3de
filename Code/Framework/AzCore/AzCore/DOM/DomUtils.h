/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ::Dom::Utils
{
    Visitor::Result ReadFromString(Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime, Visitor& visitor);
    Visitor::Result ReadFromStringInPlace(Backend& backend, AZStd::string& string, Visitor& visitor);

    AZ::Outcome<Value, AZStd::string> SerializedStringToValue(Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime);
    AZ::Outcome<void, AZStd::string> ValueToSerializedString(Backend& backend, Dom::Value value, AZStd::string& buffer);

    AZ::Outcome<Value, AZStd::string> WriteToValue(const Backend::WriteCallback& writeCallback);

    struct ComparisonParameters
    {
        //! If set, opaque values will only be compared by type and not contents
        //! This can be useful when comparing opaque values that aren't equal in-memory but shouldn't constitue a
        //! comparison failure (e.g. comparing callbacks)
        bool m_treatOpaqueValuesOfSameTypeAsEqual = false;
    };

    bool DeepCompareIsEqual(const Value& lhs, const Value& rhs, const ComparisonParameters& parameters = {});
    Value TypeIdToDomValue(const AZ::TypeId& typeId);
    AZ::TypeId DomValueToTypeId(const AZ::Dom::Value& value, const AZ::TypeId* baseClassId = nullptr);
    JsonSerializationResult::ResultCode LoadViaJsonSerialization(
        void* object, const AZ::TypeId& typeId, const Value& root, const JsonDeserializerSettings& settings = {});
    JsonSerializationResult::ResultCode StoreViaJsonSerialization(
        const void* object,
        const void* defaultObject,
        const AZ::TypeId& typeId,
        Value& output,
        const JsonSerializerSettings& settings = {});

    template<typename T>
    JsonSerializationResult::ResultCode LoadViaJsonSerialization(
        T& object, const Value& root, const JsonDeserializerSettings& settings = {})
    {
        return LoadViaJsonSerialization(&object, azrtti_typeid<T>(), root, settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode StoreViaJsonSerialization(
        const T& object, Value& output, const JsonSerializerSettings& settings = {})
    {
        return StoreViaJsonSerialization(&object, nullptr, azrtti_typeid<T>(), output, settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode StoreViaJsonSerialization(
        const T& object, const T& defaultObject, Value& output, const JsonSerializerSettings& settings = {})
    {
        return StoreViaJsonSerialization(&object, &defaultObject, azrtti_typeid<T>(), output, settings);
    }

    Value DeepCopy(const Value& value, bool copyStrings = true);

    template <typename T>
    using is_dom_value = AZStd::bool_constant<AZStd::is_same_v<AZStd::remove_cvref_t<T>, Dom::Value>>;

    template <typename T>
    constexpr bool is_dom_value_v = is_dom_value<T>::value;

    template<typename T, typename = void>
    struct DomValueWrapper
    {
        using Type = T;
    };

    // Only add pointer to non-pointer types
    template<typename T>
    struct DomValueWrapper<T, AZStd::enable_if_t<((AZStd::is_reference_v<T> || !AZStd::is_copy_constructible_v<T>))
        && !is_dom_value_v<T>>>
    {
        // i.e don't convert `const void*&` to `const void**`, instead convert to a reference wrapper
        using Type = AZStd::reference_wrapper<AZStd::remove_reference_t<T>>;
    };

    template<typename T>
    struct DomValueWrapper<T, AZStd::enable_if_t<is_dom_value_v<T>>>
    {
        using Type = Dom::Value;
    };

    template<typename T>
    using DomValueWrapperType = typename DomValueWrapper<T>::Type;

    extern const AZ::Name TypeFieldName;
    extern const AZ::Name PointerTypeName;
    extern const AZ::Name PointerValueFieldName;
    extern const AZ::Name PointerTypeFieldName;

    Dom::Value MarshalTypedPointerToValue(const void* value, const AZ::TypeId& typeId);
    void* TryMarshalValueToPointer(const AZ::Dom::Value& value, const AZ::TypeId& expectedType = AZ::TypeId::CreateNull());

    struct MarshalTypeTraits
    {
        AZ::TypeId m_typeId{};
        bool m_isPointer{};
        bool m_isReference{};
        bool m_isCopyConstructible{};
        size_t m_typeSize{};
    };

    Dom::Value MarshalOpaqueValue(const void* valueAddress, const MarshalTypeTraits& typeTraits,
        AZStd::any::action_handler_for_t actionHandler);

    template <typename T>
    Dom::Value MarshalOpaqueValue(T value)
    {
        using WrapperType = DomValueWrapperType<T>;
        MarshalTypeTraits typeTraits;
        // Store if the WrapperType is a pointer
        typeTraits.m_isPointer = AZStd::is_pointer_v<AZStd::remove_cvref_t<WrapperType>>;
        // Store if the T type is a reference
        typeTraits.m_isReference = AZStd::is_reference_v<T>;
        // Store if the T type is a copy constructible
        typeTraits.m_isCopyConstructible = AZStd::is_copy_constructible_v<T>;

        if constexpr (AZStd::is_pointer_v<AZStd::remove_cvref_t<WrapperType>>)
        {
            using ValueType = AZStd::remove_cvref_t<WrapperType>;
            typeTraits.m_typeSize = sizeof(ValueType);
            typeTraits.m_typeId = azrtti_typeid(value);
            return MarshalOpaqueValue(AZStd::as_const(value),
                typeTraits, AZStd::any::get_action_handler_for_t<ValueType>());
        }
        else
        {
            using ValueType = AZStd::remove_cvref_t<WrapperType>;
            typeTraits.m_typeSize = sizeof(ValueType);
            typeTraits.m_typeId = azrtti_typeid(value);
            return MarshalOpaqueValue(&value,
                typeTraits, AZStd::any::get_action_handler_for_t<ValueType>());
        }
    }

    Dom::Value ValueFromType(const void* valueAddress, const MarshalTypeTraits& typeTraits,
        AZStd::any::action_handler_for_t actionHandler);

    template<typename T>
    Dom::Value ValueFromType(T value)
    {
        using WrapperType = DomValueWrapperType<T>;
        MarshalTypeTraits typeTraits;
        // Store if the WrapperType is a pointer
        typeTraits.m_isPointer = AZStd::is_pointer_v<AZStd::remove_cvref_t<WrapperType>>;
        // Store if the T type is a reference
        typeTraits.m_isReference = AZStd::is_reference_v<T>;
        // Store if the T type is a copy constructible
        typeTraits.m_isCopyConstructible = AZStd::is_copy_constructible_v<T>;

        AZStd::any::action_handler_for_t actionHandler;

        if constexpr (AZStd::is_pointer_v<AZStd::remove_cvref_t<WrapperType>>)
        {
            using ValueType = AZStd::remove_cvref_t<WrapperType>;
            // Store the size of the decayed wrapper type
            typeTraits.m_typeSize = sizeof(ValueType);
            typeTraits.m_typeId = azrtti_typeid(value);

            actionHandler = AZStd::any::get_action_handler_for_t<ValueType>();

            return ValueFromType(AZStd::as_const(value),
                typeTraits, actionHandler);
        }
        else
        {
            using ValueType = AZStd::remove_cvref_t<WrapperType>;
            typeTraits.m_typeId = azrtti_typeid<ValueType>();
            typeTraits.m_typeSize = sizeof(ValueType);

            // Lifetime variables provides storage for Dom::Value variable
            // long enough to complete the call to the ValueFromType overload which accepts a void pointer
            Dom::Value domValueLifetime;
            const void* valueAddress = &value;
            if constexpr (AZStd::is_reference_wrapper<ValueType>())
            {
                return Dom::Value::FromOpaqueValue(AZStd::any(ValueType(value)));
            }
            else if constexpr (AZStd::is_constructible_v<AZStd::string_view, WrapperType>)
            {
                constexpr bool deepCopyString = true;
                return Dom::Value(value, deepCopyString);
            }
            else if constexpr (AZStd::is_constructible_v<Dom::Value, const WrapperType&>)
            {
                return Dom::Value(value);
            }
            else
            {
                // The type is a non-referenced opaque value type that needs to be stored using
                // an AZStd::any
                actionHandler = AZStd::any::get_action_handler_for_t<ValueType>();
                return ValueFromType(valueAddress, typeTraits, actionHandler);
            }
        }
    }

    AZ::TypeId GetValueTypeId(const Dom::Value& value);

    template<typename T>
    bool CanConvertValueToType(const Dom::Value& value)
    {
        using WrapperType = DomValueWrapperType<T>;
        if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, Dom::Value>)
        {
            return true;
        }
        else if constexpr (AZStd::is_same_v<WrapperType, bool>)
        {
            return value.IsBool();
        }
        else if constexpr (AZStd::is_integral_v<WrapperType> || AZStd::is_floating_point_v<WrapperType>
            || AZStd::is_enum_v<WrapperType>)
        {
            return value.IsNumber();
        }
        else if constexpr (AZStd::is_constructible_v<AZStd::string_view, WrapperType>)
        {
            return value.IsString();
        }
        else
        {
            if constexpr (AZStd::is_pointer_v<WrapperType>)
            {
                if (TryMarshalValueToPointer(value) != nullptr)
                {
                    return true;
                }
            }
            if (!value.IsOpaqueValue())
            {
                return false;
            }
            const AZStd::any& opaqueValue = value.GetOpaqueValue();
            return opaqueValue.is<WrapperType>();
        }
    }

    template<typename T>
    AZStd::optional<DomValueWrapperType<T>> ValueToType(const Dom::Value& value)
    {
        using WrapperType = DomValueWrapperType<T>;
        if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, Dom::Value>)
        {
            return value;
        }
        else if constexpr (AZStd::is_same_v<WrapperType, Dom::Value>)
        {
            return value;
        }
        else if constexpr (AZStd::is_same_v<WrapperType, bool>)
        {
            if (!value.IsBool())
            {
                return {};
            }
            return value.GetBool();
        }
        else if constexpr (AZStd::is_integral_v<WrapperType> && AZStd::is_signed_v<WrapperType>)
        {
            if (!value.IsNumber())
            {
                return {};
            }
            return aznumeric_cast<WrapperType>(value.GetInt64());
        }
        else if constexpr (AZStd::is_integral_v<WrapperType> && !AZStd::is_signed_v<WrapperType>)
        {
            if (!value.IsNumber())
            {
                return {};
            }
            return aznumeric_cast<WrapperType>(value.GetUint64());
        }
        else if constexpr (AZStd::is_floating_point_v<WrapperType>)
        {
            if (!value.IsNumber())
            {
                return {};
            }
            return aznumeric_cast<WrapperType>(value.GetDouble());
        }
        else if constexpr (
            AZStd::is_same_v<AZStd::decay_t<WrapperType>, AZStd::string> ||
            AZStd::is_same_v<AZStd::decay_t<WrapperType>, AZStd::string_view>)
        {
            if (!value.IsString())
            {
                return {};
            }
            return value.GetString();
        }
        else if constexpr (AZStd::is_enum_v<WrapperType>)
        {
            if (!value.IsNumber())
            {
                return {};
            }
            return static_cast<WrapperType>(ValueToType<AZStd::underlying_type_t<WrapperType>>(value).value());
        }
        else
        {
            auto ExtractOpaqueValue = [&value]() -> AZStd::optional<WrapperType>
            {
                if constexpr (AZStd::is_pointer_v<WrapperType>)
                {
                    void* valuePointer = TryMarshalValueToPointer(value);
                    if (valuePointer != nullptr)
                    {
                        return reinterpret_cast<WrapperType>(valuePointer);
                    }
                }
                if (value.IsOpaqueValue())
                {
                    const AZStd::any& opaqueValue = value.GetOpaqueValue();
                    if (!opaqueValue.is<WrapperType>())
                    {
                        // Marshal void* into our type - CanConvertToType will not register this as correct,
                        // but this is an important safety hatch for marshalling out non-primitive UI elements in the DocumentPropertyEditor
                        if (opaqueValue.is<void*>())
                        {
                            return *reinterpret_cast<WrapperType*>(AZStd::any_cast<void*>(opaqueValue));
                        }
                        return {};
                    }
                    return AZStd::any_cast<WrapperType>(opaqueValue);
                }
                else
                {
                    void* valuePointer = TryMarshalValueToPointer(value);
                    if (valuePointer != nullptr)
                    {
                        return WrapperType(*reinterpret_cast<WrapperType*>(valuePointer));
                    }
                    return {};
                }

            };

            return ExtractOpaqueValue();
        }
    }

    template<typename T>
    T ValueToTypeUnsafe(const Dom::Value& value)
    {
        if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, Dom::Value>)
        {
            return value;
        }
        else
        {
            using ValueTypeNoQualifier = AZStd::conditional_t<AZStd::is_pointer_v<AZStd::remove_cvref_t<T>>,
                AZStd::remove_pointer_t<AZStd::remove_cvref_t<T>>,
                AZStd::remove_cvref_t<T>>;
            auto convertedValue = ValueToType<T>(value);
            if constexpr (AZStd::is_void_v<ValueTypeNoQualifier>)
            {
                // Return the raw void pointer
                return convertedValue.value();
            }
            else if constexpr (AZStd::is_reference_v<T>)
            {
                // The convertedValue is a reference wrapper in this case
                return convertedValue.value();
            }
            else if constexpr (AZStd::is_constructible_v<T>)
            {
                // Fall back on default T{} if possible
                return convertedValue.value_or(T());
            }
            else
            {
                // Crash if value is not set
                return convertedValue.value();
            }
        }
    }
} // namespace AZ::Dom::Utils

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
    JsonSerializationResult::ResultCode LoadViaJsonSerialization(void* object, const AZ::TypeId& typeId, const Value& root, const JsonDeserializerSettings& settings = {});
    JsonSerializationResult::ResultCode StoreViaJsonSerialization(const void* object, const void* defaultObject, const AZ::TypeId& typeId, Value& output, const JsonSerializerSettings& settings = {});

    template <typename T>
    JsonSerializationResult::ResultCode LoadViaJsonSerialization(T& object, const Value& root, const JsonDeserializerSettings& settings = {})
    {
        return LoadViaJsonSerialization(&object, azrtti_typeid<T>(), root, settings);
    }

    template <typename T>
    JsonSerializationResult::ResultCode StoreViaJsonSerialization(const T& object, Value& output, const JsonSerializerSettings& settings = {})
    {
        return StoreViaJsonSerialization(&object, nullptr, azrtti_typeid<T>(), output, settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode StoreViaJsonSerialization(const T& object, const T& defaultObject, Value& output, const JsonSerializerSettings& settings = {})
    {
        return StoreViaJsonSerialization(&object, &defaultObject, azrtti_typeid<T>(), output, settings);
    }

    Value DeepCopy(const Value& value, bool copyStrings = true);

    template<typename T>
    Dom::Value ValueFromType(const T& value)
    {
        if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, Dom::Value>)
        {
            return value;
        }
        else if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string> || AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string_view>)
        {
            return Dom::Value(value, true);
        }
        else if constexpr (AZStd::is_constructible_v<Dom::Value, const T&>)
        {
            return Dom::Value(value);
        }
        else if constexpr (AZStd::is_enum_v<T>)
        {
            return ValueFromType(static_cast<AZStd::underlying_type_t<T>>(value));
        }
        else
        {
            return Dom::Value::FromOpaqueValue(AZStd::any(value));
        }
    }

    const AZ::TypeId& GetValueTypeId(const Dom::Value& value);

    template <typename T>
    bool CanConvertValueToType(const Dom::Value& value)
    {
        if constexpr (AZStd::is_same_v<T, bool>)
        {
            return value.IsBool();
        }
        else if constexpr (AZStd::is_integral_v<T> || AZStd::is_floating_point_v<T>)
        {
            return value.IsNumber();
        }
        else if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string> || AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string_view>)
        {
            return value.IsString();
        }
        else if constexpr (AZStd::is_enum_v<T>)
        {
            return CanConvertValueToType<AZStd::underlying_type_t<T>>(value);
        }
        else
        {
            if (!value.IsOpaqueValue())
            {
                return false;
            }
            const AZStd::any& opaqueValue = value.GetOpaqueValue();
            return opaqueValue.is<T>();
        }
    }

    template <typename T>
    AZStd::optional<T> ValueToType(const Dom::Value& value)
    {
        if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, Dom::Value>)
        {
            return value;
        }
        else if constexpr (AZStd::is_same_v<T, bool>)
        {
            if (!value.IsBool())
            {
                return {};
            }
            return value.GetBool();
        }
        else if constexpr (AZStd::is_integral_v<T> && AZStd::is_signed_v<T>)
        {
            if (!value.IsNumber())
            {
                return {};
            }
            return aznumeric_cast<T>(value.GetInt64());
        }
        else if constexpr (AZStd::is_integral_v<T> && !AZStd::is_signed_v<T>)
        {
            if (!value.IsNumber())
            {
                return {};
            }
            return aznumeric_cast<T>(value.GetUint64());
        }
        else if constexpr (AZStd::is_floating_point_v<T>)
        {
            if (!value.IsNumber())
            {
                return {};
            }
            return aznumeric_cast<T>(value.GetDouble());
        }
        else if constexpr (AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string> || AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string_view>)
        {
            if (!value.IsString())
            {
                return {};
            }
            return value.GetString();
        }
        else if constexpr (AZStd::is_enum_v<T>)
        {
            return static_cast<T>(ValueToType<AZStd::underlying_type_t<T>>(value));
        }
        else
        {
            if (!value.IsOpaqueValue())
            {
                return {};
            }
            const AZStd::any& opaqueValue = value.GetOpaqueValue();
            if (!opaqueValue.is<T>())
            {
                // Marshal void* into our type - CanConvertToType will not register this as correct,
                // but this is an important safety hatch for marshalling out non-primitive UI elements in the DocumentPropertyEditor
                if (opaqueValue.is<void*>())
                {
                    return *reinterpret_cast<T*>(AZStd::any_cast<void*>(opaqueValue));
                }
                return {};
            }
            return AZStd::any_cast<T>(opaqueValue);
        }
    }
} // namespace AZ::Dom::Utils

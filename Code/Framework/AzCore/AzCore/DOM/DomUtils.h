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

namespace AZ::Dom::Utils
{
    Visitor::Result ReadFromString(Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime, Visitor& visitor);
    Visitor::Result ReadFromStringInPlace(Backend& backend, AZStd::string& string, Visitor& visitor);

    AZ::Outcome<Value, AZStd::string> SerializedStringToValue(Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime);
    AZ::Outcome<void, AZStd::string> ValueToSerializedString(Backend& backend, Dom::Value value, AZStd::string& buffer);

    AZ::Outcome<Value, AZStd::string> WriteToValue(const Backend::WriteCallback& writeCallback);

    bool DeepCompareIsEqual(const Value& lhs, const Value& rhs);
    Value DeepCopy(const Value& value, bool copyStrings = true);

    template<typename T>
    Dom::Value ValueFromType(const T& value)
    {
        if constexpr (AZStd::is_same_v<AZStd::string_view, T>)
        {
            return Dom::Value(value, true);
        }
        else if constexpr (AZStd::is_constructible_v<Dom::Value, const T&>)
        {
            return Dom::Value(value);
        }
        else
        {
            return Dom::Value::FromOpaqueValue(AZStd::any(value));
        }
    }

    template <typename T>
    AZStd::optional<T> ValueToType(const Dom::Value& value)
    {
        if constexpr (AZStd::is_same_v<T, bool>)
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
        else
        {
            if (!value.IsOpaqueValue())
            {
                return {};
            }
            const AZStd::any& opaqueValue = value.GetOpaqueValue();
            if (!opaqueValue.is<T>())
            {
                return {};
            }
            return AZStd::any_cast<T>(value.GetOpaqueValue());
        }
    }
} // namespace AZ::Dom::Utils

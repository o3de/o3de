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
    T ConvertValueToPrimitive(const Dom::Value& value, AZStd::enable_if_t<AZStd::is_same_v<T, bool>>* = nullptr)
    {
        return value.GetBool();
    }

    template<typename T>
    T ConvertValueToPrimitive(
        const Dom::Value& value,
        AZStd::enable_if_t<AZStd::is_integral_v<T> && AZStd::is_signed_v<T> && !AZStd::is_same_v<T, bool>>* = nullptr)
    {
        return aznumeric_cast<T>(value.GetInt64());
    }

    template<typename T>
    T ConvertValueToPrimitive(
        const Dom::Value& value,
        AZStd::enable_if_t<AZStd::is_integral_v<T> && !AZStd::is_signed_v<T> && !AZStd::is_same_v<T, bool>>* = nullptr)
    {
        return aznumeric_cast<T>(value.GetUint64());
    }

    template<typename T>
    T ConvertValueToPrimitive(const Dom::Value& value, AZStd::enable_if_t<AZStd::is_floating_point_v<T>>* = nullptr)
    {
        return aznumeric_cast<T>(value.GetDouble());
    }

    template<typename T>
    T ConvertValueToPrimitive(
        const Dom::Value& value,
        AZStd::enable_if_t<AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string_view> || AZStd::is_same_v<AZStd::decay_t<T>, AZStd::string>>* = nullptr)
    {
        return value.GetString();
    }
} // namespace AZ::Dom::Utils

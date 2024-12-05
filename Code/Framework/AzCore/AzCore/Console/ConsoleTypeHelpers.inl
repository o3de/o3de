/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace ConsoleTypeHelpers
    {
        template <typename TYPE>
        inline auto ValueToString(const TYPE& value)
            -> AZStd::enable_if_t<AZStd::is_void_v<decltype(AZStd::to_string(AZStd::declval<CVarFixedString&>(), AZStd::declval<TYPE>()))>, CVarFixedString>
        {
            CVarFixedString resultString;
            AZStd::to_string(resultString, value);
            return resultString;
        }

        inline CVarFixedString ValueToString(const bool& value)
        {
            return value ? "true" : "false";
        }

        inline CVarFixedString ValueToString(const char& value)
        {
            return CVarFixedString(1, value);
        }

        inline CVarFixedString ValueToString(const AZ::CVarFixedString& value)
        {
            return value;
        }

        inline CVarFixedString ValueToString(const AZStd::string& value)
        {
            return CVarFixedString(value.c_str(), value.size());
        }

        inline CVarFixedString ValueToString(const AZ::Vector2& value)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale; // interpret %0.2f as using the "C" locale
            return CVarFixedString::format("%0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()));
        }

        inline CVarFixedString ValueToString(const AZ::Vector3& value)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale; // interpret %0.2f as using the "C" locale
            return CVarFixedString::format("%0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
        }

        inline CVarFixedString ValueToString(const AZ::Vector4& value)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale; // interpret %0.2f as using the "C" locale
            return CVarFixedString::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
        }

        inline CVarFixedString ValueToString(const AZ::Quaternion& value)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale; // interpret %0.2f as using the "C" locale
            return CVarFixedString::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
        }

        inline CVarFixedString ValueToString(const AZ::Color& value)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale; // interpret %0.2f as using the "C" locale
            return CVarFixedString::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetR()), static_cast<float>(value.GetG()), static_cast<float>(value.GetB()), static_cast<float>(value.GetA()));
        }

        template <>
        inline bool StringSetToValue<bool>(bool& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                AZ::CVarFixedString lower{ arguments.front() };
                AZStd::to_lower(lower.begin(), lower.end());

                if ((lower == "false") || (lower == "0"))
                {
                    outValue = false;
                    return true;
                }
                else if ((lower == "true") || (lower == "1"))
                {
                    outValue = true;
                    return true;
                }
                else
                {
                    AZ_Warning("Az Console", false, "Invalid input for boolean variable");
                }
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<char>(char& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                AZStd::string_view frontArg = arguments.front();
                outValue = frontArg[0];
            }
            else
            {
                // An empty string is actually a null character '\0'
                outValue = '\0';
            }
            return true;
        }
                
        // Note that string-stream doesn't behave nicely with char and uint8_t..
        // So instead we string-stream to a very large type that does work, and downcast if the result is within the limits of the target type
        template <typename TYPE, typename MAX_TYPE>
        inline bool StringSetToIntegralValue(TYPE& outValue, const AZ::ConsoleCommandContainer& arguments, [[maybe_unused]] const char* typeName, [[maybe_unused]] const char* errorString)
        {
            if (!arguments.empty())
            {
                AZ::CVarFixedString convertCandidate{ arguments.front() };
                char* endPtr = nullptr;
                MAX_TYPE value;
                if constexpr (AZStd::is_unsigned_v<MAX_TYPE>)
                {
                    value = aznumeric_cast<MAX_TYPE>(strtoull(convertCandidate.c_str(), &endPtr, 0));
                }
                else
                {
                    value = aznumeric_cast<MAX_TYPE>(strtoll(convertCandidate.c_str(), &endPtr, 0));
                }

                if (endPtr == convertCandidate.c_str())
                {
                    AZ_Warning("Az Console", false, "stringstream failed to convert value %s to %s type\n", convertCandidate.c_str(), typeName);
                    return false;
                }

                //Note: std::numeric_limits<>::min and max have extra parentheses here to min/max being evaluated as macros, to avoid colliding with min/max in Windows.h
                if ((value >= (std::numeric_limits<TYPE>::min)()) && (value <= (std::numeric_limits<TYPE>::max)()))
                {
                    outValue = static_cast<TYPE>(value);
                    return true;
                }
                else
                {
                    AZ_Warning("Az Console", false, errorString, static_cast<MAX_TYPE>(outValue));
                }
            }

            return false;
        }

#define INTEGRAL_NUMERIC_HANDLER(TYPE, MAX_TYPE, FMT_STRING) \
        template <> \
        inline bool StringSetToValue<TYPE>(TYPE& outValue, const AZ::ConsoleCommandContainer& arguments) \
        { \
            return StringSetToIntegralValue<TYPE, MAX_TYPE>(outValue, arguments, #TYPE, "attempted to assign out of range value %" #FMT_STRING "\n"); \
        }

        INTEGRAL_NUMERIC_HANDLER(int8_t, long long, lld);
        INTEGRAL_NUMERIC_HANDLER(int16_t, long long, lld);
        INTEGRAL_NUMERIC_HANDLER(int32_t, long long, lld);
        INTEGRAL_NUMERIC_HANDLER(long, long long, lld);
        INTEGRAL_NUMERIC_HANDLER(long long, long long, lld);

        INTEGRAL_NUMERIC_HANDLER(uint8_t, unsigned long long, llu);
        INTEGRAL_NUMERIC_HANDLER(uint16_t, unsigned long long, llu);
        INTEGRAL_NUMERIC_HANDLER(uint32_t, unsigned long long, llu);
        INTEGRAL_NUMERIC_HANDLER(unsigned long, unsigned long long, llu);
        INTEGRAL_NUMERIC_HANDLER(unsigned long long, unsigned long long, llu);

#undef INTEGRAL_NUMERIC_HANDLER

        template <>
        inline bool StringSetToValue<float>(float& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                AZ::Locale::ScopedSerializationLocale scopedLocale; // interpret floats using the "C" locale for strod

                AZ::CVarFixedString convertCandidate{ arguments.front() };
                char* endPtr = nullptr;
                const float converted = static_cast<float>(strtod(convertCandidate.c_str(), &endPtr));
                if (endPtr == convertCandidate.c_str())
                {
                    AZ_Warning("Az Console", false, "Invalid input for float variable");
                    return false;
                }
                outValue = converted;
                return true;
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<double>(double& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                AZ::Locale::ScopedSerializationLocale scopedLocale; // interpret floats using the "C" locale for strod

                AZ::CVarFixedString convertCandidate{ arguments.front() };
                char* endPtr = nullptr;
                const double converted = strtod(convertCandidate.c_str(), &endPtr);
                if (endPtr == convertCandidate.c_str())
                {
                    AZ_Warning("Az Console", false, "Invalid input for double variable");
                    return false;
                }
                outValue = converted;
                return true;
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<AZ::CVarFixedString>(AZ::CVarFixedString& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                outValue.clear();
                bool addSpace = false;
                for (AZStd::string_view argument : arguments)
                {
                    if (addSpace)
                    {
                        outValue.push_back(' ');
                    }
                    outValue += argument;
                    addSpace = true;
                }
                return true;
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<AZStd::string>(AZStd::string& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                outValue.clear();
                StringFunc::Join(outValue, arguments.begin(), arguments.end(), " ");
                return true;
            }
            return false;
        }

        template <typename TYPE, uint32_t ELEMENT_COUNT>
        inline bool StringSetToVectorValue(TYPE& outValue, const AZ::ConsoleCommandContainer& arguments, [[maybe_unused]] const char* typeName)
        {
            if (arguments.size() < ELEMENT_COUNT)
            {
                AZ_Warning("Az Console", false, "Not enough arguments provided to %s StringSetToValue()", typeName);
                return false;
            }

            AZ::CVarFixedString convertCandidate;
            for (uint32_t i = 0; i < ELEMENT_COUNT; ++i)
            {
                convertCandidate = arguments[i];
                outValue.SetElement(i, static_cast<float>(strtod(convertCandidate.c_str(), nullptr)));
            }

            return true;
        }

        inline bool StringSetToRgbaValue(AZ::Color& outColor, const AZ::ConsoleCommandContainer& arguments)
        {
            const uint32_t ArgumentCount = 4;

            if (arguments.size() < ArgumentCount)
            {
                AZ_Warning("Az Console", false, "Not enough arguments provided to AZ::Color StringSetToValue()");
                return false;
            }

            using ColorSetter = void (AZ::Color::*)(AZ::u8);
            ColorSetter rgbaSetters[] = { &AZ::Color::SetR8, &AZ::Color::SetG8, &AZ::Color::SetB8, &AZ::Color::SetA8 };

            AZ::CVarFixedString convertCandidate;
            for (uint32_t i = 0; i < ArgumentCount; ++i)
            {
                convertCandidate = arguments[i];
                AZStd::invoke(rgbaSetters[i], outColor, static_cast<AZ::u8>(strtoll(convertCandidate.c_str(), nullptr, 0)));
            }

            return true;
        }

        template <>
        inline bool StringSetToValue<AZ::Vector2>(AZ::Vector2& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            return StringSetToVectorValue<AZ::Vector2, 2>(outValue, arguments, "AZ::Vector2");
        }

        template <>
        inline bool StringSetToValue<AZ::Vector3>(AZ::Vector3& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            return StringSetToVectorValue<AZ::Vector3, 3>(outValue, arguments, "AZ::Vector3");
        }

        template <>
        inline bool StringSetToValue<AZ::Vector4>(AZ::Vector4& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            return StringSetToVectorValue<AZ::Vector4, 4>(outValue, arguments, "AZ::Vector4");
        }

        template <>
        inline bool StringSetToValue<AZ::Quaternion>(AZ::Quaternion& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            return StringSetToVectorValue<AZ::Quaternion, 4>(outValue, arguments, "AZ::Quaternion");
        }

        template <>
        inline bool StringSetToValue<AZ::Color>(AZ::Color& outValue, const AZ::ConsoleCommandContainer& arguments)
        {
            const bool decimal = AZStd::any_of(
                AZStd::cbegin(arguments), AZStd::cend(arguments), [](const AZStd::string argument)
                {
                    return argument.find(".") != AZStd::string::npos;
                });

            if (decimal)
            {
                return StringSetToVectorValue<AZ::Color, 4>(outValue, arguments, "AZ::Color");
            }
            else
            {
                return StringSetToRgbaValue(outValue, arguments);
            }
        }
    }
}


namespace AZ::ConsoleTypeHelpers
{
    namespace Internal
    {
        template<class T, class = void>
        inline constexpr bool HasValueToString = false;
        template<class T>
        inline constexpr bool HasValueToString<T, AZStd::enable_if_t<
            AZStd::convertible_to<decltype(ValueToString(AZStd::declval<T>())), CVarFixedString>>> = true;

        struct ToStringFn
        {
            // Converts type to string if the expression ValueToString(const T&) is well formed
            // and returns a type convertible to a CVarFixedString(AZStd::fixed_string)
            template<class T>
            constexpr auto operator()(const T& value) const
                -> AZStd::enable_if_t<HasValueToString<T>, CVarFixedString>
            {
                return ValueToString(value);
            }

            // Converts an enum to a enum option string if the EnumType specializes AzEnumTraits
            // otherwise converts the enum to a numeric string
            template<class T>
            constexpr auto operator()(const T& value) const
                -> AZStd::enable_if_t<!HasValueToString<T>&& AZStd::is_enum_v<AZStd::remove_cvref_t<T>>, CVarFixedString>
            {
                if constexpr (AZ::HasAzEnumTraits_v<T>)
                {
                    // For enum types which specialize AzEnumTraits
                    // attempt to use the enum option name if the value matches an enum value
                    // in the Enum Traits Members array
                    using EnumMemberPair = typename AZ::AzEnumTraits<T>::MembersArrayType::value_type;
                    auto FindEnumOptionString = [value](EnumMemberPair enumPair)
                    {
                        return value == enumPair.m_value;
                    };
                    if (auto foundIt = AZStd::ranges::find_if(AZ::AzEnumTraits<T>::Members, FindEnumOptionString);
                        foundIt != AZ::AzEnumTraits<T>::Members.end())
                    {
                        return CVarFixedString(foundIt->m_string);
                    }
                }

                // Convert the enum numeric value to a string if AzEnumTraits isn't specialized
                CVarFixedString resultString;
                AZStd::to_string(resultString, static_cast<AZStd::underlying_type_t<AZStd::remove_cvref_t<T>>>(value));
                return resultString;
            }
        };

        template<class T, class = void>
        inline constexpr bool HasStringSetToValue = false;
        template<class T>
        inline constexpr bool HasStringSetToValue<T, AZStd::enable_if_t<
            AZStd::convertible_to<decltype(StringSetToValue(AZStd::declval<AZStd::remove_const_t<T>&>(), AZStd::declval<AZ::ConsoleCommandContainer>())),
            bool>>> = true;

        struct ToValueFn
        {
            template<class T>
            constexpr auto operator()(T& outValue,
                const AZ::ConsoleCommandContainer& arguments) const
                -> AZStd::enable_if_t<HasStringSetToValue<T>, bool>
            {
                return StringSetToValue(outValue, arguments);
            }

            // Allows converting a string to an enum type
            // If the enum type specialized AzEnumTraits,
            // then converting a string version of the enum option to the enum value is also supported
            // Given an enum declared as followed AZ_ENUM(EnumName, Option1, Option2)
            // Then a string of "Option1" can be converted to the enum value of EnumName::Option1 == 0
            // and a string of "Option2" can be converted to the enum value of EnumName::Option2 == 1
            template<class T>
            constexpr auto operator()(T& outValue,
                const AZ::ConsoleCommandContainer& arguments) const
                -> AZStd::enable_if_t<!HasStringSetToValue<T>&& AZStd::is_enum_v<AZStd::remove_cvref_t<T>>, bool>
            {
                if (arguments.empty())
                {
                    // No string arguments to convert to type so return
                    return false;
                }

                if constexpr (AZ::HasAzEnumTraits_v<T>)
                {
                    using EnumMemberPair = typename AZ::AzEnumTraits<T>::MembersArrayType::value_type;
                    auto FindEnumOptionValue = [firstArg = arguments.front()](EnumMemberPair enumPair)
                    {
                        return firstArg == enumPair.m_string;
                    };
                    if (auto foundIt = AZStd::ranges::find_if(AZ::AzEnumTraits<T>::Members, FindEnumOptionValue);
                        foundIt != AZ::AzEnumTraits<T>::Members.end())
                    {
                        outValue = foundIt->m_value;
                        return true;
                    }
                }
                AZStd::underlying_type_t<AZStd::remove_cvref_t<T>> underlyingValue;
                const bool result = StringSetToValue(underlyingValue, arguments);
                if (result)
                {
                    outValue = static_cast<T>(underlyingValue);
                }
                return result;
            }

            template<class T>
            auto operator()(T& outValue, AZStd::string_view inputString) const
            {
                AZ::ConsoleCommandContainer arguments;
                auto splitToVector = [&arguments](AZStd::string_view token)
                {
                    arguments.emplace_back(token);
                };
                StringFunc::TokenizeVisitor(inputString, splitToVector, " ");
                return operator()(outValue, arguments);
            }
        };
    }

    inline namespace CustomizationPoint
    {
        // Customization point for allow types to convert to and from a string
        // for the purpose of interacting with the AZ::Console
        constexpr Internal::ToStringFn ToString{};

        constexpr Internal::ToValueFn ToValue{};
    }

    template <typename _TYPE>
    inline bool StringToValue(_TYPE& outValue, AZStd::string_view string)
    {
        return ConsoleTypeHelpers::ToValue(outValue, string);
    }
}

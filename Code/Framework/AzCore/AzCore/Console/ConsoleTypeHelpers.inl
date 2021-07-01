/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/conversions.h>
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
        inline AZStd::string ConvertString(const CVarFixedString& value)
        {
            return AZStd::string(value.c_str(), value.size());
        }

        inline AZ::CVarFixedString ConvertString(const AZStd::string& value)
        {
            return AZ::CVarFixedString(value.c_str(), value.size());
        }

        template <typename TYPE>
        inline CVarFixedString ValueToString(const TYPE& value)
        {
            return ConvertString(AZStd::to_string(value));
        }

        template <>
        inline CVarFixedString ValueToString(const bool& value)
        {
            return value ? "true" : "false";
        }

        template <>
        inline CVarFixedString ValueToString(const char& value)
        {
            return CVarFixedString(1, value);
        }

        template <>
        inline CVarFixedString ValueToString<AZ::CVarFixedString>(const AZ::CVarFixedString& value)
        {
            return value;
        }

        template <>
        inline CVarFixedString ValueToString<AZStd::string>(const AZStd::string& value)
        {
            return ConvertString(value);
        }

        template <>
        inline CVarFixedString ValueToString<AZ::Vector2>(const AZ::Vector2& value)
        {
            return CVarFixedString::format("%0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()));
        }

        template <>
        inline CVarFixedString ValueToString<AZ::Vector3>(const AZ::Vector3& value)
        {
            return CVarFixedString::format("%0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
        }

        template <>
        inline CVarFixedString ValueToString<AZ::Vector4>(const AZ::Vector4& value)
        {
            return CVarFixedString::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
        }

        template <>
        inline CVarFixedString ValueToString<AZ::Quaternion>(const AZ::Quaternion& value)
        {
            return CVarFixedString::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
        }

        template <>
        inline CVarFixedString ValueToString<AZ::Color>(const AZ::Color& value)
        {
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

        template <typename _TYPE>
        inline bool StringToValue(_TYPE& outValue, AZStd::string_view string)
        {
            AZ::ConsoleCommandContainer arguments;
            auto splitToVector = [&arguments](AZStd::string_view token)
            {
                arguments.emplace_back(token);
            };
            StringFunc::TokenizeVisitor(string, splitToVector, " ");
            return StringSetToValue(outValue, arguments);
        }
    }
}

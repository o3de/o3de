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

#include <cerrno>
#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    namespace SerializerInternal
    {
        // Helper function for string to signed integer
        // Note - assumes that the rapidjson Value is actually a kStringType
        template <typename T, typename AZStd::enable_if_t<AZStd::is_signed<T>::value, int> = 0>
        static JsonSerializationResult::Result TextToValue(T* outputValue, const char* text, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            char* parseEnd = nullptr;
            errno = 0;
            AZ::s64 parsedVal = strtoll(text, &parseEnd, 0);
            if (errno == ERANGE)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Integer is too big or too small to fit in the target.");
            }

            if (parseEnd != text)
            {
                JSR::ResultCode result = JsonNumericCast<T>(*outputValue, parsedVal, context.GetPath(), context.GetReporter());
                AZStd::string_view message = result.GetOutcome() == JSR::Outcomes::Success ?
                    "Successfully read integer from string." : "Unable to read integer from string.";
                return context.Report(result, message);

            }
            else
            {
                AZStd::string_view message = (text && text[0]) ?
                    "Unable to read integer value from provided string." : "String used to read integer from was empty.";
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, message);
            }
        }

        // Helper function for string to unsigned integer
        // Note - assumes that the rapidjson Value is actually a kStringType
        template <typename T, typename AZStd::enable_if_t<AZStd::is_unsigned<T>::value, int> = 0>
        static JsonSerializationResult::Result TextToValue(T* outputValue, const char* text, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            char* parseEnd = nullptr;

            // strtoull, when there is a leading negative sign, returns the negation of the parsed integer value as unsigned value.
            // This means that a value of "-34" would actually return std::numeric_limits<unsigned long long>::max - 34
            // While that is valid behavior for the API, it's unexpected coming from user data, so we'll see if we got a signed value first
            // Note that we do not care about underflow or overflow, this is just checking if a negative number is parsed
            if (strtoll(text, nullptr, 0) < 0 && parseEnd != text)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Unable to read negative integer as only positive numbers can be stored.");
            }

            errno = 0;
            AZ::u64 parsedVal = strtoull(text, &parseEnd, 0);
            if (errno == ERANGE)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Integer is too big or too small to fit in the target.");
            }

            if (parseEnd != text)
            {
                JSR::ResultCode result = JsonNumericCast<T>(*outputValue, parsedVal, context.GetPath(), context.GetReporter());
                AZStd::string_view message = result.GetOutcome() == JSR::Outcomes::Success ?
                    "Successfully read integer from string." : "Unable to read integer value from string.";
                return context.Report(result, message);
            }
            else
            {
                AZStd::string_view message = (text && text[0]) ?
                    "Unable to read integer value from provided string." : "String used to read integer from was empty.";
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, message);
            }
        }
    }
}

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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    //! A helper function to casts between numeric types, and consider the data which is being converted comse from user data.
    //! If a conversion from FromType to ToType will not cause overflow or underflow, the result is stored in result, and the function returns Success
    //! Otherwise, the target is left untouched.
    template <typename ToType, typename FromType>
    JsonSerializationResult::ResultCode JsonNumericCast(ToType& result, FromType value, AZStd::string_view path,
        const AZ::JsonSerializationResult::JsonIssueCallback& reporting)
    {
        using namespace JsonSerializationResult;

        if (NumericCastInternal::FitsInToType<ToType>(value))
        {
            result = aznumeric_cast<ToType>(value);
            return reporting("Successfully cast number.", ResultCode(Tasks::Convert, Outcomes::Success), path);
        }
        else
        {
            return reporting(AZ::OSString::format("Casted value could not be fitted in destination type {%s} -> {%s}",
                AzTypeInfo<FromType>::Name(), AzTypeInfo<ToType>::Name()), ResultCode(Tasks::Convert, Outcomes::Unsupported), path);
        }
    }
} // namespace AZ

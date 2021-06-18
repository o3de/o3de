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

#include <AzCore/std/string/string.h>

#include <stdexcept>

//! Evaluates the specified condition and throws the specified exception with the specified 
// !message upon failure.
#define AZ_TestImpact_Eval(CONDITION, EXCEPTION_TYPE, MSG)                          \
    do                                                                              \
    {                                                                               \
        static_assert(                                                              \
            AZStd::is_base_of_v<TestImpact::Exception, EXCEPTION_TYPE>,             \
            "TestImpact Eval macro must only be used with TestImpact exceptions");  \
        if(!(CONDITION))                                                            \
        {                                                                           \
            throw(EXCEPTION_TYPE(MSG));                                             \
        }                                                                           \
    }                                                                               \
    while (0)

namespace TestImpact
{
    //! Base class for test impact framework exceptions.
    //! @note The message passed in to the constructor is copied and thus safe with dynamic strings.
    class Exception
        : public std::exception
    {
    public:
        explicit Exception() = default;
        explicit Exception(const AZStd::string& msg);
        explicit Exception(const char* msg);
        const char* what() const noexcept override;

    private:
        //! Error message detailing the reason for the exception.
        AZStd::string m_msg;
    };
} // namespace TestImpact

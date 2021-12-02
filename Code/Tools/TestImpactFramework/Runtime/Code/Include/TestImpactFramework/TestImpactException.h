/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

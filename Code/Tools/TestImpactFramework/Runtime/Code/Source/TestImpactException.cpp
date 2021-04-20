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

#include <TestImpactFramework/TestImpactException.h>

namespace TestImpact
{
    Exception::Exception(const AZStd::string& msg)
        : m_msg(msg)
    {
    }

    Exception::Exception(const char* msg)
        : m_msg(msg)
    {
    }

    const char* Exception::what() const noexcept
    {
        return m_msg.c_str();
    }
} // namespace TestImpact

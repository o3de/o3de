/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

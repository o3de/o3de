/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestDeviceAttribute.h"

namespace UnitTest
{
    TestDeviceAttribute::TestDeviceAttribute(AZStd::string_view name, AZStd::string_view description, AZStd::any value, EvalFunc eval)
        : m_name(name)
        , m_description(description)
        , m_eval(eval)
        , m_value(value)
    {
    }

    AZStd::string_view TestDeviceAttribute::GetName() const
    {
        return m_name;
    }

    AZStd::string_view TestDeviceAttribute::GetDescription() const
    {
        return m_description;
    }

    bool TestDeviceAttribute::Evaluate(AZStd::string_view rule) const
    {
        return m_eval(rule);
    }

    AZStd::any TestDeviceAttribute::GetValue() const
    {
        return m_value; 
    }

} // namespace UnitTest


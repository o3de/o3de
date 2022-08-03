/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Artifact describing basic information about a test case.
    struct TestCase
    {
        AZStd::string m_name;
        bool m_enabled = false;
    };

    //! Artifact describing basic information about a test suite.
    template<typename Test>
    struct TestSuite
    {
        AZStd::string m_name;
        bool m_enabled = false;
        AZStd::vector<Test> m_tests;
    };
} // namespace TestImpact

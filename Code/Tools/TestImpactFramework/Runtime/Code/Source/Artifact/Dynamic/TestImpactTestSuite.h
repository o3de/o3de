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

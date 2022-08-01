/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Run/TestImpactTestCoverage.h>

#pragma once

namespace TestImpact
{
    //! Representation of per test case coverage results.
    class PythonTestCaseCoverage
    {
    public:
        PythonTestCaseCoverage(const AZStd::string& parentScript, TestCaseCoverage&& testCaseCoverage);

        const AZStd::string& GetParentScript() const;
        const TestCaseCoverage& GetTestCaseCoverage() const;
    private:
        AZStd::string m_parentScript;
        TestCaseCoverage m_testCaseCoverage;
    };
} // namespace TestImpact

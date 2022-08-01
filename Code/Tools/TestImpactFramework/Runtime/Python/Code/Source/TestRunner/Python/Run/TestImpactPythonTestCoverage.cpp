/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Python/Run/TestImpactPythonTestCoverage.h>

namespace TestImpact
{
    PythonTestCaseCoverage::PythonTestCaseCoverage(const AZStd::string& parentTarget, TestCaseCoverage&& testCaseCoverage)
        : m_parentTarget(parentTarget)
        , m_testCaseCoverage(AZStd::move(testCaseCoverage))
    {
    }

    const AZStd::string& PythonTestCaseCoverage::GetParentTarget() const
    {
        return m_parentTarget;
    }

    const TestCaseCoverage& PythonTestCaseCoverage::GetTestCaseCoverage() const
    {
        return m_testCaseCoverage;
    }
} // namespace TestImpact

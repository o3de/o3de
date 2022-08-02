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

    PythonTestCoverage::PythonTestCoverage(const PythonTestCoverage& other)
        : TestCoverage(other)
        , m_parentScript(other.m_parentScript)
    {
    }

    PythonTestCoverage::PythonTestCoverage(PythonTestCoverage&& other) noexcept
        : TestCoverage(AZStd::move(other))
        , m_parentScript(AZStd::move(other.m_parentScript))
    {
    }

    PythonTestCoverage::PythonTestCoverage(const AZStd::string& parentScript, AZStd::vector<ModuleCoverage>&& moduleCoverages)
        : TestCoverage(AZStd::move(moduleCoverages))
        , m_parentScript(parentScript)
    {
    }

    PythonTestCoverage::PythonTestCoverage(const AZStd::string& parentScript, const AZStd::vector<ModuleCoverage>& moduleCoverages)
        : TestCoverage(moduleCoverages)
        , m_parentScript(parentScript)
    {
    }

    PythonTestCoverage& PythonTestCoverage::operator=(const PythonTestCoverage& other)
    {
        if (this != &other)
        {
            TestCoverage::operator=(other);
            m_parentScript = other.m_parentScript;
        }

        return *this;
    }

    PythonTestCoverage& PythonTestCoverage::operator=(PythonTestCoverage&& other) noexcept
    {
        if (this != &other)
        {
            TestCoverage::operator=(AZStd::move(other));
            m_parentScript = AZStd::move(other.m_parentScript);
        }

        return *this;
    }

    const AZStd::string& PythonTestCoverage::GetParentScript() const
    {
        return m_parentScript;
    }
} // namespace TestImpact

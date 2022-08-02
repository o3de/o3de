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
    class PythonTestCoverage
        : public TestCoverage
    {
    public:
        PythonTestCoverage(const PythonTestCoverage&);
        PythonTestCoverage(PythonTestCoverage&&) noexcept;
        PythonTestCoverage(const AZStd::string& parentScript, AZStd::vector<ModuleCoverage>&& moduleCoverages);
        PythonTestCoverage(const AZStd::string& parentScript, const AZStd::vector<ModuleCoverage>& moduleCoverages);

        PythonTestCoverage& operator=(const PythonTestCoverage&);
        PythonTestCoverage& operator=(PythonTestCoverage&&) noexcept;

        const AZStd::string& GetParentScript() const;
    private:
        AZStd::string m_parentScript;
    };
} // namespace TestImpact

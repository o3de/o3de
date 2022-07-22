/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactCoverage.h>

#include <AzCore/std/containers/set.h>

namespace TestImpact
{
    //! Scope of coverage data.
    enum class CoverageLevel : bool
    {
        Source, //!< Line-level coverage data.
        Line //!< Source-level coverage data.
    };

    //! Representation of a given test target's test coverage results.
    class TestCoverage
    {
    public:
        TestCoverage(const TestCoverage&);
        TestCoverage(TestCoverage&&) noexcept;
        TestCoverage(AZStd::vector<ModuleCoverage>&& moduleCoverages) noexcept;
        TestCoverage(const AZStd::vector<ModuleCoverage>& moduleCoverages);

        TestCoverage& operator=(const TestCoverage&);
        TestCoverage& operator=(TestCoverage&&) noexcept;

        //! Returns the number of unique sources covered.
        size_t GetNumSourcesCovered() const;

        //! Returns the number of modules (dynamic libraries, child processes, etc.) covered.
        size_t GetNumModulesCovered() const;

        //! Returns the sorted set of unique sources covered (empty if no coverage).
        const AZStd::vector<RepoPath>& GetSourcesCovered() const;

        //! Returns the modules covered (empty if no coverage).
        const AZStd::vector<ModuleCoverage>& GetModuleCoverages() const;

        //! Returns the coverage level (empty if no coverage).
        AZStd::optional<CoverageLevel> GetCoverageLevel() const;

    private:
        void CalculateTestMetrics();

        AZStd::vector<ModuleCoverage> m_modules;
        AZStd::vector<RepoPath> m_sourcesCovered;
        AZStd::optional<CoverageLevel> m_coverageLevel;
    };
} // namespace TestImpact

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

#include <TestEngine/Run/TestImpactTestCoverage.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

namespace TestImpact
{
    TestCoverage::TestCoverage(const TestCoverage& other)
        : m_modules(other.m_modules)
        , m_sourcesCovered(other.m_sourcesCovered)
        , m_coverageLevel(other.m_coverageLevel)
    {
    }

    TestCoverage::TestCoverage(TestCoverage&& other) noexcept
        : m_modules(AZStd::move(other.m_modules))
        , m_sourcesCovered(AZStd::move(other.m_sourcesCovered))
    {
        AZStd::swap(m_coverageLevel, other.m_coverageLevel);
    }

    TestCoverage::TestCoverage(const AZStd::vector<ModuleCoverage>& moduleCoverages)
        : m_modules(moduleCoverages)
    {
        CalculateTestMetrics();
    }

    TestCoverage::TestCoverage(AZStd::vector<ModuleCoverage>&& moduleCoverages) noexcept
        : m_modules(AZStd::move(moduleCoverages))
    {
        CalculateTestMetrics();
    }

    TestCoverage& TestCoverage::operator=(const TestCoverage& other)
    {
        if (this != &other)
        {
            m_modules = other.m_modules;
            m_sourcesCovered = other.m_sourcesCovered;
            m_coverageLevel = other.m_coverageLevel;
        }

        return *this;
    }

    TestCoverage& TestCoverage::operator=(TestCoverage&& other) noexcept
    {
        if (this != &other)
        {
            m_modules = AZStd::move(other.m_modules);
            m_sourcesCovered = other.m_sourcesCovered;
            m_coverageLevel = other.m_coverageLevel;
        }

        return *this;
    }

    void TestCoverage::CalculateTestMetrics()
    {
        m_coverageLevel.reset();
        m_sourcesCovered.clear();

        for (const auto& moduleCovered : m_modules)
        {
            for (const auto& sourceCovered : moduleCovered.m_sources)
            {
                m_sourcesCovered.emplace_back(sourceCovered.m_path);
                if (!sourceCovered.m_coverage.empty())
                {
                    m_coverageLevel = CoverageLevel::Line;
                }
            }
        }

        AZStd::sort(m_sourcesCovered.begin(), m_sourcesCovered.end());
        m_sourcesCovered.erase(AZStd::unique(m_sourcesCovered.begin(), m_sourcesCovered.end()), m_sourcesCovered.end());

        if (!m_coverageLevel.has_value() && !m_sourcesCovered.empty())
        {
            m_coverageLevel = CoverageLevel::Source;
        }
    }

    size_t TestCoverage::GetNumSourcesCovered() const
    {
        return m_sourcesCovered.size();
    }

    size_t TestCoverage::GetNumModulesCovered() const
    {
        return m_modules.size();
    }

    const AZStd::vector<AZStd::string>& TestCoverage::GetSourcesCovered() const
    {
        return m_sourcesCovered;
    }

    const AZStd::vector<ModuleCoverage>& TestCoverage::GetModuleCoverages() const
    {
        return m_modules;
    }

    AZStd::optional<CoverageLevel> TestCoverage::GetCoverageLevel() const
    {
        return m_coverageLevel;
    }
} // namespace TestImpact

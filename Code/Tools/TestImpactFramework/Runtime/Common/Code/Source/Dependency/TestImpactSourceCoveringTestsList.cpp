/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Dependency/TestImpactSourceCoveringTestsList.h>

#include <AzCore/std/sort.h>

namespace TestImpact
{
    AZStd::vector<AZStd::string> ExtractTargetsFromSet(AZStd::unordered_set<AZStd::string>&& coveringTestTargets)
    {
        AZStd::vector<AZStd::string> testTargets;
        testTargets.reserve(coveringTestTargets.size());
        for (auto it = coveringTestTargets.begin(); it != coveringTestTargets.end(); )
        {
            testTargets.push_back(std::move(coveringTestTargets.extract(it++).value()));
        }

        return testTargets;
    }

    SourceCoveringTests::SourceCoveringTests(const RepoPath& path)
        : m_path(path)
    {
    }

    SourceCoveringTests::SourceCoveringTests(const RepoPath& path, AZStd::vector<AZStd::string>&& coveringTestTargets)
        : m_path(path)
        , m_coveringTestTargets(AZStd::move(coveringTestTargets))
    {
    }

    SourceCoveringTests::SourceCoveringTests(const RepoPath& path, AZStd::unordered_set<AZStd::string>&& coveringTestTargets)
        : m_path(path)
        , m_coveringTestTargets(ExtractTargetsFromSet(AZStd::move(coveringTestTargets)))
    {
    }

    const RepoPath& SourceCoveringTests::GetPath() const
    {
        return m_path;
    }

    size_t SourceCoveringTests::GetNumCoveringTestTargets() const
    {
        return m_coveringTestTargets.size();
    }

    const AZStd::vector<AZStd::string>& SourceCoveringTests::GetCoveringTestTargets() const
    {
        return m_coveringTestTargets;
    }

    SourceCoveringTestsList::SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>&& sourceCoveringTests)
        : m_coverage(AZStd::move(sourceCoveringTests))
    {
        AZStd::sort(m_coverage.begin(), m_coverage.end(), [](const SourceCoveringTests& lhs, const SourceCoveringTests& rhs)
        {
            return lhs.GetPath().String() < rhs.GetPath().String();
        });
    }

    size_t SourceCoveringTestsList::GetNumSources() const
    {
        return m_coverage.size();
    }

    const AZStd::vector<SourceCoveringTests>& SourceCoveringTestsList::GetCoverage() const
    {
        return m_coverage;
    }
} // namespace TestImpact

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

#include <Dependency/TestImpactSourceCoveringTestsList.h>

#include <AzCore/std/sort.h>

namespace TestImpact
{
    //SourceCoveringTests::SourceCoveringTests(const SourceCoveringTests& other)
    //{
    //
    //}

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
            return lhs.GetPath() < rhs.GetPath();
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

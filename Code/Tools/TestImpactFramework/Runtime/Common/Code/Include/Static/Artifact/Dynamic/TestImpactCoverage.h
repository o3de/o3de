/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Coverage information about a particular line.
    struct LineCoverage
    {
        size_t m_lineNumber = 0; //!< The source line number this covers.
        size_t m_hitCount = 0; //!< Number of times this line was covered (zero if not covered).
    };

    //! Coverage information about a particular source file.
    struct SourceCoverage
    {
        RepoPath m_path; //!< Source file path.
        AZStd::vector<LineCoverage> m_lineCoverage; //!< Source file line coverage (empty if source level coverage only).
    };

    //! Coverage information about a particular module (executable, shared library).
    struct ModuleCoverage
    {
        ModuleCoverage() = default;

        ModuleCoverage(RepoPath path, AZStd::vector<SourceCoverage>&& sources)
            : m_path(path)
            , m_sources(AZStd::move(sources))
        {
        }

        RepoPath m_path; //!< Module path.
        AZStd::vector<SourceCoverage> m_sources; //!< Sources of this module that are covered.
    };
} // namespace TestImpact

namespace AZStd
{
    //! Less function for SourceCoverage types for use in maps and sets.
    template<>
    struct less<TestImpact::SourceCoverage>
    {
        bool operator()(const TestImpact::SourceCoverage& lhs, const TestImpact::SourceCoverage& rhs) const
        {
            return lhs.m_path < rhs.m_path;
        }
    };

    //! Less function for ModuleCoverage types for use in maps and sets.
    template<>
    struct less<TestImpact::ModuleCoverage>
    {
        bool operator()(const TestImpact::ModuleCoverage& lhs, const TestImpact::ModuleCoverage& rhs) const
        {
            return lhs.m_path < rhs.m_path;
        }
    };

    //! Hash function for SourceCoverage types for use in unordered maps and sets.
    template<>
    struct hash<TestImpact::SourceCoverage>
    {
        bool operator()(const TestImpact::SourceCoverage& key) const
        {
            return hash<TestImpact::RepoPath>()(key.m_path);
        }
    };

    //! Hash function for ModuleCoverage types for use in unordered maps and sets.
    template<>
    struct hash<TestImpact::ModuleCoverage>
    {
        bool operator()(const TestImpact::ModuleCoverage& key) const
        {
            return hash<TestImpact::RepoPath>()(key.m_path);
        }
    };
} // namespace std

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Represents the unresolved test target coverage for a given source file.
    class SourceCoveringTests
    {
    public:
        //SourceCoveringTests(const SourceCoveringTests&);
        explicit SourceCoveringTests(const RepoPath& path);
        SourceCoveringTests(const RepoPath& path, AZStd::vector<AZStd::string>&& coveringTestTargets);
        SourceCoveringTests(const RepoPath& path, AZStd::unordered_set<AZStd::string>&& coveringTestTargets);

        //! Returns the path of this source file.
        const RepoPath& GetPath() const;

        //! Returns the number of unresolved test targets covering this source file.
        size_t GetNumCoveringTestTargets() const;

        //! Returns the unresolved test targets covering this source file.
        const AZStd::vector<AZStd::string>& GetCoveringTestTargets() const;
    private:
        RepoPath m_path; //!< The path of this source file.
        AZStd::vector<AZStd::string> m_coveringTestTargets; //!< The unresolved test targets that cover this source file.
    };

    //! Sorted collection of source file test coverage.
    class SourceCoveringTestsList
    {
    public:
        explicit SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>&& sourceCoveringTests);

        //! Returns the number of source files in the collection.
        size_t GetNumSources() const;

        //! Returns the source file coverages.
        const AZStd::vector<SourceCoveringTests>& GetCoverage() const;
    private:
        AZStd::vector<SourceCoveringTests> m_coverage; //!< The collection of source file coverages.
    };
} // namespace TestImpact

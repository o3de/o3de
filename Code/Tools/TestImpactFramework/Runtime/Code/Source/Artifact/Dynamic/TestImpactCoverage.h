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
        AZStd::vector<LineCoverage> m_coverage; //!< Source file line coverage (empty if source level coverage only).
    };

    //! Coverage information about a particular module (executable, shared library).
    struct ModuleCoverage
    {
        RepoPath m_path; //!< Module path.
        AZStd::vector<SourceCoverage> m_sources; //!< Sources of this module that are covered.
    };
} // namespace TestImpact

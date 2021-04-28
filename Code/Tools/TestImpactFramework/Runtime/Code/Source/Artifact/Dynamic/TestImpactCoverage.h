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

#pragma once

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
        AZStd::string m_path; //!< Source file path.
        AZStd::vector<LineCoverage> m_coverage; //!< Source file line coverage (empty if source level coverage only).
    };

    //! Coverage information about a particular module (executable, shared library).
    struct ModuleCoverage
    {
        AZStd::string m_path; //!< Module path.
        AZStd::vector<SourceCoverage> m_sources; //!< Sources of this module that are covered.
    };
} // namespace TestImpact

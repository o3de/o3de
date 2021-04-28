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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Represents the unresolved test target coverage for a given source file.
    class SourceCoveringTests
    {
    public:
        SourceCoveringTests(const AZStd::string& path, AZStd::vector<AZStd::string>&& coveringTestTargets);

        //! Returns the path of this source file.
        const AZStd::string& GetPath() const;

        //! Returns the number of unresolved test targets covering this source file.
        size_t GetNumCoveringTestTargets() const;

        //! Returns the unresolved test targets covering this source file.
        const AZStd::vector<AZStd::string>& GetCoveringTestTargets() const;
    private:
        AZStd::string m_path; //!< The path of this source file.
        AZStd::vector<AZStd::string> m_coveringTestTargets; //!< The unresolved test targets that cover this source file.
    };

    //! Sorted collection of source file test coverage.
    class SourceCoveringTestsList
    {
    public:
        SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>&& sourceCoveringTests);

        //! Returns the number of source files in the collection.
        size_t GetNumSources() const;

        //! Returns the source file coverages.
        const AZStd::vector<SourceCoveringTests>& GetCoverage() const;
    private:
        AZStd::vector<SourceCoveringTests> m_coverage; //!< The collection of source file coverages.
    };
} // namespace TestImpact

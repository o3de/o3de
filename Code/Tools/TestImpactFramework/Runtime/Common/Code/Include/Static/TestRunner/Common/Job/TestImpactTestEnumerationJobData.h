/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Job/TestImpactTestRunJobData.h>

#include <AzCore/std/optional.h>

namespace TestImpact
{
    //! Per-job data for test enumerations.
    class TestEnumerationJobData
    {
    public:
        //! Policy for how a test enumeration will be written/read from the a previous cache instead of enumerated from the test target.
        enum class CachePolicy
        {
            Read, //!< Do read from a cache file but do not overwrite any existing cache file.
            Write //!< Do not read from a cache file but instead overwrite any existing cache file.
        };

        //! Cache configuration for a given test enumeration command.
        struct Cache
        {
            CachePolicy m_policy;
            RepoPath m_file;
        };

        TestEnumerationJobData(const RepoPath& enumerationArtifact, AZStd::optional<Cache>&& cache);

        //! Returns the path to the enumeration artifact produced by the test target.
        const RepoPath& GetEnumerationArtifactPath() const;

        //! Returns the cache details for this job.
        const AZStd::optional<Cache>& GetCache() const;

    private:
        RepoPath m_enumerationArtifact; //!< Path to enumeration artifact to be processed.
        AZStd::optional<Cache> m_cache = AZStd::nullopt; //!< No caching takes place if cache is empty.
    };
} // namespace TestImpact

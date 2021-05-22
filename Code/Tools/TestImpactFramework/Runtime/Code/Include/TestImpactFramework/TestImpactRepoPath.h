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

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Wrapper class to ensure that all paths have the same path separator regardless of how they are sourced. This is critical
    //! to the test impact analysis data as otherwise querying/retrieving test impact analysis data for the same source albeit
    //! with different path separators will be considered different files entirely.
    class RepoPath
        : public AZ::IO::Path
    {
    public:
        constexpr RepoPath() = default;
        constexpr RepoPath(const RepoPath&) = default;
        constexpr RepoPath(RepoPath&&) noexcept = default;
        constexpr RepoPath(const string_type&) noexcept;
        constexpr RepoPath(const value_type*) noexcept;
        constexpr RepoPath(const AZ::IO::PathView&);
        constexpr RepoPath(const AZ::IO::Path&);

        RepoPath& operator=(const RepoPath&) noexcept = default;
        RepoPath& operator=(const string_type&) noexcept;
        RepoPath& operator=(const value_type*) noexcept;
        RepoPath& operator=(const AZ::IO::Path& str) noexcept;

        using AZ::IO::Path::operator AZ::IO::PathView;
    };
} // namespace TestImpact

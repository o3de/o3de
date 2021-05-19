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
#include <AzCore/IO/Path/Path.h>

namespace TestImpact
{
    class RepoPath
        : public AZ::IO::Path
    {
    public:
        constexpr RepoPath() = default;
        explicit RepoPath(AZStd::string&&);
        explicit RepoPath(const AZStd::string&);
        explicit RepoPath(const char*);
        explicit RepoPath(AZ::IO::Path&&);
        explicit RepoPath(const AZ::IO::Path&);
        explicit RepoPath(RepoPath&) = default;
        explicit RepoPath(RepoPath&&) noexcept = default;

        RepoPath& operator=(const RepoPath&) noexcept = default;
        RepoPath& operator=(const AZStd::string&) noexcept;
        RepoPath& operator=(const char*) noexcept;
        RepoPath& operator=(const AZ::IO::Path& str) noexcept;
    };
} // namespace TestImpact

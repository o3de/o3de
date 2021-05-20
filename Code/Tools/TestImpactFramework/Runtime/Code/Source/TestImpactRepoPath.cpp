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

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace TestImpact
{
    constexpr RepoPath::RepoPath(const string_type& path) noexcept
        : AZ::IO::Path(AZ::IO::Path(path).MakePreferred())
    {
    }

    constexpr RepoPath::RepoPath(const value_type* path) noexcept
        : AZ::IO::Path(AZ::IO::Path(path).MakePreferred())
    {
    }

    constexpr RepoPath::RepoPath(const AZ::IO::PathView& path)
        : AZ::IO::Path(AZ::IO::Path(path).MakePreferred())
    {
    }
        
    constexpr RepoPath::RepoPath(const AZ::IO::Path& path)
        : AZ::IO::Path(AZ::IO::Path(path).MakePreferred())
    {
    }

    RepoPath& RepoPath::operator=(const string_type& other) noexcept
    {
        this->~RepoPath();
        new(this) RepoPath(other);
        return *this;
    }

    RepoPath& RepoPath::operator=(const value_type* other) noexcept
    {
        this->~RepoPath();
        new(this) RepoPath(other);
        return *this;
    }

    RepoPath& RepoPath::operator=(const AZ::IO::Path& other) noexcept
    {
        this->~RepoPath();
        new(this) RepoPath(other);
        return *this;
    }
} // namespace TestImpact

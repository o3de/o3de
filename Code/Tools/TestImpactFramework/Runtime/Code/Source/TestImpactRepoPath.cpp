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

#include <TestImpactRepoPath.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace TestImpact
{
    RepoPath::RepoPath(AZStd::string&& path)
        : AZ::IO::Path(AZStd::move(path), AZ_CORRECT_FILESYSTEM_SEPARATOR)
    {
    }

    RepoPath::RepoPath(const AZStd::string& path)
        : AZ::IO::Path(path, AZ_CORRECT_FILESYSTEM_SEPARATOR)
    {
    }

    RepoPath::RepoPath(const char* path)
        : AZ::IO::Path(path, AZ_CORRECT_FILESYSTEM_SEPARATOR)
    {
    }

    RepoPath::RepoPath(AZ::IO::Path&& path)
        : AZ::IO::Path(AZStd::move(path.MakePreferred()))
    {
    }

    RepoPath::RepoPath(const AZ::IO::Path& path)
        : AZ::IO::Path(AZ::IO::Path(path).MakePreferred())
    {
    }

    RepoPath& RepoPath::operator=(const AZStd::string& other) noexcept
    {
        this->~RepoPath();
        new(this) RepoPath(other);
        return *this;
    }

    RepoPath& RepoPath::operator=(const char* other) noexcept
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

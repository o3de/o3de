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

namespace TestImpact
{
    

    RepoPath& RepoPath::operator=(const string_type& other) noexcept
    {
        m_path = AZ::IO::Path(other).MakePreferred();
        return *this;
    }

    RepoPath& RepoPath::operator=(const value_type* other) noexcept
    {
        m_path = AZ::IO::Path(other).MakePreferred();
        return *this;
    }

    RepoPath& RepoPath::operator=(const AZ::IO::Path& other) noexcept
    {
        m_path = AZ::IO::Path(other).MakePreferred();
        return *this;
    }

    inline RepoPath operator/(const RepoPath& lhs, const AZ::IO::PathView& rhs)
    {
        RepoPath result(lhs);
        result.m_path /= RepoPath(rhs).m_path;
        return result;
    }

    inline RepoPath operator/(const RepoPath& lhs, AZStd::string_view rhs)
    {
        RepoPath result(lhs);
        result.m_path /= RepoPath(rhs).m_path;
        return result;
    }

    inline RepoPath operator/(const RepoPath& lhs, const RepoPath::value_type* rhs)
    {
        RepoPath result(lhs);
        result.m_path /= RepoPath(rhs).m_path;
        return result;
    }

    inline RepoPath operator/(const RepoPath& lhs, const RepoPath& rhs)
    {
        RepoPath result(lhs);
        result.m_path /= rhs.m_path;
        return result;
    }

    inline RepoPath& RepoPath::operator/=(const AZ::IO::PathView& rhs)
    {
        m_path /= RepoPath(rhs).m_path;
        return *this;
    }


    inline RepoPath& RepoPath::operator/=(AZStd::string_view rhs)
    {
        m_path /= RepoPath(rhs).m_path;
        return *this;
    }

    inline RepoPath& RepoPath::operator/=(const RepoPath::value_type* rhs)
    {
        m_path /= RepoPath(rhs).m_path;
        return *this;
    }

    inline RepoPath& RepoPath::operator/=(const RepoPath& rhs)
    {
        m_path /= rhs.m_path;
        return *this;
    }

    inline bool operator==(const RepoPath& lhs, const RepoPath& rhs) noexcept
    {
        return lhs.m_path.Compare(rhs.m_path) == 0;
    }

    inline bool operator!=(const RepoPath& lhs, const RepoPath& rhs) noexcept
    {
        return lhs.m_path.Compare(rhs.m_path) != 0;
    }

    inline bool operator<([[maybe_unused]] const RepoPath& lhs, [[maybe_unused]] const RepoPath& rhs) noexcept
    {
        return  lhs.m_path.String() < rhs.m_path.String();
    }
} // namespace TestImpact

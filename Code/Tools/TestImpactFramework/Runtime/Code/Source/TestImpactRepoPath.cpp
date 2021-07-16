/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    RepoPath operator/(const RepoPath& lhs, const AZ::IO::PathView& rhs)
    {
        RepoPath result(lhs);
        result.m_path /= RepoPath(rhs).m_path;
        return result;
    }

    RepoPath operator/(const RepoPath& lhs, AZStd::string_view rhs)
    {
        RepoPath result(lhs);
        result.m_path /= RepoPath(rhs).m_path;
        return result;
    }

    RepoPath operator/(const RepoPath& lhs, const RepoPath::value_type* rhs)
    {
        RepoPath result(lhs);
        result.m_path /= RepoPath(rhs).m_path;
        return result;
    }

    RepoPath operator/(const RepoPath& lhs, const RepoPath& rhs)
    {
        RepoPath result(lhs);
        result.m_path /= rhs.m_path;
        return result;
    }

    RepoPath& RepoPath::operator/=(const AZ::IO::PathView& rhs)
    {
        m_path /= RepoPath(rhs).m_path;
        return *this;
    }


    RepoPath& RepoPath::operator/=(AZStd::string_view rhs)
    {
        m_path /= RepoPath(rhs).m_path;
        return *this;
    }

    RepoPath& RepoPath::operator/=(const RepoPath::value_type* rhs)
    {
        m_path /= RepoPath(rhs).m_path;
        return *this;
    }

    RepoPath& RepoPath::operator/=(const RepoPath& rhs)
    {
        m_path /= rhs.m_path;
        return *this;
    }

    bool operator==(const RepoPath& lhs, const RepoPath& rhs) noexcept
    {
        return lhs.m_path.Compare(rhs.m_path) == 0;
    }

    bool operator!=(const RepoPath& lhs, const RepoPath& rhs) noexcept
    {
        return lhs.m_path.Compare(rhs.m_path) != 0;
    }

    bool operator<([[maybe_unused]] const RepoPath& lhs, [[maybe_unused]] const RepoPath& rhs) noexcept
    {
        return  lhs.m_path.String() < rhs.m_path.String();
    }
} // namespace TestImpact

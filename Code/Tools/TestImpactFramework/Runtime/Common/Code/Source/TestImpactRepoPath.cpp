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
    RepoPath::RepoPath(const string_type& path) noexcept
        : m_path(path)
    {
        m_path.MakePreferred();
    }

    RepoPath::RepoPath(const string_view_type& path) noexcept
        : m_path(path)
    {
        m_path.MakePreferred();
    }

    RepoPath::RepoPath(const value_type* path) noexcept
        : m_path(path)
    {
        m_path.MakePreferred();
    }

    RepoPath::RepoPath(const AZ::IO::PathView& path)
        : m_path(path)
    {
        m_path.MakePreferred();
    }

    RepoPath::RepoPath(const AZ::IO::Path& path)
        : m_path(path)
    {
        m_path.MakePreferred();
    }

    const char* RepoPath::c_str() const { return m_path.c_str(); }
    AZStd::string RepoPath::String() const { return m_path.String(); }
    AZ::IO::PathView RepoPath::Stem() const { return m_path.Stem(); }
    AZ::IO::PathView RepoPath::Extension() const { return m_path.Extension(); }
    bool RepoPath::empty() const { return m_path.empty(); }
    AZ::IO::PathView RepoPath::ParentPath() const { return m_path.ParentPath(); }
    AZ::IO::PathView RepoPath::Filename() const { return m_path.Filename(); }
    AZ::IO::Path RepoPath::LexicallyRelative(const RepoPath& base) const { return m_path.LexicallyRelative(base.m_path); }
    [[nodiscard]] bool RepoPath::IsRelativeTo(const RepoPath& base) const { return m_path.IsRelativeTo(base.m_path); }
    AZ::IO::PathView RepoPath::RootName() const { return m_path.RootName(); }
    AZ::IO::PathView RepoPath::RelativePath() const { return m_path.RelativePath(); }

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

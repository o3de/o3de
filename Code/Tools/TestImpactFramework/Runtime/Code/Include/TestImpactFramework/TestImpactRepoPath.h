/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    {
    public:
        using string_type = AZ::IO::Path::string_type;
        using string_view_type = AZ::IO::Path::string_view_type;
        using value_type = AZ::IO::Path::value_type;

        constexpr RepoPath() = default;
        constexpr RepoPath(const RepoPath&) = default;
        constexpr RepoPath(RepoPath&&) noexcept = default;
        constexpr RepoPath(const string_type& path) noexcept;
        constexpr RepoPath(const string_view_type& path) noexcept;
        constexpr RepoPath(const value_type* path) noexcept;
        constexpr RepoPath(const AZ::IO::PathView& path);
        constexpr RepoPath(const AZ::IO::Path& path);

        RepoPath& operator=(const RepoPath&) noexcept = default;
        RepoPath& operator=(const string_type&) noexcept;
        RepoPath& operator=(const value_type*) noexcept;
        RepoPath& operator=(const AZ::IO::Path& str) noexcept;

        const char* c_str() const { return m_path.c_str(); }
        AZStd::string  String() const { return m_path.String(); }
        constexpr AZ::IO::PathView Stem() const { return m_path.Stem(); }
        constexpr AZ::IO::PathView Extension() const { return m_path.Extension(); }
        constexpr bool empty() const { return m_path.empty(); }
        constexpr AZ::IO::PathView ParentPath() const { return m_path.ParentPath(); }
        constexpr AZ::IO::PathView Filename() const { return m_path.Filename(); }
        AZ::IO::Path LexicallyRelative(const RepoPath& base) const { return m_path.LexicallyRelative(base.m_path); }
        [[nodiscard]] bool IsRelativeTo(const RepoPath& base) const { return m_path.IsRelativeTo(base.m_path); }
        constexpr AZ::IO::PathView RootName() const { return m_path.RootName(); }
        constexpr AZ::IO::PathView RelativePath() const { return m_path.RelativePath(); }

        // Wrappers around the AZ::IO::Path concatenation operator
        friend RepoPath operator/(const RepoPath& lhs, const AZ::IO::PathView& rhs);
        friend RepoPath operator/(const RepoPath& lhs, AZStd::string_view rhs);
        friend RepoPath operator/(const RepoPath& lhs, const typename value_type* rhs);
        friend RepoPath operator/(const RepoPath& lhs, const RepoPath& rhs);
        RepoPath& operator/=(const AZ::IO::PathView& rhs);
        RepoPath& operator/=(AZStd::string_view rhs);
        RepoPath& operator/=(const typename value_type* rhs);
        RepoPath& operator/=(const RepoPath& rhs);

        friend bool operator==(const RepoPath& lhs, const RepoPath& rhs) noexcept;
        friend bool operator!=(const RepoPath& lhs, const RepoPath& rhs) noexcept;
        friend bool operator<(const RepoPath& lhs, const RepoPath& rhs) noexcept;

    private:
        AZ::IO::Path m_path;
    };

    constexpr RepoPath::RepoPath(const string_type& path) noexcept
        : m_path(AZ::IO::Path(path).MakePreferred())
    {
    }

    constexpr RepoPath::RepoPath(const string_view_type& path) noexcept
        : m_path(AZ::IO::Path(path).MakePreferred())
    {
    }

    constexpr RepoPath::RepoPath(const value_type* path) noexcept
        : m_path(AZ::IO::Path(path).MakePreferred())
    {
    }

    constexpr RepoPath::RepoPath(const AZ::IO::PathView& path)
        : m_path(AZ::IO::Path(path).MakePreferred())
    {
    }

    constexpr RepoPath::RepoPath(const AZ::IO::Path& path)
        : m_path(AZ::IO::Path(path).MakePreferred())
    {
    }
} // namespace TestImpact

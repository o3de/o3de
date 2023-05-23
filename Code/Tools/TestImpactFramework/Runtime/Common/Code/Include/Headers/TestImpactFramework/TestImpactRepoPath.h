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

        RepoPath() = default;
        RepoPath(const RepoPath&) = default;
        RepoPath(RepoPath&&) noexcept = default;
        RepoPath(const string_type& path) noexcept;
        RepoPath(const string_view_type& path) noexcept;
        RepoPath(const value_type* path) noexcept;
        RepoPath(const AZ::IO::PathView& path);
        RepoPath(const AZ::IO::Path& path);

        RepoPath& operator=(const RepoPath&) noexcept = default;
        RepoPath& operator=(const string_type&) noexcept;
        RepoPath& operator=(const value_type*) noexcept;
        RepoPath& operator=(const AZ::IO::Path& str) noexcept;

        const char* c_str() const;
        AZStd::string  String() const;
        AZ::IO::PathView Stem() const;
        AZ::IO::PathView Extension() const;
        bool empty() const;
        AZ::IO::PathView ParentPath() const;
        AZ::IO::PathView Filename() const;
        AZ::IO::Path LexicallyRelative(const RepoPath& base) const;
        [[nodiscard]] bool IsRelativeTo(const RepoPath& base) const;
        AZ::IO::PathView RootName() const;
        AZ::IO::PathView RelativePath() const;

        constexpr RepoPath& ReplaceFilename(const AZ::IO::PathView& replacementFilename)
        {
            m_path.ReplaceFilename(replacementFilename);
            return *this;
        }
        
        constexpr RepoPath& ReplaceExtension(const AZ::IO::PathView& replacementExtension = {})
        {
            m_path.ReplaceExtension(replacementExtension);
            return *this;
        }

        // Wrappers around the AZ::IO::Path concatenation operator
        friend RepoPath operator/(const RepoPath& lhs, const AZ::IO::PathView& rhs);
        friend RepoPath operator/(const RepoPath& lhs, AZStd::string_view rhs);
        friend RepoPath operator/(const RepoPath& lhs, const value_type* rhs);
        friend RepoPath operator/(const RepoPath& lhs, const RepoPath& rhs);
        RepoPath& operator/=(const AZ::IO::PathView& rhs);
        RepoPath& operator/=(AZStd::string_view rhs);
        RepoPath& operator/=(const value_type* rhs);
        RepoPath& operator/=(const RepoPath& rhs);

        friend bool operator==(const RepoPath& lhs, const RepoPath& rhs) noexcept;
        friend bool operator!=(const RepoPath& lhs, const RepoPath& rhs) noexcept;
        friend bool operator<(const RepoPath& lhs, const RepoPath& rhs) noexcept;

    private:
        AZ::IO::Path m_path;
    };
} // namespace TestImpact

namespace AZStd
{
    //! Less function for RepoPath types for use in maps and sets.
    template<>
    struct less<TestImpact::RepoPath>
    {
        bool operator()(const TestImpact::RepoPath& lhs, const TestImpact::RepoPath& rhs) const
        {
            return lhs.String() < rhs.String();
        }
    };

    //! Hash function for RepoPath types for use in unordered maps and sets.
    template<>
    struct hash<TestImpact::RepoPath>
    {
        bool operator()(const TestImpact::RepoPath& key) const
        {
            return hash<AZStd::string>()(key.String());
        }
    };
} // namespace AZStd

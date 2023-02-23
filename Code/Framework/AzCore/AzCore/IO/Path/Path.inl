/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/wildcard.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/IO/Path/PathIterable.inl>

//! PathView implementation
namespace AZ::IO
{
    // BasicPathView implementation
    constexpr PathView::PathView(AZStd::string_view pathView) noexcept
        : m_path(pathView)
    {}

    constexpr PathView::PathView(AZStd::string_view pathView, const char preferredSeparator) noexcept
        : m_path(pathView)
        , m_preferred_separator(preferredSeparator)
    {}

    constexpr PathView::PathView(const value_type* path) noexcept
        : m_path(path)
    {}
    constexpr PathView::PathView(const value_type* path, const char preferredSeparator) noexcept
        : m_path(path)
        , m_preferred_separator(preferredSeparator)
    {}
    constexpr PathView::PathView(const char preferredSeparator) noexcept
        : m_preferred_separator(preferredSeparator)
    {}

    constexpr auto PathView::operator=(AZStd::string_view path) noexcept -> PathView&
    {
        m_path = path;
        return *this;
    }
    constexpr auto PathView::operator=(const value_type* pathView) noexcept -> PathView&
    {
        m_path = pathView;
        return *this;
    }

    constexpr void PathView::swap(PathView& rhs) noexcept
    {
        m_path.swap(rhs.m_path);
        const char tmpSeparator = rhs.m_preferred_separator;
        rhs.m_preferred_separator = m_preferred_separator;
        m_preferred_separator = tmpSeparator;
    }

    // native format observers
    constexpr auto PathView::Native() const noexcept -> const AZStd::string_view&
    {
        return m_path;
    }
    constexpr auto PathView::Native() noexcept -> AZStd::string_view&
    {
        return m_path;
    }

    constexpr PathView::operator AZStd::string_view() const noexcept
    {
        return m_path;
    }

    // path.decompose
    constexpr auto PathView::root_name_view() const -> AZStd::string_view
    {
        auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        if (pathParser.m_parser_state == parser::PS_InRootName)
        {
            return *pathParser;
        }
        return {};
    }

    constexpr auto PathView::root_directory_view() const -> AZStd::string_view
    {
        auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        if (pathParser.m_parser_state == parser::PS_InRootName)
        {
            ++pathParser;
        }
        if (pathParser.m_parser_state == parser::PS_InRootDir)
        {
            return *pathParser;
        }
        return {};
    }

    constexpr auto PathView::root_path_view() const -> AZStd::string_view
    {
        auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        AZStd::string_view rootNameView;
        if (pathParser.m_parser_state == parser::PS_InRootName)
        {
            rootNameView = *pathParser;
            ++pathParser;
        }
        if (pathParser.m_parser_state == parser::PS_InRootDir)
        {
            // Return from the beginning of the path to the right after the root directory character
            // This returns any RootName before the RootDir plus the RootDir
            // psuedo regular expression wise it would look like: <RootName>?<RootDir>
            return AZStd::string_view{ m_path.begin(), pathParser.m_path_raw_entry.end() };
        }

        // logic to take care of the case of a relative root path
        // This can only occur on Windows with paths of the form "C:relative/path/to/file"
        // In that the path has a RootName, but not a RootDir
        // psuedo regular expression wise it would look like: <RootName>?
        return rootNameView;
    }

    constexpr auto PathView::relative_path_view() const -> AZStd::string_view
    {
        auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        if (parser::ConsumeRootDir(&pathParser))
        {
            return {};
        }

        AZStd::string_view relativePathStart = pathParser.m_path_raw_entry;
        // Retrieve the last filename entry and use it's end as the end of the relative path
        AZStd::string_view lastFilenameEntry = *(--parser::PathParser::CreateEnd(m_path, m_preferred_separator));
        return AZStd::string_view(relativePathStart.begin(), lastFilenameEntry.end());
    }

    constexpr auto PathView::parent_path_view() const -> AZStd::string_view
    {
        if (empty())
        {
            return {};
        }
        // Determine if we have a root path but not a relative path. In that case
        // return *this.
        {
            auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
            if (parser::ConsumeRootDir(&pathParser))
            {
                return m_path;
            }
        }
        // Otherwise remove a single element from the end of the path, and return
        // a string representing that path
        {
            auto pathParser = parser::PathParser::CreateEnd(m_path, m_preferred_separator);
            --pathParser;
            if (pathParser.m_path_raw_entry.data() == m_path.data())
            {
                return {};
            }
            --pathParser;
            return AZStd::string_view(m_path.data(), pathParser.m_path_raw_entry.data() + pathParser.m_path_raw_entry.size());
        }
    }

    constexpr auto PathView::filename_view() const -> AZStd::string_view
    {
        if (empty())
        {
            return {};
        }
        {
            parser::PathParser pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
            if (parser::ConsumeRootDir(&pathParser))
            {
                return {};
            }
        }
        return *(--parser::PathParser::CreateEnd(m_path, m_preferred_separator));
    }

    constexpr auto PathView::stem_view() const -> AZStd::string_view
    {
        return parser::SeparateFilename(filename_view()).first;
    }

    constexpr auto PathView::extension_view() const -> AZStd::string_view
    {
        return parser::SeparateFilename(filename_view()).second;
    }

    // compare
    constexpr int PathView::Compare(const PathView& other) const noexcept
    {
        return ComparePathView(other);
    }
    constexpr int PathView::Compare(AZStd::string_view pathView) const noexcept
    {
        return ComparePathView(PathView(pathView, m_preferred_separator));
    }
    constexpr int PathView::Compare(const value_type* path) const noexcept
    {
        return ComparePathView(PathView(path, m_preferred_separator));
    }

    constexpr AZStd::fixed_string<MaxPathLength> PathView::FixedMaxPathString() const noexcept
    {
        return AZStd::fixed_string<MaxPathLength>(m_path.begin(), m_path.end());
    }

    // as_posix
    constexpr AZStd::fixed_string<MaxPathLength> PathView::FixedMaxPathStringAsPosix() const noexcept
    {
        AZStd::fixed_string<MaxPathLength> resultPath(m_path.begin(), m_path.end());
        AZStd::replace(resultPath.begin(), resultPath.end(), AZ::IO::WindowsPathSeparator, AZ::IO::PosixPathSeparator);
        return resultPath;
    }

    // decomposition
    constexpr auto PathView::RootName() const -> PathView
    {
        return PathView(root_name_view(), m_preferred_separator);
    }

    constexpr auto PathView::RootDirectory() const -> PathView
    {
        return PathView(root_directory_view(), m_preferred_separator);
    }

    constexpr auto PathView::RootPath() const -> PathView
    {
        return PathView(root_path_view(), m_preferred_separator);
    }

    constexpr auto PathView::RelativePath() const -> PathView
    {
        return PathView(relative_path_view(), m_preferred_separator);
    }

    constexpr auto PathView::ParentPath() const -> PathView
    {
        return PathView(parent_path_view(), m_preferred_separator);
    }

    constexpr auto PathView::Filename() const -> PathView
    {
        return PathView(filename_view(), m_preferred_separator);
    }

    constexpr auto PathView::Stem() const -> PathView
    {
        return PathView(stem_view(), m_preferred_separator);
    }

    constexpr auto PathView::Extension() const -> PathView
    {
        return PathView(extension_view(), m_preferred_separator);
    }

    // query
    [[nodiscard]] constexpr bool PathView::empty() const noexcept
    {
        return m_path.empty();
    }

    constexpr const char PathView::PreferredSeparator() const noexcept
    {
        return m_preferred_separator;
    }

    [[nodiscard]] constexpr bool PathView::HasRootName() const
    {
        return !root_name_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::HasRootDirectory() const
    {
        return !root_directory_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::HasRootPath() const
    {
        return !root_path_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::HasRelativePath() const
    {
        return !relative_path_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::HasParentPath() const
    {
        return !parent_path_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::HasFilename() const
    {
        return !filename_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::HasStem() const
    {
        return !stem_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::HasExtension() const
    {
        return !extension_view().empty();
    }

    [[nodiscard]] constexpr bool PathView::IsAbsolute() const
    {
        return Internal::IsAbsolute(m_path, m_preferred_separator);
    }

    [[nodiscard]] constexpr bool PathView::IsRelative() const
    {
        return !IsAbsolute();
    }

    [[nodiscard]] constexpr bool PathView::Match(AZStd::string_view pathPattern) const
    {
        // Initialize a path view for the pattern using preferred separator for the current path
        AZ::IO::PathView pathPatternView{ pathPattern, m_preferred_separator };
        if (pathPatternView.empty()
            || (pathPatternView.HasRootName() && pathPatternView.RootName().Compare(RootName()) != 0)
            || (pathPatternView.HasRootDirectory() && pathPatternView.RootDirectory().Compare(RootDirectory()) != 0))
        {
            // 1. Pattern is empty so no match can occur
            // 2. Pattern to match has a non-empty root name which does not match this path root name
            // 3. Pattern to match has a root_directory, but this path does not have a root directory, therefore a match cannot occur
            return false;
        }

        int pathPartCount = parser::DetermineLexicalElementCount(
            parser::PathParser::CreateBegin(m_path, m_preferred_separator));
        int patternPartCount = parser::DetermineLexicalElementCount(
            parser::PathParser::CreateBegin(pathPatternView.m_path, pathPatternView.m_preferred_separator));
        // If the pattern has a root path then all path parts must match against each pattern part
        // In this case the pattern is anchored to the root of path
        if (pathPatternView.HasRootPath() && pathPartCount != patternPartCount)
        {
            return false;
        }
        if (pathPartCount < patternPartCount)
        {
            // The path doesn't have enough parts to match the pattern parts
            return false;
        }

        // Iterate from the end of the relative path portion backwards through the path match any wild cards that come up
        parser::PathParser pathParserEnd(relative_path_view(), parser::ParserState::PS_AtEnd, m_preferred_separator);
        parser::PathParser patternParserEnd(pathPatternView.relative_path_view(), parser::ParserState::PS_AtEnd, pathPatternView.m_preferred_separator);

        // move the parser from the end to a valid filename by decrementing
        // Windows Paths are case-insensitive, while Posix paths are case-sensitive
        if (m_preferred_separator == PosixPathSeparator)
        {
            for (--pathParserEnd, --patternParserEnd; pathParserEnd && patternParserEnd; --pathParserEnd, --patternParserEnd)
            {
                if (!AZStd::wildcard_match_case(*patternParserEnd, *pathParserEnd))
                {
                    return false;
                }
            }
        }
        else
        {
            for (--pathParserEnd, --patternParserEnd; pathParserEnd && patternParserEnd; --pathParserEnd, --patternParserEnd)
            {
                if (!AZStd::wildcard_match(*patternParserEnd, *pathParserEnd))
                {
                    return false;
                }
            }
        }

        // If the pattern parser state is PS_BeforeBegin then it has been completely matched.
        return true;
    }

    constexpr int PathView::ComparePathView(const PathView& other) const
    {
        auto lhsPathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        auto rhsPathParser = parser::PathParser::CreateBegin(other.m_path, other.m_preferred_separator);

        if (int res = CompareRootName(&lhsPathParser, &rhsPathParser); res != 0)
        {
            return res;
        }

        if (int res = CompareRootDir(&lhsPathParser, &rhsPathParser); res != 0)
        {
            return res;
        }

        if (int res = CompareRelative(&lhsPathParser, &rhsPathParser); res != 0)
        {
            return res;
        }

        return CompareEndState(&lhsPathParser, &rhsPathParser);
    }

    // PathView swap
    constexpr void swap(PathView& lhs, PathView& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    // PathView iterator
    constexpr auto PathView::begin() const -> const_iterator
    {
        auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        const_iterator it;
        it.m_path_ref = this;
        it.m_state = static_cast<typename const_iterator::ParserState>(pathParser.m_parser_state);
        it.m_path_entry_view = pathParser.m_path_raw_entry;
        it.m_stashed_elem = *pathParser;
        return it;
    }

    constexpr auto PathView::end() const -> const_iterator
    {
        const_iterator it;
        it.m_state = const_iterator::AtEnd;
        it.m_path_ref = this;
        return it;
    }

    // PathView Comparison
    constexpr bool operator==(const PathView& lhs, const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) == 0;
    }
    constexpr bool operator!=(const PathView& lhs, const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) != 0;
    }
    constexpr bool operator<(const PathView& lhs, const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) < 0;
    }
    constexpr bool operator<=(const PathView& lhs, const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) <= 0;
    }
    constexpr bool operator>(const PathView& lhs, const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) > 0;
    }
    constexpr bool operator>=(const PathView& lhs, const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) >= 0;
    }

    constexpr void PathView::MakeRelativeTo(PathIterable& pathIterable, const AZ::IO::PathView& path, const AZ::IO::PathView& base) noexcept
    {
        const bool exactCaseCompare = path.m_preferred_separator == PosixPathSeparator
            || base.m_preferred_separator == PosixPathSeparator;
        {
            // perform root-name/root-directory mismatch checks
            auto pathParser = parser::PathParser::CreateBegin(path.m_path, path.m_preferred_separator);
            auto pathParserBase = parser::PathParser::CreateBegin(base.m_path, base.m_preferred_separator);
            auto CheckIterMismatchAtBase = [&]()
            {
                return pathParser.m_parser_state != pathParserBase.m_parser_state &&
                    (pathParser.InRootPath() || pathParserBase.InRootPath());
            };
            if (pathParser.InRootName() && pathParserBase.InRootName())
            {
                if (int res = Internal::ComparePathSegment(*pathParser, *pathParserBase, exactCaseCompare);
                    res != 0)
                {
                    return;
                }
            }
            else if (CheckIterMismatchAtBase())
            {
                return;
            }

            if (pathParser.InRootPath())
            {
                ++pathParser;
            }
            if (pathParserBase.InRootPath())
            {
                ++pathParserBase;
            }
            if (CheckIterMismatchAtBase())
            {
                return;
            }
        }

        // Find the first mismatching element
        auto pathParser = parser::PathParser::CreateBegin(path.m_path, path.m_preferred_separator);
        auto pathParserBase = parser::PathParser::CreateBegin(base.m_path, base.m_preferred_separator);
        while (pathParser && pathParserBase && pathParser.m_parser_state == pathParserBase.m_parser_state &&
            Internal::ComparePathSegment(*pathParser, *pathParserBase, exactCaseCompare) == 0)
        {
            ++pathParser;
            ++pathParserBase;
        }

        // If there is no mismatch, return ".".
        if (!pathParser && !pathParserBase)
        {
            pathIterable.emplace_back(".", parser::PathPartKind::PK_Dot);
            return;
        }

        // Otherwise, determine the number of elements, 'n', which are not dot or
        // dot-dot minus the number of dot-dot elements.
        int elemCount = parser::DetermineLexicalElementCount(pathParserBase);
        if (elemCount < 0)
        {
            return;
        }

        // if elemCount == 0 and (pathParser == end() || pathParser->empty()), returns path("."); otherwise
        if (elemCount == 0 && (pathParser.AtEnd() || *pathParser == ""))
        {
            pathIterable.emplace_back(".", parser::PathPartKind::PK_Dot);
            return;
        }

        // return a path constructed with 'n' dot-dot elements, followed by the
        // elements of '*this' after the mismatch.
        while (elemCount--)
        {
            pathIterable.emplace_back("..", parser::PathPartKind::PK_DotDot);
        }
        for (; pathParser; ++pathParser)
        {
            pathIterable.emplace_back(*pathParser, parser::ClassifyPathPart(pathParser));
        }
    }

    constexpr auto PathView::AppendNormalPathParts(PathIterable& pathIterable, const AZ::IO::PathView& path) noexcept -> void
    {
        if (path.m_path.empty())
        {
            return;
        }

        // Build a stack containing the remaining elements of the path, popping off
        // elements which occur before a '..' entry.
        for (auto pathParser = parser::PathParser::CreateBegin(path.m_path, path.m_preferred_separator); pathParser; ++pathParser)
        {
            switch (const parser::PathPartKind Kind = parser::ClassifyPathPart(pathParser); Kind)
            {
            case parser::PathPartKind::PK_RootName:
            {
                // Root Name normalization is a bit tricky.
                // A path of C:/foo/C:bar = C:/foo/bar and a path of C:foo/C:bar = C:foo/bar
                // A path of C:/foo/C: = C:/foo
                // A path of C:/foo/C:/bar = C:/bar
                // Also a path of C:/foo/C: = C:/foo, but C:/foo/C:/ = C:/
                // A path of C:foo/D:bar = D:bar
                // The pathIterable only stores the Root Name at the front
                if (const auto [firstPartView, firstPartKind] = !pathIterable.empty() ? pathIterable.front() : PathIterable::PartKindPair{};
                    firstPartKind != parser::PathPartKind::PK_RootName || firstPartView != *pathParser)
                {
                    // The root name has changed or this is the first time a root name has been seen,
                    // discard the accumulated path parts
                    pathIterable.clear();
                    pathIterable.emplace_back(*pathParser, Kind);
                }
                break;
            }
            case parser::PathPartKind::PK_RootSep:
            {
                // If a root directory has been found, discard the accumulated path parts so far
                // but not before storing of the first path part in case it is a Root Name
                const auto [firstPartView, firstPartKind] = !pathIterable.empty() ? pathIterable.front() : PathIterable::PartKindPair{};
                pathIterable.clear();

                if (firstPartKind == parser::PathPartKind::PK_RootName)
                {
                    pathIterable.emplace_back(firstPartView, firstPartKind);
                }
                pathIterable.emplace_back(*pathParser, Kind);
                break;
            }
            case parser::PathPartKind::PK_Filename:
            {
                // Special Case: The "filename" starts with a root name
                // i.e D:/foo/C:/baz
                //            ^
                // The result should be C:/baz
                // In this case restart path parsing at this element into the same PathIterable
                // using tail recursion
                AZStd::string_view filenameView{ *pathParser };
                if (auto filenameParser = parser::PathParser::CreateBegin(filenameView, pathParser.m_preferred_separator);
                    filenameParser && parser::ClassifyPathPart(filenameParser) == parser::PathPartKind::PK_RootName)
                {
                    AZ::IO::PathView fileNamePath{ AZStd::string_view{ filenameView.begin(), path.m_path.end() },
                        pathParser.m_preferred_separator };
                    AppendNormalPathParts(pathIterable, fileNamePath);
                    return;
                }
                else
                {
                    // Normal Case: The "filename" does not start with a root name
                    // Add all non-dot and non-dot-dot elements to the stack of elements.
                    pathIterable.emplace_back(*pathParser, Kind);
                }
                break;
            }
            case parser::PathPartKind::PK_DotDot:
            {
                // Only push a ".." element if there are no elements preceding the "..",
                // or if the preceding element is itself "..".

                if (const auto lastPartKind = !pathIterable.empty() ? pathIterable.back().second : parser::PathPartKind::PK_None;
                    lastPartKind == parser::PathPartKind::PK_Filename)
                {
                    // Due to the previous path part being a filename, the <filename> and the ".." cancels each other
                    // So remove the filename from the normalized path
                    pathIterable.pop_back();
                }
                else if (lastPartKind != parser::PathPartKind::PK_RootSep)
                {
                    pathIterable.emplace_back("..", parser::PathPartKind::PK_DotDot);
                }
                break;
            }
            case parser::PathPartKind::PK_Dot:
                break;
            case parser::PathPartKind::PK_None:
                AZ_Assert(false, "Path Parser is in an invalid state");
            }
        }
    }

    constexpr auto PathView::GetNormalPathParts(const AZ::IO::PathView& path) noexcept -> PathIterable
    {
        PathIterable pathIterable;
        AppendNormalPathParts(pathIterable, path);
        return pathIterable;
    }
}

namespace AZ::IO
{
    // Basic Path implementation

    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(const PathView& other) noexcept
        : m_path(other.m_path)
        , m_preferred_separator(other.m_preferred_separator) {}

    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(const string_type& pathString) noexcept
        : m_path(pathString) {}
    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(const string_type& pathString, const char preferredSeparator) noexcept
        : m_path(pathString)
        , m_preferred_separator(preferredSeparator) {}

    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(string_type&& pathString) noexcept
        : m_path(AZStd::move(pathString)) {}
    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(string_type&& pathString, const char preferredSeparator) noexcept
        : m_path(AZStd::move(pathString))
        , m_preferred_separator(preferredSeparator) {}


    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(AZStd::string_view src, const char preferredSeparator) noexcept
        : m_path(src)
        , m_preferred_separator(preferredSeparator) {}

    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(const char preferredSeparator) noexcept
        : m_preferred_separator(preferredSeparator) {}

    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(const value_type* src) noexcept
        : m_path(src) {}

    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(const value_type* src, const char preferredSeparator) noexcept
        : m_path(src)
        , m_preferred_separator(preferredSeparator) {}
    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(AZStd::string_view src) noexcept
        : m_path(src) {}

    template <typename StringType>
    template <typename InputIt>
    constexpr BasicPath<StringType>::BasicPath(InputIt first, InputIt last)
        : m_path(first, last) {}

    template <typename StringType>
    template <typename InputIt>
    constexpr BasicPath<StringType>::BasicPath(InputIt first, InputIt last, const char preferredSeparator)
        : m_path(first, last)
        , m_preferred_separator(preferredSeparator) {}


    template <typename StringType>
    constexpr BasicPath<StringType>::operator PathView() const noexcept
    {
        return PathView(m_path, m_preferred_separator);
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator=(const PathView& other) noexcept -> BasicPath&
    {
        m_path = other.m_path;
        m_preferred_separator = other.m_preferred_separator;
        return *this;
    }

    template<typename StringType>
    constexpr auto BasicPath<StringType>::operator=(const string_type& pathString) noexcept -> BasicPath&
    {
        m_path = pathString;
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator=(string_type&& pathString) noexcept -> BasicPath&
    {
        m_path = AZStd::move(pathString);
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator=(AZStd::string_view src) noexcept -> BasicPath&
    {
        return Assign(src);
    }

    template<typename StringType>
    constexpr auto BasicPath<StringType>::operator=(const value_type* src) noexcept -> BasicPath&
    {
        m_path = src;
        return *this;
    }

    template<typename StringType>
    inline constexpr auto BasicPath<StringType>::operator=(value_type src) noexcept -> BasicPath&
    {
        m_path.push_back(src);
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Assign(const PathView& pathView) noexcept -> BasicPath&
    {
        m_path = pathView.m_path;
        m_preferred_separator = pathView.m_preferred_separator;
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Assign(const string_type& pathString) noexcept -> BasicPath&
    {
        m_path = pathString;
        return *this;
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Assign(string_type&& pathString) noexcept -> BasicPath&
    {
        m_path = AZStd::move(pathString);
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Assign(AZStd::string_view src) noexcept -> BasicPath&
    {
        m_path.assign(src.begin(), src.end());
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Assign(const value_type* src) noexcept -> BasicPath&
    {
        m_path = src;
        return *this;
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Assign(value_type src) noexcept -> BasicPath&
    {
        m_path.push_back(src);
        return *this;
    }

    template <typename StringType>
    template <typename InputIt>
    constexpr auto BasicPath<StringType>::Assign(InputIt first, InputIt last) -> BasicPath&
    {
        m_path.assign(first, last);
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator/=(const PathView& other) -> BasicPath&
    {
        return Append(other.m_path.begin(), other.m_path.end());
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator/=(const string_type& src) -> BasicPath&
    {
        return Append(src.begin(), src.end());
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator/=(AZStd::string_view src) -> BasicPath&
    {
        return Append(src.begin(), src.end());
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator/=(const value_type* src) -> BasicPath&
    {
        AZStd::string_view pathView(src);
        return Append(pathView.begin(), pathView.end());
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::operator/=(value_type src) -> BasicPath&
    {
        AZStd::string_view pathView(&src, 1);
        return Append(pathView.begin(), pathView.end());
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Append(const PathView& pathView) -> BasicPath&
    {
        return Append(pathView.m_path.begin(), pathView.m_path.end());
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Append(const string_type& src) -> BasicPath&
    {
        return Append(src.begin(), src.end());
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Append(AZStd::string_view src) -> BasicPath&
    {
        return Append(src.begin(), src.end());
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Append(const value_type* src) -> BasicPath&
    {
        AZStd::string_view pathView(src);
        return Append(pathView.begin(), pathView.end());
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Append(value_type src) -> BasicPath&
    {
        AZStd::string_view pathView(&src, 1);
        return Append(pathView.begin(), pathView.end());
    }

    template <typename StringType>
    template <typename InputIt>
    constexpr auto BasicPath<StringType>::Append(InputIt first, InputIt last) -> BasicPath&
    {
        if (first == last)
        {
            // If the path to append is empty do not append anything
            // This follows the behavior of python Pathlib
            return *this;
        }
        if (Internal::IsAbsolute(first, last, m_preferred_separator))
        {
            m_path.assign(first, last);
            return *this;
        }

        // Check if the other path has a root name and
        // that the root name doesn't match the current path root name
        // The scenario where this would occur was if the current path object had a path of
        // "C:foo" and the other path object had a path "F:bar".
        // As the root names are different the other path replaces current path in it's entirety
        auto postRootNameIter = Internal::ConsumeRootName(m_path.begin(), m_path.end(), m_preferred_separator);
        auto otherPostRootNameIter = Internal::ConsumeRootName(first, last, m_preferred_separator);
        AZStd::string_view rootNameView{ m_path.begin(), postRootNameIter };

        // The RootName can only ever be two characters long which is "<drive letter>:"
        auto ToLower = [](const char element) constexpr -> char
        {
            return element >= 'A' && element <= 'Z' ? (element - 'A') + 'a' : element;
        };
        auto compareRootName = [ToLower = AZStd::move(ToLower), path_separator = m_preferred_separator](const char lhs, const char rhs) constexpr
        {
            return path_separator == PosixPathSeparator ? lhs == rhs : ToLower(lhs) == ToLower(rhs);
        };
        if (first != otherPostRootNameIter && !AZStd::equal(rootNameView.begin(), rootNameView.end(), first, otherPostRootNameIter, compareRootName))
        {
            m_path.assign(first, last);
            return *this;
        }

        if (otherPostRootNameIter != last && Internal::IsSeparator(*otherPostRootNameIter))
        {
            // If the other path has a root directory then remove the portion of the path after the root name
            // from this path
            m_path.erase(postRootNameIter, m_path.end());
        }
        else
        {
            // If the this path root name is at the end of the path string,
            // then it has no root directory nor filename
            if (rootNameView.end() == m_path.end())
            {
                /* has_root_directory || has_filename = false
                   If the root name is of the form
                   C: - then it isn't absolute unless it has a root directory C:\.
                   \\?\ = is a UNC path that can't exist without a root directory.
                   \\server - Is absolute, but has no root directory.
                   Therefore if the rootName is larger than three characters
                   then append the path separator. */
                if (rootNameView.size() >= 3)
                {
                    m_path.push_back(m_preferred_separator);
                }
            }
            else
            {
                // has_root_directory || has_filename = true
                // Potential root name forms are
                // # C:\ - already has the path separator
                // # \\?\ - can only exist with a path separator
                // # \\server\share - must a root directory to have a relative path
                // append path separator if one doesn't this path doesn't end with one
                if (!Internal::IsSeparator(m_path.back()))
                {
                    m_path.push_back(m_preferred_separator);
                }
            }
        }

        // Append the native path name of other skipping omitting the root name
        m_path.append(otherPostRootNameIter, last);
        return *this;
    }

    // modifiers
    template <typename StringType>
    constexpr void BasicPath<StringType>::clear() noexcept
    {
        m_path.clear();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::MakePreferred() -> BasicPath&
    {
        if (m_preferred_separator != PosixPathSeparator)
        {
            AZStd::replace(m_path.begin(), m_path.end(), PosixPathSeparator, m_preferred_separator);
        }
        else
        {
            AZStd::replace(m_path.begin(), m_path.end(), WindowsPathSeparator, m_preferred_separator);
        }
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::RemoveFilename() -> BasicPath&
    {
        PathView parentPath = ParentPath();
        m_path.erase(parentPath.Native().size());
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::ReplaceFilename(const PathView& replacementFilename) -> BasicPath&
    {
        RemoveFilename();
        Append(replacementFilename);
        return *this;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::ReplaceExtension(const PathView& replacementExt) -> BasicPath&
    {
        PathView p = Extension();
        if (!p.empty())
        {
            m_path.erase(m_path.size() - p.Native().size());
        }
        if (!replacementExt.empty())
        {
            if (replacementExt.Native()[0] != '.')
            {
                m_path += ".";
            }
            m_path.append(replacementExt.m_path);
        }
        return *this;
    }

    template <typename StringType>
    AZStd::string BasicPath<StringType>::String() const
    {
        return AZStd::string(m_path.begin(), m_path.end());
    }

    // extension: fixed string types with MaxPathLength capacity
    template <typename StringType>
    constexpr AZStd::fixed_string<MaxPathLength> BasicPath<StringType>::FixedMaxPathString() const
    {
        return AZStd::fixed_string<MaxPathLength>(m_path.begin(), m_path.end());
    }

    // as_posix
    // Returns a copy of the path with the path separators converted to PosixPathSeparator
    template <typename StringType>
    constexpr auto BasicPath<StringType>::AsPosix() const -> string_type
    {
        string_type resultPath(m_path.begin(), m_path.end());
        AZStd::replace(resultPath.begin(), resultPath.end(), WindowsPathSeparator, PosixPathSeparator);
        return resultPath;
    }
    template <typename StringType>
    AZStd::string BasicPath<StringType>::StringAsPosix() const
    {
        AZStd::string resultPath(m_path.begin(), m_path.end());
        AZStd::replace(resultPath.begin(), resultPath.end(), WindowsPathSeparator, PosixPathSeparator);
        return resultPath;
    }

    template <typename StringType>
    constexpr AZStd::fixed_string<MaxPathLength> BasicPath<StringType>::FixedMaxPathStringAsPosix() const noexcept
    {
        AZStd::fixed_string<MaxPathLength> resultPath(m_path.begin(), m_path.end());
        AZStd::replace(resultPath.begin(), resultPath.end(), WindowsPathSeparator, PosixPathSeparator);
        return resultPath;
    }

    template <typename StringType>
    constexpr void BasicPath<StringType>::swap(BasicPath& rhs) noexcept
    {
        m_path.swap(rhs.m_path);
        const char tmpSeparator = rhs.m_preferred_separator;
        rhs.m_preferred_separator = m_preferred_separator;
        m_preferred_separator = tmpSeparator;
    }

    // native format observers
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Native() const & noexcept -> const string_type&
    {
        return m_path;
    }
    template <typename StringType>
    constexpr auto BasicPath<StringType>::Native() const && noexcept -> const string_type&&
    {
        return AZStd::move(m_path);
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Native() & noexcept -> string_type&
    {
        return m_path;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Native() && noexcept -> string_type&&
    {
        return AZStd::move(m_path);
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::c_str() const noexcept -> const value_type*
    {
        return m_path.c_str();
    }

    template <typename StringType>
    constexpr BasicPath<StringType>::operator string_type() const
    {
        return m_path;
    }

    // compare
    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(const PathView& other) const noexcept
    {
        return static_cast<PathView>(*this).ComparePathView(other);
    }

    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(const string_type& pathString) const
    {
        return static_cast<PathView>(*this).ComparePathView(PathView(pathString, m_preferred_separator));
    }

    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(AZStd::string_view pathView) const noexcept
    {
        return static_cast<PathView>(*this).ComparePathView(pathView);
    }

    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(const value_type* pathString) const noexcept
    {
        return static_cast<PathView>(*this).ComparePathView(pathString);
    }

    // decomposition
    template <typename StringType>
    constexpr auto BasicPath<StringType>::RootName() const -> PathView
    {
        return static_cast<PathView>(*this).RootName();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::RootDirectory() const -> PathView
    {
        return static_cast<PathView>(*this).RootDirectory();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::RootPath() const -> PathView
    {
        return static_cast<PathView>(*this).RootPath();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::RelativePath() const -> PathView
    {
        return static_cast<PathView>(*this).RelativePath();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::ParentPath() const -> PathView
    {
        return static_cast<PathView>(*this).ParentPath();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Filename() const -> PathView
    {
        return static_cast<PathView>(*this).Filename();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Stem() const -> PathView
    {
        return static_cast<PathView>(*this).Stem();
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Extension() const -> PathView
    {
        return static_cast<PathView>(*this).Extension();
    }

    // query
    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::empty() const noexcept
    {
        return m_path.empty();
    }

    template <typename StringType>
    constexpr const char BasicPath<StringType>::PreferredSeparator() const noexcept
    {
        return m_preferred_separator;
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasRootName() const
    {
        return static_cast<PathView>(*this).HasRootName();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasRootDirectory() const
    {
        return static_cast<PathView>(*this).HasRootDirectory();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasRootPath() const
    {
        return static_cast<PathView>(*this).HasRootPath();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasRelativePath() const
    {
        return static_cast<PathView>(*this).HasRelativePath();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasParentPath() const
    {
        return static_cast<PathView>(*this).HasParentPath();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasFilename() const
    {
        return static_cast<PathView>(*this).HasFilename();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasStem() const
    {
        return static_cast<PathView>(*this).HasStem();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::HasExtension() const
    {
        return static_cast<PathView>(*this).HasExtension();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::IsAbsolute() const
    {
        return static_cast<PathView>(*this).IsAbsolute();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::IsRelative() const
    {
        return static_cast<PathView>(*this).IsRelative();
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::IsRelativeTo(const PathView& base) const
    {
        return static_cast<PathView>(*this).IsRelativeTo(base);
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::LexicallyNormal() const -> BasicPath
    {
        BasicPath pathResult(m_preferred_separator);
        PathView::PathIterable pathIterable = PathView::GetNormalPathParts(*this);
        for ([[maybe_unused]] auto [pathPartView, pathPartKind] : pathIterable)
        {
            pathResult /= pathPartView;
        }

        return pathResult;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::LexicallyRelative(const PathView& base) const -> BasicPath
    {
        BasicPath pathResult(m_preferred_separator);
        PathView::PathIterable pathIterable;
        PathView::MakeRelativeTo(pathIterable, *this, base);
        for ([[maybe_unused]] auto [pathPartView, pathPartKind] : pathIterable)
        {
            pathResult /= pathPartView;
        }

        return pathResult;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::LexicallyProximate(const PathView& base) const -> BasicPath
    {
        BasicPath result = LexicallyRelative(base);
        if (result.empty())
        {
            return *this;
        }
        return result;
    }

    template <typename StringType>
    [[nodiscard]] constexpr bool BasicPath<StringType>::Match(AZStd::string_view pathPattern) const
    {
        return static_cast<PathView>(*this).Match(pathPattern);
    }

    // Path Iterator
    template <typename StringType>
    constexpr auto BasicPath<StringType>::begin() const -> const_iterator
    {
        auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        const_iterator it;
        it.m_path_ref = this;
        it.m_state = static_cast<typename const_iterator::ParserState>(pathParser.m_parser_state);
        it.m_path_entry_view = pathParser.m_path_raw_entry;
        it.m_stashed_elem = *pathParser;
        return it;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::end() const -> const_iterator
    {
        const_iterator it;
        it.m_state = const_iterator::AtEnd;
        it.m_path_ref = this;
        return it;
    }

    // path.append
    template <typename StringType>
    constexpr BasicPath<StringType> operator/(const BasicPath<StringType>& lhs, const PathView& rhs)
    {
        BasicPath<StringType> result(lhs);
        result /= rhs;
        return result;
    }


    template <typename StringType>
    constexpr BasicPath<StringType> operator/(const BasicPath<StringType>& lhs, AZStd::string_view rhs)
    {
        BasicPath<StringType> result(lhs);
        result /= rhs;
        return result;
    }

    template <typename StringType>
    constexpr BasicPath<StringType> operator/(const BasicPath<StringType>& lhs, const typename BasicPath<StringType>::value_type* rhs)
    {
        BasicPath<StringType> result(lhs);
        result /= rhs;
        return result;
    }

    template <typename StringType>
    constexpr void swap(BasicPath<StringType>& lhs, BasicPath<StringType>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <typename StringType>
    constexpr size_t hash_value(const BasicPath<StringType>& pathToHash)
    {
        return AZStd::hash<BasicPath<StringType>>{}(pathToHash);
    }
}


//! AZ::PathView functions which returns FixedMaxPath
namespace AZ::IO
{
    [[nodiscard]] constexpr bool PathView::IsRelativeTo(const PathView& base) const
    {
        // PathView::LexicallyRelative is not being used as it returns a FixedMaxPath
        // which has a limitation that it requires the relative path to fit within
        // an AZ::IO::MaxPathLength buffer
        const bool exactCaseCompare = m_preferred_separator == PosixPathSeparator
            || base.m_preferred_separator == PosixPathSeparator;
        auto ComparePathPart = [exactCaseCompare](
            const PathIterable::PartKindPair& left, const PathIterable::PartKindPair& right) -> bool
        {
            return Internal::ComparePathSegment(left.first, right.first, exactCaseCompare) == 0;
        };

        const PathIterable thisPathParts = GetNormalPathParts(*this);
        const PathIterable basePathParts = GetNormalPathParts(base);
        [[maybe_unused]] auto [thisPathIter, basePathIter] = AZStd::mismatch(thisPathParts.begin(), thisPathParts.end(),
            basePathParts.begin(), basePathParts.end(), ComparePathPart);
        // Check if the entire base path has been consumed. If not, *this path cannot be relative to it
        if (basePathIter != basePathParts.end())
        {
            return false;
        }

        // If the base path isn't empty and has been fully consumed, then *this path is relative
        // Also if the base path is empty, then any relative path is relative to an empty path('.')
        return !basePathParts.empty() || !thisPathParts.IsAbsolute();
    }

    constexpr auto PathView::LexicallyNormal() const -> FixedMaxPath
    {
        FixedMaxPath pathResult(m_preferred_separator);
        PathIterable pathIterable = GetNormalPathParts(*this);
        for ([[maybe_unused]] auto [pathPartView, pathPartKind] : pathIterable)
        {
            pathResult /= pathPartView;
        }

        return pathResult;
    }

    constexpr auto PathView::LexicallyRelative(const PathView& base) const -> FixedMaxPath
    {
        FixedMaxPath pathResult(m_preferred_separator);
        PathIterable pathIterable;
        MakeRelativeTo(pathIterable, *this, base);
        for ([[maybe_unused]] auto [pathPartView, pathPartKind] : pathIterable)
        {
            pathResult /= pathPartView;
        }

        return pathResult;
    }

    constexpr auto PathView::LexicallyProximate(const PathView& base) const -> FixedMaxPath
    {
        FixedMaxPath pathResult = LexicallyRelative(base);
        if (pathResult.empty())
        {
            return FixedMaxPath(*this);
        }

        return pathResult;
    }
}

//! AZ::Path iterator implementation
namespace AZ::IO
{
    // PathIterator
    template <typename PathType>
    constexpr auto PathIterator<PathType>::operator*() const -> reference
    {
        return m_stashed_elem;
    }

    template <typename PathType>
    constexpr auto PathIterator<PathType>::operator++() -> PathIterator&
    {
        AZ_Assert(m_state != Singular, "attempting to increment a singular iterator (i.e a default constructed iterator)");
        AZ_Assert(m_state != AtEnd, "attempting to increment the path end iterator is not allowed");
        return increment();
    }

    template <typename PathType>
    constexpr auto PathIterator<PathType>::operator++(int) -> PathIterator
    {
        PathIterator iter(*this);
        this->operator++();
        return iter;
    }

    template <typename PathType>
    constexpr auto PathIterator<PathType>::operator--() -> PathIterator&
    {
        AZ_Assert(m_state != Singular, "attempting to decrement a singular iterator (i.e a default constructed iterator");
        AZ_Assert(m_path_entry_view.data() != m_path_ref->Native().data(), "attempting to decrement the path begin iterator is not allowed");
        return decrement();
    }

    template <typename PathType>
    constexpr auto PathIterator<PathType>::operator--(int) -> PathIterator
    {
        PathIterator iter(*this);
        this->operator--();
        return iter;
    }

    template <typename PathType>
    constexpr auto PathIterator<PathType>::increment() -> PathIterator&
    {
        parser::PathParser pathParser(m_path_ref->Native(), m_path_entry_view,
            static_cast<parser::ParserState>(m_state), m_path_ref->m_preferred_separator);
        ++pathParser;
        m_state = static_cast<ParserState>(pathParser.m_parser_state);
        m_path_entry_view = pathParser.m_path_raw_entry;
        m_stashed_elem = *pathParser;
        return *this;
    }

    template <typename PathType>
    constexpr auto PathIterator<PathType>::decrement() -> PathIterator&
    {
        parser::PathParser pathParser(m_path_ref->Native(), m_path_entry_view,
            static_cast<parser::ParserState>(m_state), m_path_ref->m_preferred_separator);
        --pathParser;
        m_state = static_cast<ParserState>(pathParser.m_parser_state);
        m_path_entry_view = pathParser.m_path_raw_entry;
        m_stashed_elem = *pathParser;
        return *this;
    }

    template <typename PathType>
    constexpr bool operator==(const PathIterator<PathType>& lhs, const PathIterator<PathType>& rhs)
    {
        // Compare the address of the full path reference as well as the address of the path component
        return lhs.m_path_ref == rhs.m_path_ref && lhs.m_path_entry_view.data() == rhs.m_path_entry_view.data();
    }
    template <typename PathType>
    constexpr bool operator!=(const PathIterator<PathType>& lhs, const PathIterator<PathType>& rhs)
    {
        return !operator==(lhs, rhs);
    }
}

namespace AZStd
{
    template <>
    struct hash<AZ::IO::PathView>
    {
        constexpr size_t operator()(const AZ::IO::PathView& pathToHash) const noexcept
        {
            auto pathParser = AZ::IO::parser::PathParser::CreateBegin(pathToHash.Native(), pathToHash.m_preferred_separator);
            return AZ::IO::parser::HashPath(pathParser);
        }
    };
    template <typename StringType>
    struct hash<AZ::IO::BasicPath<StringType>>
    {
        constexpr size_t operator()(const AZ::IO::BasicPath<StringType>& pathToHash) const noexcept
        {
            return AZStd::hash<AZ::IO::PathView>{}(pathToHash);
        }
    };

    // Instantiate the hash function for Path and FixedMaxPath
    template struct hash<AZ::IO::Path>;
    template struct hash<AZ::IO::FixedMaxPath>;
}

// Explicit instantiations of our support Path classes
namespace AZ::IO
{
    // PathView hash
    constexpr size_t hash_value(const PathView& pathToHash) noexcept
    {
        return AZStd::hash<PathView>{}(pathToHash);
    }
}

// extern instantiations of Path templates to prevent implicit instantiations
namespace AZ::IO
{
    // Swap function explicit declarations
    extern template void swap<AZStd::string>(Path& lhs, Path& rhs) noexcept;
    extern template void swap<FixedMaxPathString>(FixedMaxPath& lhs, FixedMaxPath& rhs) noexcept;

    // Hash function explicit declarations
    extern template size_t hash_value<AZStd::string>(const Path& pathToHash);
    extern template size_t hash_value<FixedMaxPathString>(const FixedMaxPath& pathToHash);

    // Append operator explicit declarations
    extern template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs, const PathView& rhs);
    extern template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, const PathView& rhs);
    extern template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs, AZStd::string_view rhs);
    extern template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, AZStd::string_view rhs);
    extern template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs,
        const typename BasicPath<AZStd::string>::value_type* rhs);
    extern template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs,
        const typename BasicPath<FixedMaxPathString>::value_type* rhs);

    // Iterator compare explicit declarations
    extern template bool operator==<const PathView>(const PathIterator<const PathView>& lhs,
        const PathIterator<const PathView>& rhs);
    extern template bool operator==<const Path>(const PathIterator<const Path>& lhs,
        const PathIterator<const Path>& rhs);
    extern template bool operator==<const FixedMaxPath>(const PathIterator<const FixedMaxPath>& lhs,
        const PathIterator<const FixedMaxPath>& rhs);
    extern template bool operator!=<const PathView>(const PathIterator<const PathView>& lhs,
        const PathIterator<const PathView>& rhs);
    extern template bool operator!=<const Path>(const PathIterator<const Path>& lhs,
        const PathIterator<const Path>& rhs);
    extern template bool operator!=<const FixedMaxPath>(const PathIterator<const FixedMaxPath>& lhs,
        const PathIterator<const FixedMaxPath>& rhs);
}

namespace AZStd::ranges
{
    // A PathView is a borrowed range, it does not own the content of the Path it is viewing
    template<>
    inline constexpr bool enable_borrowed_range<AZ::IO::PathView> = true;

    template<>
    inline constexpr bool enable_view<AZ::IO::PathView> = true;
}

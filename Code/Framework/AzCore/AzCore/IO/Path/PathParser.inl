/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/AzCore_Traits_Platform.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/concepts/concepts.h>

namespace AZ::IO::Internal
{
    constexpr bool IsSeparator(const char elem)
    {
        return elem == '/' || elem == '\\';
    }
    template <typename InputIt, typename EndIt, typename = AZStd::enable_if_t<AZStd::input_iterator<InputIt>>>
    static constexpr bool HasDrivePrefix(InputIt first, EndIt last)
    {
        size_t prefixSize = AZStd::distance(first, last);
        if (prefixSize < 2 || *AZStd::next(first, 1) != ':')
        {
            // Drive prefix must be at least two characters and have a colon for the second character
            return false;
        }

        constexpr size_t ValidDrivePrefixRange = 26;
        // Uppercase the drive letter by bitwise and'ing out the the 2^5 bit
        unsigned char driveLetter = static_cast<unsigned char>(*first);

        driveLetter &= 0b1101'1111;
        // normalize the character value in the range of A-Z -> 0-25
        driveLetter -= 'A';
        return driveLetter < ValidDrivePrefixRange;
    }

    static constexpr bool HasDrivePrefix(AZStd::string_view prefix)
    {
        return HasDrivePrefix(prefix.begin(), prefix.end());
    }

    // Returns whether the path prefix models a Windows UNC Path
    // https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats#unc-paths
    static constexpr bool IsUncPath(AZStd::string_view path, const char preferredSeparator)
    {
        // Posix paths are never a Windows UNC path
        if (preferredSeparator == PosixPathSeparator)
        {
            return false;
        }

        // A windows network drive has the form of \\<text> such as "\\server"
        return path.size() >= 3 && IsSeparator(path[0]) && IsSeparator(path[1]) && !IsSeparator(path[2]);
    }

    //! Returns an iterator past the end of the consumed root name
    //! Windows root names can have include drive letter within them
    template <typename InputIt>
    constexpr auto ConsumeRootName(InputIt entryBeginIter, InputIt entryEndIter, const char preferredSeparator)
        -> AZStd::enable_if_t<AZStd::forward_iterator<InputIt>, InputIt>
    {
        if (preferredSeparator == PosixPathSeparator)
        {
            // If the preferred separator is forward slash the parser is in posix path
            // parsing mode, which doesn't have a root name,
            // unless we're on a posix platform that uses a custom path root separator
#if defined(AZ_TRAIT_CUSTOM_PATH_ROOT_SEPARATOR)
            const AZStd::string_view path{ entryBeginIter, entryEndIter };
            const auto positionOfPathSeparator = path.find(AZ_TRAIT_CUSTOM_PATH_ROOT_SEPARATOR);
            if (positionOfPathSeparator != AZStd::string_view::npos)
            {
                return AZStd::next(entryBeginIter, positionOfPathSeparator + 1);
            }
#endif
            return entryBeginIter;
        }
        else
        {
            // Information for GetRootName has been gathered from Microsoft <filesystem> header
            // Below are examples of paths and what there root-name will return
            // "/" - returns ""
            // "foo/" - returns ""
            // "C:DriveRelative" - returns "C:"
            // "C:\\DriveAbsolute" - returns "C:"
            // "C://DriveAbsolute" - returns "C:"
            // "\\server\share" - returns "\\server"
            // The following paths are based on the UNC specification to work with paths longer than the 260 character path limit
            // https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file?redirectedfrom=MSDN#maximum-path-length-limitation
            // \\?\device - returns "\\?"
            // \??\device - returns "\??"
            // \\.\device - returns "\\."


            AZStd::string_view path{ entryBeginIter, entryEndIter };

            if (path.size() < 2)
            {
                // A root name is either <drive letter><colon> or a network path
                // therefore it has a least two characters
                return entryBeginIter;
            }

            if (HasDrivePrefix(path))
            {
                // If the path has a drive prefix, then it has a root name of <driver letter><colon>
                return AZStd::next(entryBeginIter, 2);
            }

            if (!Internal::IsSeparator(path[0]))
            {
                // At this point all other root names start with a path separator
                return entryBeginIter;
            }

            // Check if the path has the form of "\\?\, "\??\" or "\\.\"
            const bool pathInUncForm = path.size() >= 4 && Internal::IsSeparator(path[3])
                && (path.size() == 4 || !Internal::IsSeparator(path[4]));
            if (pathInUncForm)
            {
                // \\?\<0 or more> or \\.\$<zero or more>
                const bool slashQuestionMark = Internal::IsSeparator(path[1]) && (path[2] == '?' || path[2] == '.');
                // \??\<0 or more>
                const bool questionMarkTwice = path[1] == '?' && path[2] == '?';
                if (slashQuestionMark || questionMarkTwice)
                {
                    // Return the root value root slash - i.e "\\?"
                    return AZStd::next(entryBeginIter, 3);
                }
            }

            if (IsUncPath(path, preferredSeparator))
            {
                // Find the next path separator for network paths that have the form of \\server\share
                constexpr AZStd::string_view PathSeparators = { "/\\" };
                size_t nextPathSeparatorOffset = path.find_first_of(PathSeparators, 3);
                return AZStd::next(entryBeginIter, nextPathSeparatorOffset != AZStd::string_view::npos ? nextPathSeparatorOffset : path.size());
            }

            return entryBeginIter;
        }
    }

    //! Returns an iterator past the end of the consumed path separator(s)
    template <typename InputIt>
    constexpr InputIt ConsumeSeparator(InputIt entryBeginIter, InputIt entryEndIter) noexcept
    {
        return AZStd::find_if_not(entryBeginIter, entryEndIter, [](const char elem) { return Internal::IsSeparator(elem); });
    }

    //! Returns an iterator past the end of the consumed filename
    template <typename InputIt>
    constexpr InputIt ConsumeName(InputIt entryBeginIter, InputIt entryEndIter) noexcept
    {
        return AZStd::find_if(entryBeginIter, entryEndIter, [](const char elem) { return Internal::IsSeparator(elem); });
    }

    //! Check if a path is absolute on a OS basis
    //! If the preferred separator is '/' just checks if the path starts with a '/
    //! Otherwise a check for a Windows absolute path occurs
    //! Windows absolute paths can include a RootName
    template <typename InputIt, typename EndIt, typename = AZStd::enable_if_t<AZStd::input_iterator<InputIt>>>
    static constexpr bool IsAbsolute(InputIt first, EndIt last, const char preferredSeparator)
    {
        size_t pathSize = AZStd::distance(first, last);

        // If the preferred separator is a forward slash
        // than an absolute path is simply one that starts with a forward slash,
        // unless we're on a posix platform that uses a custom path root separator
        if (preferredSeparator == PosixPathSeparator)
        {
#if defined(AZ_TRAIT_CUSTOM_PATH_ROOT_SEPARATOR)
            const AZStd::string_view path{ first, last };
            return path.find(AZ_TRAIT_CUSTOM_PATH_ROOT_SEPARATOR) != AZStd::string_view::npos;
#else
            return pathSize > 0 && IsSeparator(*first);
#endif
        }
        else
        {
            if (Internal::HasDrivePrefix(first, last))
            {
                // If a windows path ends starts with C:foo it is a root relative path
                // A path is absolute root absolute on windows if it starts with <drive_letter><colon><path_separator>
                return pathSize > 2 && Internal::IsSeparator(*AZStd::next(first, 2));
            }

            return first != ConsumeRootName(first, last, preferredSeparator);
        }
    }
    static constexpr bool IsAbsolute(AZStd::string_view pathView, const char preferredSeparator)
    {
        // Uses the template preferred to branch on the absolute path check
        // logic
        return IsAbsolute(pathView.begin(), pathView.end(), preferredSeparator);
    }

    // Compares path segments using either Posix or Windows path rules based on the exactCaseCompare option
    static constexpr int ComparePathSegment(AZStd::string_view left, AZStd::string_view right, bool exactCaseCompare)
    {
        const size_t maxCharsToCompare = (AZStd::min)(left.size(), right.size());

        if (exactCaseCompare)
        {
            const int charCompareResult = maxCharsToCompare
                ? AZStd::char_traits<char>::compare(left.data(), right.data(), maxCharsToCompare)
                : 0;
            return charCompareResult == 0
                ? static_cast<int>(static_cast<ptrdiff_t>(left.size()) - static_cast<ptrdiff_t>(right.size()))
                : charCompareResult;
        }
        else
        {
            if (az_builtin_is_constant_evaluated())
            {
                // compile time implementation
                auto ToLower = [](const char element) constexpr -> char
                {
                    return element >= 'A' && element <= 'Z' ? (element - 'A') + 'a' : element;
                };
                const auto mismatchResult = AZStd::ranges::mismatch(left, right, AZStd::char_traits<char>::eq, ToLower, ToLower);
                const size_t leftMismatchCount = AZStd::ranges::distance(mismatchResult.in1, left.end());
                const size_t rightMismatchCount = AZStd::ranges::distance(mismatchResult.in2, right.end());
                if (leftMismatchCount == 0 && rightMismatchCount == 0)
                {
                    // Both path segments are equal return 0
                    return 0;
                }
                else
                {
                    // There is a mismatch, so determine if it was due to characters not comparing equal
                    if (leftMismatchCount != 0 && rightMismatchCount != 0)
                    {
                        return ToLower(*mismatchResult.in1) < ToLower(*mismatchResult.in2) ? -1 : 1;
                    }
                    else if (leftMismatchCount != 0)
                    {
                        // The left path segment has more characters left so it is greater than the right
                        return 1;
                    }
                    else
                    {
                        // The left path segment has less characters left so it is less than the right
                        return -1;
                    }
                }

            }
            else
            {
                const int charCompareResult = maxCharsToCompare
                    ? azstrnicmp(left.data(), right.data(), maxCharsToCompare)
                    : 0;
                return charCompareResult == 0
                    ? static_cast<int>(aznumeric_cast<ptrdiff_t>(left.size()) - aznumeric_cast<ptrdiff_t>(right.size()))
                    : charCompareResult;
            }
        }
    }
}

//! PathParser implementation
//! For internal use only
namespace AZ::IO::parser
{
    using parser_path_type = PathView;
    using string_view_pair = AZStd::pair<AZStd::string_view, AZStd::string_view>;
    using PosPtr = const typename parser_path_type::value_type*;

    enum ParserState : uint8_t
    {
        // Zero is a special sentinel value used by default constructed iterators.
        PS_BeforeBegin = PathView::const_iterator::BeforeBegin,
        PS_InRootName = PathView::const_iterator::InRootName,
        PS_InRootDir = PathView::const_iterator::InRootDir,
        PS_InFilenames = PathView::const_iterator::InFilenames,
        PS_AtEnd = PathView::const_iterator::AtEnd
    };

    struct PathParser
    {
        AZStd::string_view m_path_view;
        AZStd::string_view m_path_raw_entry;
        ParserState m_parser_state{};
        const char m_preferred_separator{ AZ_TRAIT_OS_PATH_SEPARATOR };

        constexpr PathParser(AZStd::string_view path, ParserState state, const char preferredSeparator) noexcept
            : m_path_view(path)
            , m_parser_state(state)
            , m_preferred_separator(preferredSeparator)
        {
        }

        constexpr PathParser(AZStd::string_view path, AZStd::string_view entry, ParserState state, const char preferredSeparator) noexcept
            : m_path_view(path)
            , m_path_raw_entry(entry)
            , m_parser_state(static_cast<ParserState>(state))
            , m_preferred_separator(preferredSeparator)
        {
        }

        constexpr static PathParser CreateBegin(AZStd::string_view path, const char preferredSeparator) noexcept
        {
            PathParser pathParser(path, PS_BeforeBegin, preferredSeparator);
            pathParser.Increment();
            return pathParser;
        }

        constexpr static PathParser CreateEnd(AZStd::string_view path, const char preferredSeparator) noexcept
        {
            PathParser pathParser(path, PS_AtEnd, preferredSeparator);
            return pathParser;
        }

        //! Returns the pointer to the beginning of the next parser token
        //! If there are no more tokens, nullptr is returned
        constexpr PosPtr Peek() const noexcept
        {
            auto tokenEnd = getNextTokenStartPos();
            auto pathEnd = m_path_view.end();
            return tokenEnd == pathEnd ? nullptr : tokenEnd;
        }

        constexpr void Increment() noexcept
        {
            const PosPtr pathEnd = m_path_view.end();
            const PosPtr currentPathEntry = getNextTokenStartPos();
            if (currentPathEntry == pathEnd)
            {
                return MakeState(PS_AtEnd);
            }

            switch (m_parser_state)
            {
            case PS_BeforeBegin:
            {
                /*
                * First the determine if the path contains only a root-name such as "C:" or is a filename such as "foo"
                * root-relative path(Windows only) - C:foo
                * root-absolute path - C:\foo
                * root-absolute path - /foo
                * relative path - foo
                *
                * Try to consume the root-name then the root directory to determine if path entry
                * being parsed is a root-name or filename
                * The State transitions from BeforeBegin are
                * "C:", "\\server\", "\\?\", "\??\", "\\.\" -> Root Name
                * "/",  "\" -> Root Directory
                * "path/foo", "foo" -> Filename
                */
                auto rootNameEnd = Internal::ConsumeRootName(currentPathEntry, pathEnd, m_preferred_separator);
                if (currentPathEntry != rootNameEnd)
                {
                    // Transition to the Root Name state
                    return MakeState(PS_InRootName, currentPathEntry, rootNameEnd);
                }
                [[fallthrough]];
            }
            case PS_InRootName:
            {
                auto rootDirEnd = Internal::ConsumeSeparator(currentPathEntry, pathEnd);
                if (currentPathEntry != rootDirEnd)
                {
                    // Transition to Root Directory state
                    return MakeState(PS_InRootDir, currentPathEntry, rootDirEnd);
                }
                [[fallthrough]];
            }
            case PS_InRootDir:
            {
                auto filenameEnd = Internal::ConsumeName(currentPathEntry, pathEnd);
                if (currentPathEntry != filenameEnd)
                {
                    return MakeState(PS_InFilenames, currentPathEntry, filenameEnd);
                }
                [[fallthrough]];
            }
            case PS_InFilenames:
            {
                auto separatorEnd = Internal::ConsumeSeparator(currentPathEntry, pathEnd);
                if (separatorEnd != pathEnd)
                {
                    // find the end of the current filename entry
                    auto filenameEnd = Internal::ConsumeName(separatorEnd, pathEnd);
                    return MakeState(PS_InFilenames, separatorEnd, filenameEnd);
                }
                // If after consuming the separator that path entry is at the end iterator
                // move the path state to AtEnd
                return MakeState(PS_AtEnd);
            }
            case PS_AtEnd:
                AZ_Assert(false, "Path Parser cannot be incremented when it is in the AtEnd state");
            }
        }

        constexpr void Decrement() noexcept
        {
            auto pathStart = m_path_view.begin();
            auto currentPathEntry = getCurrentTokenStartPos();

            if (currentPathEntry == pathStart)
            {
                // we're decrementing the begin
                return MakeState(PS_BeforeBegin);
            }
            switch (m_parser_state)
            {
            case PS_AtEnd:
            {
                /*
                * First the determine if the path contains only a root-name such as "C:" or is a filename such as "foo"
                * root-relative path(Windows only) - C:foo
                * root-absolute path - C:\foo
                * root-absolute path - /foo
                * relative path - foo
                * Try to consume the root-name then the root directory to determine if path entry
                * being parsed is a root-name or filename
                * The State transitions from AtEnd are
                * "/path/foo/", "foo/", "C:foo\", "C:\foo\" -> Trailing Separator
                * "/path/foo", "foo", "C:foo", "C:\foo" -> Filename
                * "/", "C:\" or "\\server\" -> Root Directory
                * "C:", "\\server", "\\?", "\??", "\\." -> Root Name
                */
                auto rootNameEnd = Internal::ConsumeRootName(pathStart, currentPathEntry, m_preferred_separator);
                if (pathStart != rootNameEnd && currentPathEntry == rootNameEnd)
                {
                    // Transition to the Root Name state
                    return MakeState(PS_InRootName, pathStart, currentPathEntry);
                }

                auto rootDirEnd = Internal::ConsumeSeparator(rootNameEnd, currentPathEntry);
                if (rootNameEnd != rootDirEnd && currentPathEntry == rootDirEnd)
                {
                    // Transition to Root Directory state
                    return MakeState(PS_InRootDir, rootNameEnd, currentPathEntry);
                }

                auto filenameEnd = currentPathEntry;
                if (Internal::IsSeparator(*(filenameEnd - 1)))
                {
                    // The last character a path separator that isn't root directory
                    // consume all the preceding path separators
                    filenameEnd = Internal::ConsumeSeparator(AZStd::make_reverse_iterator(filenameEnd),
                        AZStd::make_reverse_iterator(rootDirEnd)).base();
                }

                // The previous state will be Filename, so the beginning of the filename is searched found
                auto filenameBegin = Internal::ConsumeName(AZStd::make_reverse_iterator(filenameEnd),
                    AZStd::make_reverse_iterator(rootDirEnd)).base();
                return MakeState(PS_InFilenames, filenameBegin, filenameEnd);
            }
            case PS_InFilenames:
            {
                /* The State transitions from Filename are
                * "/path/foo" -> Filename
                *        ^
                * "C:\foo" -> Root Directory
                *     ^
                * "C:foo" -> Root Name
                *    ^
                * "foo" -> This case has been taken care of by the current path entry != path start check
                *  ^
                */
                auto rootNameEnd = Internal::ConsumeRootName(pathStart, currentPathEntry, m_preferred_separator);
                if (pathStart != rootNameEnd && currentPathEntry == rootNameEnd)
                {
                    // Transition to the Root Name state
                    return MakeState(PS_InRootName, pathStart, rootNameEnd);
                }

                auto rootDirEnd = Internal::ConsumeSeparator(rootNameEnd, currentPathEntry);
                if (rootNameEnd != rootDirEnd && currentPathEntry == rootDirEnd)
                {
                    // Transition to Root Directory state
                    return MakeState(PS_InRootDir, rootNameEnd, rootDirEnd);
                }
                // The previous state will be Filename again, so first the end of that filename is found
                // proceeded by finding the beginning of that filename
                auto filenameEnd = Internal::ConsumeSeparator(AZStd::make_reverse_iterator(currentPathEntry),
                    AZStd::make_reverse_iterator(rootDirEnd)).base();
                auto filenameBegin = Internal::ConsumeName(AZStd::make_reverse_iterator(filenameEnd),
                    AZStd::make_reverse_iterator(rootDirEnd)).base();
                return MakeState(PS_InFilenames, filenameBegin, filenameEnd);
            }
            case PS_InRootDir:
            {
                /* The State transitions from Root Directory are
                * "C:\"  "\\server\", "\\?\", "\??\", "\\.\" -> Root Name
                *    ^            ^        ^      ^       ^
                * "/" -> This case has been taken care of by the current path entry != path start check
                *  ^
                */
                return MakeState(PS_InRootName, pathStart, currentPathEntry);
            }
            case PS_InRootName:
                // The only valid state transition from Root Name is BeforeBegin
                return MakeState(PS_BeforeBegin);
            case PS_BeforeBegin:
                AZ_Assert(false, "Path Parser cannot be decremented when it is in the BeforeBegin State");
            }
        }

        //! Return a view of the current element in the path processor state
        constexpr AZStd::string_view operator*() const noexcept
        {
            switch (m_parser_state)
            {
            case PS_BeforeBegin:
                [[fallthrough]];
            case PS_AtEnd:
                break;
            case PS_InRootDir:
                return m_preferred_separator == '/' ? "/" : "\\";
            case PS_InRootName:
            case PS_InFilenames:
                return m_path_raw_entry;
            default:
                AZ_Assert(false, "Path Parser is in an invalid state");
            }
            return {};
        }

        constexpr explicit operator bool() const noexcept
        {
            return m_parser_state != PS_BeforeBegin && m_parser_state != PS_AtEnd;
        }

        constexpr PathParser& operator++() noexcept
        {
            Increment();
            return *this;
        }

        constexpr PathParser& operator--() noexcept
        {
            Decrement();
            return *this;
        }

        constexpr bool AtEnd() const noexcept
        {
            return m_parser_state == PS_AtEnd;
        }

        constexpr bool InRootDir() const noexcept
        {
            return m_parser_state == PS_InRootDir;
        }

        constexpr bool InRootName() const noexcept
        {
            return m_parser_state == PS_InRootName;
        }

        constexpr bool InRootPath() const noexcept
        {
            return InRootName() || InRootDir();
        }

    private:
        constexpr void MakeState(ParserState newState, typename AZStd::string_view::iterator start, typename AZStd::string_view::iterator end) noexcept
        {
            m_parser_state = newState;
            m_path_raw_entry = AZStd::string_view(start, end);
        }
        constexpr void MakeState(ParserState newState) noexcept
        {
            m_parser_state = newState;
            m_path_raw_entry = {};
        }

        //! Return a pointer to the first character after the currently lexed element.
        constexpr typename AZStd::string_view::iterator getNextTokenStartPos() const noexcept
        {
            switch (m_parser_state)
            {
            case PS_BeforeBegin:
                return m_path_view.begin();
            case PS_InRootName:
            case PS_InRootDir:
            case PS_InFilenames:
                return m_path_raw_entry.end();
            case PS_AtEnd:
                return m_path_view.end();
            default:
                AZ_Assert(false, "Path Parser is in an invalid state");
            }
            return m_path_view.end();
        }

        //! Return a pointer to the first character in the currently lexed element.
        constexpr typename AZStd::string_view::iterator getCurrentTokenStartPos() const noexcept
        {
            switch (m_parser_state)
            {
            case PS_BeforeBegin:
            case PS_InRootName:
                return m_path_view.begin();
            case PS_InRootDir:
            case PS_InFilenames:
                return m_path_raw_entry.begin();
            case PS_AtEnd:
                return m_path_view.end();
            default:
                AZ_Assert(false, "Path Parser is in an invalid state");
            }
            return m_path_view.end();
        }
    };

    constexpr string_view_pair SeparateFilename(const AZStd::string_view& srcView)
    {
        if (srcView == "." || srcView == ".." || srcView.empty())
        {
            return string_view_pair{ srcView, "" };
        }
        auto pos = srcView.find_last_of('.');
        if (pos == AZStd::string_view::npos || pos == 0)
        {
            return string_view_pair{ srcView, AZStd::string_view{} };
        }
        return string_view_pair{ srcView.substr(0, pos), srcView.substr(pos) };
    }

    // path part decomposition
    enum class PathPartKind : uint8_t
    {
        PK_None,
        PK_RootName,
        PK_RootSep,
        PK_Filename,
        PK_Dot,
        PK_DotDot,
    };

    constexpr PathPartKind ClassifyPathPart(const PathParser& parser)
    {
        // Check each parser state to determine the PathPartKind
        if (parser.m_parser_state == PS_InRootDir)
        {
            return PathPartKind::PK_RootSep;
        }
        if (parser.m_parser_state == PS_InRootName)
        {
            return PathPartKind::PK_RootName;
        }

        // Fallback to checking parser pathEntry view value
        // to determine if the special "." or ".." values are being used
        AZStd::string_view pathPart = *parser;
        if (pathPart == ".")
        {
            return PathPartKind::PK_Dot;
        }
        if (pathPart == "..")
        {
            return PathPartKind::PK_DotDot;
        }

        // Return PathPartKind of PK_Filename if the parser state doesn't match
        // the states of InRootDir or InRootName and the filename
        // isn't made up of the special directory values of "." and ".."
        return PathPartKind::PK_Filename;
    }

    // path part consumption
    constexpr bool ConsumeRootName(PathParser* pathParser)
    {
        static_assert(PS_BeforeBegin == 1 && PS_InRootName == 2,
            "PathParser must be in state before begin or in the root name in order to consume the root name");
        while (pathParser->m_parser_state <= PS_InRootName)
        {
            ++(*pathParser);
        }
        return pathParser->m_parser_state == PS_AtEnd;
    }
    constexpr bool ConsumeRootDir(PathParser* pathParser)
    {
        static_assert(PS_BeforeBegin == 1 && PS_InRootName == 2 && PS_InRootDir == 3,
            "PathParser must be in state before begin, in the root name or in the root directory in order to consume the root directory");
        while (pathParser->m_parser_state <= PS_InRootDir)
        {
            ++(*pathParser);
        }
        return pathParser->m_parser_state == PS_AtEnd;
    }

    // path.comparisons
    constexpr int CompareRootName(PathParser* lhsPathParser, PathParser* rhsPathParser)
    {
        if (!lhsPathParser->InRootName() && !rhsPathParser->InRootName())
        {
            return 0;
        }

        auto GetRootName = [](PathParser* pathParser) constexpr -> AZStd::string_view
        {
            return pathParser->InRootName() ? **pathParser : "";
        };

        const bool exactCaseCompare = lhsPathParser->m_preferred_separator == PosixPathSeparator
            || rhsPathParser->m_preferred_separator == PosixPathSeparator;
        int res = Internal::ComparePathSegment(GetRootName(lhsPathParser), GetRootName(rhsPathParser), exactCaseCompare);
        ConsumeRootName(lhsPathParser);
        ConsumeRootName(rhsPathParser);
        return res;
    }
    constexpr int CompareRootDir(PathParser* lhsPathParser, PathParser* rhsPathParser)
    {
        if (!lhsPathParser->InRootDir() && rhsPathParser->InRootDir())
        {
            return -1;
        }
        else if (lhsPathParser->InRootDir() && !rhsPathParser->InRootDir())
        {
            return 1;
        }
        else
        {
            ConsumeRootDir(lhsPathParser);
            ConsumeRootDir(rhsPathParser);
            return 0;
        }
    }
    constexpr int CompareRelative(PathParser* lhsPathParserPtr, PathParser* rhsPathParserPtr)
    {
        auto& lhsPathParser = *lhsPathParserPtr;
        auto& rhsPathParser = *rhsPathParserPtr;

        const bool exactCaseCompare = lhsPathParser.m_preferred_separator == PosixPathSeparator
            || rhsPathParser.m_preferred_separator == PosixPathSeparator;
        while (lhsPathParser && rhsPathParser)
        {
            bool leftDotPathSkipped{};
            for (PathPartKind lhsPartKind = ClassifyPathPart(lhsPathParser); lhsPathParser && lhsPartKind == PathPartKind::PK_Dot;
                lhsPartKind = ClassifyPathPart(lhsPathParser))
            {
                ++lhsPathParser;
                leftDotPathSkipped = true;
            }

            if (leftDotPathSkipped)
            {
                continue;
            }

            bool rightDotPathSkipped{};
            for (PathPartKind rhsPartKind = ClassifyPathPart(rhsPathParser); rhsPathParser && rhsPartKind == PathPartKind::PK_Dot;
                rhsPartKind = ClassifyPathPart(rhsPathParser))
            {
                ++rhsPathParser;
                rightDotPathSkipped = true;
            }

            if (rightDotPathSkipped)
            {
                continue;
            }

            if (int res = Internal::ComparePathSegment(*lhsPathParser, *rhsPathParser, exactCaseCompare);
                res != 0)
            {
                return res;
            }
            ++lhsPathParser;
            ++rhsPathParser;
        }

        // Advance past any trailing single dot segements of a path
        // Such as /foo/bar/.
        // This make sure that paths that end with a dot('.')
        // will properly advance to the PS_AtEnd state
        for (; lhsPathParser; ++lhsPathParser)
        {
            if (PathPartKind lhsPartKind = ClassifyPathPart(lhsPathParser);
                lhsPartKind != PathPartKind::PK_Dot)
            {
                break;
            }
        }

        // Advance logic for the right path argument
        for (; rhsPathParser; ++rhsPathParser)
        {
            if (PathPartKind rhsPartKind = ClassifyPathPart(rhsPathParser);
                rhsPartKind != PathPartKind::PK_Dot)
            {
                break;
            }
        }
        return 0;
    }
    constexpr int CompareEndState(PathParser* lhsPathParser, PathParser* rhsPathParser)
    {
        if (lhsPathParser->AtEnd() && !rhsPathParser->AtEnd())
        {
            return -1;
        }
        else if (!lhsPathParser->AtEnd() && rhsPathParser->AtEnd())
        {
            return 1;
        }
        return 0;
    }

    //path.hash
    /// Path is using FNV-1a algorithm 64 bit version.
    constexpr size_t HashSegment(AZStd::string_view pathSegment, bool hashExactPath)
    {
        size_t hash = 14695981039346656037ULL;
        constexpr size_t fnvPrime = 1099511628211ULL;
        auto ToLower = [](const char element) constexpr -> char
        {
            if (az_builtin_is_constant_evaluated())
            {
                // compile time implementation
                return element >= 'A' && element <= 'Z' ? (element - 'A') + 'a' : element;
            }
            else
            {
                // run time implementation
                return static_cast<char>(tolower(element));
            }
        };

        for (const char first : pathSegment)
        {
            hash ^= static_cast<size_t>(hashExactPath ? first : ToLower(first));
            hash *= fnvPrime;
        }
        return hash;
    }
    constexpr size_t HashPath(PathParser& pathParser)
    {
        size_t hash_value = 0;
        const bool hashExactPath = pathParser.m_preferred_separator == AZ::IO::PosixPathSeparator;
        while (pathParser)
        {
            switch (pathParser.m_parser_state)
            {
            case PS_InRootName:
            case PS_InFilenames:
                AZStd::hash_combine(hash_value, HashSegment(*pathParser, hashExactPath));
                break;
            case PS_InRootDir:
                // Only hash the PosixPathSeparator when a root directory is seen
                // This makes the hash consistent for root directories path of C:\ and C:/
                AZStd::hash_combine(hash_value, HashSegment("/", hashExactPath));
                break;
            default:
                // The BeforeBegin and AtEnd states contain no segments to hash
                break;
            }
            ++pathParser;
        }
        return hash_value;
    }

    constexpr int DetermineLexicalElementCount(PathParser pathParser)
    {
        int count = 0;
        for (; pathParser; ++pathParser)
        {
            auto pathElement = *pathParser;
            if (pathElement == "..")
            {
                --count;
            }
            else if (pathElement != "." && pathElement != "")
            {
                ++count;
            }
        }
        return count;
    }
}

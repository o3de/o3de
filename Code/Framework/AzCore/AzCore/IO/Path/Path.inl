/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/wildcard.h>

// extern instantiations of Path templates to prevent implicit instantiations
namespace AZ::IO
{
    // Class templates explicit declarations
    extern template class BasicPath<AZStd::string>;
    extern template class BasicPath<FixedMaxPathString>;
    extern template class PathIterator<PathView>;
    extern template class PathIterator<Path>;
    extern template class PathIterator<FixedMaxPath>;

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
    extern template bool operator==<PathView>(const PathIterator<PathView>& lhs,
        const PathIterator<PathView>& rhs);
    extern template bool operator==<Path>(const PathIterator<Path>& lhs,
        const PathIterator<Path>& rhs);
    extern template bool operator==<FixedMaxPath>(const PathIterator<FixedMaxPath>& lhs,
        const PathIterator<FixedMaxPath>& rhs);
    extern template bool operator!=<PathView>(const PathIterator<PathView>& lhs,
        const PathIterator<PathView>& rhs);
    extern template bool operator!=<Path>(const PathIterator<Path>& lhs,
        const PathIterator<Path>& rhs);
    extern template bool operator!=<FixedMaxPath>(const PathIterator<FixedMaxPath>& lhs,
        const PathIterator<FixedMaxPath>& rhs);
}

namespace AZ::IO::Internal
{
    constexpr bool IsSeparator(const char elem)
    {
        return elem == '/' || elem == '\\';
    }
    template <typename InputIt, typename EndIt, typename = AZStd::enable_if_t<AZStd::Internal::is_input_iterator_v<InputIt>>>
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

    //! Returns an iterator past the end of the consumed root name
    //! Windows root names can have include drive letter within them
    template <typename InputIt>
    constexpr auto ConsumeRootName(InputIt entryBeginIter, InputIt entryEndIter, const char preferredSeparator)
        -> AZStd::enable_if_t<AZStd::Internal::is_forward_iterator_v<InputIt>, InputIt>
    {
        if (preferredSeparator == '/')
        {
            // If the preferred separator is forward slash the parser is in posix path
            // parsing mode, which doesn't have a root name
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

            if (path.size() >= 3 && Internal::IsSeparator(path[1]) && !Internal::IsSeparator(path[2]))
            {
                // Find the next path separator  for network paths that have the form of \\server\share
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
    template <typename InputIt, typename EndIt, typename = AZStd::enable_if_t<AZStd::Internal::is_input_iterator_v<InputIt>>>
    static constexpr bool IsAbsolute(InputIt first, EndIt last, const char preferredSeparator)
    {
        size_t pathSize = AZStd::distance(first, last);

        // If the preferred separator is a forward slash
        // than an absolute path is simply one that starts with a forward slash
        if (preferredSeparator == '/')
        {
            return pathSize > 0 && IsSeparator(*first);
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

    // Compares path segments using either Posix  or Windows path rules based on the path separator in use
    // Posix paths perform a case-sensitive comparison, while Windows paths perform a case-insensitive comparison
    static int ComparePathSegment(AZStd::string_view left, AZStd::string_view right, char pathSeparator)
    {
        const size_t maxCharsToCompare = (AZStd::min)(left.size(), right.size());

        int charCompareResult = pathSeparator == PosixPathSeparator
            ? strncmp(left.data(), right.data(), maxCharsToCompare)
            : azstrnicmp(left.data(), right.data(), maxCharsToCompare);
        return charCompareResult == 0
            ? aznumeric_cast<ptrdiff_t>(left.size()) - aznumeric_cast<ptrdiff_t>(right.size())
            : charCompareResult;
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
        PS_BeforeBegin = PathIterator<PathView>::BeforeBegin,
        PS_InRootName = PathIterator<PathView>::InRootName,
        PS_InRootDir = PathIterator<PathView>::InRootDir,
        PS_InFilenames = PathIterator<PathView>::InFilenames,
        PS_AtEnd = PathIterator<PathView>::AtEnd
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

        constexpr PosPtr Peek() const noexcept
        {
            auto tokenEnd = getNextTokenStartPos();
            auto End = m_path_view.end();
            return tokenEnd == End ? nullptr : tokenEnd;
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
                [[fallthrough]];
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
        int res = Internal::ComparePathSegment(GetRootName(lhsPathParser), GetRootName(rhsPathParser), lhsPathParser->m_preferred_separator);
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

        while (lhsPathParser && rhsPathParser)
        {
            if (int res = Internal::ComparePathSegment(*lhsPathParser, *rhsPathParser, lhsPathParser.m_preferred_separator);
                res != 0)
            {
                return res;
            }
            ++lhsPathParser;
            ++rhsPathParser;
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

        // Return PathPartKind of Filename if the parser state doesn't match
        // the states of InRootDir or InRootName and the filename
        // isn't made up of the special directory values of "." and ".."
        return PathPartKind::PK_Filename;
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
    constexpr auto PathView::Native() const noexcept -> AZStd::string_view
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

    constexpr auto PathView::root_path_raw_view() const -> AZStd::string_view
    {
        auto pathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        if (pathParser.m_parser_state == parser::PS_InRootName)
        {
            auto NextCh = pathParser.Peek();
            if (NextCh && *NextCh == m_preferred_separator)
            {
                ++pathParser;
                return AZStd::string_view{ m_path.begin(), pathParser.m_path_raw_entry.end() };
            }
            return pathParser.m_path_raw_entry;
        }
        return (pathParser.m_parser_state == parser::PS_InRootDir) ? *pathParser : AZStd::string_view{};
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
        return compare_string_view(other.m_path);
    }
    constexpr int PathView::Compare(AZStd::string_view pathView) const noexcept
    {
        return compare_string_view(pathView);
    }
    constexpr int PathView::Compare(const value_type* path) const noexcept
    {
        return compare_string_view(path);
    }

    constexpr AZStd::fixed_string<MaxPathLength> PathView::FixedMaxPathString() const noexcept
    {
        return AZStd::fixed_string<MaxPathLength>(m_path.begin(), m_path.end());
    }

    // decomposition
    constexpr auto PathView::RootName() const -> PathView
    {
        return PathView(root_name_view());
    }

    constexpr auto PathView::RootDirectory() const -> PathView
    {
        return PathView(root_directory_view());
    }

    constexpr auto PathView::RootPath() const -> PathView
    {
        return PathView(root_path_raw_view());
    }

    constexpr auto PathView::RelativePath() const -> PathView
    {
        return PathView(relative_path_view());
    }

    constexpr auto PathView::ParentPath() const -> PathView
    {
        return PathView(parent_path_view());
    }

    constexpr auto PathView::Filename() const -> PathView
    {
        return PathView(filename_view());
    }

    constexpr auto PathView::Stem() const -> PathView
    {
        return PathView(stem_view());
    }

    constexpr auto PathView::Extension() const -> PathView
    {
        return PathView(extension_view());
    }

    // query
    [[nodiscard]] constexpr bool PathView::empty() const noexcept
    {
        return m_path.empty();
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
        return !root_path_raw_view().empty();
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

    constexpr int PathView::compare_string_view(AZStd::string_view pathView) const
    {
        auto lhsPathParser = parser::PathParser::CreateBegin(m_path, m_preferred_separator);
        auto rhsPathParser = parser::PathParser::CreateBegin(pathView, m_preferred_separator);

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
        PathIterator<PathView> it;
        it.m_path_ref = this;
        it.m_state = static_cast<typename const_iterator::ParserState>(pathParser.m_parser_state);
        it.m_path_entry_view = pathParser.m_path_raw_entry;
        it.m_stashed_elem = *pathParser;
        return it;
    }

    constexpr auto PathView::end() const -> const_iterator
    {
        PathIterator<PathView> it;
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

    template <typename PathResultType>
    constexpr void PathView::MakeRelativeTo(PathResultType& pathResult, const AZ::IO::PathView& path, const AZ::IO::PathView& base)
    {
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
                if (*pathParser != *pathParserBase)
                {
                    pathResult.m_path = AZStd::string_view{};
                    return;
                }
            }
            else if (CheckIterMismatchAtBase())
            {
                pathResult.m_path = AZStd::string_view{};
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
                pathResult.m_path = AZStd::string_view{};
                return;
            }
        }

        // Find the first mismatching element
        auto pathParser = parser::PathParser::CreateBegin(path.m_path, path.m_preferred_separator);
        auto pathParserBase = parser::PathParser::CreateBegin(base.m_path, base.m_preferred_separator);
        while (pathParser && pathParserBase && pathParser.m_parser_state == pathParserBase.m_parser_state && *pathParser == *pathParserBase)
        {
            ++pathParser;
            ++pathParserBase;
        }

        // If there is no mismatch, return ".".
        if (!pathParser && !pathParserBase)
        {
            pathResult.m_path = AZStd::string_view{ "." };
            return;
        }

        // Otherwise, determine the number of elements, 'n', which are not dot or
        // dot-dot minus the number of dot-dot elements.
        int elemCount = parser::DetermineLexicalElementCount(pathParserBase);
        if (elemCount < 0)
        {
            pathResult.m_path = AZStd::string_view{};
            return;
        }

        // if elemCount == 0 and (pathParser == end() || pathParser->empty()), returns path("."); otherwise
        if (elemCount == 0 && (pathParser.AtEnd() || *pathParser == ""))
        {
            pathResult.m_path = AZStd::string_view{ "." };
            return;
        }

        // return a path constructed with 'n' dot-dot elements, followed by the
        // elements of '*this' after the mismatch.
        pathResult = PathResultType(path.m_preferred_separator);
        while (elemCount--)
        {
            pathResult /= "..";
        }
        for (; pathParser; ++pathParser)
        {
            pathResult /= *pathParser;
        }
    }

    template <typename PathResultType>
    constexpr void PathView::LexicallyNormalInplace(PathResultType& pathResult, const AZ::IO::PathView& path)
    {
        if (path.m_path.empty())
        {
            pathResult = path;
            return;
        }

        using PartKindPair = AZStd::pair<AZStd::string_view, parser::PathPartKind>;
        // Max number of path parts supported when normalizing a path
        constexpr size_t MaxPathParts = 64;
        AZStd::array<PartKindPair, MaxPathParts> pathParts{};
        size_t currentPartSize = 0;

        // Track the total size of the parts as we collect them. This allows the
        // resulting path to reserve the correct amount of memory.
        size_t newPathSize = 0;
        auto AddPart = [&newPathSize, &pathParts, &currentPartSize](parser::PathPartKind pathKind, AZStd::string_view parserPathPart) constexpr
        {
            newPathSize += parserPathPart.size();
            pathParts[currentPartSize++] = { parserPathPart, pathKind };
        };
        auto LastPartKind = [&pathParts, &currentPartSize]() constexpr
        {
            if (currentPartSize == 0)
            {
                return parser::PathPartKind::PK_None;
            }
            return pathParts[currentPartSize - 1].second;
        };

        // Build a stack containing the remaining elements of the path, popping off
        // elements which occur before a '..' entry.
        for (auto pathParser = parser::PathParser::CreateBegin(path.m_path, path.m_preferred_separator); pathParser; ++pathParser)
        {
            parser::PathPartKind Kind = parser::ClassifyPathPart(pathParser);
            switch (Kind)
            {
            case parser::PathPartKind::PK_RootName:
            case parser::PathPartKind::PK_Filename:
                [[fallthrough]];
            case parser::PathPartKind::PK_RootSep:
            {
                // Add all non-dot and non-dot-dot elements to the stack of elements.
                AddPart(Kind, *pathParser);
                break;
            }
            case parser::PathPartKind::PK_DotDot:
            {
                // Only push a ".." element if there are no elements preceding the "..",
                // or if the preceding element is itself "..".
                auto lastPartKind = LastPartKind();
                if (lastPartKind == parser::PathPartKind::PK_Filename)
                {
                    newPathSize -= pathParts[--currentPartSize].first.size();
                }
                else if (lastPartKind != parser::PathPartKind::PK_RootSep)
                {
                    AddPart(parser::PathPartKind::PK_DotDot, "..");
                }
                break;
            }
            case parser::PathPartKind::PK_Dot:
                break;
            case parser::PathPartKind::PK_None:
                AZ_Assert(false, "Path Parser is in an invalid state");
            }
        }
        //! If the path is empty, add a dot.
        if (currentPartSize == 0)
        {
            pathResult.m_path = AZStd::string_view{ "." };
            return;
        }

        pathResult = PathResultType(path.m_preferred_separator);
        pathResult.m_path.reserve(currentPartSize + newPathSize);
        for (size_t partIndex = 0; partIndex < currentPartSize; ++partIndex)
        {
            auto& pathPart = pathParts[partIndex];
            pathResult /= pathPart.first;
        }

    }
}

namespace AZ::IO
{
    // Basic Path implementation

    template <typename StringType>
    constexpr BasicPath<StringType>::BasicPath(const PathView& other)
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
    constexpr auto BasicPath<StringType>::operator=(const PathView& other) -> BasicPath&
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
        // "C:"foo and the other path object had a path "F:bar".
        // As the root names are different the other path replaces current path in it's entirety
        auto postRootNameIter = Internal::ConsumeRootName(m_path.begin(), m_path.end(), m_preferred_separator);
        auto otherPostRootNameIter = Internal::ConsumeRootName(first, last, m_preferred_separator);
        AZStd::string_view rootNameView{ m_path.begin(), postRootNameIter };
        if (first != otherPostRootNameIter && !AZStd::equal(rootNameView.begin(), rootNameView.end(), first, otherPostRootNameIter))
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
                // has_root_directory || has_filename = false
                // If the root name is of the form
                // # C: - then it isn't absolute unless it has a root directory C:\
                // # \\?\ = is a UNC path that can't exist without a root directory
                // # \\server - Is absolute, but has no root directory
                // Therefore if the rootName is larger than three characters
                // then append the path separator
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
        if (m_preferred_separator != '/')
        {
            AZStd::replace(m_path.begin(), m_path.end(), '/', m_preferred_separator);
        }
        else
        {
            AZStd::replace(m_path.begin(), m_path.end(), '\\', m_preferred_separator);
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
    constexpr auto BasicPath<StringType>::Native() const noexcept -> const string_type&
    {
        return m_path;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::Native() noexcept -> string_type&
    {
        return m_path;
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

    template <typename StringType>
    constexpr BasicPath<StringType>::operator string_type&() noexcept
    {
        return m_path;
    }

    // compare
    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(const PathView& other) const noexcept
    {
        return static_cast<PathView>(*this).compare_string_view(other.m_path);
    }

    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(const string_type& pathString) const
    {
        return static_cast<PathView>(*this).compare_string_view(pathString);
    }

    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(AZStd::string_view pathView) const noexcept
    {
        return static_cast<PathView>(*this).compare_string_view(pathView);
    }

    template <typename StringType>
    constexpr int BasicPath<StringType>::Compare(const value_type* pathString) const noexcept
    {
        return static_cast<PathView>(*this).compare_string_view(pathString);
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
        BasicPath pathResult;
        static_cast<PathView>(*this).LexicallyNormalInplace(pathResult, *this);
        return pathResult;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::LexicallyRelative(const PathView& base) const -> BasicPath
    {
        BasicPath pathResult;
        static_cast<PathView>(*this).MakeRelativeTo(pathResult, *this, base);
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
        PathIterator<BasicPath> it;
        it.m_path_ref = this;
        it.m_state = static_cast<typename const_iterator::ParserState>(pathParser.m_parser_state);
        it.m_path_entry_view = pathParser.m_path_raw_entry;
        it.m_stashed_elem = *pathParser;
        return it;
    }

    template <typename StringType>
    constexpr auto BasicPath<StringType>::end() const -> const_iterator
    {
        PathIterator<BasicPath> it;
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
        auto relativePath = LexicallyRelative(base);
        return !relativePath.empty() && !relativePath.Native().starts_with("..");
    }

    constexpr FixedMaxPath PathView::LexicallyNormal() const
    {
        FixedMaxPath pathResult;
        LexicallyNormalInplace(pathResult, *this);
        return pathResult;
    }

    constexpr FixedMaxPath PathView::LexicallyRelative(const PathView& base) const
    {
        FixedMaxPath pathResult;
        MakeRelativeTo(pathResult, *this, base);
        return pathResult;
    }

    constexpr FixedMaxPath PathView::LexicallyProximate(const PathView& base) const
    {
        FixedMaxPath result = LexicallyRelative(base);
        if (result.empty())
        {
            return FixedMaxPath(*this);
        }
        return result;
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
    constexpr auto PathIterator<PathType>::operator->() const -> pointer
    {
        return &m_stashed_elem;
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
        constexpr size_t operator()(const AZ::IO::PathView& pathToHash) noexcept
        {
            auto pathParser = AZ::IO::parser::PathParser::CreateBegin(pathToHash.Native(), pathToHash.m_preferred_separator);
            size_t hash_value = 0;
            while (pathParser)
            {
                AZStd::hash_combine(hash_value, AZStd::hash<AZStd::string_view>{}(*pathParser));
                ++pathParser;
            }
            return hash_value;
        }
    };
    template <typename StringType>
    struct hash<AZ::IO::BasicPath<StringType>>
    {
        constexpr size_t operator()(const AZ::IO::BasicPath<StringType>& pathToHash) noexcept
        {
            return AZStd::hash<AZ::IO::PathView>{}(pathToHash);
        }
    };

    // Instantiate the hash function for Path and FixedMaxPath
    template struct hash<AZ::IO::Path>;
    template struct hash<AZ::IO::FixedMaxPath>;
}

// Explicit instantations of our support Path classes
namespace AZ::IO
{
    // PathView hash
    constexpr size_t hash_value(const PathView& pathToHash) noexcept
    {
        return AZStd::hash<PathView>{}(pathToHash);
    }
}

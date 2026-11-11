/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class ConsoleFunctorBase;
    class IConsole;

    //! Shared name/help matcher for AZ and legacy console discovery through `?text`.
    //!
    //! The query is split into words; every word must match (case-insensitive), scored best first:
    //! the whole query equals the name, a word starts the name, a word is inside the name, a word with
    //! a `*` or `?` wildcard matches the whole name (CryEngine-style globs, `r_*`), a word is inside the
    //! help text, and, only when subsequences are allowed, the word's letters appear in order in the name.
    //! An empty query matches everything with score 0.
    class AZCORE_API ConsoleSearchTerms
    {
    public:
        static constexpr int NoMatch = -1;

        explicit ConsoleSearchTerms(AZStd::string_view query);

        //! Returns NoMatch when any word misses, else a score where higher is better.
        int Rank(AZStd::string_view name, AZStd::string_view help, bool allowSubsequence = true) const;

        bool IsEmpty() const;

    private:
        AZStd::string m_query;
        AZStd::vector<AZStd::string> m_words;
    };

    enum class ConsoleSearchKind
    {
        Any,
        Command,
        Cvar
    };

    struct ConsoleSearchMatch
    {
        ConsoleFunctorBase* m_functor = nullptr;
        int m_score = 0;
    };

    //! A functor with a value type is a cvar; a free-function functor is a command.
    AZCORE_API bool IsConsoleCvar(const ConsoleFunctorBase& functor);

    //! The first sentence (or line) of a functor's help text, end punctuation included.
    AZCORE_API AZStd::string_view GetConsoleHelpLine(AZStd::string_view help);

    //! Ranks every visible (not IsInvisible) registered command and cvar against the query, best first,
    //! then by name. Name subsequences are a fallback used only when no functor matches by name or help.
    AZCORE_API AZStd::vector<ConsoleSearchMatch> SearchConsoleFunctors(
        IConsole& console, AZStd::string_view query, ConsoleSearchKind kind = ConsoleSearchKind::Any);

    //! The console's `?text` output, one line per match after a summary line:
    //! `  name = value - help line` for a cvar, `  name (command) - help line` for a command.
    AZCORE_API AZStd::vector<AZStd::string> FormatConsoleSearch(IConsole& console, AZStd::string_view query);

    //! The `?text` summary line: `?text: N matches`, or `?text: no commands or cvars match`.
    AZCORE_API AZStd::string FormatConsoleSearchHeader(AZStd::string_view query, size_t matchCount);

    //! One `?text` line: `  name = value - help line`, or `  name (command) - help line` when value is null.
    //! Shared with consoles that rank their own entries with ConsoleSearchTerms (the legacy CryEngine console).
    AZCORE_API AZStd::string FormatConsoleSearchLine(AZStd::string_view name, const char* value, AZStd::string_view help);

    //! The console's `cvar ?` output: the cvar's value and its whole help text, one line each.
    AZCORE_API AZStd::vector<AZStd::string> FormatConsoleHelp(const ConsoleFunctorBase& functor);
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/ConsoleSearch.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    namespace ConsoleSearchDetail
    {
        AZStd::string Lower(AZStd::string_view text)
        {
            AZStd::string lower(text);
            for (char& ch : lower)
            {
                if (ch >= 'A' && ch <= 'Z')
                {
                    ch = static_cast<char>(ch + ('a' - 'A'));
                }
            }
            return lower;
        }

        bool IsSubsequence(AZStd::string_view word, AZStd::string_view name)
        {
            size_t cursor = 0;
            for (const char ch : name)
            {
                if (cursor < word.size() && ch == word[cursor])
                {
                    ++cursor;
                }
            }
            return cursor == word.size();
        }

        int RankWord(const AZStd::string& word, const AZStd::string& name, const AZStd::string& help, bool allowSubsequence)
        {
            if (word.find_first_of("*?") != AZStd::string::npos)
            {
                // A wildcard word is a glob over the whole name, so `r_*` anchors at the start.
                return AZStd::wildcard_match(word, name) ? 75 : ConsoleSearchTerms::NoMatch;
            }
            if (const size_t position = name.find(word); position != AZStd::string::npos)
            {
                return position == 0 ? 100 : 75;
            }
            if (help.find(word) != AZStd::string::npos)
            {
                return 50;
            }
            return allowSubsequence && IsSubsequence(word, name) ? 10 : ConsoleSearchTerms::NoMatch;
        }

        void CollectMatches(IConsole& console, const ConsoleSearchTerms& terms, ConsoleSearchKind kind, bool allowSubsequence,
            AZStd::vector<ConsoleSearchMatch>& matches)
        {
            console.VisitRegisteredFunctors([&](ConsoleFunctorBase* functor)
            {
                if ((functor->GetFlags() & ConsoleFunctorFlags::IsInvisible) != ConsoleFunctorFlags::Null ||
                    (kind == ConsoleSearchKind::Command && IsConsoleCvar(*functor)) ||
                    (kind == ConsoleSearchKind::Cvar && !IsConsoleCvar(*functor)))
                {
                    return;
                }
                const int score = terms.Rank(functor->GetName(), functor->GetDesc() ? functor->GetDesc() : "", allowSubsequence);
                if (score != ConsoleSearchTerms::NoMatch)
                {
                    matches.push_back({ functor, score });
                }
            });
        }
    } // namespace ConsoleSearchDetail

    ConsoleSearchTerms::ConsoleSearchTerms(AZStd::string_view query)
        : m_query(ConsoleSearchDetail::Lower(query))
    {
        StringFunc::Tokenize(m_query, m_words, " \t\r\n");
        StringFunc::TrimWhiteSpace(m_query, true, true);
    }

    bool ConsoleSearchTerms::IsEmpty() const
    {
        return m_words.empty();
    }

    int ConsoleSearchTerms::Rank(AZStd::string_view nameText, AZStd::string_view helpText, bool allowSubsequence) const
    {
        const AZStd::string name = ConsoleSearchDetail::Lower(nameText);
        const AZStd::string help = ConsoleSearchDetail::Lower(helpText);
        int score = !m_words.empty() && name == m_query ? 1000 : 0;
        for (const AZStd::string& word : m_words)
        {
            const int wordScore = ConsoleSearchDetail::RankWord(word, name, help, allowSubsequence);
            if (wordScore == NoMatch)
            {
                return NoMatch;
            }
            score += wordScore;
        }
        return score;
    }

    bool IsConsoleCvar(const ConsoleFunctorBase& functor)
    {
        return !functor.GetTypeId().IsNull();
    }

    AZStd::string_view GetConsoleHelpLine(AZStd::string_view help)
    {
        const size_t end = help.find_first_of(".!?\n");
        if (end == AZStd::string_view::npos)
        {
            return help;
        }
        return help.substr(0, help[end] == '\n' ? end : end + 1);
    }

    AZStd::vector<ConsoleSearchMatch> SearchConsoleFunctors(IConsole& console, AZStd::string_view query, ConsoleSearchKind kind)
    {
        const ConsoleSearchTerms terms(query);
        AZStd::vector<ConsoleSearchMatch> matches;
        ConsoleSearchDetail::CollectMatches(console, terms, kind, false, matches);
        if (matches.empty() && !terms.IsEmpty())
        {
            ConsoleSearchDetail::CollectMatches(console, terms, kind, true, matches);
        }
        AZStd::sort(matches.begin(), matches.end(), [](const ConsoleSearchMatch& left, const ConsoleSearchMatch& right)
        {
            if (left.m_score != right.m_score)
            {
                return left.m_score > right.m_score;
            }
            return azstricmp(left.m_functor->GetName(), right.m_functor->GetName()) < 0;
        });
        return matches;
    }

    AZStd::vector<AZStd::string> FormatConsoleSearch(IConsole& console, AZStd::string_view query)
    {
        const AZStd::vector<ConsoleSearchMatch> matches = SearchConsoleFunctors(console, query);
        AZStd::vector<AZStd::string> lines;
        lines.reserve(matches.size() + 1);
        lines.push_back(FormatConsoleSearchHeader(query, matches.size()));
        for (const ConsoleSearchMatch& match : matches)
        {
            const ConsoleFunctorBase& functor = *match.m_functor;
            CVarFixedString value;
            const bool cvar = IsConsoleCvar(functor);
            if (cvar && functor.GetValue(value) != GetValueResult::Success)
            {
                value.clear();
            }
            lines.push_back(FormatConsoleSearchLine(functor.GetName(), cvar ? value.c_str() : nullptr,
                functor.GetDesc() ? functor.GetDesc() : ""));
        }
        return lines;
    }

    AZStd::string FormatConsoleSearchHeader(AZStd::string_view query, size_t matchCount)
    {
        AZStd::string trimmedQuery(query);
        AZ::StringFunc::TrimWhiteSpace(trimmedQuery, true, true);
        if (matchCount == 0)
        {
            return AZStd::string::format("?%s: no commands or cvars match", trimmedQuery.c_str());
        }
        return AZStd::string::format("?%s: %zu match%s", trimmedQuery.c_str(), matchCount, matchCount == 1 ? "" : "es");
    }

    AZStd::string FormatConsoleSearchLine(AZStd::string_view name, const char* value, AZStd::string_view help)
    {
        AZStd::string line = AZStd::string::format("  %.*s", AZ_STRING_ARG(name));
        if (value != nullptr)
        {
            line += AZStd::string::format(" = %s", value);
        }
        else
        {
            line += " (command)";
        }
        const AZStd::string_view helpLine = GetConsoleHelpLine(help);
        if (!helpLine.empty())
        {
            line += " - ";
            line.append(helpLine.data(), helpLine.size());
        }
        return line;
    }

    AZStd::vector<AZStd::string> FormatConsoleHelp(const ConsoleFunctorBase& functor)
    {
        AZStd::vector<AZStd::string> lines;
        CVarFixedString value;
        if (IsConsoleCvar(functor) && functor.GetValue(value) == GetValueResult::Success)
        {
            lines.push_back(AZStd::string::format("%s = %s", functor.GetName(), value.c_str()));
        }
        else
        {
            lines.push_back(AZStd::string::format("%s (%s)", functor.GetName(), IsConsoleCvar(functor) ? "cvar" : "command"));
        }
        const AZStd::string_view help(functor.GetDesc() ? functor.GetDesc() : "");
        if (help.empty())
        {
            lines.push_back("  No help available.");
            return lines;
        }
        AZ::StringFunc::TokenizeVisitor(help, [&lines](AZStd::string_view helpLine)
        {
            lines.push_back(AZStd::string::format("  %.*s", AZ_STRING_ARG(helpLine)));
        }, '\n');
        return lines;
    }
} // namespace AZ

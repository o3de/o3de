/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <cctype>
#include <AzCore/std/string/string.h>

namespace AZStd
{
    /**
     * Wildcard '*' and '?' matching support.
     *
     * Example:
     * if (wildcard_match("bl?h.*", "blah.jpg"))
     * {
     *      we have a match!
     * } else
     * {
     *      no match
     * }
     * TODO: wchar_t version
     */
    // BinaryOp is a function object that should contain the signature of bool(ConvertibleFromChar1 filterChar, ConvertibleFromChar2 findChar)
    // i.e bool(char, char) is valid function object for this method
    template<typename BinaryOp>
    constexpr bool wildcard_match(AZStd::string_view filter, AZStd::string_view name, BinaryOp binaryOp)
    {
        AZStd::string_view findCharPos = name;
        AZStd::string_view findFilterPos = filter;

        // If the filter is empty then, then a successful match is based on the name being empty as well
        // i.e an empty filter can only match an empty name
        if (findFilterPos.empty())
        {
            return findCharPos.empty();
        }

        if (findFilterPos.front() == '*')
        {
            // The first filter character is '*', so find the next non-wild card character
            size_t nonMatchZeroOrMoreOffset = findFilterPos.find_first_not_of('*');
            // Skip over consecutive '*' wild card characters and put the matching logic in "backtrack from the end" approach
            findFilterPos.remove_prefix(nonMatchZeroOrMoreOffset != AZStd::string_view::npos ? nonMatchZeroOrMoreOffset : findFilterPos.size());

            // If the find filter is empty after removing the '*' wild cards, then the name is considered a match
            if (findFilterPos.empty())
            {
                return true;
            }

            // Our strategy is that we will start from the end and continue backtracking until either we find a match or we reach to the beginning of the name,
            // for eg if the name is                  "123.systemfile.pak.3" and the filter is  "*.pak.*" , the intial positions at which the pointers point to are as below
            //                                         ^                  ^                       ^
            //                                   findStartCharPos       findEndCharPos      findFilterPos
            // And than basically we will check whether the find filter character matches the EndCharPosition character and if so than see whether there is a match.
            // This will result in checking whether ".3" matches ".pak.*", ".pak.3" matches ".pak.*" and so no untill either there is a match or we reach to the beginning of the name. 

            for (auto findEndCharPos = findCharPos.rbegin(); findEndCharPos != findCharPos.rend(); ++findEndCharPos)
            {
                // If the next find character succeeds when passed to the binary operation using the supplied filter character
                // or the filter character is the "match any single character" wild card of '?', then the wildcard_match
                // will proceed to attempt to match from the find character forward
                if (findFilterPos.front() == '?' || binaryOp(findFilterPos.front(), *findEndCharPos))
                {
                    // Make sub string view from the found position to the end of the string
                    AZStd::string_view startFindSubView{ findCharPos.data() + AZStd::distance(findCharPos.begin(), findEndCharPos.base()) - 1, findCharPos.data() + findCharPos.size() };
                    // the character matched so move both the find view and filter view to the next character
                    if (wildcard_match(findFilterPos.substr(1), startFindSubView.substr(1), binaryOp))
                    {
                        return true;
                    }
                }
            }

            return false;
        }
        else
        {
            // An empty name cannot match a non-empty filter non '*' character
            // Check if an ordinary character matches
            if (findCharPos.empty() || findFilterPos.front() != '?' && !binaryOp(findCharPos.front(), findFilterPos.front()))
            {
                return false;
            }

            // Move ahead a character in the find and filter views
            findCharPos.remove_prefix(1);
            findFilterPos.remove_prefix(1);
        }

        return wildcard_match(findFilterPos, findCharPos, binaryOp);
    }

    // Case insensitive wild card match function against wildcards of '*' and '?'
    inline constexpr bool wildcard_match(AZStd::string_view filter, AZStd::string_view name)
    {
        return wildcard_match(filter, name, [](char filterChar, char findChar) {
            return tolower(filterChar) == tolower(findChar);
        });
    }

    // Case sensitive wild card match function against wildcards of '*' and '?'
    inline constexpr bool wildcard_match_case(AZStd::string_view filter, AZStd::string_view name)
    {
        return wildcard_match(filter, name, [](char filterChar, char findChar) {
            return filterChar == findChar;
        });
    }
} // namespace AZStd



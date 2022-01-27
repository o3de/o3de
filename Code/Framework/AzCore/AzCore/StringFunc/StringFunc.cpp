/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ctype.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/AzCore_Traits_Platform.h>

namespace AZ::StringFunc::Internal
{
    template<typename StringType>
    StringType& LChop(StringType& in, size_t num = 1)
    {
        size_t len = in.length();
        if (num == StringType::npos)
        {
            num = 0;
        }
        if (num > len)
        {
            in = StringType{};
            return in;
        }
        if constexpr (AZStd::is_same_v<StringType, AZStd::string_view>)
        {
            in = in.substr(0, num);
        }
        else
        {
            in.erase(0, num);
        }
        return in;
    }

    template<typename StringType>
    StringType& RChop(StringType& in, size_t num = 1)
    {
        size_t len = in.length();
        if (num == StringType::npos)
        {
            num = 0;
        }
        if (num > len)
        {
            in = StringType{};
            return in;
        }
        if constexpr (AZStd::is_same_v<StringType, AZStd::string_view>)
        {
            in = in.substr(len - num);
        }
        else
        {
            in.erase(len - num, len);
        }
        return in;
    }


    template<typename StringType>
    StringType& LKeep(StringType& inout, size_t pos, bool bKeepPosCharacter = false)
    {
        size_t len = inout.length();
        if (pos == StringType::npos)
        {
            pos = 0;
        }
        if (pos > len)
        {
            return inout;
        }
        if (bKeepPosCharacter)
        {
            return inout.erase(pos + 1, len);
        }
        else
        {
            return inout.erase(pos, len);
        }
    }

    template<typename StringType>
    StringType& RKeep(StringType& inout, size_t pos, bool bKeepPosCharacter = false)
    {
        size_t len = inout.length();
        if (pos == StringType::npos)
        {
            pos = 0;
        }
        if (pos > len)
        {
            inout.clear();
            return inout;
        }
        if (bKeepPosCharacter)
        {
            return inout.erase(0, pos);
        }
        else
        {
            return inout.erase(0, pos + 1);
        }
    }

    template<typename StringType>
    bool Replace(StringType& inout, const char replaceA, const char withB, bool bCaseSensitive = false, bool bReplaceFirst = false, bool bReplaceLast = false)
    {
        bool bSomethingWasReplaced = false;
        size_t pos = 0;

        if (!bReplaceFirst && !bReplaceLast)
        {
            //replace all
            if (bCaseSensitive)
            {
                bSomethingWasReplaced = false;
                while ((pos = inout.find(replaceA, pos)) != StringType::npos)
                {
                    bSomethingWasReplaced = true;
                    inout.replace(pos, 1, 1, withB);
                    pos++;
                }
            }
            else
            {
                StringType lowercaseIn(inout);
                AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                char lowercaseReplaceA = (char)tolower(replaceA);

                while ((pos = lowercaseIn.find(lowercaseReplaceA, pos)) != StringType::npos)
                {
                    bSomethingWasReplaced = true;
                    inout.replace(pos, 1, 1, withB);
                    pos++;
                }
            }
        }
        else
        {
            if (bCaseSensitive)
            {
                if (bReplaceFirst)
                {
                    //replace first
                    if ((pos = inout.find_first_of(replaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, 1, 1, withB);
                    }
                }

                if (bReplaceLast)
                {
                    //replace last
                    if ((pos = inout.find_last_of(replaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, 1, 1, withB);
                    }
                }
            }
            else
            {
                StringType lowercaseIn(inout);
                AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                char lowercaseReplaceA = (char)tolower(replaceA);

                if (bReplaceFirst)
                {
                    //replace first
                    if ((pos = lowercaseIn.find_first_of(lowercaseReplaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, 1, 1, withB);
                        if (bReplaceLast)
                        {
                            lowercaseIn.replace(pos, 1, 1, withB);
                        }
                    }
                }

                if (bReplaceLast)
                {
                    //replace last
                    if ((pos = lowercaseIn.find_last_of(lowercaseReplaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, 1, 1, withB);
                    }
                }
            }
        }

        return bSomethingWasReplaced;
    }

    template<typename StringType>
    bool Replace(StringType& inout, const char* replaceA, const char* withB, bool bCaseSensitive = false, bool bReplaceFirst = false, bool bReplaceLast = false)
    {
        if (!replaceA) //withB can be nullptr
        {
            return false;
        }

        size_t lenA = strlen(replaceA);
        if (!lenA)
        {
            return false;
        }

        const char* emptystring = "";
        if (!withB)
        {
            withB = emptystring;
        }

        size_t lenB = strlen(withB);

        bool bSomethingWasReplaced = false;

        size_t pos = 0;

        if (!bReplaceFirst && !bReplaceLast)
        {
            //replace all
            if (bCaseSensitive)
            {
                while ((pos = inout.find(replaceA, pos)) != StringType::npos)
                {
                    bSomethingWasReplaced = true;
                    inout.replace(pos, lenA, withB, lenB);
                    pos += lenB;
                }
            }
            else
            {
                StringType lowercaseIn(inout);
                AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                StringType lowercaseReplaceA(replaceA);
                AZStd::to_lower(lowercaseReplaceA.begin(), lowercaseReplaceA.end());

                while ((pos = lowercaseIn.find(lowercaseReplaceA, pos)) != StringType::npos)
                {
                    bSomethingWasReplaced = true;
                    lowercaseIn.replace(pos, lenA, withB, lenB);
                    inout.replace(pos, lenA, withB, lenB);
                    pos += lenB;
                }
            }
        }
        else
        {
            if (bCaseSensitive)
            {
                if (bReplaceFirst)
                {
                    //replace first
                    if ((pos = inout.find(replaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, lenA, withB, lenB);
                    }
                }

                if (bReplaceLast)
                {
                    //replace last
                    if ((pos = inout.rfind(replaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, lenA, withB, lenB);
                    }
                }
            }
            else
            {
                StringType lowercaseIn(inout);
                AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                StringType lowercaseReplaceA(replaceA);
                AZStd::to_lower(lowercaseReplaceA.begin(), lowercaseReplaceA.end());

                if (bReplaceFirst)
                {
                    //replace first
                    if ((pos = lowercaseIn.find(lowercaseReplaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, lenA, withB, lenB);
                        if (bReplaceLast)
                        {
                            lowercaseIn.replace(pos, lenA, withB, lenB);
                        }
                    }
                }

                if (bReplaceLast)
                {
                    //replace last
                    if ((pos = lowercaseIn.rfind(lowercaseReplaceA)) != StringType::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, lenA, withB, lenB);
                    }
                }
            }
        }

        return bSomethingWasReplaced;
    }

    template<typename StringType>
    bool Strip(StringType& inout, AZStd::string_view stripCharacters, bool bCaseSensitive = false, bool bStripBeginning = false, bool bStripEnding = false)
    {
        bool bSomethingWasStripped = false;
        size_t pos = 0;

        AZStd::string_view stripCharactersSet = stripCharacters;
        // combinedStripCharacters is for storage of the set of case-insensitive strip characters
        // the stripCharacterSet string_view references the combinedStripCharacters only
        // when case-sensitivity is off.
        StringType combinedStripCharacters;
        if (!bCaseSensitive)
        {
            for (const char stripCharacter : stripCharacters)
            {
                const char lower = static_cast<char>(tolower(stripCharacter));
                const char upper = static_cast<char>(toupper(stripCharacter));
                if (lower != upper)
                {
                    combinedStripCharacters.push_back(lower);
                    combinedStripCharacters.push_back(upper);
                }
                else
                {
                    combinedStripCharacters.push_back(stripCharacter);
                }
            }

            stripCharactersSet = combinedStripCharacters;
        }

        if (!bStripBeginning && !bStripEnding)
        {
            //strip all
            while ((pos = inout.find_first_of(stripCharactersSet, pos)) != StringType::npos)
            {
                bSomethingWasStripped = true;
                inout.erase(pos, 1);
            }
        }
        else
        {
            if (bStripBeginning)
            {
                //strip beginning
                if ((pos = inout.find_first_not_of(stripCharactersSet)) != StringType::npos)
                {
                    if (pos != 0)
                    {
                        bSomethingWasStripped = true;
                        StringFunc::Internal::RKeep(inout, pos, true);
                    }
                }
                else
                {
                    bSomethingWasStripped = true;
                    StringFunc::Internal::LChop(inout, 1);
                }
            }

            if (bStripEnding)
            {
                //strip ending
                if ((pos = inout.find_last_not_of(stripCharactersSet)) != StringType::npos)
                {
                    if (pos != inout.length())
                    {
                        bSomethingWasStripped = true;
                        StringFunc::Internal::LKeep(inout, pos, true);
                    }
                }
                else
                {
                    bSomethingWasStripped = true;
                    StringFunc::Internal::RChop(inout, 1);
                }
            }
        }
        return bSomethingWasStripped;
    }

    template<typename StringType>
    bool Strip(StringType& inout, const char stripCharacter, bool bCaseSensitive = false, bool bStripBeginning = false, bool bStripEnding = false)
    {
        return Strip(inout, { &stripCharacter, 1 }, bCaseSensitive, bStripBeginning, bStripEnding);
    }

}

namespace AZ::StringFunc
{
    AZStd::string_view LStrip(AZStd::string_view in, AZStd::string_view stripCharacters)
    {
        if (size_t pos = in.find_first_not_of(stripCharacters); pos != AZStd::string_view::npos)
        {
            return in.substr(pos);
        }

        return {};
    };

    AZStd::string_view RStrip(AZStd::string_view in, AZStd::string_view stripCharacters)
    {
        if (size_t pos = in.find_last_not_of(stripCharacters); pos != AZStd::string_view::npos)
        {
            return in.substr(0, pos < in.size() ? pos + 1 : pos);
        }

        return {};
    };

    AZStd::string_view StripEnds(AZStd::string_view in, AZStd::string_view stripCharacters)
    {
        return LStrip(RStrip(in, stripCharacters), stripCharacters);
    };

    bool Equal(const char* inA, const char* inB, bool bCaseSensitive /*= false*/, size_t n /*= 0*/)
    {
        if (!inA || !inB)
        {
            return false;
        }

        if (inA == inB)
        {
            return true;
        }

        if (bCaseSensitive)
        {
            if (n)
            {
                return !strncmp(inA, inB, n);
            }
            else
            {
                return !strcmp(inA, inB);
            }
        }
        else
        {
            if (n)
            {
                return !azstrnicmp(inA, inB, n);
            }
            else
            {
                return !azstricmp(inA, inB);
            }
        }
    }
    bool Equal(AZStd::string_view inA, AZStd::string_view inB, bool bCaseSensitive)
    {
        const size_t maxCharsToCompare = inA.size();

        return inA.size() == inB.size() && (bCaseSensitive
            ? strncmp(inA.data(), inB.data(), maxCharsToCompare) == 0
            : azstrnicmp(inA.data(), inB.data(), maxCharsToCompare) == 0);
    }

    bool StartsWith(AZStd::string_view sourceValue, AZStd::string_view prefixValue, bool bCaseSensitive)
    {
        return sourceValue.size() >= prefixValue.size()
            && Equal(sourceValue.data(), prefixValue.data(), bCaseSensitive, prefixValue.size());
    }

    bool EndsWith(AZStd::string_view sourceValue, AZStd::string_view suffixValue, bool bCaseSensitive)
    {
        return sourceValue.size() >= suffixValue.size()
            && Equal(sourceValue.substr(sourceValue.size() - suffixValue.size(), AZStd::string_view::npos).data(), suffixValue.data(), bCaseSensitive, suffixValue.size());
    }

    bool Contains(AZStd::string_view in, char ch, bool bCaseSensitive)
    {
        return Find(in, ch, 0, false, bCaseSensitive) != AZStd::string_view::npos;
    }
    bool Contains(AZStd::string_view in, AZStd::string_view sv, bool bCaseSensitive)
    {
        return Find(in, sv, 0, false, bCaseSensitive) != AZStd::string_view::npos;
    }

    size_t Find(AZStd::string_view in, char c, size_t pos /*= 0*/, bool bReverse /*= false*/, bool bCaseSensitive /*= false*/)
    {
        if (in.empty())
        {
            return AZStd::string::npos;
        }

        if (pos == AZStd::string::npos)
        {
            pos = 0;
        }

        size_t inLen = in.size();
        if (inLen < pos)
        {
            return AZStd::string::npos;
        }

        if (!bCaseSensitive)
        {
            c = (char)tolower(c);
        }

        if (bReverse)
        {
            pos = inLen - pos - 1;
        }

        char character;

        do
        {
            if (!bCaseSensitive)
            {
                character = (char)tolower(in[pos]);
            }
            else
            {
                character = in[pos];
            }

            if (character == c)
            {
                return pos;
            }

            if (bReverse)
            {
                pos = pos > 0 ? pos-1 : pos;
            }
            else
            {
                pos++;
            }
        } while (bReverse ? pos : character != '\0');

        return AZStd::string::npos;
    }

    size_t Find(AZStd::string_view in, AZStd::string_view s, size_t offset /*= 0*/, bool bReverse /*= false*/, bool bCaseSensitive /*= false*/)
    {
        // Formally an empty string matches at the offset if it is <= to the size of the input string
        if (s.empty() && offset <= in.size())
        {
            return offset;
        }

        if (in.empty())
        {
            return AZStd::string::npos;
        }

        const size_t inlen = in.size();
        const size_t slen = s.size();

        if (offset == AZStd::string::npos)
        {
            offset = 0;
        }

        if (offset + slen > inlen)
        {
            return AZStd::string::npos;
        }

        const char* pCur;

        if (bReverse)
        {
            // Start at the end (- pos)
            pCur = in.data() + inlen - slen - offset;
        }
        else
        {
            // Start at the beginning (+ pos)
            pCur = in.data() + offset;
        }

        do
        {
            if (bCaseSensitive)
            {
                if (!strncmp(pCur, s.data(), slen))
                {
                    return static_cast<size_t>(pCur - in.data());
                }
            }
            else
            {
                if (!azstrnicmp(pCur, s.data(), slen))
                {
                    return static_cast<size_t>(pCur - in.data());
                }
            }

            if (bReverse)
            {
                pCur--;
            }
            else
            {
                pCur++;
            }
        } while (bReverse ? pCur >= in.data() : pCur - in.data() <= static_cast<ptrdiff_t>(inlen));

        return AZStd::string::npos;
    }

    char FirstCharacter(const char* in)
    {
        if (!in)
        {
            return '\0';
        }
        if (in[0] == '\n')
        {
            return '\0';
        }
        return in[0];
    }

    char LastCharacter(const char* in)
    {
        if (!in)
        {
            return '\0';
        }
        size_t len = strlen(in);
        if (!len)
        {
            return '\0';
        }
        return in[len - 1];
    }

    AZStd::string& Append(AZStd::string& inout, const char s)
    {
        return inout.append(1, s);
    }

    AZStd::string& Append(AZStd::string& inout, const char* str)
    {
        if (!str)
        {
            return inout;
        }
        return inout.append(str);
    }

    AZStd::string& Prepend(AZStd::string& inout, const char s)
    {
        return inout.insert((size_t)0, 1, s);
    }

    AZStd::string& Prepend(AZStd::string& inout, const char* str)
    {
        if (!str)
        {
            return inout;
        }
        return inout.insert(0, str);
    }

    AZStd::string& LChop(AZStd::string& inout, size_t num)
    {
        return Internal::LChop(inout, num);
    }

    AZStd::string_view LChop(AZStd::string_view in, size_t num)
    {
        return Internal::LChop(in, num);
    }

    AZStd::string& RChop(AZStd::string& inout, size_t num)
    {
        return Internal::RChop(inout, num);
    }

    AZStd::string_view RChop(AZStd::string_view in, size_t num)
    {
        return Internal::RChop(in, num);
    }

    AZStd::string& LKeep(AZStd::string& inout, size_t pos, bool bKeepPosCharacter)
    {
        return Internal::LKeep(inout, pos, bKeepPosCharacter);
    }

    AZStd::string& RKeep(AZStd::string& inout, size_t pos, bool bKeepPosCharacter)
    {
        return Internal::RKeep(inout, pos, bKeepPosCharacter);
    }

    bool Replace(AZStd::string& inout, const char replaceA, const char withB, bool bCaseSensitive, bool bReplaceFirst, bool bReplaceLast)
    {
        return Internal::Replace(inout, replaceA, withB, bCaseSensitive, bReplaceFirst, bReplaceLast);
    }

    bool Replace(AZStd::string& inout, const char* replaceA, const char* withB, bool bCaseSensitive, bool bReplaceFirst, bool bReplaceLast)
    {
        return Internal::Replace(inout, replaceA, withB, bCaseSensitive, bReplaceFirst, bReplaceLast);
    }

    bool Strip(AZStd::string& inout, const char stripCharacter, bool bCaseSensitive, bool bStripBeginning, bool bStripEnding)
    {
        return Internal::Strip(inout, stripCharacter, bCaseSensitive, bStripBeginning, bStripEnding);
    }

    bool Strip(AZStd::string& inout, const char* stripCharacters, bool bCaseSensitive, bool bStripBeginning, bool bStripEnding)
    {
        return Internal::Strip(inout, stripCharacters, bCaseSensitive, bStripBeginning, bStripEnding);
    }

    AZStd::string& TrimWhiteSpace(AZStd::string& value, bool leading, bool trailing)
    {
        static const char* trimmable = " \t\r\n";
        if (value.length() > 0)
        {
            if (leading)
            {
                value.erase(0, value.find_first_not_of(trimmable));
            }
            if (trailing)
            {
                value.erase(value.find_last_not_of(trimmable) + 1);
            }
        }
        return value;
    }

    void Tokenize(AZStd::string_view in, AZStd::vector<AZStd::string>& tokens, const char delimiter, bool keepEmptyStrings, bool keepSpaceStrings)
    {
        return Tokenize(in, tokens, { &delimiter, 1 }, keepEmptyStrings, keepSpaceStrings);
    }

    void Tokenize(AZStd::string_view in, AZStd::vector<AZStd::string>& tokens, AZStd::string_view delimiters, bool keepEmptyStrings, bool keepSpaceStrings)
    {
        auto insertVisitor = [&tokens](AZStd::string_view token)
        {
            tokens.push_back(token);
        };
        return TokenizeVisitor(in, insertVisitor, delimiters, keepEmptyStrings, keepSpaceStrings);
    }

    void TokenizeVisitor(AZStd::string_view in, const TokenVisitor& tokenVisitor, const char delimiter, bool keepEmptyStrings, bool keepSpaceStrings)
    {
        return TokenizeVisitor(in, tokenVisitor, { &delimiter, 1 }, keepEmptyStrings, keepSpaceStrings);
    }

    void TokenizeVisitor(AZStd::string_view in, const TokenVisitor& tokenVisitor, AZStd::string_view delimiters,
        bool keepEmptyStrings, bool keepSpaceStrings)
    {
        if (delimiters.empty() || in.empty())
        {
            return;
        }

        while (AZStd::optional<AZStd::string_view> nextToken = TokenizeNext(in, delimiters))
        {
            bool bIsEmpty = nextToken->empty();
            bool bIsSpaces = false;
            if (!bIsEmpty)
            {
                AZStd::string_view strippedNextToken = StripEnds(*nextToken, " ");
                bIsSpaces = strippedNextToken.empty();
            }

            if ((bIsEmpty && keepEmptyStrings) ||
                (bIsSpaces && keepSpaceStrings) ||
                (!bIsSpaces && !bIsEmpty))
            {
                tokenVisitor(*nextToken);
            }
        }
    }

    void TokenizeVisitorReverse(AZStd::string_view in, const TokenVisitor& tokenVisitor, const char delimiter, bool keepEmptyStrings, bool keepSpaceStrings)
    {
        return TokenizeVisitorReverse(in, tokenVisitor, { &delimiter, 1 }, keepEmptyStrings, keepSpaceStrings);
    }

    void TokenizeVisitorReverse(AZStd::string_view in, const TokenVisitor& tokenVisitor, AZStd::string_view delimiters,
        bool keepEmptyStrings, bool keepSpaceStrings)
    {
        if (delimiters.empty() || in.empty())
        {
            return;
        }

        while (AZStd::optional<AZStd::string_view> nextToken = TokenizeLast(in, delimiters))
        {
            bool bIsEmpty = nextToken->empty();
            bool bIsSpaces = false;
            if (!bIsEmpty)
            {
                AZStd::string_view strippedNextToken = StripEnds(*nextToken, " ");
                bIsSpaces = strippedNextToken.empty();
            }

            if ((bIsEmpty && keepEmptyStrings) ||
                (bIsSpaces && keepSpaceStrings) ||
                (!bIsSpaces && !bIsEmpty))
            {
                tokenVisitor(*nextToken);
            }
        }
    }

    AZStd::optional<AZStd::string_view> TokenizeNext(AZStd::string_view& inout, const char delimiter)
    {
        return TokenizeNext(inout, { &delimiter, 1 });
    }
    AZStd::optional<AZStd::string_view> TokenizeNext(AZStd::string_view& inout, AZStd::string_view delimiters)
    {
        if (delimiters.empty() || inout.empty())
        {
            return AZStd::nullopt;
        }

        AZStd::string_view resultToken;
        if (size_t pos = inout.find_first_of(delimiters); pos == AZStd::string_view::npos)
        {
            // The delimiter has not been found, a new view containing the entire
            // string will be returned and the input parameter will be set to empty
            resultToken.swap(inout);
        }
        else
        {
            resultToken = { inout.data(), pos };
            // Strip off all previous characters before the delimiter plus
            // the delimiter itself from the input view
            inout.remove_prefix(pos + 1);
        }

        return resultToken;
    }

    AZStd::optional<AZStd::string_view> TokenizeLast(AZStd::string_view& inout, const char delimiter)
    {
        return TokenizeLast(inout, { &delimiter, 1 });
    }
    AZStd::optional<AZStd::string_view> TokenizeLast(AZStd::string_view& inout, AZStd::string_view delimiters)
    {
        if (delimiters.empty() || inout.empty())
        {
            return AZStd::nullopt;
        }

        AZStd::string_view resultToken;
        if (size_t pos = inout.find_last_of(delimiters); pos == AZStd::string_view::npos)
        {
            // The delimiter has not been found, a new view containing the entire
            // string will be returned and the input parameter will be set to empty
            resultToken.swap(inout);
        }
        else
        {
            resultToken = inout.substr(pos + 1);
            // Strip off all previous characters before the delimiter plus
            // the delimiter itself from the input view
            inout = inout.substr(0, pos);
        }

        return resultToken;
    }

    bool FindFirstOf(AZStd::string_view inString, size_t offset, const AZStd::vector<AZStd::string_view>& searchStrings, uint32_t& outIndex, size_t& outOffset)
    {
        bool found = false;

        outIndex = 0;
        outOffset = AZStd::string::npos;
        for (int32_t i = 0; i < searchStrings.size(); ++i)
        {
            const AZStd::string& search = searchStrings[i];

            size_t entry = inString.find(search, offset);
            if (entry != AZStd::string::npos)
            {
                if (!found || (entry < outOffset))
                {
                    found = true;
                    outIndex = i;
                    outOffset = entry;
                }
            }
        }

        return found;
    }

    void Tokenize(AZStd::string_view input, AZStd::vector<AZStd::string>& tokens, const AZStd::vector<AZStd::string_view>& delimiters, bool keepEmptyStrings /*= false*/, bool keepSpaceStrings /*= false*/)
    {
        if (input.empty())
        {
            return;
        }

        size_t offset = 0;
        for (;;)
        {
            uint32_t nextMatch = 0;
            size_t nextOffset = offset;
            if (!FindFirstOf(input, offset, delimiters, nextMatch, nextOffset))
            {
                // No more occurrences of a separator, consume whatever is left and exit
                tokens.push_back(input.substr(offset));
                break;
            }

            // Take the substring, not including the separator, and increment our offset
            AZStd::string nextSubstring = input.substr(offset, nextOffset - offset);
            if (keepEmptyStrings || keepSpaceStrings || !nextSubstring.empty())
            {
                tokens.push_back(nextSubstring);
            }

            offset = nextOffset + delimiters[nextMatch].size();
        }
    }

    int ToInt(const char* in)
    {
        if (!in)
        {
            return 0;
        }
        return atoi(in);
    }

    bool LooksLikeInt(const char* in, int* pInt /*=nullptr*/)
    {
        if (!in)
        {
            return false;
        }

        //if pos is past then end of the string false
        size_t len = strlen(in);
        if (!len)//must at least 1 characters to work with "1"
        {
            return false;
        }

        const char* pStr = in;

        size_t countNeg = 0;
        while (*pStr != '\0' &&
               (isdigit(*pStr) ||
                *pStr == '-'))
        {
            if (*pStr == '-')
            {
                countNeg++;
            }
            pStr++;
        }

        if (*pStr == '\0' &&
            countNeg < 2)
        {
            if (pInt)
            {
                *pInt = ToInt(in);
            }

            return true;
        }
        return false;
    }

    double ToDouble(const char* in)
    {
        if (!in)
        {
            return 0.;
        }
        return atof(in);
    }

    bool LooksLikeDouble(const char* in, double* pDouble)
    {
        if (!in)
        {
            return false;
        }

        size_t len = strlen(in);
        if (len < 2)//must have at least 2 characters to work with "1."
        {
            return false;
        }

        const char* pStr = in;

        size_t countDot = 0;
        size_t countNeg = 0;
        while (*pStr != '\0' &&
            (isdigit(*pStr) ||
            (*pStr == '-' ||
                *pStr == '.')))
        {
            if (*pStr == '.')
            {
                countDot++;
            }
            if (*pStr == '-')
            {
                countNeg++;
            }
            pStr++;
        }

        if (*pStr == '\0' &&
            countDot == 1 &&
            countNeg < 2)
        {
            if (pDouble)
            {
                *pDouble = ToDouble(in);
            }

            return true;
        }

        return false;
    }

    float ToFloat(const char* in)
    {
        if (!in)
        {
            return 0.f;
        }
        return (float)atof(in);
    }

    bool LooksLikeFloat(const char* in, float* pFloat /* = nullptr */)
    {
        bool result = false;

        if (pFloat)
        {
            double doubleValue = 0.0;
            result = LooksLikeDouble(in, &doubleValue);

            (*pFloat) = aznumeric_cast<float>(doubleValue);
        }
        else
        {
            result = LooksLikeDouble(in);
        }

        return result;
    }

    bool ToBool(const char* in)
    {
        bool boolValue = false;
        if (LooksLikeBool(in, &boolValue))
        {
            return boolValue;
        }
        return false;
    }

    bool LooksLikeBool(const char* in, bool* pBool /* = nullptr */)
    {
        if (!in)
        {
            return false;
        }

        if (!azstricmp(in, "true") || !azstricmp(in, "1"))
        {
            if (pBool)
            {
                *pBool = true;
            }
            return true;
        }

        if (!azstricmp(in, "false") || !azstricmp(in, "0"))
        {
            if (pBool)
            {
                *pBool = false;
            }
            return true;
        }

        return false;
    }

    template <typename VECTOR_TYPE, uint32_t ELEMENT_COUNT>
    bool LooksLikeVectorHelper(const char* in, VECTOR_TYPE* outVector)
    {
        AZStd::vector<AZStd::string> tokens;
        Tokenize(in, tokens, ',', false, true);
        if (tokens.size() == ELEMENT_COUNT)
        {
            float vectorValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

            for (uint32_t element = 0; element < ELEMENT_COUNT; ++element)
            {
                if (!LooksLikeFloat(tokens[element].c_str(), outVector ? &vectorValues[element] : nullptr))
                {
                    return false;
                }
            }

            if (outVector)
            {
                for (uint32_t element = 0; element < ELEMENT_COUNT; ++element)
                {
                    outVector->SetElement(element, vectorValues[element]);
                }
            }

            return true;
        }

        return false;
    }

    bool LooksLikeVector2(const char* in, AZ::Vector2* outVector)
    {
        return LooksLikeVectorHelper<AZ::Vector2, 2>(in, outVector);
    }

    AZ::Vector2 ToVector2(const char* in)
    {
        AZ::Vector2 vector;
        LooksLikeVector2(in, &vector);
        return vector;
    }

    bool LooksLikeVector3(const char* in, AZ::Vector3* outVector)
    {
        return LooksLikeVectorHelper<AZ::Vector3, 3>(in, outVector);
    }

    AZ::Vector3 ToVector3(const char* in)
    {
        AZ::Vector3 vector;
        LooksLikeVector3(in, &vector);
        return vector;
    }

    bool LooksLikeVector4(const char* in, AZ::Vector4* outVector)
    {
        return LooksLikeVectorHelper<AZ::Vector4, 4>(in, outVector);
    }

    AZ::Vector4 ToVector4(const char* in)
    {
        AZ::Vector4 vector;
        LooksLikeVector4(in, &vector);
        return vector;
    }

    bool ToHexDump(const char* in, AZStd::string& out)
    {
        struct TInline
        {
            static void ByteToHex(char* pszHex, unsigned char bValue)
            {
                pszHex[0] = bValue / 16;

                if (pszHex[0] < 10)
                {
                    pszHex[0] += '0';
                }
                else
                {
                    pszHex[0] -= 10;
                    pszHex[0] += 'A';
                }

                pszHex[1] = bValue % 16;

                if (pszHex[1] < 10)
                {
                    pszHex[1] += '0';
                }
                else
                {
                    pszHex[1] -= 10;
                    pszHex[1] += 'A';
                }
            }
        };

        size_t len = strlen(in);
        if (len < 1) //must be at least 1 character to work with
        {
            return false;
        }

        size_t nBytes = len;

        char* pszData = reinterpret_cast<char*>(azmalloc((nBytes * 2) + 1));

        for (size_t ii = 0; ii < nBytes; ++ii)
        {
            TInline::ByteToHex(&pszData[ii * 2], in[ii]);
        }

        pszData[nBytes * 2] = 0x00;
        out = pszData;
        azfree(pszData);

        return true;
    }

    bool FromHexDump(const char* in, AZStd::string& out)
    {
        struct TInline
        {
            static unsigned char HexToByte(const char* pszHex)
            {
                unsigned char bHigh = 0;
                unsigned char bLow = 0;

                if ((pszHex[0] >= '0') && (pszHex[0] <= '9'))
                {
                    bHigh = pszHex[0] - '0';
                }
                else if ((pszHex[0] >= 'A') && (pszHex[0] <= 'F'))
                {
                    bHigh = (pszHex[0] - 'A') + 10;
                }

                bHigh = bHigh << 4;

                if ((pszHex[1] >= '0') && (pszHex[1] <= '9'))
                {
                    bLow = pszHex[1] - '0';
                }
                else if ((pszHex[1] >= 'A') && (pszHex[1] <= 'F'))
                {
                    bLow = (pszHex[1] - 'A') + 10;
                }

                return bHigh | bLow;
            }
        };

        size_t len = strlen(in);
        if (len < 2) //must be at least 2 characters to work with
        {
            return false;
        }

        size_t nBytes = len / 2;
        char* pszData = reinterpret_cast<char*>(azmalloc(nBytes + 1));

        for (size_t ii = 0; ii < nBytes; ++ii)
        {
            pszData[ii] = TInline::HexToByte(&in[ii * 2]);
        }

        pszData[nBytes] = 0x00;
        out = pszData;
        azfree(pszData);

        return true;
    }

    namespace NumberFormatting
    {
        int GroupDigits(char* buffer, size_t bufferSize, size_t decimalPosHint, char digitSeparator, char decimalSeparator, int groupingSize, int firstGroupingSize)
        {
            static const int MAX_SEPARATORS = 16;

            AZ_Assert(buffer, "Null string buffer");
            AZ_Assert(bufferSize > decimalPosHint, "Decimal position %lu cannot be located beyond bufferSize %lu", decimalPosHint, bufferSize);
            AZ_Assert(groupingSize > 0, "Grouping size must be a positive integer");

            int numberEndPos = 0;
            int stringEndPos = 0;

            if (decimalPosHint > 0 && decimalPosHint < (bufferSize - 1) && buffer[decimalPosHint] == decimalSeparator)
            {
                // Assume the number ends at the supplied location
                numberEndPos = (int)decimalPosHint;
                stringEndPos = numberEndPos + (int)strnlen(buffer + numberEndPos, bufferSize - numberEndPos);
            }
            else
            {
                // Search for the final digit or separator while obtaining the string length
                int lastDigitSeenPos = 0;

                while (stringEndPos < bufferSize)
                {
                    char c = buffer[stringEndPos];

                    if (!c)
                    {
                        break;
                    }
                    else if (c == decimalSeparator)
                    {
                        // End the number if there's a decimal
                        numberEndPos = stringEndPos;
                    }
                    else if (numberEndPos <= 0 && c >= '0' && c <= '9')
                    {
                        // Otherwise keep track of where the last digit we've seen is
                        lastDigitSeenPos = stringEndPos;
                    }

                    stringEndPos++;
                }

                if (numberEndPos <= 0)
                {
                    if (lastDigitSeenPos > 0)
                    {
                        // No decimal, so use the last seen digit as the end of the number
                        numberEndPos = lastDigitSeenPos + 1;
                    }
                    else
                    {
                        // No digits, no decimals, therefore no change in the string
                        return stringEndPos;
                    }
                }
            }

            if (firstGroupingSize <= 0)
            {
                firstGroupingSize = groupingSize;
            }

            // Determine where to place the separators
            int groupingSizes[] = { firstGroupingSize + 1, groupingSize };  // First group gets +1 since we begin all subsequent groups at the second digit
            int groupingOffsetsToNext[] = { 1, 0 };  // We will offset from the first entry to the second, then stay at the second for remaining iterations
            const int* currentGroupingSize = groupingSizes;
            const int* currentGroupingOffsetToNext = groupingOffsetsToNext;
            AZStd::fixed_vector<char*, MAX_SEPARATORS> separatorLocations;
            int groupCounter = 0;
            int digitPosition = numberEndPos - 1;

            while (digitPosition >= 0)
            {
                // Walk backwards in the string from the least significant digit to the most significant, demarcating consecutive groups of digits
                char c = buffer[digitPosition];

                if (c >= '0' && c <= '9')
                {
                    if (++groupCounter == *currentGroupingSize)
                    {
                        // Demarcate a new group of digits at this location
                        separatorLocations.push_back(buffer + digitPosition);
                        currentGroupingSize += *currentGroupingOffsetToNext;
                        currentGroupingOffsetToNext += *currentGroupingOffsetToNext;
                        groupCounter = 0;
                    }

                    digitPosition--;
                }
                else
                {
                    break;
                }
            }

            if (stringEndPos + separatorLocations.size() >= bufferSize)
            {
                // Won't fit into buffer, so return unchanged
                return stringEndPos;
            }

            // Insert the separators by shifting characters forward in the string, starting at the end and working backwards
            const char* src = buffer + stringEndPos;
            char* dest = buffer + stringEndPos + separatorLocations.size();
            auto separatorItr = separatorLocations.begin();

            while (separatorItr != separatorLocations.end())
            {
                while (src > *separatorItr)
                {
                    *dest-- = *src--;
                }

                // Insert the separator and reduce the distance between our destination and source by one
                *dest-- = digitSeparator;
                ++separatorItr;
            }

            return (int)(stringEndPos + separatorLocations.size());
        }
    }

    namespace AssetPath
    {
        namespace Internal
        {
            AZ::u32 CalculateBranchTokenHash(AZStd::string_view engineRootPath)
            {
                // Normalize the token to prepare for CRC32 calculation
                auto NormalizeEnginePath = [](const char element) -> char
                {
                    // Substitute path separators with '_' and lower case
                    return element == AZ::IO::WindowsPathSeparator || element == AZ::IO::PosixPathSeparator
                        ? '_'
                        : static_cast<char>(std::tolower(element));
                };

                // Trim off trailing path separators
                engineRootPath = RStrip(engineRootPath, AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR);
                AZ::IO::FixedMaxPathString enginePath;
                AZStd::transform(
                    engineRootPath.begin(), engineRootPath.end(), AZStd::back_inserter(enginePath), AZStd::move(NormalizeEnginePath));

                // Perform the CRC32 calculation
                constexpr bool forceLowercase = true;
                return static_cast<AZ::u32>(AZ::Crc32(enginePath.c_str(), enginePath.size(), forceLowercase));
            }
        } // namespace Internal
        void CalculateBranchToken(AZStd::string_view engineRootPath, AZStd::string& token)
        {
            token = AZStd::string::format("0x%08X", Internal::CalculateBranchTokenHash(engineRootPath));
        }
        void CalculateBranchToken(AZStd::string_view engineRootPath, AZ::IO::FixedMaxPathString& token)
        {
            token = AZ::IO::FixedMaxPathString::format("0x%08X", Internal::CalculateBranchTokenHash(engineRootPath));
        }
    } // namespace AssetPath

    namespace AssetDatabasePath
    {
        bool Normalize(AZStd::string& inout)
        {
            // Asset Paths uses the forward slash for the database separator
            AZ::IO::Path path(AZStd::move(inout), AZ_CORRECT_DATABASE_SEPARATOR);
            bool appendTrailingSlash = (path.Native().ends_with(AZ_CORRECT_DATABASE_SEPARATOR) || path.Native().ends_with(AZ_WRONG_DATABASE_SEPARATOR))
                && path.HasRelativePath();
            inout = AZStd::move(path.LexicallyNormal().Native());
            if (appendTrailingSlash)
            {
                inout.push_back(AZ_CORRECT_DATABASE_SEPARATOR);
            }
            return IsValid(inout.c_str());
        }

        bool IsValid(const char* in)
        {
            if (!in)
            {
                return false;
            }

            if (!strlen(in))
            {
                return false;
            }

            if (Find(in, AZ_DATABASE_INVALID_CHARACTERS) != AZStd::string::npos)
            {
                return false;
            }

            if (Find(in, AZ_WRONG_DATABASE_SEPARATOR) != AZStd::string::npos)
            {
                return false;
            }

#ifndef AZ_FILENAME_ALLOW_SPACES
            if (Find(in, AZ_SPACE_CHARACTERS) != AZStd::string::npos)
            {
                return false;
            }
#endif // AZ_FILENAME_ALLOW_SPACES

            if (LastCharacter(in) == AZ_CORRECT_DATABASE_SEPARATOR)
            {
                return false;
            }

            return true;
        }

        bool Split(const char* in, [[maybe_unused]] AZStd::string* pDstProjectRootOut, AZStd::string* pDstDatabaseRootOut,
            AZStd::string* pDstDatabasePathOut , AZStd::string* pDstFileOut, AZStd::string* pDstFileExtensionOut)
        {
            AZStd::string_view path{ in };
            if (path.empty())
            {
                return false;
            }

            AZ::IO::PathView pathView(path, AZ_CORRECT_DATABASE_SEPARATOR);
            if (pDstDatabaseRootOut)
            {
                AZStd::string_view rootNameView = pathView.RootName().Native();
                if (rootNameView.size() > pDstDatabaseRootOut->max_size())
                {
                    return false;
                }
                *pDstDatabaseRootOut = rootNameView;
            }
            if (pDstDatabasePathOut)
            {
                AZStd::string_view rootPathView = pathView.RootPath().Native();
                AZStd::string_view relPathParentView = pathView.ParentPath().RelativePath().Native();
                if (rootPathView.size() + relPathParentView.size() > pDstDatabasePathOut->max_size())
                {
                    return false;
                }
                // Append the root directory if there is one
                *pDstDatabasePathOut = rootPathView;
                // Append the relative path portion of the split path excluding the filename
                *pDstDatabasePathOut += relPathParentView;
            }
            if (pDstFileOut)
            {
                AZStd::string_view stemView = pathView.Stem().Native();
                if (stemView.size() > pDstFileOut->max_size())
                {
                    return false;
                }
                *pDstFileOut = stemView;
            }
            if (pDstFileExtensionOut)
            {
                AZStd::string_view extensionView = pathView.Extension().Native();
                if (extensionView.size() > pDstFileExtensionOut->max_size())
                {
                    return false;
                }
                *pDstFileExtensionOut = extensionView;
            }

            return true;
        }

        bool Join(const char* pFirstPart, const char* pSecondPart, AZStd::string& out, [[maybe_unused]] bool bCaseInsensitive /*= true*/, bool bNormalize /*= true*/)
        {
            // both paths cannot be empty
            if (!pFirstPart || !pSecondPart)
            {
                return false;
            }

            AZ::IO::PathView secondPath(pSecondPart);
            AZ_Warning("StringFunc", secondPath.IsRelative(), "The second join parameter %s is an absolute path,"
                " this will replace the first part of the path resulting in an output of just the second part",
                pSecondPart);

            AZ::IO::Path resultPath(pFirstPart, AZ_CORRECT_DATABASE_SEPARATOR);
            resultPath /= secondPath;
            out = bNormalize ? AZStd::move(resultPath.LexicallyNormal().Native()) : AZStd::move(resultPath.Native());
            return true;
        }
    } //namespace AssetDatabasePath

    namespace Root
    {
        bool Normalize(AZStd::string& inout)
        {
            AZ::IO::Path path(AZStd::move(inout));
            path = path.LexicallyNormal();
            // After normalization check if the path contains a relative path
            bool appendTrailingSlash = path.HasRelativePath();
            inout = AZStd::move(path.Native());
            if (appendTrailingSlash)
            {
                inout.push_back(AZ_CORRECT_FILESYSTEM_SEPARATOR); // Append a trailing separator for Root path normalization
            }
            return IsValid(inout.c_str());
        }

        bool IsValid(const char* in)
        {
            if (!in)
            {
                return false;
            }

            if (!strlen(in))
            {
                return false;
            }

            if (Find(in, AZ_FILESYSTEM_INVALID_CHARACTERS) != AZStd::string::npos)
            {
                return false;
            }

            if (Find(in, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
            {
                return false;
            }

#ifndef AZ_FILENAME_ALLOW_SPACES
            if (Find(in, AZ_SPACE_CHARACTERS) != AZStd::string::npos)
            {
                return false;
            }
#endif // AZ_FILENAME_ALLOW_SPACES

            AZ::IO::PathView pathView(in);
            if (!pathView.HasRootPath())
            {
                return false;
            }

            if (LastCharacter(in) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
            {
                return false;
            }

            return true;
        }
    }//namespace Root

    namespace RelativePath
    {
        bool Normalize(AZStd::string& inout)
        {
            AZ::IO::Path path(AZStd::move(inout));
            path = path.LexicallyNormal();
            // After normalization check if the path contains a relative path
            bool appendTrailingSlash = path.HasRelativePath();
            inout = AZStd::move(path.Native());
            if (appendTrailingSlash)
            {
                inout.push_back(AZ_CORRECT_FILESYSTEM_SEPARATOR); // Append trailing separator for Relative path normalization if it it is not empty
            }
            return IsValid(inout.c_str());
        }

        bool IsValid(const char* in)
        {
            if (!in)
            {
                return false;
            }

            if (!strlen(in))
            {
                return true;
            }

            if (Find(in, AZ_FILESYSTEM_INVALID_CHARACTERS) != AZStd::string::npos)
            {
                return false;
            }

            if (Find(in, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
            {
                return false;
            }

#ifndef AZ_FILENAME_ALLOW_SPACES
            if (Find(in, AZ_SPACE_CHARACTERS) != AZStd::string::npos)
            {
                return false;
            }
#endif // AZ_FILENAME_ALLOW_SPACES

            if (Path::HasDrive(in))
            {
                return false;
            }

            if (FirstCharacter(in) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
            {
                return false;
            }

            if (LastCharacter(in) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
            {
                return false;
            }

            return true;
        }
    }//namespace RelativePath

    namespace Path
    {
        bool Normalize(AZStd::string& inout)
        {
            AZ::IO::Path path(AZStd::move(inout));
            bool appendTrailingSlash = (path.Native().ends_with(AZ_CORRECT_FILESYSTEM_SEPARATOR) || path.Native().ends_with(AZ_WRONG_FILESYSTEM_SEPARATOR));
            path = path.LexicallyNormal();
            // After normalization check if the path contains a relative path and addition to ending with a path separator before
            appendTrailingSlash = appendTrailingSlash && path.HasRelativePath();
            inout = AZStd::move(path.Native());
            if (appendTrailingSlash)
            {
                inout.push_back(AZ_CORRECT_FILESYSTEM_SEPARATOR);
            }
            return IsValid(inout.c_str());
        }

        bool Normalize(FixedString& inout)
        {
            AZ::IO::FixedMaxPath path(AZStd::move(inout));
            bool appendTrailingSlash = (path.Native().ends_with(AZ_CORRECT_FILESYSTEM_SEPARATOR) || path.Native().ends_with(AZ_WRONG_FILESYSTEM_SEPARATOR));
            path = path.LexicallyNormal();
            // After normalization check if the path contains a relative path and addition to ending with a path separator before
            appendTrailingSlash = appendTrailingSlash && path.HasRelativePath();
            inout = AZStd::move(path.Native());
            if (appendTrailingSlash)
            {
                inout.push_back(AZ_CORRECT_FILESYSTEM_SEPARATOR);
            }
            return IsValid(inout.c_str());
        }

        bool IsValid(const char* in, bool bHasDrive /*= false*/, bool bHasExtension /*= false*/, AZStd::string* errors /*= nullptr*/)
        {
            //if they gave us a error reporting string empty it.
            if (errors)
            {
                errors->clear();
            }

            //empty is not a valid path
            if (!in)
            {
                if (errors)
                {
                    *errors += "The path is Empty.";
                }
                return false;
            }

            //empty is not a valid path
            size_t length = strlen(in);
            if (!length)
            {
                if (errors)
                {
                    *errors += "The path is Empty.";
                }
                return false;
            }

            //invalid characters
            const char* inEnd = in + length;
            const char* invalidCharactersBegin = AZ_FILESYSTEM_INVALID_CHARACTERS;
            const char* invalidCharactersEnd = invalidCharactersBegin + AZ_ARRAY_SIZE(AZ_FILESYSTEM_INVALID_CHARACTERS);
            if (AZStd::find_first_of(in, inEnd, invalidCharactersBegin, invalidCharactersEnd) != inEnd)
            {
                if (errors)
                {
                    *errors += "The path has invalid characters.";
                }
                return false;
            }

            //invalid characters
            if (Find(in, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
            {
                if (errors)
                {
                    *errors += "The path has wrong separator.";
                }
                return false;
            }

#ifndef AZ_FILENAME_ALLOW_SPACES
            const char* spaceCharactersBegin = AZ_SPACE_CHARACTERS;
            const char* spaceCharactersEnd = spaceCharactersBegin + AZ_ARRAY_SIZE(AZ_SPACE_CHARACTERS);
            if (AZStd::find_first_of(in, inEnd, spaceCharactersBegin, spaceCharactersEnd) != inEnd)
            {
                if (errors)
                {
                    *errors += "The path has space characters.";
                }
                return false;
            }
#endif // AZ_FILENAME_ALLOW_SPACES

            //does it have a drive if specified
            if (bHasDrive && !HasDrive(in))
            {
                if (errors)
                {
                    *errors += "The path should have a drive. The path [";
                    *errors += in;
                    *errors += "] is invalid.";
                }
                return false;
            }

            //does it have and extension if specified
            if (bHasExtension && !HasExtension(in))
            {
                if (errors)
                {
                    *errors += "The path should have the a file extension. The path [";
                    *errors += in;
                    *errors += "] is invalid.";
                }
                return false;
            }

            //start at the beginning and walk down the characters of the path
            const char* elementStart = in;
            const char* walk = elementStart;
            while (*walk)
            {
                if (*walk == AZ_CORRECT_FILESYSTEM_SEPARATOR) //is this the correct separator
                {
                    elementStart = walk;
                }
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
                else if (*walk == AZ_FILESYSTEM_DRIVE_SEPARATOR) //is this the drive separator
                {
                    //A AZ_FILESYSTEM_DRIVE_SEPARATOR character con only occur in the first
                    //component of a valid path. If the elementStart is not GetBufferPtr()
                    //then we have past the first component
                    if (elementStart != in)
                    {
                        if (errors)
                        {
                            *errors += "There is a stray AZ_FILESYSTEM_DRIVE_SEPARATOR = ";
                            *errors += AZ_FILESYSTEM_DRIVE_SEPARATOR;
                            *errors += " found after the first component. The path [";
                            *errors += in;
                            *errors += "] is invalid.";
                        }
                        return false;
                    }
                }
#endif
#ifndef AZ_FILENAME_ALLOW_SPACES
                else if (*walk == ' ') //is this a space
                {
                    if (errors)
                    {
                        *errors += "The component [";
                        for (const char* c = elementStart + 1; c != walk; ++c)
                        {
                            *errors += *c;
                        }
                        *errors += "] has a SPACE character. The path [";
                        *errors += in;
                        *errors += "] is invalid.";
                    }
                    return false;
                }
#endif

                ++walk;
            }

#if !AZ_TRAIT_OS_ALLOW_UNLIMITED_PATH_COMPONENT_LENGTH
            //is this full path longer than AZ::IO::MaxPathLength (The longest a path with all components can possibly be)?
            if (walk - in > AZ::IO::MaxPathLength)
            {
                if (errors != 0)
                {
                    *errors += "The path [";
                    *errors += in;
                    *errors += "] is over the AZ::IO::MaxPathLength = ";
                    char buf[64];
                    _itoa_s(AZ::IO::MaxPathLength, buf, 10);
                    *errors += buf;
                    *errors += " characters total length limit.";
                }
                return false;
            }
#endif

            return true;
        }

        bool ConstructFull(const char* pRootPath, const char* pFileName, AZStd::string& out, bool bNormalize /* = false*/)
        {
            if (!pRootPath || AZ::IO::PathView(pRootPath).IsRelative()
                || !pFileName || AZ::IO::PathView(pFileName).IsAbsolute())
            {
                return false;
            }
            AZ::IO::Path path(pRootPath);
            path /= pFileName;
            if (bNormalize)
            {
                out = AZStd::move(path.LexicallyNormal().Native());
            }
            else
            {
                out = AZStd::move(path.Native());
            }
            return IsValid(out.c_str());
        }

        bool ConstructFull(const char* pRootPath, const char* pFileName, const char* pFileExtension, AZStd::string& out, bool bNormalize /* = false*/)
        {
            if (!pRootPath || AZ::IO::PathView(pRootPath).IsRelative()
                || !pFileName || AZ::IO::PathView(pFileName).IsAbsolute())
            {
                return false;
            }
            AZ::IO::Path path(pRootPath);
            path /= pFileName;
            if (pFileExtension)
            {
                path.ReplaceExtension(pFileExtension);
            }
            if (bNormalize)
            {
                out = AZStd::move(path.LexicallyNormal().Native());
            }
            else
            {
                out = AZStd::move(path.Native());
            }
            return IsValid(out.c_str());
        }

        bool ConstructFull(const char* pRoot, const char* pRelativePath, const char* pFileName, const char* pFileExtension, AZStd::string& out, bool bNormalize /* = false*/)
        {
            if (!pRoot  || AZ::IO::PathView(pRoot).IsRelative()
                || !pRelativePath || AZ::IO::PathView(pRelativePath).IsAbsolute()
                || !pFileName || AZ::IO::PathView(pFileName).IsAbsolute())
            {
                return false;
            }
            AZ::IO::Path path(pRoot);
            path /= pRelativePath;
            path /= pFileName;
            if (pFileExtension)
            {
                path.ReplaceExtension(pFileExtension);
            }
            if (bNormalize)
            {
                out = AZStd::move(path.LexicallyNormal().Native());
            }
            else
            {
                out = AZStd::move(path.Native());
            }
            return IsValid(out.c_str());
        }

        bool Split(const char* in, AZStd::string* pDstDrive, AZStd::string* pDstPath, AZStd::string* pDstName, AZStd::string* pDstExtension)
        {
            AZStd::string_view path{ in };
            if (path.empty())
            {
                return false;
            }

            AZ::IO::PathView pathView(path);
            if (pDstDrive)
            {
                AZStd::string_view rootNameView = pathView.RootName().Native();
                if (rootNameView.size() > pDstDrive->max_size())
                {
                    return false;
                }
                *pDstDrive = rootNameView;
            }
            if (pDstPath)
            {
                AZStd::string_view rootDirectoryView = pathView.RootDirectory().Native();
                AZStd::string_view relPathParentView = pathView.ParentPath().RelativePath().Native();
                if (rootDirectoryView.size() + relPathParentView.size() > pDstPath->max_size())
                {
                    return false;
                }
                // Append the root directory if there is one
                *pDstPath = rootDirectoryView;
                // Append the relative path portion of the split path excluding the filename
                *pDstPath += relPathParentView;
            }
            if (pDstName)
            {
                AZStd::string_view stemView = pathView.Stem().Native();
                if (stemView.size() > pDstName->max_size())
                {
                    return false;
                }
                *pDstName = stemView;
            }
            if (pDstExtension)
            {
                AZStd::string_view extensionView = pathView.Extension().Native();
                if (extensionView.size() > pDstExtension->max_size())
                {
                    return false;
                }
                *pDstExtension = extensionView;
            }

            return true;
        }

        bool Join(const char* pFirstPart, const char* pSecondPart, AZStd::string& out, [[maybe_unused]] bool bCaseInsensitive, bool bNormalize)
        {
            if (!pFirstPart || !pSecondPart)
            {
                return false;
            }

            AZ::IO::PathView secondPath(pSecondPart);
            AZ_Warning("StringFunc", secondPath.IsRelative(), "The second join parameter %s is an absolute path,"
                " this will replace the first part of the path resulting in an output of just the second part",
                pSecondPart);

            AZ::IO::Path resultPath(pFirstPart);
            resultPath /= secondPath;
            out = bNormalize ? AZStd::move(resultPath.LexicallyNormal().Native()) : AZStd::move(resultPath.Native());
            return true;
        }

        bool Join(const char* pFirstPart, const char* pSecondPart, FixedString& out, [[maybe_unused]] bool bCaseInsensitive, bool bNormalize)
        {
            if (!pFirstPart || !pSecondPart)
            {
                return false;
            }

            AZ::IO::PathView secondPath(pSecondPart);
            AZ_Warning("StringFunc", secondPath.IsRelative(), "The second join parameter %s is an absolute path,"
                " this will replace the first part of the path resulting in an output of just the second part",
                pSecondPart);

            AZ::IO::FixedMaxPath resultPath(pFirstPart);
            resultPath /= secondPath;
            out = bNormalize ? AZStd::move(resultPath.LexicallyNormal().Native()) : AZStd::move(resultPath.Native());
            return true;
        }

        bool HasDrive(const char* in, bool bCheckAllFileSystemFormats /*= false*/)
        {
            // no drive if empty
            if (!in || in[0] == '\0')
            {
                return false;
            }
            AZ::IO::PathView pathView(in);
            return pathView.HasRootName() || (bCheckAllFileSystemFormats && pathView.HasRootDirectory());
        }

        bool HasExtension(const char* in)
        {
            //it doesn't have an extension if it's empty
            if (!in || in[0] == '\0')
            {
                return false;
            }

            return AZ::IO::PathView(in).HasExtension();
        }

        bool IsExtension(const char* in, const char* pExtension, bool bCaseInsenitive /*= false*/)
        {
            //it doesn't have an extension if it's empty
            if (!in || in[0] == '\0' || !pExtension || pExtension[0] == '\0')
            {
                return false;
            }

            AZStd::string_view pathExtension = AZ::IO::PathView(in).Extension().Native();
            if (pathExtension.starts_with(AZ_FILESYSTEM_EXTENSION_SEPARATOR))
            {
                pathExtension.remove_prefix(1);
            }
            AZStd::string_view extensionView(pExtension);
            if (extensionView.starts_with(AZ_FILESYSTEM_EXTENSION_SEPARATOR))
            {
                extensionView.remove_prefix(1);
            }

            return AZStd::equal(pathExtension.begin(), pathExtension.end(), extensionView.begin(), extensionView.end(),
                [bCaseInsenitive](const char lhs, const char rhs)
            {
                return !bCaseInsenitive ? lhs == rhs : tolower(lhs) == tolower(rhs);
            });
        }

        bool IsRelative(const char* in)
        {
            //not relative if empty
            if (!in || in[0] == '\0')
            {
                return false;
            }

            return AZ::IO::PathView(in).IsRelative();
        }

        bool StripDrive(AZStd::string& inout)
        {
            AZ::IO::PathView pathView(inout);
            AZ::IO::PathView rootNameView(pathView.RootName());
            if (!rootNameView.empty())
            {
                inout.replace(0, rootNameView.Native().size(), "");
                return true;
            }
            return false;
        }

        void StripPath(AZStd::string& inout)
        {
            inout = AZ::IO::PathView(inout).Filename().Native();
        }

        void StripFullName(AZStd::string& inout)
        {
            inout = AZ::IO::Path(AZStd::move(inout)).RemoveFilename().Native();
        }

        void StripExtension(AZStd::string& inout)
        {
            AZ::IO::Path path(AZStd::move(inout));
            path.ReplaceExtension();
            inout = AZStd::move(path.Native());
        }

        bool StripComponent(AZStd::string& inout, bool bLastComponent /* = false*/)
        {
            AZ::IO::PathView pathView(inout);
            auto pathBeginIter = pathView.begin();
            auto pathEndIter = pathView.end();
            if (pathBeginIter == pathEndIter)
            {
                return false;
            }
            AZ::IO::Path resultPath;
            if (!bLastComponent)
            {
                // Removing leading path component
                AZStd::advance(pathBeginIter, 1);
            }
            else
            {
                // Remove trailing path component
                AZStd::advance(pathEndIter, -1);
            }
            for (; pathBeginIter != pathEndIter; ++pathBeginIter)
            {
                resultPath /= *pathBeginIter;
            }
            if (resultPath.empty())
            {
                return false;
            }
            inout = AZStd::move(resultPath.Native());
            return true;
        }

        bool GetDrive(const char* in, AZStd::string& out)
        {
            if (!in || in[0] == '\0')
            {
                return false;
            }

            out = AZ::IO::PathView(in).RootName().Native();
            return !out.empty();
        }

        AZStd::optional<AZStd::string_view> GetParentDir(AZStd::string_view path)
        {
            if (path.empty())
            {
                return {};
            }

            AZStd::string_view parentDir = AZ::IO::PathView(path).ParentPath().Native();
            return !parentDir.empty() ? AZStd::make_optional<AZStd::string_view>(parentDir) : AZStd::nullopt;
        }

        bool GetFullPath(const char* in, AZStd::string& out)
        {
            if (!in || in[0] == '\0')
            {
                return false;
            }

            out = AZ::IO::PathView(in).ParentPath().Native();
            return !out.empty();
        }

        bool GetFolderPath(const char* in, AZStd::string& out)
        {
            return GetFullPath(in, out);
        }

        bool GetFolder(const char* in, AZStd::string& out, bool bFirst /* = false*/)
        {
            if (!in || in[0] == '\0')
            {
                return false;
            }

            if (!bFirst)
            {
                out = AZ::IO::PathView(in).ParentPath().Filename().Native();
                return !out.empty();
            }
            else
            {
                AZStd::string_view relativePath = AZ::IO::PathView(in).RelativePath().Native();
                size_t nextSeparator = relativePath.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                out = nextSeparator != AZStd::string_view::npos ? relativePath.substr(0, nextSeparator) : relativePath;
                return !out.empty();
            }
        }

        bool GetFullFileName(const char* in, AZStd::string& out)
        {
            if (!in || in[0] == '\0')
            {
                return false;
            }

            out = AZ::IO::PathView(in).Filename().Native();
            return !out.empty();
        }

        bool GetFileName(const char* in, AZStd::string& out)
        {
            if (!in || in[0] == '\0')
            {
                return false;
            }

            out = AZ::IO::PathView(in).Stem().Native();
            return !out.empty();
        }

        bool GetExtension(const char* in, AZStd::string& out, bool includeDot)
        {
            if (!in || in[0] == '\0')
            {
                return false;
            }

            AZStd::string_view extensionView = AZ::IO::PathView(in).Extension().Native();
            // PathView returns extensions with the <dot> character, so remove the <dot>
            // if it is not included
            if (!includeDot && extensionView.starts_with(AZ_FILESYSTEM_EXTENSION_SEPARATOR))
            {
                extensionView.remove_prefix(1);
            }
            out = extensionView;
            return !out.empty();
        }

        void ReplaceFullName(AZStd::string& inout, const char* pFileName /* = nullptr*/, const char* pFileExtension /* = nullptr*/)
        {
            //strip the full file name if it has one
            AZ::IO::Path path(AZStd::move(inout));
            path.RemoveFilename();
            if (pFileName)
            {
                path /= pFileName;
            }
            if (pFileExtension)
            {
                path.ReplaceExtension(pFileExtension);
            }
            inout = AZStd::move(path.Native());
        }

        void ReplaceExtension(AZStd::string& inout, const char* newExtension /* = nullptr*/)
        {
            //treat this as a strip
            if (!newExtension || newExtension[0] == '\0')
            {
                return;
            }
            AZ::IO::Path path(AZStd::move(inout));
            path.ReplaceExtension(newExtension);
            inout = AZStd::move(path.Native());
        }

        AZStd::string& AppendSeparator(AZStd::string& inout)
        {
            if (inout.ends_with(AZ_WRONG_FILESYSTEM_SEPARATOR))
            {
                inout.replace(inout.end() - 1, inout.end(), 1, AZ_CORRECT_FILESYSTEM_SEPARATOR);
            }
            else if (!inout.ends_with(AZ_CORRECT_FILESYSTEM_SEPARATOR))
            {
                inout.append(1, AZ_CORRECT_FILESYSTEM_SEPARATOR);
            }
            return inout;
        }
    } // namespace Path

    namespace Json
    {
        /*
        According to http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf:
        A string is a sequence of Unicode code points wrapped with quotation marks (U+0022). All characters may be
        placed within the quotation marks except for the characters that must be escaped: quotation mark (U+0022),
        reverse solidus (U+005C), and the control characters U+0000 to U+001F.
        */
        AZStd::string& ToEscapedString(AZStd::string& inout)
        {
            size_t strSize = inout.size();

            for (size_t i = 0; i < strSize; ++i)
            {
                char character = inout[i];

                // defaults to 1 if it hits any cases except default
                size_t jumpChar = 1;
                switch (character)
                {
                case '"':
                    inout.insert(i, "\\");
                    break;

                case '\\':
                    inout.insert(i, "\\");
                    break;

                case '/':
                    inout.insert(i, "\\");
                    break;

                case '\b':
                    inout.replace(i, i + 1, "\\b");
                    break;

                case '\f':
                    inout.replace(i, i + 1, "\\f");
                    break;

                case '\n':
                    inout.replace(i, i + 1, "\\n");
                    break;

                case '\r':
                    inout.replace(i, i + 1, "\\r");
                    break;

                case '\t':
                    inout.replace(i, i + 1, "\\t");
                    break;

                default:
                    /*
                    Control characters U+0000 to U+001F may be represented as a six - character sequence : a reverse solidus,
                    followed by the lowercase letter u, followed by four hexadecimal digits that encode the code point.
                    */
                    if (character >= '\x0000' && character <= '\x001f')
                    {
                        // jumping "\uXXXX" characters
                        jumpChar = 6;

                        AZStd::string hexStr = AZStd::string::format("\\u%04x", static_cast<int>(character));
                        inout.replace(i, i + 1, hexStr);
                    }
                    else
                    {
                        jumpChar = 0;
                    }
                }

                i += jumpChar;
                strSize += jumpChar;
            }

            return inout;
        }
    } // namespace Json

    namespace Base64
    {
        static const char base64pad = '=';

        static const char c_base64Table[] =
        {
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/"
        };

        static const AZ::u8 c_inverseBase64Table[] =
        {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
            0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
            0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
            0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
        };

        bool IsValidEncodedChar(const char encodedChar)
        {
            return c_inverseBase64Table[static_cast<size_t>(encodedChar)] != 0xff;
        }

        AZStd::string Encode(const AZ::u8* in, const size_t size)
        {
            /*
            figure retrieved from the Base encoding rfc https://tools.ietf.org/html/rfc4648
            +--first octet--+-second octet--+--third octet--+
            |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
            +-----------+---+-------+-------+---+-----------+
            |5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|
            +--1.index--+--2.index--+--3.index--+--4.index--+
            */
            AZStd::string result;

            const size_t remainder = size % 3;
            const size_t alignEndSize = size - remainder;
            const AZ::u8* encodeBuf = in;
            size_t encodeIndex = 0;
            for (; encodeIndex < alignEndSize; encodeIndex += 3)
            {
                encodeBuf = &in[encodeIndex];

                result.push_back(c_base64Table[(encodeBuf[0] & 0xfc) >> 2]);
                result.push_back(c_base64Table[((encodeBuf[0] & 0x03) << 4) | ((encodeBuf[1] & 0xf0) >> 4)]);
                result.push_back(c_base64Table[((encodeBuf[1] & 0x0f) << 2) | ((encodeBuf[2] & 0xc0) >> 6)]);
                result.push_back(c_base64Table[encodeBuf[2] & 0x3f]);
            }

            encodeBuf = &in[encodeIndex];
            if (remainder == 2)
            {
                result.push_back(c_base64Table[(encodeBuf[0] & 0xfc) >> 2]);
                result.push_back(c_base64Table[((encodeBuf[0] & 0x03) << 4) | ((encodeBuf[1] & 0xf0) >> 4)]);
                result.push_back(c_base64Table[((encodeBuf[1] & 0x0f) << 2)]);
                result.push_back(base64pad);
            }
            else if (remainder == 1)
            {
                result.push_back(c_base64Table[(encodeBuf[0] & 0xfc) >> 2]);
                result.push_back(c_base64Table[(encodeBuf[0] & 0x03) << 4]);
                result.push_back(base64pad);
                result.push_back(base64pad);
            }

            return result;
        }

        bool Decode(AZStd::vector<AZ::u8>& out, const char* in, const size_t size)
        {
            if (size % 4 != 0)
            {
                AZ_Warning("StringFunc", size % 4 == 0, "Base 64 encoded data length must be multiple of 4");
                return false;
            }

            AZStd::vector<AZ::u8> result;
            result.reserve(size * 3 / 4);
            const char* decodeBuf = in;
            size_t decodeIndex = 0;
            for (; decodeIndex < size; decodeIndex += 4)
            {
                decodeBuf = &in[decodeIndex];
                //Check if each character is a valid Base64 encoded character
                {
                    // First Octet
                    if (!IsValidEncodedChar(decodeBuf[0]))
                    {
                        AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[0]));
                        return false;
                    }
                    if (!IsValidEncodedChar(decodeBuf[1]))
                    {
                        AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[1]));
                        return false;
                    }

                    result.push_back((c_inverseBase64Table[static_cast<size_t>(decodeBuf[0])] << 2) | ((c_inverseBase64Table[static_cast<size_t>(decodeBuf[1])] & 0x30) >> 4));
                }

                {
                    // Second Octet
                    if (decodeBuf[2] == base64pad)
                    {
                        break;
                    }

                    if (!IsValidEncodedChar(decodeBuf[2]))
                    {
                        AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[2]));
                        return false;
                    }

                    result.push_back(((c_inverseBase64Table[static_cast<size_t>(decodeBuf[1])] & 0x0f) << 4) | ((c_inverseBase64Table[static_cast<size_t>(decodeBuf[2])] & 0x3c) >> 2));
                }

                {
                    // Third Octet
                    if (decodeBuf[3] == base64pad)
                    {
                        break;
                    }

                    if (!IsValidEncodedChar(decodeBuf[3]))
                    {
                        AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[3]));
                        return false;
                    }

                    result.push_back(((c_inverseBase64Table[static_cast<size_t>(decodeBuf[2])] & 0x03) << 6) | (c_inverseBase64Table[static_cast<size_t>(decodeBuf[3])] & 0x3f));
                }
            }

            out = AZStd::move(result);
            return true;
        }
    }

    namespace Utf8
    {
        bool CheckNonAsciiChar(const AZStd::string& in)
        {
            for (int i = 0; i < in.length(); ++i)
            {
                char byte = in[i];
                if (byte & 0x80)
                {
                    return true;
                }
            }
            return false;
        }
    }
} // namespace AZ::StringFunc

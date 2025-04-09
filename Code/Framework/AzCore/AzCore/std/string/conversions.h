/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/Serialization/Locale.h>

#include <ctype.h>
#include <wctype.h>
#include <locale>

//////////////////////////////////////////////////////////////////////////
// utf8 cpp lib
#include <AzCore/std/string/utf8/unchecked.h>
//////////////////////////////////////////////////////////////////////////



namespace AZStd
{
    namespace Internal
    {
        inline bool IsValidUtf8(const char* str, size_t length)
        {
            return Utf8::is_valid(str, str + length);
        }

        template<size_t Size = sizeof(wchar_t)>
        struct WCharTPlatformConverter
        {
            static_assert(Size == size_t{ 2 } || Size == size_t{ 4 }, "only wchar_t types of size 2 or 4 can be converted to utf8");

            template<class Allocator>
            static inline void to_string(AZStd::basic_string<string::value_type, string::traits_type, Allocator>& dest, AZStd::wstring_view src)
            {
                if constexpr (Size == 2)
                {
                    Utf8::Unchecked::utf16to8(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                }
                else if constexpr (Size == 4)
                {
                    Utf8::Unchecked::utf32to8(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                }
            }

            template<size_t MaxElementCount>
            static inline void to_string(AZStd::basic_fixed_string<string::value_type, MaxElementCount, string::traits_type>& dest, AZStd::wstring_view src)
            {
                if constexpr (Size == 2)
                {
                    Utf8::Unchecked::utf16to8(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                }
                else if constexpr (Size == 4)
                {
                    Utf8::Unchecked::utf32to8(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                }
            }

            static inline char* to_string(char* dest, size_t destSize, AZStd::wstring_view src)
            {
                if constexpr (Size == 2)
                {
                    return Utf8::Unchecked::utf16to8(src.begin(), src.end(), dest, destSize);
                }
                else if constexpr (Size == 4)
                {
                    return Utf8::Unchecked::utf32to8(src.begin(), src.end(), dest, destSize);
                }
            }

            static inline size_t to_string_length(AZStd::wstring_view src)
            {
                if constexpr (Size == 2)
                {
                    return Utf8::Unchecked::utf16ToUtf8BytesRequired(src.begin(), src.end());
                }
                else if constexpr (Size == 4)
                {
                    return Utf8::Unchecked::utf32ToUtf8BytesRequired(src.begin(), src.end());
                }
            }

            template<class Allocator>
            static inline bool to_wstring(AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator>& dest, AZStd::string_view src)
            {
                if (!IsValidUtf8(src.begin(), src.size()))
                {
                    return false;
                }

                if constexpr (Size == 2)
                {
                    Utf8::Unchecked::utf8to16(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                    return true;
                }
                else if constexpr (Size == 4)
                {
                    Utf8::Unchecked::utf8to32(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                    return true;
                }
            }

            // note that this template is only ever instantiated on size 2 and 4 so adding anything after the if statements will result in unreachable code.
            template<size_t MaxElementCount>
            static inline bool to_wstring(AZStd::basic_fixed_string<wstring::value_type, MaxElementCount, wstring::traits_type>& dest, AZStd::string_view src)
            {
                if (!IsValidUtf8(src.begin(), src.size()))
                {
                    return false;
                }

                if constexpr (Size == 2)
                {
                    Utf8::Unchecked::utf8to16(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                    return true;
                }
                else if constexpr (Size == 4)
                {
                    Utf8::Unchecked::utf8to32(src.begin(), src.end(), AZStd::back_inserter(dest), dest.max_size());
                    return true;
                }
            }

            static inline wchar_t* to_wstring(wchar_t* dest, size_t destSize, AZStd::string_view src)
            {
                if constexpr (Size == 2)
                {
                    return Utf8::Unchecked::utf8to16(src.begin(), src.end(), dest, destSize);
                }
                else if constexpr (Size == 4)
                {
                    return Utf8::Unchecked::utf8to32(src.begin(), src.end(), dest, destSize);
                }
            }
        };
    }
    // 21.5: numeric conversions
    template<class Allocator>
    int stoi(const AZStd::basic_string<AZStd::string::value_type, AZStd::string::traits_type, Allocator>& str, AZStd::size_t* idx = 0, int base = 10)
    {
        char* ptr;
        const char* sChar = str.c_str();
        int result = (int)strtol(sChar, &ptr, base);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    long stol(const AZStd::basic_string<AZStd::string::value_type, AZStd::string::traits_type, Allocator>& str, AZStd::size_t* idx = 0, int base = 10)
    {
        char* ptr;
        const char* sChar = str.c_str();
        long result = strtol(sChar, &ptr, base);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    unsigned long stoul(const AZStd::basic_string<AZStd::string::value_type, AZStd::string::traits_type, Allocator>& str, AZStd::size_t* idx = 0, int base = 10)
    {
        char* ptr;
        const char* sChar = str.c_str();
        unsigned long result = strtoul(sChar, &ptr, base);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    long long stoll(const AZStd::basic_string<AZStd::string::value_type, AZStd::string::traits_type, Allocator>& str, size_t* idx = 0, int base = 10)
    {
        char* ptr;
        const char* sChar = str.c_str();
        long long result = strtoll(sChar, &ptr, base);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    unsigned long long stoull(const AZStd::basic_string<AZStd::string::value_type, AZStd::string::traits_type, Allocator>& str, size_t* idx = 0, int base = 10)
    {
        char* ptr;
        const char* sChar = str.c_str();
        unsigned long long result = strtoull(sChar, &ptr, base);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }

    template<class Allocator>
    float stof(const AZStd::basic_string<AZStd::string::value_type, AZStd::string::traits_type, Allocator>& str, AZStd::size_t* idx = 0)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // invariant locale for converting strings to values.

        char* ptr;
        const char* sChar = str.c_str();
        float result = (float)strtod(sChar, &ptr);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    double stod(const AZStd::basic_string<AZStd::string::value_type, AZStd::string::traits_type, Allocator>& str, AZStd::size_t* idx = 0)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // invariant locale for converting strings to values.
        char* ptr;
        const char* sChar = str.c_str();
        double result = strtod(sChar, &ptr);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }

    // Standard is messy when it comes to custom string. Let's say we have a string with different allocator ???
    // so we have our (custom) implementations here and wrappers so we are compatible with the standard.
    template<class Str>
    void to_string(Str& str, int value)
    {
        char buf[16];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%d", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, unsigned int value)
    {
        char buf[16];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%u", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, float value)
    {
        char buf[64];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%f", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, double value)
    {
        char buf[64];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%f", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, long value)
    {
        char buf[32];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%ld", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, unsigned long value)
    {
        char buf[32];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%lu", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, long long value)
    {
        char buf[32];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%lld", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, unsigned long long value)
    {
        char buf[32];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%llu", value);
        str = buf;
    }
    template<class Str>
    void to_string(Str& str, long double value)
    {
        char buf[128];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "%Lf", value);
        str = buf;
    }

    template<class Str, class BoolType>
    auto to_string(Str& str, BoolType value)
        -> enable_if_t<same_as<remove_cvref_t<BoolType>, bool>>
    {
        str = value ? "true" : "false";
    }

    inline AZStd::string to_string(int val)                 { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(unsigned int val)        { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(float val)               { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(double val)              { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(long val)                { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(unsigned long val)       { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(long long val)           { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(unsigned long long val)  { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(long double val)         { AZStd::string str; to_string(str, val); return str; }
    template<class BoolType>
    auto to_string(BoolType value) -> enable_if_t<same_as<remove_cvref_t<BoolType>, bool>, AZStd::string>
    {
        AZStd::string str;
        to_string(str, value);
        return str;
    }

    // In our engine we assume AZStd::string is Utf8 encoded!
    template<class Allocator>
    void to_string(AZStd::basic_string<string::value_type, string::traits_type, Allocator>& dest, AZStd::wstring_view src)
    {
        dest.clear();
        Internal::WCharTPlatformConverter<>::to_string(dest, src);
    }

    template<size_t MaxElementCount>
    void to_string(AZStd::basic_fixed_string<string::value_type, MaxElementCount, string::traits_type>& dest, AZStd::wstring_view src)
    {
        dest.clear();
        Internal::WCharTPlatformConverter<>::to_string(dest, src);
    }

    inline void to_string(char* dest, size_t destSize, AZStd::wstring_view src)
    {
        char* endStr = Internal::WCharTPlatformConverter<>::to_string(dest, destSize, src);
        if (endStr < (dest + destSize))
        {
            *endStr = '\0'; // null terminator
        }
    }

    inline size_t to_string_length(AZStd::wstring_view src)
    {
        return Internal::WCharTPlatformConverter<>::to_string_length(src);
    }

    template<class Allocator>
    int stoi(const AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator>& str, AZStd::size_t* idx = 0, int base = 10)
    {
        const wchar_t* sChar = str.c_str();
        wchar_t* ptr;

        int result = (int)wcstol(sChar, &ptr, base);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    long stol(const AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator>& str, AZStd::size_t* idx = 0, int base = 10)
    {
        const wchar_t* sChar = str.c_str();
        wchar_t* ptr;
        long result = wcstol(sChar, &ptr, base);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    unsigned long stoul(const AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator>& str, AZStd::size_t* idx = 0, int base = 10)
    {
        const wchar_t* sChar = str.c_str();
        wchar_t* ptr;
        unsigned long result = wcstoul(sChar, &ptr, base);

        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    /*long long stoll(const AZStd::basic_string<wstring::value_type,wstring::traits_type,Allocator>& str, size_t *idx = 0, int base = 10)
    {}*/
    /*unsigned long long stoull(const wstring& str, size_t *idx = 0, int base = 10)
    {

    }*/
    template<class Allocator>
    float stof(const AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator>& str, AZStd::size_t* idx = 0)
    {
        wchar_t* ptr;
        const wchar_t* sChar = str.c_str();
        float result = (float)wcstod(sChar, &ptr);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    template<class Allocator>
    double stod(const AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator>& str, AZStd::size_t* idx = 0)
    {
        wchar_t* ptr;
        const wchar_t* sChar = str.c_str();
        double result = wcstod(sChar, &ptr);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    /*long double stold(const AZStd::basic_string<wstring::value_type,wstring::traits_type,Allocator>& str, size_t *idx = 0)
    {
    }*/
    template<class Str>
    void to_wstring(Str& str, long long value)
    {
        wchar_t buf[64];
        azsnwprintf(buf, 64, L"%lld", value);
        str = buf;
    }
    template<class Str>
    void to_wstring(Str& str, unsigned long long value)
    {
        wchar_t buf[64];
        azsnwprintf(buf, 64, L"%llu", value);
        str = buf;
    }
    template<class Str>
    void to_wstring(Str& str, long double value)
    {
        wchar_t buf[64];
        azsnwprintf(buf, 64, L"%Lf", value);
        str = buf;
    }
    inline AZStd::wstring to_wstring(long long val)             { AZStd::wstring wstr; to_wstring(wstr, val); return wstr; }
    inline AZStd::wstring to_wstring(unsigned long long val)    { AZStd::wstring wstr; to_wstring(wstr, val); return wstr; }
    inline AZStd::wstring to_wstring(long double val)           { AZStd::wstring wstr; to_wstring(wstr, val); return wstr; }

    template<class Allocator>
    bool to_wstring(AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator>& dest, AZStd::string_view src)
    {
        dest.clear();
        if (!Internal::IsValidUtf8(src.begin(), src.size()))
        {
            return false;
        }
        Internal::WCharTPlatformConverter<>::to_wstring(dest, src);
        return true;
    }

    template<size_t MaxElementCount>
    bool to_wstring(AZStd::basic_fixed_string<wstring::value_type, MaxElementCount, wstring::traits_type>& dest, AZStd::string_view src)
    {
        dest.clear();
        if (!Internal::IsValidUtf8(src.begin(), src.size()))
        {
            return false;
        }
        Internal::WCharTPlatformConverter<>::to_wstring(dest, src);
        return true;
    }

    inline bool to_wstring(wchar_t* dest, size_t destSize, AZStd::string_view src)
    {
        if (!Internal::IsValidUtf8(src.begin(), src.size()))
        {
            return false;
        }
        wchar_t* endWStr = Internal::WCharTPlatformConverter<>::to_wstring(dest, destSize, src);
        if (endWStr < (dest + destSize))
        {
            *endWStr = '\0'; // null terminator
        }
        return true;
    }


    AZ_FORCE_INLINE size_t str_transform(char* destination, const char* source, size_t count)
    {
        return strxfrm(destination, source, count);
    }

    AZ_FORCE_INLINE size_t str_transform(wchar_t* destination, const wchar_t* source, size_t count)
    {
        return wcsxfrm(destination, source, count);
    }

    // Use the C++ standard character classification functions and avoid the issue with negative integer values
    template<class CharT> bool isalnum(CharT ch, const std::locale& loc = {})  { return std::isalnum(ch, loc); }
    template<class CharT> bool isalpha(CharT ch, const std::locale& loc = {})  { return std::isalpha(ch, loc); }
    template<class CharT> bool isblank(CharT ch, const std::locale& loc = {})  { return std::isalnum(ch, loc); }
    template<class CharT> bool iscntrl(CharT ch, const std::locale& loc = {})  { return std::iscntrl(ch, loc); }
    template<class CharT> bool isdigit(CharT ch, const std::locale& loc = {})  { return std::isdigit(ch, loc); }
    template<class CharT> bool isgraph(CharT ch, const std::locale& loc = {})  { return std::isgraph(ch, loc); }
    template<class CharT> bool islower(CharT ch, const std::locale& loc = {})  { return std::islower(ch, loc); }
    template<class CharT> bool isprint(CharT ch, const std::locale& loc = {})  { return std::isprint(ch, loc); }
    template<class CharT> bool ispunct(CharT ch, const std::locale& loc = {})  { return std::ispunct(ch, loc); }
    template<class CharT> bool isspace(CharT ch, const std::locale& loc = {})  { return std::isspace(ch, loc); }
    template<class CharT> bool isupper(CharT ch, const std::locale& loc = {})  { return std::isupper(ch, loc); }
    template<class CharT> bool isxdigit(CharT ch, const std::locale& loc = {}) { return std::isxdigit(ch, loc); }

    template<class CharT> bool is_alnum(CharT ch, const std::locale& loc = {})  { return std::isalnum(ch, loc); }
    template<class CharT> bool is_alpha(CharT ch, const std::locale& loc = {})  { return std::isalpha(ch, loc); }
    template<class CharT> bool is_blank(CharT ch, const std::locale& loc = {})  { return std::isalnum(ch, loc); }
    template<class CharT> bool is_cntrl(CharT ch, const std::locale& loc = {})  { return std::iscntrl(ch, loc); }
    template<class CharT> bool is_digit(CharT ch, const std::locale& loc = {})  { return std::isdigit(ch, loc); }
    template<class CharT> bool is_graph(CharT ch, const std::locale& loc = {})  { return std::isgraph(ch, loc); }
    template<class CharT> bool is_lower(CharT ch, const std::locale& loc = {})  { return std::islower(ch, loc); }
    template<class CharT> bool is_print(CharT ch, const std::locale& loc = {})  { return std::isprint(ch, loc); }
    template<class CharT> bool is_punct(CharT ch, const std::locale& loc = {})  { return std::ispunct(ch, loc); }
    template<class CharT> bool is_space(CharT ch, const std::locale& loc = {})  { return std::isspace(ch, loc); }
    template<class CharT> bool is_upper(CharT ch, const std::locale& loc = {})  { return std::isupper(ch, loc); }
    template<class CharT> bool is_xdigit(CharT ch, const std::locale& loc = {}) { return std::isxdigit(ch, loc); }
    // Wrap the std::locale tolower and toupper functions with a default std::locale argument
    template<class charT>
    charT tolower(charT ch, const std::locale& loc = {})
    {
        return std::tolower(ch , loc);
    }

    template<class charT>
    charT toupper(charT ch, const std::locale& loc = {})
    {
        return std::toupper(ch , loc);
    }


    // Convert a range of chars to lower case
    template<class Iterator>
    void to_lower(Iterator first, Iterator last, const std::locale& loc = {})
    {
        for (; first != last; ++first)
        {
            *first = std::tolower(*first, loc);
        }
    }

    template<class Range>
    auto to_lower(Range&& r, const std::locale& loc = {})
        -> enable_if_t<ranges::range<Range>
            && indirectly_copyable<ranges::iterator_t<Range>, ranges::iterator_t<Range>>>
    {
        to_lower(ranges::begin(r), ranges::end(r), loc);
    }

    // Convert a range of chars to upper case
    template<class Iterator>
    void to_upper(Iterator first, Iterator last, const std::locale& loc = {})
    {
        for (; first != last; ++first)
        {
            *first = std::toupper(*first, loc);
        }
    }

    template<class Range>
    auto to_upper(Range&& r, const std::locale& loc = {})
        -> enable_if_t<ranges::range<Range>
            && indirectly_copyable<ranges::iterator_t<Range>, ranges::iterator_t<Range>>>
    {
        to_upper(ranges::begin(r), ranges::end(r), loc);
    }
}

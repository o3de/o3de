/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

#include <ctype.h>
#include <wctype.h>

//////////////////////////////////////////////////////////////////////////
// utf8 cpp lib
#include <AzCore/std/string/utf8/unchecked.h>
//////////////////////////////////////////////////////////////////////////

# define AZSTD_USE_OLD_RW_STL

#if !defined(AZSTD_USE_OLD_RW_STL)
#   include <locale>
#endif

namespace AZStd
{
    namespace Internal
    {
        template<size_t Size = sizeof(wchar_t)>
        struct WCharTPlatformConverter
        {
            template<class Allocator1>
            static inline void to_string(AZStd::basic_string<string::value_type, string::traits_type, Allocator1>& dest, const wchar_t* first, const wchar_t* last)
            {
                if constexpr (Size == 2)
                {
                    Utf8::Unchecked::utf16to8(first, last, AZStd::back_inserter(dest));
                }
                else if constexpr (Size == 4)
                {
                    Utf8::Unchecked::utf32to8(first, last, AZStd::back_inserter(dest));
                }
                else
                {
                    // Workaround to defer static_assert evaluation until this function is invoked by using the template parameter
                    using StringType = AZStd::basic_string<string::value_type, string::traits_type, Allocator1>;
                    static_assert(!AZStd::is_same_v<StringType, StringType>, "only wchar_t types of size 2 or 4 can be converted to utf8");
                }
            }

            template<class Allocator1>
            static inline void to_wstring(AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator1>& dest, const char* first, const char* last)
            {
                if constexpr (Size == 2)
                {
                    Utf8::Unchecked::utf8to16(first, last, AZStd::back_inserter(dest));
                }
                else if constexpr (Size == 4)
                {
                    Utf8::Unchecked::utf8to32(first, last, AZStd::back_inserter(dest));
                }
                else
                {
                    // Workaround to defer static_assert evaluation until this function is invoked by using the template parameter
                    using StringType = AZStd::basic_string<string::value_type, string::traits_type, Allocator1>;
                    static_assert(!AZStd::is_same_v<StringType, StringType>, "Cannot convert a utf8 string to a wchar_t that isn't size 2 or 4");
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
        char* ptr;
        const char* sChar = str.c_str();
        double result = strtod(sChar, &ptr);
        if (idx)
        {
            *idx = ptr - sChar;
        }
        return result;
    }
    /*
    template<class Allocator>
    long double stold(const AZStd::basic_string<string::value_type,string::traits_type,Allocator>& str, size_t *idx = 0)
    {
        char* ptr;
        const char * sChar = str.c_str();
        long double result = strtold(sChar,&ptr);
        if(idx)
            *idx = ptr - sChar;
        return result;
    }*/

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

    inline AZStd::string to_string(int val)                 { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(unsigned int val)        { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(float val)               { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(double val)              { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(long val)                { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(unsigned long val)       { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(long long val)           { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(unsigned long long val)  { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(long double val)         { AZStd::string str; to_string(str, val); return str; }

    // In our engine we assume AZStd::string is Utf8 encoded!
    template<class Allocator1>
    void to_string(AZStd::basic_string<string::value_type, string::traits_type, Allocator1>& dest, const wchar_t* str, size_t srcLen = 0)
    {
        dest.clear();

        if (srcLen == 0)
        {
            srcLen = wcslen(str);
        }

        if (srcLen > 0)
        {
            Internal::WCharTPlatformConverter<>::to_string(dest, str, str + srcLen);
        }
    }

    template<class Allocator1, class Allocator2>
    void to_string(AZStd::basic_string<string::value_type, string::traits_type, Allocator1>& dest, const AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator2>& src)
    {
        return to_string(dest, src.c_str(), src.length());
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

    template<class Allocator1>
    void to_wstring(AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator1>& dest, const char* str, size_t strLen = 0)
    {
        dest.clear();

        if (strLen == 0)
        {
            strLen = strlen(str);
        }

        if (strLen > 0)
        {
            Internal::WCharTPlatformConverter<>::to_wstring(dest, str, str + strLen);
        }
    }

    template<class Allocator1, class Allocator2>
    void to_wstring(AZStd::basic_string<wstring::value_type, wstring::traits_type, Allocator1>& dest, const AZStd::basic_string<string::value_type, string::traits_type, Allocator2>& src)
    {
        return to_wstring(dest, src.c_str(), src.length());
    }

    // Convert a range of chars to lower case
#if defined(AZSTD_USE_OLD_RW_STL)
    template<class Iterator>
    void to_lower(Iterator first, Iterator last)
    {
        for (; first != last; ++first)
        {
            *first = static_cast<typename AZStd::iterator_traits<Iterator>::value_type>(tolower(*first));
        }
    }

    // Convert a range of chars to upper case
    template<class Iterator>
    void to_upper(Iterator first, Iterator last)
    {
        for (; first != last; ++first)
        {
            *first = static_cast<typename AZStd::iterator_traits<Iterator>::value_type>(toupper(*first));
        }
    }

    AZ_FORCE_INLINE size_t str_transform(char* destination, const char* source, size_t count)
    {
        return strxfrm(destination, source, count);
    }

    AZ_FORCE_INLINE size_t str_transform(wchar_t* destination, const wchar_t* source, size_t count)
    {
        return wcsxfrm(destination, source, count);
    }

    // C standard requires that is_alpha and all the other below functions should be passed a positive integer
    // (it will error if a negative value is passed in such as -128), which is what happens if you pass a char
    // from the extended character set, without first converting it to an unsigned char, because it will
    // automatically convert it to an int (-128) instead of to its ascii value (127), which is what these functions expect.
    AZ_FORCE_INLINE bool is_alnum(char ch)          { return isalnum((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_alpha(char ch)          { return isalpha((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_cntrl(char ch)          { return iscntrl((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_digit(char ch)          { return isdigit((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_graph(char ch)          { return isgraph((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_lower(char ch)          { return islower((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_print(char ch)          { return isprint((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_punct(char ch)          { return ispunct((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_space(char ch)          { return isspace((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_upper(char ch)          { return isupper((unsigned char)ch) != 0; }
    AZ_FORCE_INLINE bool is_xdigit(char ch)         { return isxdigit((unsigned char)ch) != 0; }

    AZ_FORCE_INLINE bool is_alnum(wchar_t ch)       { return iswalnum(ch) != 0; }
    AZ_FORCE_INLINE bool is_alpha(wchar_t ch)       { return iswalpha(ch) != 0; }
    AZ_FORCE_INLINE bool is_cntrl(wchar_t ch)       { return iswcntrl(ch) != 0; }
    AZ_FORCE_INLINE bool is_digit(wchar_t ch)       { return iswdigit(ch) != 0; }
    AZ_FORCE_INLINE bool is_graph(wchar_t ch)       { return iswgraph(ch) != 0; }
    AZ_FORCE_INLINE bool is_lower(wchar_t ch)       { return iswlower(ch) != 0; }
    AZ_FORCE_INLINE bool is_print(wchar_t ch)       { return iswprint(ch) != 0; }
    AZ_FORCE_INLINE bool is_punct(wchar_t ch)       { return iswpunct(ch) != 0; }
    AZ_FORCE_INLINE bool is_space(wchar_t ch)       { return iswspace(ch) != 0; }
    AZ_FORCE_INLINE bool is_upper(wchar_t ch)       { return iswupper(ch) != 0; }
    AZ_FORCE_INLINE bool is_xdigit(wchar_t ch)      { return iswxdigit(ch) != 0; }

#else // default standard implementation
    template<class Iterator>
    void to_lower(Iterator first, Iterator last, const std::locale& loc = std::locale())
    {
        for (; first != last; ++first)
        {
            *first = std::tolower(*first, loc);
        }
    }

    // Convert a range of chars to upper case
    template<class Iterator>
    void to_upper(Iterator first, Iterator last, const std::locale& loc = std::locale())
    {
        for (; first != last; ++first)
        {
            *first = std::toupper(*first, loc);
        }
    }
#endif
    // Add case insensitive compares
}

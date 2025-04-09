/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/exceptions.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/limits.h>
#include <AzCore/Memory/SystemAllocator.h>


// used for std::pointer_traits \note do an AZStd version
#include <memory>

 #ifndef AZ_REGEX_MAX_COMPLEXITY_COUNT
  #define AZ_REGEX_MAX_COMPLEXITY_COUNT 10000000L   /* set to 0 to disable */
 #endif /* AZ_REGEX_MAX_COMPLEXITY_COUNT */

 #ifndef AZ_REGEX_MAX_STACK_COUNT
  #define AZ_REGEX_MAX_STACK_COUNT  1000L   /* set to 0 to disable */
 #endif /* AZ_REGEX_MAX_STACK_COUNT */

/**
 * Cx11 Regular expressions based on STL from manufacturers. All allocations are piped through the system allocator by default.
 * we don't use std::locale/std::facet/std::collate/etc. as the facets do allocate global memory which is freed atexit(). This of course
 * is not compliant with our allocation schema.
 */
namespace AZStd
{
  #define AZ_REGEX_DIFFT(iter)  typename iterator_traits<iter>::difference_type
  #define AZ_REGEX_VALT(iter)   typename iterator_traits<iter>::value_type

    // NAMED CONSTANTS
    enum MetaType
    {   // meta character representations for parser
        Meta_lpar = '(',
        Meta_rpar = ')',
        Meta_dlr = '$',
        Meta_caret = '^',
        Meta_dot = '.',
        Meta_star = '*',
        Meta_plus = '+',
        Meta_query = '?',
        Meta_lsq = '[',
        Meta_rsq = ']',
        Meta_bar = '|',
        Meta_esc = '\\',
        Meta_dash = '-',
        Meta_lbr = '{',
        Meta_rbr = '}',
        Meta_comma = ',',
        Meta_colon = ':',
        Meta_equal = '=',
        Meta_exc = '!',
        Meta_eos = -1,
        Meta_nl = '\n',
        Meta_cr = '\r',
        Meta_bsp = '\b',
        Meta_chr = 0,

        Esc_bsl = '\\',
        Esc_word = 'b',
        Esc_not_word = 'B',
        Esc_ctrl_a = 'a',
        Esc_ctrl_b = 'b',
        Esc_ctrl_f = 'f',
        Esc_ctrl_n = 'n',
        Esc_ctrl_r = 'r',
        Esc_ctrl_t = 't',
        Esc_ctrl_v = 'v',
        Esc_ctrl = 'c',
        Esc_hex = 'x',
        Esc_uni = 'u'
    };

    // NAMESPACE regex_constants
    namespace regex_constants
    {
        // constants used in regular expressions
        enum syntax_option_type
        {   // specify RE syntax rules
            ECMAScript = 0x01,
            basic = 0x02,
            extended = 0x04,
            awk = 0x08,
            grep = 0x10,
            egrep = 0x20,
            gmask = 0x3F,

            icase = 0x0100,
            nosubs = 0x0200,
            optimize = 0x0400,
            collate = 0x0800,
            invalid = 0,
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(syntax_option_type)

        enum match_flag_type
        {   // specify matching and formatting rules
            match_default = 0x0000,
            match_not_bol = 0x0001,
            match_not_eol = 0x0002,
            match_not_bow = 0x0004,
            match_not_eow = 0x0008,
            match_any = 0x0010,
            match_not_null = 0x0020,
            match_continuous = 0x0040,
            match_partial = 0x0080,
            match_prev_avail = 0x0100,
            format_default = 0x0000,
            format_sed = 0x0400,
            format_no_copy = 0x0800,
            format_first_only = 0x1000,
            match_not_null1 = 0x2000
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(match_flag_type)

        enum error_type
        {   // identify error
            error_collate,
            error_ctype,
            error_escape,
            error_backref,
            error_brack,
            error_paren,
            error_brace,
            error_badbrace,
            error_range,
            error_space,
            error_badrepeat,
            error_complexity,
            error_stack,
            error_parse,
            error_syntax
        };
    }   // namespace regex_constants

    namespace Internal
    {
        inline const char* RegexError(regex_constants::error_type code)
        {
            const char* stringError = "regex_error";
            switch (code)
            {   // select known error_type message
            case regex_constants::error_collate:
                stringError = ("regex_error(error_collate): The expression contained an invalid collating element name.");
                break;
            case regex_constants::error_ctype:
                stringError = ("regex_error(error_ctype): The expression contained an invalid character class name.");
                break;
            case regex_constants::error_escape:
                stringError = ("regex_error(error_escape): The expression contained an invalid escaped character, " "or a trailing escape.");
                break;
            case regex_constants::error_backref:
                stringError = ("regex_error(error_backref): The expression contained an invalid back reference.");
                break;
            case regex_constants::error_brack:
                stringError = ("regex_error(error_brack): The expression contained mismatched [ and ].");
                break;
            case regex_constants::error_paren:
                stringError = ("regex_error(error_paren): The expression contained mismatched ( and ).");
                break;
            case regex_constants::error_brace:
                stringError = ("regex_error(error_brace): The expression contained mismatched { and }.");
                break;
            case regex_constants::error_badbrace:
                stringError = ("regex_error(error_badbrace): The expression contained an invalid range in a { expression }.");
                break;
            case regex_constants::error_range:
                stringError = ("regex_error(error_range): The expression contained an invalid character range, such as [b-a] in most encodings.");
                break;
            case regex_constants::error_space:
                stringError = ("regex_error(error_space): There was insufficient memory to convert the expression into a finite state machine.");
                break;
            case regex_constants::error_badrepeat:
                stringError = ("regex_error(error_badrepeat): One of *?+{ was not preceded by a valid regular expression.");
                break;
            case regex_constants::error_complexity:
                stringError = ("regex_error(error_complexity): The complexity of an attempted match against a regular expression exceeded a pre-set level.");
                break;
            case regex_constants::error_stack:
                stringError = ("regex_error(error_stack): There was insufficient memory to determine whether the regular expression could match the specified character sequence.");
                break;
            case regex_constants::error_parse:
                stringError = ("regex_error(error_parse)");
                break;
            case regex_constants::error_syntax:
                stringError = ("regex_error(error_syntax)");
                break;
            default:
                break;
            }

            return stringError;
        }

        inline const char* MatchError(regex_constants::error_type code)
        {
            const char* stringError = RegexError(code);
            AZ_Assert(false, stringError);
            return stringError;
        }

        struct ErrorSink
        {
            virtual ~ErrorSink() = default;
            virtual void RegexError(regex_constants::error_type code) = 0;
        };
    }

    // TEMPLATE CLASS regex_traits
    template<class Element>
    class regex_traits;

    template<class RegExTraits>
    struct CmpCS
    {   // functor to compare two character values for equality
        typedef typename RegExTraits::char_type Element;
        // return true if equal
        bool operator()(Element e1, Element e2) { return (e1 == e2); }
    };

    template<class RegExTraits>
    struct CmpIcase
    {   // functor to compare for case-insensitive equality
        typedef typename RegExTraits::char_type Element;
        CmpIcase(const RegExTraits& traits)
            : m_traits(traits)
        {
        }

        // return true if equal
        bool operator()(Element e1, Element e2) {   return (m_traits.translate_nocase(e1) == m_traits.translate_nocase(e2)); }
        const RegExTraits& m_traits;
    private:
        CmpIcase& operator=(const CmpIcase&);
    };

    template<class RegExTraits>
    struct CmpCollate
    {   // functor to compare for locale-specific equality
        typedef typename RegExTraits::char_type Element;
        CmpCollate(const RegExTraits& traits)
            : m_traits(traits)
        {
        }
        // return true if equal
        bool operator()(Element e1, Element e2) { return (m_traits.translate(e1) == m_traits.translate(e2)); }
        const RegExTraits& m_traits;
    private:
        CmpCollate& operator=(const CmpCollate&);
    };

    template<class InputIterator1, class InputIterator2, class Predicate>
    bool IsSame(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, Predicate pred)
    {   // return true if two sequences match using pred
        while (first1 != last1 && first2 != last2)
        {
            if (!pred(*first1++, *first2++))
            {
                return (false);
            }
        }
        return (first1 == last1 && first2 == last2);
    }

    struct RegexTraitsBase
    {   // base of all regular expression traits
        enum CharClassType
        {
            Ch_none = 0,
            Ch_invalid = 0xffff,
            Ch_alnum = (1 << 0),// = std::ctype_base::alnum,
            Ch_alpha = (1 << 1),// = std::ctype_base::alpha,
            Ch_cntrl = (1 << 2),// = std::ctype_base::cntrl,
            Ch_digit = (1 << 3),// = std::ctype_base::digit,
            Ch_graph = (1 << 4),// = std::ctype_base::graph,
            Ch_lower = (1 << 5),// = std::ctype_base::lower,
            Ch_print = (1 << 6),// = std::ctype_base::print,
            Ch_punct = (1 << 7),// = std::ctype_base::punct,
            Ch_space = (1 << 8),// = std::ctype_base::space,
            Ch_upper = (1 << 9),// = std::ctype_base::upper,
            Ch_xdigit = (1 << 10),// = std::ctype_base::xdigit,
            //Ch_blank = std::ctype_base::blank
        };

        typedef short char_class_type;
    };

    template<class Element>
    struct ClassNames
    {   // structure to associate class name with mask value
        const Element* m_element;
        unsigned int m_length;
        int m_type;
    };

    template<class Element>
    class RegexTraits
        : public RegexTraitsBase
    {
    public:
        typedef RegexTraits<Element> this_type;
        typedef Element char_type;
        typedef size_t size_type;
        typedef typename AZStd::basic_string<Element> string_type;

        // return length of string
        static size_type length(const Element* string)
        {
            return char_traits<Element>::length(string);
        }

        // provide locale-sensitive mapping
        Element translate(Element ch) const
        {
            Element result;
            return (AZStd::str_transform(&result, &ch, 1) == 1) ? result : ch;
        }

        // provide case-insensitive mapping
        Element translate_nocase(Element ch) const  {   return static_cast<Element>(tolower(ch)); }

        template<class ForwardIterator>
        string_type transform(ForwardIterator first, ForwardIterator last) const
        {   // apply locale-specific transformation
            size_t length = last - first;
            string_type result(length);
            if (length)
            {
                AZStd::str_transform(result.data(), first, length);
            }
            return result;
        }

        template<class ForwardIterator>
        string_type transform_primary(ForwardIterator first, ForwardIterator last) const
        {   // apply locale-specific case-insensitive transformation
            string_type result;
            if (first != last)
            {   // non-empty string, transform it
                string_type temp(first, last);
                result.resize(last - first);
                AZStd::to_lower(temp.data(), temp.data() + temp.size());
                AZStd::str_transform(result.data(), temp.data(), temp.size());
            }
            return result;
        }

        // return true if ch is in character class fx
        bool isctype(Element ch, char_class_type fx) const
        {
            switch (fx)
            {
            case Ch_alnum:
                return AZStd::is_alnum(ch) != 0;
            case Ch_alpha:
                return AZStd::is_alpha(ch) != 0;
            case Ch_cntrl:
                return AZStd::is_cntrl(ch) != 0;
            case Ch_digit:
                return AZStd::is_digit(ch) != 0;
            case Ch_graph:
                return AZStd::is_graph(ch) != 0;
            case Ch_lower:
                return AZStd::is_lower(ch) != 0;
            case Ch_print:
                return AZStd::is_print(ch) != 0;
            case Ch_punct:
                return AZStd::is_punct(ch) != 0;
            case Ch_space:
                return AZStd::is_space(ch) != 0;
            case Ch_upper:
                return AZStd::is_upper(ch) != 0;
            case Ch_xdigit:
                return AZStd::is_xdigit(ch) != 0;
            default:
                return (ch == '_' || AZStd::is_alnum(ch) != 0);             // assumes L'_' == '_'
            }
        }

        template<class Iterator>
        char_class_type lookup_classname(Iterator first, Iterator last, bool ignoreCase = false) const
        {   // map [first, last) to character class mask value
            int index = 0;
            for (; m_names[index].m_element; ++index)
            {
                if (IsSame(first, last,
                        m_names[index].m_element, m_names[index].m_element + m_names[index].m_length,
                        CmpIcase<RegexTraits<Element> >(*this)))
                {
                    break;
                }
            }

            char_class_type mask = Ch_none;
            if (m_names[index].m_element != 0)
            {
                mask = (char_class_type)m_names[index].m_type;
            }
            if (ignoreCase && mask & (Ch_lower | Ch_upper))
            {
                mask |= Ch_lower | Ch_upper;
            }
            return mask;
        }

        template<class ForwardIterator>
        string_type lookup_collatename(ForwardIterator first, ForwardIterator last) const
        {   // map [first, last) to collation element
            return (string_type(first, last));
        }
    private:
        static const ClassNames<Element> m_names[];
    };

    // CLASS regex_traits<char>
    template<>
    class regex_traits<char>
        : public RegexTraits<char>
    {
    public:
        int value(char ch, int base) const
        {   // map character value to numeric value
            if ((base != 8 && '0' <= ch && ch <= '9') || (base == 8 && '0' <= ch && ch <= '7'))
            {
                return (ch - '0');
            }
            else if (base != 16)
            {
                ;
            }
            else if ('a' <= ch && ch <= 'f')
            {
                return (ch - 'a' + 10);
            }
            else if ('A' <= ch && ch <= 'F')
            {
                return (ch - 'A' + 10);
            }
            return (-1);
        }
    };

    // CLASS regex_traits<wchar_t>
    template<>
    class regex_traits<wchar_t>
        : public RegexTraits<wchar_t>
    {   // specialization for wchar_t
    public:
        int value(wchar_t ch, int base) const
        {   // map character value to numeric value
            if ((base != 8 && L'0' <= ch && ch <= L'9') || (base == 8 && L'0' <= ch && ch <= L'7'))
            {
                return (ch - L'0');
            }
            else if (base != 16)
            {
                ;
            }
            else if (L'a' <= ch && ch <= L'f')
            {
                return (ch - L'a' + 10);
            }
            else if (L'A' <= ch && ch <= L'F')
            {
                return (ch - L'A' + 10);
            }
            return (-1);
        }
    };

    // TEMPLATE CLASS sub_match
    template<class BidirectionalIterator>
    class sub_match
        : public pair<BidirectionalIterator, BidirectionalIterator>
    {   // class to hold contents of a capture group
    public:
        typedef AZ_REGEX_VALT (BidirectionalIterator) value_type;
        typedef AZ_REGEX_DIFFT (BidirectionalIterator) difference_type;
        typedef BidirectionalIterator iterator;
        typedef basic_string<value_type> string_type;

        sub_match()
            : matched(false)
        {
        }

        bool matched;

        difference_type length() const
        {       // return length of matched text
            return (matched ? distance(this->first, this->second) : 0);
        }

        operator string_type() const
        {       // convert matched text to string
            return (str());
        }

        string_type str() const
        {       // convert matched text to string
            return (matched ?
                    string_type(this->first, this->second)
                    : string_type());
        }

        int compare(const sub_match& right) const
        {       // compare *this to right
            return (str().compare(right.str()));
        }

        int compare(const string_type& right) const
        {       // compare *this to right
            return (str().compare(right));
        }

        int compare(const value_type* value) const
        {       // compare *this to array pointed to by value
            return (str().compare(value));
        }
    };

    typedef sub_match<const char*> csub_match;
    typedef sub_match<const wchar_t*> wcsub_match;
    typedef sub_match<string::const_iterator> ssub_match;
    typedef sub_match<wstring::const_iterator> wssub_match;

    template<class BidirectionalIterator>
    inline bool operator==(const sub_match<BidirectionalIterator>& left, const sub_match<BidirectionalIterator>& right) { return (left.compare(right) == 0); }

    template<class BidirectionalIterator>
    inline bool operator!=(const sub_match<BidirectionalIterator>& left, const sub_match<BidirectionalIterator>& right) { return (!(left == right)); }

    template<class BidirectionalIterator>
    inline bool operator<(const sub_match<BidirectionalIterator>& left, const sub_match<BidirectionalIterator>& right) {    return (left.compare(right) < 0); }

    template<class BidirectionalIterator>
    inline bool operator>(const sub_match<BidirectionalIterator>& left, const sub_match<BidirectionalIterator>& right) {    return (right < left); }

    template<class BidirectionalIterator>
    inline bool operator<=(const sub_match<BidirectionalIterator>& left, const sub_match<BidirectionalIterator>& right) { return (!(right < left)); }

    template<class BidirectionalIterator>
    inline bool operator>=(const sub_match<BidirectionalIterator>& left, const sub_match<BidirectionalIterator>& right) {   return (!(left < right)); }

    template<class BidirectionalIterator>
    inline bool operator==(const AZ_REGEX_VALT(BidirectionalIterator)* left, const sub_match<BidirectionalIterator>& right) { return (right.compare(left) == 0); }

    template<class BidirectionalIterator>
    inline bool operator!=(const AZ_REGEX_VALT(BidirectionalIterator)* left, const sub_match<BidirectionalIterator>& right) { return (!(left == right)); }

    template<class BidirectionalIterator>
    inline bool operator<(const AZ_REGEX_VALT(BidirectionalIterator)* left, const sub_match<BidirectionalIterator>& right) { return (0 < right.compare(left)); }

    template<class BidirectionalIterator>
    inline bool operator>(const AZ_REGEX_VALT(BidirectionalIterator)* left, const sub_match<BidirectionalIterator>& right)  { return (right < left); }

    template<class BidirectionalIterator>
    inline bool operator<=(const AZ_REGEX_VALT(BidirectionalIterator)* left, const sub_match<BidirectionalIterator>& right) { return (!(right < left)); }
    template<class BidirectionalIterator>
    inline bool operator>=(const AZ_REGEX_VALT(BidirectionalIterator)* left, const sub_match<BidirectionalIterator>& right)  {  return (!(left < right)); }

    template<class BidirectionalIterator>
    inline bool operator==(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)* right) { return (left.compare(right) == 0); }

    template<class BidirectionalIterator>
    inline bool operator!=(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)* right) { return (!(left == right)); }

    template<class BidirectionalIterator>
    inline bool operator<(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)* right) { return (left.compare(right) < 0); }

    template<class BidirectionalIterator>
    inline bool operator>(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)* right) { return (right < left); }

    template<class BidirectionalIterator>
    inline bool operator<=(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)* right) { return (!(right < left)); }

    template<class BidirectionalIterator>
    inline bool operator>=(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)* right) { return (!(left < right)); }

    template<class BidirectionalIterator>
    inline bool operator==(const AZ_REGEX_VALT(BidirectionalIterator)& left,    const sub_match<BidirectionalIterator>& right) { typename sub_match<BidirectionalIterator>::string_type string(1, left); return (right.compare(string) == 0);   }

    template<class BidirectionalIterator>
    inline bool operator!=(const AZ_REGEX_VALT(BidirectionalIterator)& left,    const sub_match<BidirectionalIterator>& right)  { return (!(left == right)); }

    template<class BidirectionalIterator>
    inline bool operator<(const AZ_REGEX_VALT(BidirectionalIterator)& left, const sub_match<BidirectionalIterator>& right)  { typename sub_match<BidirectionalIterator>::string_type string(1, left); return (0 < right.compare(string)); }

    template<class BidirectionalIterator>
    inline bool operator>(const AZ_REGEX_VALT(BidirectionalIterator)& left, const sub_match<BidirectionalIterator>& right)  { return (right < left); }

    template<class BidirectionalIterator>
    inline bool operator<=(const AZ_REGEX_VALT(BidirectionalIterator)& left,    const sub_match<BidirectionalIterator>& right)  { return (!(right < left)); }

    template<class BidirectionalIterator>
    inline bool operator>=(const AZ_REGEX_VALT(BidirectionalIterator)& left, const sub_match<BidirectionalIterator>& right) { return (!(left < right)); }

    template<class BidirectionalIterator>
    inline bool operator==(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)& right) { typename sub_match<BidirectionalIterator>::string_type string(1, right); return (left.compare(string) == 0); }

    template<class BidirectionalIterator>
    inline bool operator!=(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)& right) { return (!(left == right)); }

    template<class BidirectionalIterator>
    inline bool operator<(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)& right) { typename sub_match<BidirectionalIterator>::string_type string(1, right); return (left.compare(string) < 0); }

    template<class BidirectionalIterator>
    inline bool operator>(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)& right) { return (right < left); }

    template<class BidirectionalIterator>
    inline bool operator<=(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)& right) { return (!(right < left)); }

    template<class BidirectionalIterator>
    inline bool operator>=(const sub_match<BidirectionalIterator>& left, const AZ_REGEX_VALT(BidirectionalIterator)& right) { return (!(left < right)); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator==(const sub_match<BidirectionalIterator>& left, const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& right)    { return (left.compare(right.c_str()) == 0); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator!=(const sub_match<BidirectionalIterator>& left, const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& right)    { return (!(left == right)); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator<(const sub_match<BidirectionalIterator>& left, const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& right) { return (left.compare(right.c_str()) < 0); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline  bool operator>(const sub_match<BidirectionalIterator>& left, const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& right) { return (right < left); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator<=(const sub_match<BidirectionalIterator>& left, const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& right) { return (!(right < left)); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator>=(const sub_match<BidirectionalIterator>& left, const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& right) { return (!(left < right)); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator==(const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& left, const sub_match<BidirectionalIterator>& right) { return (right.compare(left.c_str()) == 0); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator!=(const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& left, const sub_match<BidirectionalIterator>& right) { return (!(left == right)); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator<(const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& left, const sub_match<BidirectionalIterator>& right) {   return (0 < right.compare(left.c_str())); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator>(const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& left, const sub_match<BidirectionalIterator>& right) {   return (right < left); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator<=(const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& left, const sub_match<BidirectionalIterator>& right) { return (!(right < left)); }

    template<class BidirectionalIterator, class Traits, class Allocator>
    inline bool operator>=(const basic_string<AZ_REGEX_VALT(BidirectionalIterator), Traits, Allocator>& left, const sub_match<BidirectionalIterator>& right) { return (!(left < right)); }

    // FORWARD DECLARATIONS
    template<class BidirectionalIterator, class Allocator = AZStd::allocator >
    class match_results;

    template<class BidirectionalIterator, class Allocator, class InputIterator, class OutputIterator>
    OutputIterator FormatDefault(const match_results<BidirectionalIterator, Allocator>& match, OutputIterator output, InputIterator first, InputIterator last,  regex_constants::match_flag_type flags = regex_constants::format_default);

    template<class BidirectionalIterator, class Allocator, class InputIterator, class OutputIterator>
    OutputIterator FormatSed(const match_results<BidirectionalIterator, Allocator>& match,  OutputIterator output, InputIterator first, InputIterator last, regex_constants::match_flag_type flags = regex_constants::format_default);

    // TEMPLATE CLASS match_results
    template<class BidirectionalIterator, class Allocator>
    class match_results
    {   // class to hold contents of all capture groups
    public:
        typedef match_results<BidirectionalIterator, Allocator> this_type;
        typedef sub_match<BidirectionalIterator> Element;
        typedef vector<Element, Allocator> ContainerType;

        typedef Element value_type;
        typedef const value_type& const_reference;
        typedef const_reference reference;
        typedef typename ContainerType::const_iterator const_iterator;
        typedef const_iterator iterator;
        typedef AZ_REGEX_DIFFT (BidirectionalIterator) difference_type;
        typedef typename Allocator::size_type size_type;
        typedef Allocator allocator_type;
        typedef AZ_REGEX_VALT (BidirectionalIterator) char_type;
        typedef basic_string<char_type> string_type;

        match_results()
            : m_isReady(false)
        {
        }

        explicit match_results(const Allocator& allocator)
            : m_isReady(false)
            , m_matches(allocator)
        {
        }

        match_results(const this_type& right)
            : m_isReady(right.m_isReady)
            , m_original(right.m_original)
            , m_matches(right.m_matches)
            , m_prefix(right.m_prefix)
            , m_suffix(right.m_suffix)
        {
        }

        this_type& operator=(const this_type& right)
        {       // assign
            m_isReady = right.m_isReady;
            m_original = right.m_original;
            m_matches = right.m_matches;
            m_prefix = right.m_prefix;
            m_suffix = right.m_suffix;
            return (*this);
        }

        match_results(this_type&& right)
            : m_isReady(right.m_isReady)
            , m_original(right.m_original)
            , m_matches(AZStd::move(right.m_matches))
            , m_prefix(right.m_prefix)
            , m_suffix(right.m_suffix)
            , m_nullElement(right.m_nullElement)
        {
        }

        this_type& operator=(this_type&& right)
        {
            if (this != &right)
            {
                m_isReady = right.m_isReady;
                m_original = right.m_original;
                m_matches = AZStd:: move(right.m_matches);
                m_prefix = right.m_prefix;
                m_suffix = right.m_suffix;
                m_nullElement = right.m_nullElement;
            }
            return (*this);
        }

        bool ready() const              { return m_isReady; }

        size_type size() const          { return m_matches.size(); }

        size_type max_size() const      { return m_matches.max_size(); }

        bool empty() const              { return m_matches.empty(); }

        difference_type length(size_type index = 0) const   { return (*this)[index].length();   }

        difference_type position(size_type index = 0) const { return AZStd:: distance(m_original, (*this)[index].first); }

        string_type str(size_type index = 0) const          { return string_type((*this)[index]); }

        const_reference operator[](size_type index) const   { return (m_matches.size() <= index ? m_nullElement : m_matches[index]); }

        const_reference prefix() const                      { return m_prefix; }

        const_reference suffix() const                      { return m_suffix; }

        // return iterator for beginning of sequence of submatches
        const_iterator begin() const    { return m_matches.begin(); }
        // return iterator for end of sequence of submatches
        const_iterator end() const      { return m_matches.end(); }
        // return iterator for beginning of sequence of submatches
        const_iterator cbegin() const   { return m_matches.begin(); }
        // return iterator for end of sequence of submatches
        const_iterator cend() const     { return m_matches.end(); }

        template<class OutputIterator>
        OutputIterator Format(OutputIterator output, const char_type* begin, const char_type* end, regex_constants::match_flag_type flags) const
        {   // format text, replacing matches
            return (flags & regex_constants::format_sed ?
                    FormatSed(*this, output, begin, end, flags) : FormatDefault(*this, output, begin, end, flags));
        }

        template<class OutputIterator>
        OutputIterator format(OutputIterator output, const char_type* begin, const char_type* end, regex_constants::match_flag_type flags = regex_constants::format_default) const
        {   // format text, replacing matches
            output = Format(output, begin, end, flags);
            return output;
        }


        template<class OutputIterator, class _Traits1, class _Alloc1>
        OutputIterator format(OutputIterator output, const basic_string<char_type, _Traits1, _Alloc1>& format, regex_constants::match_flag_type flags = regex_constants::format_default) const
        {   // format text, replacing matches
            output = Format(output, format.begin(), format.end(), flags);
            return output;
        }

        template<class _Traits1, class _Alloc1>
        basic_string<char_type, _Traits1, _Alloc1> format(const basic_string<char_type, _Traits1, _Alloc1>& format, regex_constants::match_flag_type flags = regex_constants::format_default) const
        {   // format text, replacing matches
            basic_string<char_type, _Traits1, _Alloc1> string;
            format(AZStd:: back_inserter(string), format.begin(), format.end(), flags);
            return (string);
        }

        string_type format(const char_type* string, regex_constants::match_flag_type flags = regex_constants::format_default) const
        {   // format text, replacing matches
            string_type format(string);
            string_type str;
            format(AZStd:: back_inserter(str), format.begin(), format.end(), flags);
            return str;
        }

        allocator_type get_allocator() const    { return (m_matches.get_allocator()); }

        void swap(match_results& right)
        {   // exchange contents with right
            AZStd::swap(m_isReady, right.m_isReady);
            AZStd::swap(m_original, right.m_original);
            m_matches.swap(right.m_matches);
            AZStd::swap(m_prefix, right.m_prefix);
            AZStd::swap(m_suffix, right.m_suffix);
        }

        // allocate space for newSize submatches
        void Resize(unsigned int newSize)   { m_matches.resize(newSize); }

        // return modifiable pair of iterators to prefix
        Element& Prefix() { return (m_prefix); }

        // return modifiable pair of iterators to suffix
        Element& Suffix() { return (m_suffix); }

        // return modifiable pair of iterators for null element
        Element& Null() { return (m_nullElement); }

        // unchecked access to element at index
        Element& at(unsigned int index) {   return (m_matches[index]); }

        Element at(unsigned int index) const  {return (m_matches[index]); }

        BidirectionalIterator m_original;
        bool m_isReady;

    private:
        ContainerType m_matches;
        Element m_prefix;
        Element m_suffix;
        Element m_nullElement;
    };

    template<class BidirectionalIterator, class Allocator>
    bool operator==(const match_results<BidirectionalIterator, Allocator>& left, const match_results<BidirectionalIterator, Allocator>& right)
    {
        if (!left.ready() && !right.ready())
        {
            return (true);
        }
        else if (!left.ready() || !right.ready())
        {
            return (false);
        }
        else if (left.empty() && right.empty())
        {
            return (true);
        }
        else if (left.empty() || right.empty())
        {
            return (false);
        }
        else
        {
            return (left.size() == right.size()
                    && left.prefix() == right.prefix()
                    && left.suffix() == right.suffix()
                    && AZStd:: equal(left.begin(), left.end(), right.begin()));
        }
    }

    template<class BidirectionalIterator, class Allocator>
    bool operator!=(const match_results<BidirectionalIterator, Allocator>& left, const match_results<BidirectionalIterator, Allocator>& right)
    {
        return (!(left == right));
    }

    const int NFA_BRE_MAX_GRP = 9;

    const int BITMAP_max = 256; // must fit in an int
    const int BITMAP_shift = 3;
    const int BITMAP_chrs = 1 << BITMAP_shift;  // # of bits to be stored in each char
    const int BITMAP_mask = BITMAP_chrs - 1;
    const int BITMAP_size = (BITMAP_max + BITMAP_chrs - 1) / BITMAP_chrs;

    const int BUFFER_INCREMENT = 16;
    const int BITMAP_ARRAY_THRESHOLD = 4;

    enum NodeFlags
    {   // flags for nfa nodes with special properties
        NFLG_none = 0x00,
        NFLG_negate = 0x01,
        NFLG_greedy = 0x02,
        NFLG_final = 0x04,
        NFLG_longest = 0x08
    };

    inline NodeFlags operator|(NodeFlags left, NodeFlags right)     { return (NodeFlags((int)left | right)); }
    inline NodeFlags operator|=(NodeFlags& left, NodeFlags right)   { return (left = NodeFlags((int)left | right)); }
    inline NodeFlags operator^=(NodeFlags& left, NodeFlags right)   { return (left = NodeFlags((int)left ^ right)); }

    enum NodeType
    {   // type flag for nfa nodes
        NT_none,
        NT_nop,
        NT_bol,
        NT_eol,
        NT_wbound,
        NT_dot,
        NT_str,
        NT_class,
        NT_group,
        NT_end_group,
        NT_assert,
        NT_neg_assert,
        NT_end_assert,
        NT_capture,
        NT_end_capture,
        NT_back,
        NT_if,
        NT_endif,
        NT_rep,
        NT_end_rep,
        NT_begin,
        NT_end
    };

    template<class Element>
    struct RegExBuffer
    {
        AZ_CLASS_ALLOCATOR(RegExBuffer, AZ::SystemAllocator);

        // character buffer
        RegExBuffer()
            : m_capacity(0)
            , m_size(0)
            , m_chars(0)
        {
        }
        ~RegExBuffer()
        {   // destroy
            if (m_chars)
            {
                m_allocator.deallocate(m_chars, m_capacity * sizeof(Element), 1);
            }
        }

        // return number of characters held in buffer
        int Size() const {   return m_size; }

        // return character at m_index
        Element at(unsigned int index) const
        {
            AZ_Assert(index < m_size, "Invalid index");
            return (m_chars[index]);
        }

        // return pointer to first character
        const Element* String() const   { return (m_chars); }

        // append ch
        void Insert(Element ch)
        {
            if (m_capacity <= m_size)
            {
                Expand(m_size + BUFFER_INCREMENT);
            }
            m_chars[m_size++] = ch;
        }

        // remove and return last character
        Element Delete()        { return (m_chars[--m_size]); }

        template<class ForwardIterator>
        void Insert(ForwardIterator first, ForwardIterator last)
        {   // append multiple characters
            while (first != last)
            {
                Insert(*first++);
            }
        }

    private:
        void Expand(int length)
        {   // expand buffer to hold length characters
            unsigned int requiredSize = length * sizeof(Element);
            if (m_chars == nullptr)
            {
                m_chars = (Element*)m_allocator.allocate(requiredSize, 1);
                m_capacity = length;
            }
            else
            {
                // resize didn't work, realloc instead
                Element* newBuf = static_cast<Element*>(m_allocator.allocate(requiredSize, 1));
                memcpy(newBuf, m_chars, m_capacity * sizeof(Element));
                m_allocator.deallocate(m_chars, m_capacity * sizeof(Element), 1);
                m_chars = newBuf;
                m_capacity = length;
            }
            AZ_Assert(m_chars != nullptr, "AZStd::allocator failed to allocate %u bytes!", requiredSize);
        }

        unsigned int m_capacity;
        unsigned int m_size;
        Element* m_chars;
        AZStd::allocator m_allocator;
    };

    struct RegExBitmap
    {   // accelerator table for small character values
        AZ_CLASS_ALLOCATOR(RegExBitmap, AZ::SystemAllocator)

        RegExBitmap()
        {
            memset(m_chars, '\0', BITMAP_size);
        }

        void Mark(unsigned int ch)
        {   // mark character ch
            m_chars[ch >> BITMAP_shift] |= (1 << (ch & BITMAP_mask));
        }

        bool Find(unsigned int ch) const
        {   // return true if ch is marked
            return ((m_chars[ch >> BITMAP_shift] & (1 << (ch & BITMAP_mask))) != 0);
        }

    private:
        unsigned char m_chars[BITMAP_size];
    };

    template<class Element>
    struct RegExSequence
    {   // holds sequences of m_size elements
        AZ_CLASS_ALLOCATOR(RegExSequence, AZ::SystemAllocator)
        RegExSequence(unsigned int len)
            : m_size(len) { }

        unsigned int m_size;
        RegExBuffer<Element> m_data;
        RegExSequence* m_next;
    };

    class NodeBase
    {   // base class for all nfa nodes
    public:
        AZ_CLASS_ALLOCATOR(NodeBase, AZ::SystemAllocator)
        NodeBase(NodeType _Ty, NodeFlags flags = NFLG_none)
            : m_kind(_Ty)
            , m_flags(flags)
            , m_next(0)
            , m_previous(0)
        {
        }

        NodeType m_kind;
        NodeFlags m_flags;
        NodeBase* m_next;
        NodeBase* m_previous;

        virtual ~NodeBase() = default;
    };

    inline void DestroyNode(NodeBase* node, NodeBase* end = nullptr)
    {   // destroy sublist of nodes
        while (node != end && node != 0)
        {
            NodeBase* tmp = node;
            node = node->m_next;
            tmp->m_next = 0;
            delete tmp;
        }
    }

    class RootNode
        : public NodeBase
    {   // root of parse tree
    public:
        AZ_CLASS_ALLOCATOR(RootNode, AZ::SystemAllocator);
        RootNode()
            : NodeBase(NT_begin)
            , m_loops(0)
            , m_marks(0)
            , m_refs(0)
        {
        }

        regex_constants::syntax_option_type flags;
        unsigned int m_loops;
        unsigned int m_marks;
        unsigned int m_refs;
    };

    class NodeEndGroup
        : public NodeBase
    {   // node that marks end of a group
    public:
        AZ_CLASS_ALLOCATOR(NodeEndGroup, AZ::SystemAllocator);
        NodeEndGroup(NodeType type, NodeFlags flags, NodeBase* back)
            : NodeBase(type, flags)
            , m_back(back)
        {
        }

        NodeBase* m_back;
    };

    class NodeAssert
        : public NodeBase
    {   // node that holds an ECMAScript assertion
    public:
        AZ_CLASS_ALLOCATOR(NodeAssert, AZ::SystemAllocator);
        NodeAssert(NodeType type, NodeFlags flags = NFLG_none)
            : NodeBase(type, flags)
            , m_child(0)
        {
        }

        ~NodeAssert()
        {   // destroy branch
            DestroyNode(m_child);
        }

        NodeBase* m_child;
    };

    class NodeCapture
        : public NodeBase
    {   // node that marks beginning of a capture group
    public:
        AZ_CLASS_ALLOCATOR(NodeCapture, AZ::SystemAllocator);
        NodeCapture(unsigned int index)
            : NodeBase(NT_capture, NFLG_none)
            , m_index(index)
        {
        }

        unsigned int m_index;
    };

    class NodeBack
        : public NodeBase
    {   // node that holds a back reference
    public:
        AZ_CLASS_ALLOCATOR(NodeBack, AZ::SystemAllocator);
        NodeBack(unsigned int index)
            : NodeBase(NT_back, NFLG_none)
            , m_index(index)
        {
        }

        unsigned int m_index;
    };

    template<class Element>
    class NodeString
        : public NodeBase
    {   // node that holds text
    public:
        AZ_CLASS_ALLOCATOR(NodeString, AZ::SystemAllocator);
        NodeString(NodeFlags flags = NFLG_none)
            : NodeBase(NT_str, flags)
        {
        }

        RegExBuffer<Element> m_data;
    };

    template<class Element, class RegExTraits>
    class NodeClass
        : public NodeBase
    {   // node that holds a character class (POSIX bracket expression)
    public:
        AZ_CLASS_ALLOCATOR(NodeClass, AZ::SystemAllocator);
        NodeClass(NodeType type = NT_class, NodeFlags flags = NFLG_none)
            : NodeBase(type, flags)
            , m_coll(0)
            , m_small(0)
            , m_large(0)
            , m_ranges(0)
            , _Classes((typename RegExTraits::char_class_type) 0)
            , m_equiv(0)
        {
        }

        ~NodeClass()
        {
            Clear(m_coll);
            delete m_small;
            delete m_large;
            delete m_ranges;
            Clear(m_equiv);
        }

        void Clear(RegExSequence<Element>* head)
        {   // clean up a list of sequences
            while (head)
            {   // delete the head of the list
                RegExSequence<Element>* temp = head;
                head = head->m_next;
                delete temp;
            }
        }

        RegExSequence<Element>* m_coll;
        RegExBitmap* m_small;
        RegExBuffer<Element>* m_large;
        RegExBuffer<Element>* m_ranges;
        typename RegExTraits::char_class_type _Classes;
        RegExSequence<Element>* m_equiv;
    };

    // CLASS NodeEndif
    class NodeEndif
        : public NodeBase
    {   // node that marks the end of an alternative
    public:
        AZ_CLASS_ALLOCATOR(NodeEndif, AZ::SystemAllocator);
        NodeEndif()
            : NodeBase(NT_endif, NFLG_none) {}
    };

    // CLASS NodeIf
    class NodeIf
        : public NodeBase
    {   // node that marks the beginning of an alternative
    public:
        AZ_CLASS_ALLOCATOR(NodeIf, AZ::SystemAllocator);
        NodeIf(NodeBase* m_end)
            : NodeBase(NT_if, NFLG_none)
            , _Endif((NodeEndif*)m_end)
            , m_child(0)
        {
        }

        ~NodeIf()
        {   // destroy branches of if node
            NodeIf* current = m_child;
            while (current)
            {   // destroy branch
                NodeIf* tmp = current;
                current = current->m_child;
                tmp->m_child = 0;
                DestroyNode(tmp, _Endif);
            }
        }

        NodeEndif* _Endif;
        NodeIf* m_child;
    };

    // CLASS NodeEndRepetition
    class NodeRepetition;

    class NodeEndRepetition
        : public NodeBase
    {   // node that marks the end of a repetition
    public:
        AZ_CLASS_ALLOCATOR(NodeEndRepetition, AZ::SystemAllocator);
        NodeEndRepetition()
            : NodeBase(NT_end_rep)
            , _Begin_rep(0)
        {
        }

        NodeRepetition* _Begin_rep;

    private:
        NodeEndRepetition& operator=(const NodeEndRepetition&);
    };

    // CLASS LoopValues
    struct LoopValues
    {   // storage for loop administration
        int _Loop_idx;
        void* _Loop_iter;
    };

    // CLASS NodeRepetition
    class NodeRepetition
        : public NodeBase
    {   // node that marks the beginning of a repetition
    public:
        AZ_CLASS_ALLOCATOR(NodeRepetition, AZ::SystemAllocator);
        NodeRepetition(bool isGreedy, int min, int max, NodeEndRepetition* m_end, int loopNumber)
            : NodeBase(NT_rep, isGreedy ? NFLG_greedy : NFLG_none)
            , m_min(min)
            , m_max(max)
            , m_endRep(m_end)
            , m_loopNumber(loopNumber)
            , m_sampleLoop(-1)
        {
        }

        const int m_min;
        const int m_max;
        NodeEndRepetition* m_endRep;
        unsigned int m_loopNumber;
        int m_sampleLoop;   // -1 undetermined, 0 contains if/do, 1 simple

    private:
        NodeRepetition& operator=(const NodeRepetition&);
    };

    // TEMPLATE CLASS Builder
    template<class ForwardIterator, class Element, class RegExTraits>
    class Builder
    {   // provides operations used by Parser to build the nfa
    public:
        typedef AZ_REGEX_DIFFT (ForwardIterator) DiffType;

        Builder(const RegExTraits& traits, regex_constants::syntax_option_type);
        ~Builder();

        bool BeginExpression() const;
        void SetLong();
        void DiscardPattern();
        NodeBase* GetMark() const;

        void AddNop();
        void AddBoolean();
        void AddEOL();
        void AddWbound();
        void AddDot();
        void AddChar(Element ch);
        void AddClass();
        void AddCharToClass(Element ch);
        void AddRange(Element _E0, Element e1);
        void AddNamedClass(RegexTraitsBase::char_class_type);
        void AddEquiv(ForwardIterator, ForwardIterator, DiffType);
        void AddColl(ForwardIterator, ForwardIterator, DiffType);
        NodeBase* BeginGroup();
        void EndGroup(NodeBase* back);
        NodeBase* BeginAssertGroup(bool);
        void EndAssertGroup(NodeBase*);
        NodeBase* BeginCaptureGroup(unsigned int m_index);
        void AddBackreference(unsigned int m_index);
        NodeBase* BeginIf(NodeBase* _Start);
        void ElseIf(NodeBase*, NodeBase*);
        void AddRep(int min, int max, bool isGreedy);
        void Negate();
        void MarkFinal();
        RootNode* EndPattern();

    private:
        NodeBase* LinkNode(NodeBase*);
        NodeBase* NewNode(NodeType kind);
        void AddStringNode();
        bool BeginExpression(NodeBase*) const;
        void AddCharToBitmap(Element ch);
        void AddCharToArray(Element ch);
        void AddElts(NodeClass<Element, RegExTraits>*, RegexTraitsBase::char_class_type, const RegExTraits&);
        void CharToElts(ForwardIterator, ForwardIterator, DiffType, RegExSequence<Element>**);

        RootNode* m_root;
        NodeBase* m_current;
        regex_constants::syntax_option_type m_flags;
        const RegExTraits& m_traits;
        const int m_bitmapMax;
        const int m_bitmapArrayMax;

    private:
        Builder& operator=(const Builder&);
    };

    template<class BidirectionalIterator>
    class BackTracingState
    {
    public:
        BidirectionalIterator m_current;
        vector<bool> m_groupValid;
    };

    template<class BidirectionalIterator>
    class TgtState
        : public BackTracingState<BidirectionalIterator>
    {
    public:
        struct GroupType
        {
            BidirectionalIterator m_begin;
            BidirectionalIterator m_end;
        };

        vector<GroupType> m_groups;

        // assign from object of type BackTracingState<BidirectionalIterator>
        void operator=(const BackTracingState<BidirectionalIterator>& rhs) {     *(BackTracingState<BidirectionalIterator>*) this = rhs;    }
    };

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    class Matcher
    {   // provides ways to match a regular expression to a text sequence
    public:
        Matcher(Iterator first, Iterator last, const RegExTraits& traits, RootNode* root, int _Nx, regex_constants::syntax_option_type sflags, regex_constants::match_flag_type mflags)
            : m_end(last)
            , m_first(first)
            , m_rootNode(root)
            , m_sflags(sflags)
            , m_mflags(mflags)
            , m_isMatched(false)
            , _Ncap(_Nx)
            , _Longest((root->m_flags & NFLG_longest) && !(mflags & regex_constants::match_any))
            , m_traits(traits)
        {
            m_loopVals.resize(root->m_loops);
        }

        // set specified flags
        void _Setf(regex_constants::match_flag_type flags)      { m_mflags |= flags; }
        // clear specified flags
        void _Clearf(regex_constants::match_flag_type flags)    { m_mflags &= ~flags; }

        template<class Allocator>
        bool Match(Iterator first, match_results<BidirectionalIterator, Allocator>* m_matches, bool isFullMatch)
        {   // try to match
            m_first = first;
            return Match(m_matches, isFullMatch);
        }

        template<class Allocator>
        bool Match(match_results<BidirectionalIterator, Allocator>* matches, bool isFullMatch)
        {   // try to match
            m_begin = m_first;
            m_tgtState.m_current = m_first;
            m_tgtState.m_groupValid.resize(_Ncap);
            m_tgtState.m_groups.resize(_Ncap);
            _Cap = matches != 0;
            m_isFull = isFullMatch;
            _Max_complexity_count = AZ_REGEX_MAX_COMPLEXITY_COUNT;
            _Max_stack_count = AZ_REGEX_MAX_STACK_COUNT;

            m_isMatched = false;

            if (_Match_pat(m_rootNode))
            {
                ;
            }
            else if ((m_mflags& regex_constants::match_partial) != 0 && m_tgtState.m_current == m_end && m_first != m_end)
            {   // record successful partial match
                m_result = m_tgtState;
                m_isMatched = true;
            }
            else
            {
                return false;
            }
            if (matches)
            {   // copy results to m_matches
                matches->Resize(_Ncap);
                for (int m_index = 0; m_index < _Ncap; ++m_index)
                {   // copy submatch m_index
                    if (m_result.m_groupValid[m_index])
                    {   // copy successful match
                        matches->at(m_index).matched = true;
                        matches->at(m_index).first = m_result.m_groups[m_index].m_begin;
                        matches->at(m_index).second = m_result.m_groups[m_index].m_end;
                    }
                    else
                    {   // copy failed match
                        matches->at(m_index).matched = false;
                        matches->at(m_index).first = m_end;
                        matches->at(m_index).second = m_end;
                    }
                }
                matches->m_original = m_begin;
                matches->Prefix().matched = true;
                matches->Prefix().first = m_begin;
                matches->Prefix().second = matches->at(0).first;

                matches->Suffix().matched = true;
                matches->Suffix().first = matches->at(0).second;
                matches->Suffix().second = m_end;

                matches->Null().first = m_end;
                matches->Null().second = m_end;

                matches->m_isReady = true;
            }
            return true;
        }

        BidirectionalIterator Skip(BidirectionalIterator, BidirectionalIterator, NodeBase * = 0);

    private:
        bool _Do_if(NodeIf*);
        bool _Do_rep0(NodeRepetition*, bool);
        bool _Do_rep(NodeRepetition*, bool, int);
        bool _Is_wc(Iterator, int);
        bool _Do_class(NodeBase*);
        bool _Match_pat(NodeBase*);
        bool _Better_match();

        TgtState<Iterator> m_tgtState;
        TgtState<Iterator> m_result;
        vector<LoopValues> m_loopVals;

        Iterator m_begin;
        Iterator m_end;
        Iterator m_first;
        NodeBase* m_rootNode;
        regex_constants::syntax_option_type m_sflags;
        regex_constants::match_flag_type m_mflags;
        bool m_isMatched;
        bool _Cap;
        int _Ncap;
        bool _Longest;
        const RegExTraits& m_traits;
        bool m_isFull;
        long _Max_complexity_count;
        long _Max_stack_count;

    private:
        Matcher& operator=(const Matcher&);
    };

    enum ParserClass
    {
        _Prs_none,
        _Prs_chr,
        _Prs_set
    };

    // TEMPLATE CLASS Parser
    template<class ForwardIterator, class Element, class RegExTraits>
    class Parser
    {   // parse a regular expression
    public:
        typedef typename RegExTraits::char_class_type char_class_type;

        Parser(const RegExTraits& traits, ForwardIterator first, ForwardIterator last, regex_constants::syntax_option_type flags, Internal::ErrorSink* errors);
        RootNode* Compile();

        // assign from object of type BackTracingState<BidirectionalIterator>
        unsigned int MarkCount() const { return (m_groupIndex + 1); }

    private:
        // lexing
        void Error(regex_constants::error_type);

        bool IsEsc() const;
        void Trans();
        void Next();
        void Expect(MetaType, regex_constants::error_type);

        // parsing
        int DoDigits(int base, int count);
        bool DecimalDigits();
        void HexDigits(int);
        bool _OctalDigits();
        void _Do_ex_class(MetaType);
        bool CharacterClassEscape(bool);
        ParserClass _ClassEscape(bool);
        ParserClass _ClassAtom();
        void _ClassRanges();
        void _CharacterClass();
        bool _IdentityEscape();
        bool _Do_ffn(Element);
        bool _Do_ffnx(Element);
        bool _CharacterEscape();
        void _AtomEscape();
        void _Do_capture_group();
        void _Do_noncapture_group();
        void _Do_assert_group(bool);
        bool _Wrapped_disjunction();
        void _Quantifier();
        bool _Alternative();
        void _Disjunction();

        ForwardIterator m_pat;
        ForwardIterator m_begin;
        ForwardIterator m_end;
        int m_groupIndex;
        int m_disjointIndex;
        vector<bool> m_finishedGroups;
        Builder<ForwardIterator, Element, RegExTraits> m_builder;
        const RegExTraits& m_traits;
        regex_constants::syntax_option_type m_flags;
        int m_value;
        Element m_char;
        MetaType m_metaChar;
        unsigned int m_lflags;
        Internal::ErrorSink* m_errors;
    };

    enum LanguageFlags
    {
        LF_ext_rep = 0x00000001,    // + and ? repetitions
        LF_alt_pipe = 0x00000002,   // uses '|' for alternation
        LF_alt_nl = 0x00000004,     // uses '\n' for alternation (grep, egrep)
        LF_nex_grp = 0x00000008,    // has non-escaped capture groups
        LF_nex_rep = 0x00000010,    // has non-escaped repeats
        LF_nc_grp = 0x00000020,     // has non-capture groups (?:xxx)
        LF_asrt_gen = 0x00000040,   // has generalized assertions (?=xxx), (?!xxx)
        LF_asrt_wrd = 0x00000080,   // has word boundary assertions (\b, \B)
        LF_bckr = 0x00000100,       // has backreferences (ERE doesn't)
        LF_lim_bckr = 0x00000200,   // has limited backreferences (BRE \1-\9)
        LF_ngr_rep = 0x00000400,    // has non-greedy repeats
        LF_esc_uni = 0x00000800,    // has Unicode escape sequences
        LF_esc_hex = 0x00001000,    // has hexadecimal escape sequences
        LF_esc_oct = 0x00002000,    // has octal escape sequences
        LF_esc_bsl = 0x00004000,    // has escape backslash in character classes
        LF_esc_ffnx = 0x00008000,   // has extra file escape (\a)
        LF_esc_ffn = 0x00010000,    // has limited file escapes (\[bfnrtv])
        LF_esc_wsd = 0x00020000,    // has w, s, and d character set escapes
        LF_esc_ctrl = 0x00040000,   // has control escape
        LF_no_nl = 0x00080000,      // no newline in pattern or matching text
        LF_bzr_chr = 0x00100000,    // \0 is a valid character constant
        LF_grp_esc = 0x00200000,    // \ is special character in group
        LF_ident_ECMA = 0x00400000, // ECMA identity escape (not identifierpart)
        LF_ident_ERE = 0x00800000,  // ERE identity escape (.[\*^$, plus {+?}()
        LF_ident_awk = 0x01000000,  // awk identity escape ( ERE plus "/)
        LF_anch_rstr = 0x02000000,  // anchor restricted to beginning/end
        LF_star_beg = 0x04000000,   // star okay at beginning of RE/expr (BRE)
        LF_empty_grp = 0x08000000,  // empty group allowed (ERE prohibits "()")
        LF_paren_bal = 0x10000000,  // ')'/'}'/']' special only after '('/'{'/']'
        LF_brk_rstr = 0x20000000,   // ']' not special when first character in set
        LF_mtch_long = 0x40000000,  // find longest match (ERE, BRE)
        LF_no_subs = 0x80000000     // subexpression matches not recorded
    };

    // TEMPLATE CLASS basic_regex
    template<class Element, class RegExTraits = regex_traits<Element> >
    class basic_regex
        //#ifdef AZSTD_HAS_CHECKED_ITERATORS
        //      : public Debug::checked_container_base
        //#endif
        : public Internal::ErrorSink
    {
    public:
        typedef basic_regex<Element, RegExTraits> this_type;
        typedef Element value_type;
        typedef RegExTraits traits_type;
        //typedef typename RegExTraits::locale_type locale_type;
        typedef typename RegExTraits::string_type string_type;
        typedef regex_constants::syntax_option_type flag_type;

        inline static constexpr flag_type icase = regex_constants::icase;
        inline static constexpr flag_type nosubs = regex_constants::nosubs;
        inline static constexpr flag_type optimize = regex_constants::optimize;
        inline static constexpr flag_type collate = regex_constants::collate;
        inline static constexpr flag_type ECMAScript = regex_constants::ECMAScript;
        inline static constexpr flag_type basic = regex_constants::basic;
        inline static constexpr flag_type extended = regex_constants::extended;
        inline static constexpr flag_type awk = regex_constants::awk;
        inline static constexpr flag_type grep = regex_constants::grep;
        inline static constexpr flag_type egrep = regex_constants::egrep;

        basic_regex()
            : m_rootNode(nullptr)
            , m_error(nullptr)
        {
        }

        explicit basic_regex(const Element* element, flag_type flags = regex_constants::ECMAScript)
            : m_rootNode(nullptr)
            , m_error(nullptr)
        {
            if (element == nullptr)
            {
                RegexError(regex_constants::error_parse);
            }
            Reset(element, element + RegExTraits::length(element), flags, random_access_iterator_tag());
        }

        basic_regex(const Element* element, size_t count, flag_type flags = regex_constants::ECMAScript)
            : m_rootNode(nullptr)
            , m_error(nullptr)
        {
            if (element == nullptr)
            {
                RegexError(regex_constants::error_parse);
            }
            Reset(element, element + count, flags, random_access_iterator_tag());
        }

        template<class _STtraits, class _STalloc>
        explicit basic_regex(const basic_string<Element, _STtraits, _STalloc>& string, flag_type flags = regex_constants::ECMAScript)
            : m_rootNode(nullptr)
            , m_error(nullptr)
        {
            Reset(string.begin(), string.end(), flags, random_access_iterator_tag());
        }

        template<class InputIterator>
        basic_regex(InputIterator first, InputIterator last, flag_type flags)
            : m_rootNode(nullptr)
            , m_error(nullptr)
        {
            Reset(first, last, flags, _Iter_cat(first));
        }

        template<class InputIterator>
        basic_regex(InputIterator first, InputIterator last)
            : m_rootNode(nullptr)
            , m_error(nullptr)
        {
            Reset(first, last, regex_constants::ECMAScript, _Iter_cat(first));
        }

        basic_regex(const this_type& right)
            : m_rootNode(nullptr)
            , m_error(nullptr)
            , m_traits(right.m_traits)
        {
            Reset(right.m_rootNode);
        }

        basic_regex(this_type&& right)
            : m_rootNode(nullptr)
            , m_error(nullptr)
        {
            _Assign_rv(AZStd:: move(right));
        }

        this_type& operator=(this_type&& right)
        {   // assign by moving right
            _Assign_rv(AZStd:: move(right));
            return (*this);
        }

        void _Assign_rv(this_type&& right)
        {
            // assign by moving right
            if (this != &right)
            {   // clear this and steal from right
                Clear();

                m_rootNode = right.m_rootNode;
                right.m_rootNode = 0;
            }
        }

        this_type& assign(this_type&& right)
        {   // assign by moving right
            _Assign_rv(AZStd:: move(right));
            return (*this);
        }

        ~basic_regex() override
        {   // destroy the object
            Clear();
        }

        this_type& operator=(const this_type& right)
        {   // replace with copy of right
            return (assign(right));
        }

        this_type& operator=(const Element* element)
        {   // replace with regular expression constructed from element
            if (element == nullptr)
            {
                RegexError(regex_constants::error_parse);
            }
            Reset(element, element + RegExTraits::length(element), ECMAScript, random_access_iterator_tag());
            return (*this);
        }

        // replace with regular expression constructed from string
        template<class _STtraits, class _STalloc>
        this_type& operator=(const basic_string<Element, _STtraits, _STalloc>& string)
        {
            Reset(string.begin(), string.end(),
                ECMAScript, random_access_iterator_tag());
            return (*this);
        }

        // return number of loops
        unsigned int _Loop_count() const        { return (m_rootNode != 0 ? m_rootNode->m_loops : 0); }
        // return number of capture groups
        unsigned int mark_count() const         { return (m_rootNode != 0 ? m_rootNode->m_marks - 1 : 0); }

        this_type& assign(const this_type& right)           { Reset(right.m_rootNode); return (*this); }

        // replace with regular expression constructed from element
        this_type& assign(const Element* element, flag_type flags = regex_constants::ECMAScript)
        {
            return (assign(element, RegExTraits::length(element), flags));
        }

        // replace with regular expression constructed from element, count
        this_type& assign(const Element* element, size_t count, flag_type flags = regex_constants::ECMAScript)
        {
            if (element == nullptr)
            {
                RegexError(regex_constants::error_parse);
            }
            Reset(element, element + count, flags, random_access_iterator_tag());
            return (*this);
        }

        // replace with regular expression constructed from string
        template<class _STtraits, class _STalloc>
        this_type& assign(const basic_string<Element, _STtraits, _STalloc>& string, flag_type flags = regex_constants::ECMAScript)
        {
            Reset(string.begin(), string.end(),
                flags, random_access_iterator_tag());
            return (*this);
        }

        // replace with regular expression constructed from [first, last)
        template<class InputIterator>
        this_type& assign(InputIterator first, InputIterator last,  flag_type flags = regex_constants::ECMAScript)
        {
            Reset(first, last, flags, _Iter_cat(first));
            return (*this);
        }

        // replace with regular expression constructed from [first, last)
        flag_type Flags() const { return (m_rootNode ? m_rootNode->flags : regex_constants::invalid); }

        // clear regular expression and set locale to argument
        //locale_type imbue(locale_type m_locale) { Clear(); return (m_traits.imbue(m_locale)); }

        // return copy of locale object
        //locale_type getloc() const    { return (m_traits.getloc()); }

        // exchange contents with right
        void swap(this_type& right)
        {
            AZStd::swap(m_rootNode, right.m_rootNode);
            AZStd::swap(m_error, right.m_error);
        }

        // return pointer to root node
        RootNode* Get() const   { return m_rootNode; }

        // return pointer to root node
        bool Empty() const          { return (m_rootNode == nullptr); }

        // returns true if there is a usable parsed expression to match with
        bool Valid() const { return !Empty() && GetError() == nullptr; }

        // return error string
        const char* GetError() const { return m_error; }

        // return reference to traits
        const RegExTraits& _Get_traits() const { return m_traits; }

    private:
        RootNode* m_rootNode;
        RegExTraits m_traits;
        const char* m_error;

        void Clear()
        {   // free all storage
            if (m_rootNode != 0 && --m_rootNode->m_refs == 0)
            {
                DestroyNode(m_rootNode);
            }
            m_rootNode = nullptr;
        }

        // ErrorSink::RegexError
        void RegexError(regex_constants::error_type code) override
        {
            // Record only the first error, subsequent ones are likely to be useless
            if (m_error == nullptr)
            {
                m_error = Internal::RegexError(code);
            }
        }

        template<class InputIterator>
        void Reset(InputIterator first, InputIterator last, flag_type flags, input_iterator_tag)
        {   // build regular expression from input iterators
            basic_string<AZ_REGEX_VALT(InputIterator)> string(first, last);
            Reset(string.begin(), string.end(),
                flags, forward_iterator_tag());
        }

        template<class ForwardIterator>
        void Reset(ForwardIterator first, ForwardIterator last, flag_type flags, forward_iterator_tag)
        {   // build regular expression from forward iterators
            m_error = nullptr; // clear last parse error
            Parser<ForwardIterator, Element, RegExTraits> parser(m_traits, first, last, flags, this);
            RootNode* rootNode = parser.Compile();
            Reset(rootNode);
        }

        void Reset(RootNode* rootNode)
        {   // build regular expression holding root node rootNode
            if (rootNode != 0)
            {
                ++rootNode->m_refs;
            }
            Clear();
            m_rootNode = rootNode;
        }
    };

    // AZStd::basic_regex deduction guides
    template <class ForwardIt>
    basic_regex(ForwardIt, ForwardIt, AZStd::regex_constants::syntax_option_type = AZStd::regex_constants::ECMAScript)
        -> basic_regex<typename iterator_traits<ForwardIt>::value_type>;

    // exchange contents of left with right
    template<class Element, class RegExTraits>
    void swap(basic_regex<Element, RegExTraits>& left, basic_regex<Element, RegExTraits>& right)
    {
        left.swap(right);
    }

    template<class BidirectionalIterator, class Allocator>
    void swap(match_results<BidirectionalIterator, Allocator>& left, match_results<BidirectionalIterator, Allocator>& right) AZSTD_THROW0()
    {
        left.swap(right);
    }

    typedef basic_regex<char> regex;
    typedef basic_regex<wchar_t> wregex;
    typedef match_results<const char*> cmatch;
    typedef match_results<const wchar_t*> wcmatch;
    typedef match_results<string::const_iterator> smatch;
    typedef match_results<wstring::const_iterator> wsmatch;

    #define AZ_REGEX_ISDIGIT(x) ('0' <= (x) && (x) <= '9')

    // TEMPLATE FUNCTION _Format_default
    template<class BidirectionalIterator, class Allocator, class InputIterator, class OutputIterator>
    inline OutputIterator FormatDefault(const match_results<BidirectionalIterator, Allocator>& match, OutputIterator output, InputIterator first, InputIterator last, regex_constants::match_flag_type)
    {   // format with ECMAScript rules
        while (first != last)
        {   // process one character or escape sequence
            if (*first != '$')
            {
                *output++ = *first++;
            }
            else if (++first == last)
            {
                ;
            }
            else if (*first == '$')
            {   // replace $$
                *output++ = '$';
                ++first;
            }
            else if (*first == '`')
            {   // replace $`
                output = AZStd::copy(match.prefix().first, match.prefix().second, output);
                ++first;
            }
            else if (*first == '\'')
            {   // replace $'
                output = AZStd::copy(match.suffix().first, match.suffix().second, output);
                ++first;
            }
            else
            {   // replace capture group descriptors
                int n = -1;
                if (*first == '&')
                {   // replace $&
                    n = 0;
                    ++first;
                }
                else if (AZ_REGEX_ISDIGIT(*first))
                {   // replace $n, $nn
                    n = *first++ - '0';
                    if (first != last && AZ_REGEX_ISDIGIT(*first))
                    {       // process second digit
                        n *= 10;
                        n += *first++ - '0';
                    }
                }
                else
                {   // replace $x
                    *output++ = '$';
                    *output++ = *first++;
                }
                if (0 <= n && n < (int)match.size())
                {
                    output = AZStd::copy(match.at(n).first, match.at(n).second, output);
                }
            }
        }
        return output;
    }

    // TEMPLATE FUNCTION FormatSed
    template<class BidirectionalIterator, class Allocator, class InputIterator, class OutputIterator>
    inline OutputIterator FormatSed(const match_results<BidirectionalIterator, Allocator>& match, OutputIterator output, InputIterator first, InputIterator last, regex_constants::match_flag_type)
    {   // format with sed rules
        while (first != last)
        {   // process one character or escape sequence
            if (*first == '&')
            {   // replace with full match
                output = AZStd::copy(match.at(0).first, match.at(0).second, output);
                ++first;
            }
            else if (*first != '\\')
            {
                *output++ = *first++;
            }
            else if (++first == last)
            {
                ;
            }
            else if (AZ_REGEX_ISDIGIT(*first))
            {   // replace \n
                int n = *first++ - '0';
                output = AZStd::copy(match.at(n).first, match.at(n).second, output);
            }
            else
            {
                *output++ = *first++;
            }
        }
        return (output);
    }

    #undef AZ_REGEX_ISDIGIT

    // try to match regular expression to target text
    template<class BidirectionalIterator, class Allocator, class Element, class RegExTraits, class Iterator>
    inline bool RegexMatch(Iterator first, Iterator last,   match_results<BidirectionalIterator, Allocator>* m_matches, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags, bool isFull)
    {
        if (regEx.Empty())
        {
            return (false);
        }
        Matcher<BidirectionalIterator, Element, RegExTraits, Iterator> matcher(first, last, regEx._Get_traits(), regEx.Get(), regEx.mark_count() + 1, regEx.Flags(), flags);
        return (matcher.Match(m_matches, isFull));
    }

    // try to match regular expression to target text
    template<class BidirectionalIterator, class Allocator, class Element, class RegExTraits>
    inline bool regex_match(BidirectionalIterator first, BidirectionalIterator last, match_results<BidirectionalIterator, Allocator>& m_matches, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        return (RegexMatch(first, last, &m_matches, regEx, flags, true));
    }

    // try to match regular expression to target text
    template<class BidirectionalIterator, class Element, class RegExTraits>
    inline  bool regex_match(BidirectionalIterator first, BidirectionalIterator last, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        return (RegexMatch(first, last, (match_results<BidirectionalIterator>*) 0, regEx, flags | regex_constants::match_any, true));
    }

    // try to match regular expression to target text
    template<class Element, class RegExTraits>
    inline bool regex_match(const Element* string, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        const Element* last = string + char_traits<Element>::length(string);
        return (RegexMatch(string, last, (match_results<const Element*>*) 0, regEx, flags | regex_constants::match_any, true));
    }

    // try to match regular expression to target text
    template<class Element, class Allocator,    class RegExTraits>
    inline bool regex_match(const Element* string, match_results<const Element*, Allocator>& m_matches, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        const Element* last = string + char_traits<Element>::length(string);
        return (RegexMatch(string, last, &m_matches, regEx, flags, true));
    }

    // try to match regular expression to target text
    template<class _StTraits, class _StAlloc, class Allocator, class Element, class RegExTraits>
    inline bool regex_match(const basic_string<Element, _StTraits, _StAlloc>& string, match_results<typename basic_string<Element, _StTraits, _StAlloc>::const_iterator, Allocator>& m_matches,
        const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        return (RegexMatch(string.begin(), string.end(), &m_matches, regEx, flags, true));
    }

    // try to match regular expression to target text
    template<class _StTraits, class _StAlloc, class Element, class RegExTraits>
    inline bool regex_match(const basic_string<Element, _StTraits, _StAlloc>& string, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        typedef typename basic_string<Element, _StTraits, _StAlloc>::const_iterator Iterator;
        return (RegexMatch(string.begin(), string.end(), (match_results<Iterator>*) 0, regEx, flags | regex_constants::match_any, true));
    }

    // search for regular expression match in target text
    template<class BidirectionalIterator, class Allocator, class Element, class RegExTraits, class Iterator>
    inline bool _Regex_search(Iterator first, Iterator last, match_results<BidirectionalIterator, Allocator>* m_matches, const basic_regex<Element, RegExTraits>& regEx,    regex_constants::match_flag_type flags, Iterator original)
    {
        if (regEx.Empty())
        {
            return false;
        }
        bool _Found = false;
        Iterator begin = first;
        Matcher<BidirectionalIterator, Element, RegExTraits, Iterator> matcher(first, last, regEx._Get_traits(), regEx.Get(), regEx.mark_count() + 1, regEx.Flags(), flags);

        if (matcher.Match(m_matches, false))
        {
            _Found = true;
        }
        else if (first == last || flags & regex_constants::match_continuous)
        {
            ;
        }
        else
        {   // try more on suffixes
            matcher._Setf(regex_constants::match_prev_avail);
            matcher._Clearf(regex_constants::match_not_null1);
            while ((first = matcher.Skip(++first, last)) != last)
            {
                if (matcher.Match(first, m_matches, false))
                {       // found match starting at first
                    _Found = true;
                    break;
                }
            }
            if (!_Found && matcher.Match(last, m_matches, false))
            {
                _Found = true;
            }
        }
        if (_Found && m_matches)
        {   // update m_matches
            m_matches->m_original = original;
            m_matches->Prefix().first = begin;
        }
        return _Found;
    }

    // search for regular expression match in target text
    template<class BidirectionalIterator, class Allocator, class Element, class RegExTraits>
    inline bool regex_search(BidirectionalIterator first, BidirectionalIterator last, match_results<BidirectionalIterator, Allocator>& m_matches, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        return (_Regex_search(first, last, &m_matches, regEx, flags, first));
    }

    // search for regular expression match in target text
    template<class BidirectionalIterator, class Element, class RegExTraits>
    inline bool regex_search(BidirectionalIterator first, BidirectionalIterator last, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        return (_Regex_search(first, last, (match_results<BidirectionalIterator>*) 0, regEx, flags | regex_constants::match_any, first));
    }

    // search for regular expression match in target text
    template<class Element, class RegExTraits>
    inline bool regex_search(const Element* string, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        const Element* last = string + char_traits<Element>::length(string);
        return (_Regex_search(string, last, (match_results<const Element*>*) 0, regEx, flags | regex_constants::match_any, string));
    }

    // search for regular expression match in target text
    template<class Element, class Allocator, class RegExTraits>
    inline bool regex_search(const Element* string, match_results<const Element*, Allocator>& m_matches, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        const Element* last = string + char_traits<Element>::length(string);
        return (_Regex_search(string, last, &m_matches, regEx, flags, string));
    }

    // search for regular expression match in target text
    template<class _StTraits, class _StAlloc, class Allocator, class Element, class RegExTraits>
    inline bool regex_search(const basic_string<Element, _StTraits, _StAlloc>& string, match_results<typename basic_string<Element, _StTraits, _StAlloc>::const_iterator, Allocator>& m_matches,
        const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        return (_Regex_search(string.begin(), string.end(), &m_matches, regEx, flags, string.begin()));
    }

    template<class _StTraits, class _StAlloc, class Element, class RegExTraits>
    inline bool regex_search(const basic_string<Element, _StTraits, _StAlloc>& string, const basic_regex<Element, RegExTraits>& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
    {   // search for regular expression match in target text
        typedef typename basic_string<Element, _StTraits, _StAlloc>::const_pointer Iterator;

        const Element* first = string.c_str();
        const Element* last = first + string.size();
        return (_Regex_search(first, last, (match_results<Iterator>*) 0, regEx, flags | regex_constants::match_any, first));
    }

    // search and replace
    template<class OutputIterator, class BidirectionalIterator, class RegExTraits, class Element, class Traits, class Allocator>
    inline OutputIterator _Regex_replace(OutputIterator _Result, BidirectionalIterator first, BidirectionalIterator last, const basic_regex<Element, RegExTraits>& regEx,   const basic_string<Element, Traits, Allocator>& format, regex_constants::match_flag_type flags)
    {
        match_results<BidirectionalIterator> m_matches;
        BidirectionalIterator position = first;
        regex_constants::match_flag_type mflags = flags;
        regex_constants::match_flag_type _Not_null = (regex_constants::match_flag_type)0;

        while (regex_search(position, last, m_matches, regEx, mflags | _Not_null))
        {   // replace at each match
            if (!(flags & regex_constants::format_no_copy))
            {
                _Result = AZStd::copy(m_matches.prefix().first, m_matches.prefix().second, _Result);
            }
            _Result = m_matches.Format(_Result, format.begin(), format.end(), mflags);

            position = m_matches[0].second;
            if (position == last || flags & regex_constants::format_first_only)
            {
                break;
            }

            if (m_matches[0].first == m_matches[0].second)
            {
                _Not_null = regex_constants::match_not_null1;
            }
            else
            {   // non-null match, recognize earlier text
                _Not_null = (regex_constants::match_flag_type)0;
                mflags |= regex_constants::match_prev_avail;
            }
        }
        return (flags & regex_constants::format_no_copy ? _Result : AZStd::copy(position, last, _Result));
    }

    // search and replace, iterator result, string format
    template<class OutputIterator, class BidirectionalIterator, class RegExTraits, class Element, class Traits, class Allocator>
    inline OutputIterator regex_replace(OutputIterator result,  BidirectionalIterator first, BidirectionalIterator last, const basic_regex<Element, RegExTraits>& regEx, const basic_string<Element, Traits, Allocator>& format, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        result = _Regex_replace(result, first, last, regEx, format, flags);
        return result;
    }

    template<class OutputIterator, class BidirectionalIterator, class RegExTraits, class Element>
    inline OutputIterator regex_replace(OutputIterator _Result, BidirectionalIterator first, BidirectionalIterator last, const basic_regex<Element, RegExTraits>& regEx, const Element* element, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        // search and replace, iterator result, NTBS format
        const basic_string<Element> format(element);
        return (regex_replace(_Result, first, last, regEx, format, flags));
    }

    // search and replace, string result, string target, string format
    template<class RegExTraits, class Element, class _Traits1, class _Alloc1, class _Traits2, class _Alloc2>
    basic_string<Element, _Traits1, _Alloc1>
    regex_replace(const basic_string<Element, _Traits1, _Alloc1>& string, const basic_regex<Element, RegExTraits>& regEx, const basic_string<Element, _Traits2, _Alloc2>& format,   regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        basic_string<Element, _Traits1, _Alloc1> result;
        regex_replace(AZStd:: back_inserter(result), string.begin(), string.end(), regEx, format, flags);
        return result;
    }

    // search and replace, string result, string target, NTBS format
    template<class RegExTraits, class Element, class _Traits1, class _Alloc1>
    basic_string<Element, _Traits1, _Alloc1>
    regex_replace(const basic_string<Element, _Traits1, _Alloc1>& string, const basic_regex<Element, RegExTraits>& regEx, const Element* element, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        basic_string<Element, _Traits1, _Alloc1> result;
        const basic_string<Element> format(element);
        regex_replace(AZStd:: back_inserter(result), string.begin(), string.end(), regEx, format, flags);
        return result;
    }

    // search and replace, string result, NTBS target, string format
    template<class RegExTraits, class Element, class _Traits2, class _Alloc2>
    basic_string<Element> regex_replace(const Element* _Pstr, const basic_regex<Element, RegExTraits>& regEx, const basic_string<Element, _Traits2, _Alloc2>& format, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        basic_string<Element> result;
        const basic_string<Element> string(_Pstr);
        regex_replace(AZStd:: back_inserter(result), string.begin(), string.end(), regEx, format, flags);
        return (result);
    }

    // search and replace, string result, NTBS target, NTBS format
    template<class RegExTraits, class Element>
    basic_string<Element> regex_replace(const Element* _Pstr, const basic_regex<Element, RegExTraits>& regEx,   const Element* element, regex_constants::match_flag_type flags = regex_constants::match_default)
    {
        basic_string<Element> result;
        const basic_string<Element> string(_Pstr);
        const basic_string<Element> format(element);
        regex_replace(AZStd:: back_inserter(result), string.begin(), string.end(), regEx, format, flags);
        return result;
    }

    template<class BidirectionalIterator, class Element = AZ_REGEX_VALT(BidirectionalIterator), class RegExTraits = regex_traits<Element> >
    class regex_iterator
    {   // iterator for full regular expression matches
    public:
        typedef regex_iterator<BidirectionalIterator, Element, RegExTraits> this_type;
        typedef basic_regex<Element, RegExTraits> regex_type;
        typedef match_results<BidirectionalIterator> value_type;
        typedef ptrdiff_t difference_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef forward_iterator_tag iterator_category;

        regex_iterator()
            : m_regEx(0)
        {
        }

        regex_iterator(BidirectionalIterator first, BidirectionalIterator last, const regex_type& regEx, regex_constants::match_flag_type flags = regex_constants::match_default)
            : m_begin(first)
            , m_end(last)
            , m_regEx(&regEx)
            , m_flags(flags)
        {
            if (!_Regex_search(m_begin, m_end, &m_value, *m_regEx, flags, m_begin))
            {
                m_regEx = 0;
            }
        }

        bool operator==(const this_type& right) const
        {
            if (m_regEx != right.m_regEx)
            {
                return (false);
            }
            else if (m_regEx == 0)
            {
                return (true);
            }
            return (m_begin == right.m_begin && m_end == right.m_end && m_flags == right.m_flags && m_value.at(0) == right.m_value.at(0));
        }

        bool operator!=(const this_type& right) const { return (!(*this == right)); }

        // return designated match
        const value_type& operator*() const { return m_value; }

        // return pointer to designated match
        pointer operator->() const  { return (std::pointer_traits<pointer>::pointer_to(**this)); }

        this_type& operator++()
        {
            if (m_regEx == 0)
            {
                return (*this);
            }

            BidirectionalIterator _Start = m_value.at(0).second;

            if (m_value.at(0).first == m_value.at(0).second)
            {   // handle zero-length match
                if (_Start == m_end)
                {   // store end-of-sequence iterator
                    m_regEx = 0;
                    return (*this);
                }
                if (_Regex_search(_Start, m_end, &m_value, *m_regEx, m_flags | regex_constants::match_not_null | regex_constants::match_continuous, m_begin))
                {
                    return (*this);
                }
                ++_Start;
            }
            m_flags = m_flags | regex_constants::match_prev_avail;
            if (_Regex_search(_Start, m_end, &m_value, *m_regEx, m_flags, m_begin))
            {
                return (*this);
            }
            else
            {   // mark at end of sequence
                m_regEx = 0;
                return (*this);
            }
        }

        this_type operator++(int)
        {
            this_type tmp = *this;
            ++*this;
            return tmp;
        }

        // test for end iterator
        bool _Atend() const { return (m_regEx == 0); }

    private:
        BidirectionalIterator m_begin, m_end;           // input sequence
        const regex_type* m_regEx;      // pointer to basic_regex object
        regex_constants::match_flag_type m_flags;
        match_results<BidirectionalIterator> m_value;   // lookahead value (if m_regEx not null)
    };

    typedef regex_iterator<const char*> cregex_iterator;
    typedef regex_iterator<const wchar_t*> wcregex_iterator;
    typedef regex_iterator<string::const_iterator> sregex_iterator;
    typedef regex_iterator<wstring::const_iterator> wsregex_iterator;

    // TEMPLATE CLASS regex_token_iterator
    template<class BidirectionalIterator, class Element = AZ_REGEX_VALT(BidirectionalIterator), class RegExTraits = regex_traits<Element> >
    class regex_token_iterator
    {   // iterator for regular expression submatches
    public:
        typedef regex_iterator<BidirectionalIterator, Element, RegExTraits> PositionType;
        typedef regex_token_iterator<BidirectionalIterator, Element, RegExTraits> this_type;
        typedef basic_regex<Element, RegExTraits> regex_type;
        typedef sub_match<BidirectionalIterator> value_type;
        typedef ptrdiff_t difference_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef forward_iterator_tag iterator_category;

        regex_token_iterator()
            : m_result(0)
        {
        }

        regex_token_iterator(BidirectionalIterator first, BidirectionalIterator last, const regex_type& regEx, int index = 0, regex_constants::match_flag_type flags = regex_constants::match_default)
            : m_position(first, last, regEx, flags)
            , m_current(0)
            , m_subs(&index, &index + 1)
        {
            _Init(first, last);
        }

        regex_token_iterator(BidirectionalIterator first, BidirectionalIterator last, const regex_type& regEx, const vector<int>& _Subx, regex_constants::match_flag_type flags = regex_constants::match_default)
            : m_position(first, last, regEx, flags)
            , m_current(0)
            , m_subs(_Subx.begin(), _Subx.end())
        {
            if (m_subs.empty())
            {
                m_result = 0;   // treat empty vector as end of sequence
            }
            else
            {
                _Init(first, last);
            }
        }

        template<size_t _Nx>
        regex_token_iterator(BidirectionalIterator first, BidirectionalIterator last,
            const regex_type& regEx,
            const int (&_Subx)[_Nx],
            regex_constants::match_flag_type flags =
                regex_constants::match_default)
            : m_position(first, last, regEx, flags)
            , m_current(0)
            , m_subs(_Subx, _Subx + _Nx)
        {
            _Init(first, last);
        }

        regex_token_iterator(const regex_token_iterator& right)
            : m_position(right.m_position)
            , m_current(right.m_current)
            , m_suffix(right.m_suffix)
            , m_subs(right.m_subs)
        {
            if (right.m_result == 0)
            {
                m_result = 0;
            }
            else if (right.m_result == &right.m_suffix)
            {
                m_result = &m_suffix;
            }
            else
            {
                m_result = Current();
            }
        }

        regex_token_iterator& operator=(const regex_token_iterator& right)
        {   // assign from right
            if (&right != this)
            {       // copy from right
                m_position = right.m_position;
                m_current = right.m_current;
                m_suffix = right.m_suffix;
                m_subs = right.m_subs;
                if (right.m_result == 0)
                {
                    m_result = 0;
                }
                else if (right.m_result == &right.m_suffix)
                {
                    m_result = &m_suffix;
                }
                else
                {
                    m_result = Current();
                }
            }
            return (*this);
        }

        bool operator==(const this_type& right) const
        {   // test for equality
            if (m_result == 0 || right.m_result == 0)
            {
                return (m_result == right.m_result);
            }
            else if (*m_result == *right.m_result
                     && m_position == right.m_position
                     && m_subs == right.m_subs)
            {
                return (true);
            }
            else
            {
                return (false);
            }
        }

        bool operator!=(const this_type& right) const { return (!(*this == right)); }

        // return designated submatch
        const value_type& operator*() const
        {
            _Analysis_assume_(m_result != 0);
            return (*m_result);
        }

        // return pointer to designated submatch
        pointer operator->() const
        {
            return (std::pointer_traits<pointer>::pointer_to(**this));
        }

        this_type& operator++()
        {
            PositionType previous(m_position);
            if (m_result == 0)
            {
                ;
            }
            else if (m_result == &m_suffix)
            {
                m_result = 0;
            }
            else if (++m_current < m_subs.size())
            {
                m_result = Current();
            }
            else
            {           // advance to next full match
                m_current = 0;
                ++m_position;
                if (!m_position._Atend())
                {
                    m_result = Current();
                }
                else if (_Has_suffix() && previous->suffix().length() != 0)
                {       // mark suffix
                    m_suffix.matched = true;
                    m_suffix.first = previous->suffix().first;
                    m_suffix.second = previous->suffix().second;
                    m_result = &m_suffix;
                }
                else
                {
                    m_result = 0;
                }
            }
            return (*this);
        }

        this_type operator++(int)
        {
            this_type tmp = *this;
            ++*this;
            return (tmp);
        }

    private:
        PositionType m_position;
        const value_type* m_result;
        value_type m_suffix;
        size_t m_current;
        vector<int> m_subs;

        bool _Has_suffix() const
        {   // check for suffix specifier
            return (AZStd::find(m_subs.begin(), m_subs.end(), -1) != m_subs.end());
        }

        void _Init(BidirectionalIterator first, BidirectionalIterator last)
        {   // initialize
            if (!m_position._Atend())
            {
                m_result = Current();
            }
            else if (_Has_suffix())
            {   // mark suffix (no match)
                m_suffix.matched = true;
                m_suffix.first = first;
                m_suffix.second = last;
                m_result = &m_suffix;
            }
            else
            {
                m_result = 0;
            }
        }

        // return pointer to current submatch
        const value_type* Current() const   { return (&(m_subs[m_current] == -1 ? m_position->prefix() : (*m_position)[m_subs[m_current]])); }
    };

    typedef regex_token_iterator<const char*> cregex_token_iterator;
    typedef regex_token_iterator<const wchar_t*> wcregex_token_iterator;
    typedef regex_token_iterator<string::const_iterator> sregex_token_iterator;
    typedef regex_token_iterator<wstring::const_iterator> wsregex_token_iterator;

    // IMPLEMENTATION OF Builder
    template<class ForwardIterator, class Element, class RegExTraits>
    inline Builder<ForwardIterator, Element, RegExTraits>::Builder(const RegExTraits& traits, regex_constants::syntax_option_type flags)
        : m_root(aznew RootNode)
        , m_current(m_root)
        , m_flags(flags)
        , m_traits(traits)
        , m_bitmapMax(flags & regex_constants::collate ? 0 : BITMAP_max)
        , m_bitmapArrayMax(flags & regex_constants::collate ? 0 : BITMAP_ARRAY_THRESHOLD)
    {
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline Builder<ForwardIterator, Element, RegExTraits>::~Builder()
    {
        if (m_root && m_root->m_refs == 0)
        {
            DestroyNode(m_root);
        }
    }


    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::SetLong()
    {   // set flag
        m_root->m_flags |= NFLG_longest;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::Negate()
    {   // set flag
        m_current->m_flags ^= NFLG_negate;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::MarkFinal()
    {   // set flag
        m_current->m_flags |= NFLG_final;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline NodeBase* Builder<ForwardIterator, Element, RegExTraits>::GetMark() const
    {   // return current node
        return (m_current);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Builder<ForwardIterator, Element, RegExTraits>::BeginExpression(NodeBase* _Nx) const
    {   // test for beginning of expression or subexpression
        return (_Nx->m_kind == NT_begin || _Nx->m_kind == NT_group || _Nx->m_kind == NT_capture);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline
    bool Builder<ForwardIterator, Element, RegExTraits>::BeginExpression() const
    {   // test for beginning of expression or subexpression
        return (BeginExpression(m_current) || (m_current->m_kind == NT_bol && BeginExpression(m_current->m_previous)));
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline NodeBase* Builder<ForwardIterator, Element, RegExTraits>::LinkNode(NodeBase* _Nx)
    {   // insert _Nx at current location
        _Nx->m_previous = m_current;
        if (m_current->m_next)
        {   // set back pointer
            _Nx->m_next = m_current->m_next;
            m_current->m_next->m_previous = _Nx;
        }
        m_current->m_next = _Nx;
        m_current = _Nx;
        return _Nx;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline NodeBase* Builder<ForwardIterator, Element, RegExTraits>::NewNode(NodeType kind)
    {   // allocate and link simple node
        return LinkNode(aznew NodeBase(kind));
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddNop()
    {   // add nop node
        NewNode(NT_nop);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddBoolean()
    {   // add bol node
        NewNode(NT_bol);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddEOL()
    {   // add eol node
        NewNode(NT_eol);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddWbound()
    {   // add wbound node
        NewNode(NT_wbound);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddDot()
    {   // add dot node
        NewNode(NT_dot);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddStringNode()
    {   // add string node
        LinkNode(aznew NodeString<Element>);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddChar(Element ch)
    {   // append character
        if (m_current->m_kind != NT_str || m_current->m_flags & NFLG_final)
        {
            AddStringNode();
        }
        if (m_flags & regex_constants::icase)
        {
            ch = m_traits.translate_nocase(ch);
        }
        else if (m_flags & regex_constants::collate)
        {
            ch = m_traits.translate(ch);
        }
        NodeString<Element>* node = (NodeString<Element>*)m_current;
        node->m_data.Insert(ch);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddClass()
    {   // add bracket expression node
        LinkNode(aznew NodeClass<Element, RegExTraits>);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddCharToBitmap(Element ch)
    {   // add character to accelerator table
        if (m_flags & regex_constants::icase)
        {
            ch = m_traits.translate_nocase(ch);
        }
        NodeClass<Element, RegExTraits>* node = (NodeClass<Element, RegExTraits>*)m_current;
        if (!node->m_small)
        {
            node->m_small = aznew RegExBitmap;
        }
        node->m_small->Mark(ch);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddCharToArray(Element ch)
    {   // append character to character array
        if (m_flags & regex_constants::icase)
        {
            ch = m_traits.translate_nocase(ch);
        }
        NodeClass<Element, RegExTraits>* node =
            (NodeClass<Element, RegExTraits>*)m_current;
        if (!node->m_large)
        {
            node->m_large = aznew RegExBuffer<Element>;
        }
        node->m_large->Insert(ch);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddCharToClass(Element ch)
    {   // add character to bracket expression
        static int m_max = BITMAP_max;  // to quiet diagnostics
        static int m_min = 0;

        if (m_min <= static_cast<int>(ch) && static_cast<int>(ch) < m_max)
        {
            AddCharToBitmap(ch);
        }
        else
        {
            AddCharToArray(ch);
        }
    }

    AZ_FORCE_INLINE bool IsGreaterEqualToZero(char ch)      { return ch >= 0; }
    AZ_FORCE_INLINE bool IsGreaterEqualToZero(wchar_t ch)   { (void)ch; return true; }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddRange(Element _E0, Element e1)
    {   // add character range to set
        if (m_flags & regex_constants::icase)
        {       // change to lowercase range
            _E0 = m_traits.translate_nocase(_E0);
            e1 = m_traits.translate_nocase(e1);
        }
        NodeClass<Element, RegExTraits>* node =
            (NodeClass<Element, RegExTraits>*)m_current;
        if (IsGreaterEqualToZero(_E0))
        {
            for (; _E0 <= e1 && e1 < static_cast<Element>(m_bitmapMax); ++_E0)
            {       // set a bit
                if (!node->m_small)
                {
                    node->m_small = aznew RegExBitmap;
                }
                node->m_small->Mark(_E0);
                if (_E0 == e1)
                {
                    break;
                }
            }
        }
        if (IsGreaterEqualToZero(_E0) && e1 - _E0 < static_cast<Element>(m_bitmapArrayMax))
        {
            for (; _E0 <= e1; ++_E0)
            {       // add to list of elements
                AddCharToClass(_E0);
                if (_E0 == e1)
                {
                    break;
                }
            }
        }
        else
        {       // store remaining range as pair
            if (!node->m_ranges)
            {
                node->m_ranges = aznew RegExBuffer<Element>;
            }
            node->m_ranges->Insert(_E0);
            node->m_ranges->Insert(e1);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddElts(NodeClass<Element, RegExTraits>* node, RegexTraitsBase::char_class_type _Cl, const RegExTraits& traits)
    {   // add characters in named class to set
        for (int ch = 0; ch < BITMAP_max; ++ch)
        {
            if (traits.isctype((Element)ch, _Cl))
            {   // add contents of named class to accelerator table
                if (!node->m_small)
                {
                    node->m_small = aznew RegExBitmap;
                }
                node->m_small->Mark(ch);
            }
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddNamedClass(typename RegexTraitsBase::char_class_type _Cl)
    {   // add contents of named class to bracket expression
        /*static*/
        int m_max = BITMAP_max;             // to quiet diagnostics
        NodeClass<Element, RegExTraits>* node = (NodeClass<Element, RegExTraits>*)m_current;
        AddElts(node, _Cl, m_traits);
        if (static_cast<Element>(m_max) < (std::numeric_limits<Element>::max)())
        {
            node->_Classes = (RegexTraitsBase::char_class_type)(node->_Classes | _Cl);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::CharToElts(ForwardIterator first, ForwardIterator last, DiffType diff, RegExSequence<Element>** current)
    {   // add collation element to element sequence
        while (*current && (unsigned int)diff < (*current)->m_size)
        {
            current = &(*current)->m_next;
        }
        if (!(*current) || (unsigned int)diff != (*current)->m_size)
        {   // add new sequence holding elements of the same length
            RegExSequence<Element>* node = *current;
            *current = aznew RegExSequence<Element>((unsigned int)diff);
            (*current)->m_next = node;
        }
        (*current)->m_data.Insert(first, last);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddEquiv(ForwardIterator first, ForwardIterator last, DiffType diff)
    {   // add elements of equivalence class to bracket expression
        /*static*/
        int m_max = BITMAP_max;             // to quiet diagnostics
        NodeClass<Element, RegExTraits>* node = (NodeClass<Element, RegExTraits>*)m_current;
        typename RegExTraits::string_type string = m_traits.transform_primary(first, last);
        for (int ch = 0; ch < BITMAP_max; ++ch)
        {   // add elements
            Element _Ex = (Element)ch;
            if (m_traits.transform_primary(&_Ex, &_Ex + 1) == string)
            {   // insert equivalent character into bitmap
                if (!node->m_small)
                {
                    node->m_small = aznew RegExBitmap;
                }
                node->m_small->Mark(ch);
            }
        }
        if (static_cast<Element>(m_max) < (std::numeric_limits<Element>::max)())
        {   // map range
            RegExSequence<Element>** current = &node->m_equiv;
            CharToElts(first, last, diff, current);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddColl(ForwardIterator first, ForwardIterator last, DiffType diff)
    {   // add collation element to bracket expression
        NodeClass<Element, RegExTraits>* node = (NodeClass<Element, RegExTraits>*)m_current;
        RegExSequence<Element>** current = &node->m_coll;
        CharToElts(first, last, diff, current);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline NodeBase* Builder<ForwardIterator, Element, RegExTraits>::BeginGroup()
    {   // add group node
        return (NewNode(NT_group));
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::EndGroup(NodeBase* back)
    {   // add end of group node
        NodeType _Elt = back->m_kind == NT_group ? NT_end_group
            : back->m_kind == NT_assert ? NT_end_assert
            : back->m_kind == NT_neg_assert ? NT_end_assert
            : NT_end_capture;
        LinkNode(aznew NodeEndGroup(_Elt, NFLG_none, back));
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline NodeBase* Builder<ForwardIterator, Element, RegExTraits>::BeginAssertGroup(bool _Neg)
    {   // add assert node
        NodeAssert* _Node1 = aznew NodeAssert(_Neg
                ? NT_neg_assert : NT_assert);

        NodeBase* _Node2;
        _Node2 = aznew NodeBase(NT_nop);
        LinkNode(_Node1);
        _Node1->m_child = _Node2;
        _Node2->m_previous = _Node1;
        m_current = _Node2;
        return (_Node1);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::EndAssertGroup(NodeBase* _Nx)
    {   // add end of assert node
        EndGroup(_Nx);
        m_current = _Nx;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline NodeBase* Builder<ForwardIterator, Element, RegExTraits>::BeginCaptureGroup(unsigned int m_index)
    {   // add capture group node
        return (LinkNode(aznew NodeCapture(m_index)));
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddBackreference(unsigned int m_index)
    {   // add back reference node
        LinkNode(aznew NodeBack(m_index));
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline NodeBase* Builder<ForwardIterator, Element, RegExTraits>::BeginIf(NodeBase* _Start)
    {
        // add if node
        /* append endif node */
        NodeBase* result = aznew NodeEndif;
        LinkNode(result);

        /* insert if_node */
        NodeIf* _Node1 = aznew NodeIf(result);
        NodeBase* position = _Start->m_next;
        _Node1->m_previous = position->m_previous;
        position->m_previous->m_next = _Node1;
        _Node1->m_next = position;
        position->m_previous = _Node1;
        return result;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::ElseIf(NodeBase* _Start, NodeBase* end)
    {   // add else node
        NodeIf* parent = (NodeIf*)_Start->m_next;
        NodeBase* first = end->m_next;
        end->m_next = 0;
        NodeBase* last = m_current;
        m_current = end;
        end->m_next = 0;
        last->m_next = end;
        while (parent->m_child)
        {
            parent = parent->m_child;
        }
        parent->m_child = aznew NodeIf(end);
        parent->m_child->m_next = first;
        first->m_previous = parent->m_child;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::AddRep(int min, int max, bool isGreedy)
    {   // add repeat node
        if (m_current->m_kind == NT_str && ((NodeString<Element>*)m_current)->m_data.Size() != 1)
        {   // move final character to new string node
            NodeString<Element>* node = (NodeString<Element>*)m_current;
            AddChar(node->m_data.Delete());
        }
        NodeBase* position = m_current;
        NodeEndRepetition* node0 = aznew NodeEndRepetition();
        NodeRepetition* nodeRep = aznew NodeRepetition(isGreedy, min, max, node0, m_root->m_loops++);
        node0->_Begin_rep = nodeRep;
        LinkNode(node0);
        if (position->m_kind == NT_end_group || position->m_kind == NT_end_capture)
        {
            position = ((NodeEndGroup*)position)->m_back;
        }
        position->m_previous->m_next = nodeRep;
        nodeRep->m_previous = position->m_previous;
        position->m_previous = nodeRep;
        nodeRep->m_next = position;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline RootNode* Builder<ForwardIterator, Element, RegExTraits>::EndPattern()
    {   // wrap up
        NewNode(NT_end);
        return m_root;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Builder<ForwardIterator, Element, RegExTraits>::DiscardPattern()
    {   // free memory
        DestroyNode(m_root);
        m_root = 0;
    }

    // IMPLEMENTATION OF Matcher
    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline bool Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::_Do_if(NodeIf* node)
    {   // apply if node
        TgtState<Iterator> start = m_tgtState;
        TgtState<Iterator> end = m_tgtState;
        bool isMatched = false;
        int finalLenght = -1;

        while (node)
        {   // process one branch of if
            m_tgtState = start;
            if (!_Match_pat(node->m_next))
            {
                ;
            }
            else if (!_Longest)
            {
                return (true);
            }
            else
            {   // record match
                int length = (int)AZStd:: distance(start.m_current, m_tgtState.m_current);
                if (finalLenght < length)
                {   // memorize longest so far
                    end = m_tgtState;
                    finalLenght = length;
                }
                isMatched = true;
            }
            node = node->m_child;
        }
        m_tgtState = isMatched ? end : start;
        return isMatched;
    }

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline bool Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::_Do_rep0(NodeRepetition* node, bool isGreedy)
    {   // apply repetition to loop with no nested if/do
        int index = 0;
        TgtState<Iterator> _St = m_tgtState;

        for (; index < node->m_min; ++index)
        {       // do minimum number of reps
            Iterator current = m_tgtState.m_current;
            if (!_Match_pat(node->m_next))
            {       // didn't match minimum number of reps, fail
                m_tgtState = _St;
                return (false);
            }
            else if (current == m_tgtState.m_current)
            {
                index = node->m_min - 1;    // skip matches that don't change state
            }
        }

        TgtState<Iterator> _Final = m_tgtState;
        bool isMatched = false;
        Iterator _Saved_pos = m_tgtState.m_current;

        if (!_Match_pat(node->m_endRep->m_next))
        {
            ;   // no full match yet
        }
        else if (!isGreedy)
        {
            return (true);  // go with current match
        }
        else
        {       // record an acceptable match and continue
            _Final = m_tgtState;
            isMatched = true;
        }

        while (node->m_max == -1 || index++ < node->m_max)
        {       // try another rep/tail match
            m_tgtState.m_current = _Saved_pos;
            m_tgtState.m_groupValid = _St.m_groupValid;
            if (!_Match_pat(node->m_next))
            {
                break;  // rep match failed, quit loop
            }
            Iterator _Mid = m_tgtState.m_current;
            if (!_Match_pat(node->m_endRep->m_next))
            {
                ;   // tail match failed, continue
            }
            else if (!isGreedy)
            {
                return (true);  // go with current match
            }
            else
            {       // record match and continue
                _Final = m_tgtState;
                isMatched = true;
            }

            if (_Saved_pos == _Mid)
            {
                break;  // rep match ate no additional elements, quit loop
            }
            _Saved_pos = _Mid;
        }

        m_tgtState = isMatched ? _Final : _St;
        return isMatched;
    }

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline bool Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::_Do_rep(NodeRepetition* node, bool isGreedy, int _Init_idx)
    {   // apply repetition
        if (node->m_sampleLoop < 0)
        {       // first time, determine if loop is simple
            NodeBase* _Nx = node->m_next;
            for (; _Nx->m_kind != NT_end_rep; _Nx = _Nx->m_next)
            {
                if (_Nx->m_kind == NT_if || _Nx->m_kind == NT_rep)
                {
                    break;
                }
            }
            node->m_sampleLoop = _Nx->m_kind == NT_end_rep ? 1 : 0;
        }
        if (0 <  node->m_sampleLoop)
        {
            return (_Do_rep0(node, isGreedy));
        }

        bool isMatched = false;
        TgtState<Iterator> _St = m_tgtState;
        LoopValues* _Psav = &m_loopVals[node->m_loopNumber];
        int _Loop_idx_sav = _Psav->_Loop_idx;
        Iterator* _Loop_iter_sav = (Iterator*)_Psav->_Loop_iter;
        Iterator _Cur_iter = m_tgtState.m_current;

        bool _Progress = _Init_idx == 0 || *_Loop_iter_sav != _Cur_iter;

        if (0 <= node->m_max && node->m_max <= _Init_idx)
        {
            isMatched = _Match_pat(node->m_endRep->m_next); // reps done, try tail
        }
        else if (_Init_idx < node->m_min)
        {       // try a required rep
            if (!_Progress)
            {
                isMatched = _Match_pat(node->m_endRep->m_next); // empty, try tail
            }
            else
            {       // try another required match
                _Psav->_Loop_idx = _Init_idx + 1;
                _Psav->_Loop_iter = &_Cur_iter;
                isMatched = _Match_pat(node->m_next);
            }
        }
        else if (!isGreedy)
        {       // not greedy, favor minimum number of reps
            isMatched = _Match_pat(node->m_endRep->m_next);
            if (!isMatched && _Progress)
            {       // tail failed, try another rep
                m_tgtState = _St;
                _Psav->_Loop_idx = _Init_idx + 1;
                _Psav->_Loop_iter = &_Cur_iter;
                isMatched = _Match_pat(node->m_next);
            }
        }
        else
        {       // greedy, favor maximum number of reps
            if (_Progress)
            {       // try another rep
                _Psav->_Loop_idx = _Init_idx + 1;
                _Psav->_Loop_iter = &_Cur_iter;
                isMatched = _Match_pat(node->m_next);
            }
            if (!_Progress && 1 < _Init_idx)
            {
                ;
            }
            else if (!isMatched)
            {       // rep failed, try tail
                _Psav->_Loop_idx = _Loop_idx_sav;
                _Psav->_Loop_iter = _Loop_iter_sav;
                m_tgtState = _St;
                isMatched = _Match_pat(node->m_endRep->m_next);
            }
        }

        if (!isMatched)
        {
            m_tgtState = _St;
        }
        _Psav->_Loop_idx = _Loop_idx_sav;
        _Psav->_Loop_iter = _Loop_iter_sav;
        return isMatched;
    }

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline bool Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::_Is_wc(Iterator _Ch0, int offset)
    {   // check for word boundary
        if ((offset == -1 && _Ch0 == m_begin && !(m_mflags & regex_constants::match_prev_avail)) || (offset == 0 && _Ch0 == m_end))
        {
            return (false);
        }
        else
        {   // test for word char
            Element ch = (offset ? *--_Ch0 : *_Ch0);

            // assume L'x' == 'x'
            return (ch == (char)ch && strchr("abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "0123456789_", ch) != 0);
        }
    }

    template<class _BidIt1, class _BidIt2, class Predicate>
    inline _BidIt1 _Cmp_chrange(_BidIt1 _Begin1, _BidIt1 _End1, _BidIt2 _Begin2, _BidIt2 _End2, Predicate pred, bool _Partial)
    {   // compare character ranges
        _BidIt1 result = _Begin1;
        while (_Begin1 != _End1 && _Begin2 != _End2)
        {
            if (!pred(*_Begin1++, *_Begin2++))
            {
                return (result);
            }
        }
        return (_Begin2 == _End2 ? _Begin1 : _Partial && _Begin1 == _End1 ? _Begin1 : result);
    }

    template<class _BidIt1, class _BidIt2, class RegExTraits>
    inline _BidIt1 _Compare(_BidIt1 _Begin1, _BidIt1 _End1, _BidIt2 _Begin2, _BidIt2 _End2, const RegExTraits& m_traits, regex_constants::syntax_option_type sflags, bool _Partial)
    {   // compare character ranges
        _BidIt1 result = _End1;
        if (sflags & regex_constants::collate)
        {
            result = _Cmp_chrange(_Begin1, _End1, _Begin2, _End2, CmpCollate<RegExTraits>(m_traits), _Partial);
        }
        else if (sflags & regex_constants::icase)
        {
            result = _Cmp_chrange(_Begin1, _End1, _Begin2, _End2, CmpIcase<RegExTraits>(m_traits), _Partial);
        }
        else
        {
            result = _Cmp_chrange(_Begin1, _End1, _Begin2, _End2, CmpCS<RegExTraits>(), _Partial);
        }
        return result;
    }

    template<class Element>
    inline bool _Lookup_range(Element ch, const RegExBuffer<Element>* _Bufptr)
    {   // check whether ch is in RegExBuffer
        for (int index = 0; index < _Bufptr->Size(); index += 2)
        {   // check current position
            if (_Bufptr->at(index) <= ch && ch <= _Bufptr->at(index + 1))
            {
                return true;
            }
        }
        return false;
    }

    template<class Element, class RegExTraits>
    inline bool _Lookup_equiv(Element ch, const RegExSequence<Element>* _Eq, const RegExTraits& m_traits)
    {   // check whether ch is in _Eq
        typename RegExTraits::string_type _Str0;
        typename RegExTraits::string_type _Str1(1, ch);
        _Str1 = m_traits.transform_primary(_Str1.begin(), _Str1.end());
        while (_Eq)
        {   // look for sequence of elements that are the right size
            for (int index = 0; index < _Eq->m_data.Size(); index += _Eq->m_size)
            {   // look for ch
                _Str0.assign(_Eq->m_data.String() + index, _Eq->m_size);
                _Str0 = m_traits.transform_primary(_Str0.begin(), _Str0.end());
                if (_Str0 == _Str1)
                {
                    return true;
                }
            }
            _Eq = _Eq->m_next;
        }
        return false;
    }

    template<class BidirectionalIterator, class Element>
    inline BidirectionalIterator _Lookup_coll(BidirectionalIterator first, BidirectionalIterator last, const RegExSequence<Element>* _Eq)
    {   // look for collation element [first, last) in _Eq
        while (_Eq)
        {   // look for sequence of elements that are the right size
            for (int index = 0; index < _Eq->m_data.Size(); index += _Eq->m_size)
            {   // look for character range
                BidirectionalIterator result = first;
                for (size_t _Jx = 0; _Jx < _Eq->m_size; ++_Jx)
                {   // check current character
                    if (*result++ != *(_Eq->m_data.String() + index + _Jx))
                    {
                        break;
                    }
                }
                if (result == last)
                {
                    return (last);
                }
            }
            _Eq = _Eq->m_next;
        }
        return (first);
    }

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline bool Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::_Do_class(NodeBase* _Nx)
    {   // apply bracket expression
        /*static*/
        int m_max = BITMAP_max;             // to quiet diagnostics
        /*static*/ int m_min = 0;

        bool _Found;
        Element ch = *m_tgtState.m_current;
        if (m_sflags & regex_constants::icase)
        {
            ch = m_traits.translate_nocase(ch);
        }
        Iterator result = m_tgtState.m_current;
        ++result;
        Iterator _Resx;
        NodeClass<Element, RegExTraits>* node = (NodeClass<Element, RegExTraits>*)_Nx;
        if (node->m_coll && (_Resx = _Lookup_coll(m_tgtState.m_current, m_end, node->m_coll))
            != m_tgtState.m_current)
        {   // check for collation element
            result = _Resx;
            _Found = true;
        }
        else if (node->m_ranges && (_Lookup_range((Element)(m_sflags & regex_constants::collate ? (int)m_traits.translate(ch) : (int)ch), node->m_ranges)))
        {
            _Found = true;
        }
        else if (m_min <= static_cast<int>(ch) && static_cast<int>(ch) < m_max)
        {
            _Found = node->m_small && node->m_small->Find(ch);
        }
        else if (node->m_large && AZStd::find(node->m_large->String(), node->m_large->String() + node->m_large->Size(), ch) != node->m_large->String() + node->m_large->Size())
        {
            _Found = true;
        }
        else if (node->_Classes != 0 && m_traits.isctype(ch, node->_Classes))
        {
            _Found = true;
        }
        else if (node->m_equiv && _Lookup_equiv(ch, node->m_equiv, m_traits))
        {
            _Found = true;
        }
        else
        {
            _Found = false;
        }
        if (_Found == (node->m_flags & NFLG_negate))
        {
            return false;
        }
        else
        {   // record result
            m_tgtState.m_current = result;
            return true;
        }
    }

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline bool Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::_Better_match()
    {   // check for better match under UNIX rules
        int index;

        for (index = 0; index < _Ncap; ++index)
        {       // check each capture group
            if (!m_result.m_groupValid[index] || !m_tgtState.m_groupValid[index])
            {
                ;
            }
            else if (m_result.m_groups[index].m_begin
                     != m_tgtState.m_groups[index].m_begin)
            {
                return (AZStd::distance(m_begin, m_result.m_groups[index].m_begin)
                        < AZStd::distance(m_begin, m_tgtState.m_groups[index].m_begin));
            }
            else if (m_result.m_groups[index].m_end
                     != m_tgtState.m_groups[index].m_end)
            {
                return (AZStd::distance(m_begin, m_result.m_groups[index].m_end)
                        < AZStd::distance(m_begin, m_tgtState.m_groups[index].m_end));
            }
        }
        return false;
    }

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline bool Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::_Match_pat(NodeBase* _Nx)
    {   // check for match
        if (0 < _Max_stack_count && --_Max_stack_count <= 0)
        {
            Internal::MatchError(regex_constants::error_stack);
            return false;
        }
        if (0 < _Max_complexity_count && --_Max_complexity_count <= 0)
        {
            Internal::MatchError(regex_constants::error_complexity);
            return false;
        }

        bool isFailed = false;
        while (_Nx != 0)
        {       // match current node
            switch (_Nx->m_kind)
            {       // handle current node's type
            case NT_nop:
                break;

            case NT_bol:
                if ((m_mflags & (regex_constants::match_not_bol
                                 | regex_constants::match_prev_avail))
                    == regex_constants::match_not_bol)
                {
                    isFailed = true;
                }
                else if (m_mflags & regex_constants::match_prev_avail
                         || m_tgtState.m_current != m_begin)
                {           // check for preceding newline
                    Iterator tmp = m_tgtState.m_current;
                    isFailed = *--tmp != Meta_nl;
                }
                break;

            case NT_eol:
                if ((m_mflags& regex_constants::match_not_eol) != 0 || (m_tgtState.m_current != m_end && *m_tgtState.m_current != Meta_nl))
                {
                    isFailed = true;
                }
                break;

            case NT_wbound:
            {           // check for word boundary
                bool _Is_bound;
                if (((m_mflags& regex_constants::match_not_bow) && m_tgtState.m_current == m_begin) || ((m_mflags& regex_constants::match_not_eow) && m_tgtState.m_current == m_end))
                {
                    _Is_bound = false;
                }
                else
                {
                    _Is_bound = _Is_wc(m_tgtState.m_current, -1) != _Is_wc(m_tgtState.m_current, 0);
                }
                bool _Neg = (_Nx->m_flags & NFLG_negate) != 0;
                if (_Is_bound == _Neg)
                {
                    isFailed = true;
                }
                break;
            }

            case NT_dot:
                if (m_tgtState.m_current == m_end || *m_tgtState.m_current == Meta_nl || *m_tgtState.m_current == Meta_cr)
                {
                    isFailed = true;
                }
                else
                {
                    ++m_tgtState.m_current;
                }
                break;

            case NT_str:
            {           // check for string match
                NodeString<Element>* node = (NodeString<Element>*)_Nx;
                Iterator result;
                if ((result = _Compare(m_tgtState.m_current, m_end,
                             node->m_data.String(),
                             node->m_data.String() + node->m_data.Size(),
                             m_traits, m_sflags,
                             (m_mflags& regex_constants::match_partial) != 0))
                    != m_tgtState.m_current)
                {
                    m_tgtState.m_current = result;
                }
                else
                {
                    isFailed = true;
                }
                break;
            }

            case NT_class:
            {           // check for bracket expression match
                isFailed = m_tgtState.m_current == m_end
                    || !_Do_class(_Nx);
                break;
            }

            case NT_group:
                break;

            case NT_end_group:
                break;

            case NT_neg_assert:
            case NT_assert:
            {           // check assert
                Iterator ch = m_tgtState.m_current;
                bool _Neg = _Nx->m_kind == NT_neg_assert;
                BackTracingState<Iterator> _St = m_tgtState;
                if (_Match_pat(((NodeAssert*)_Nx)->m_child) == _Neg)
                {           // restore initial state and indicate failure
                    m_tgtState = _St;
                    isFailed = true;
                }
                else
                {
                    m_tgtState.m_current = ch;
                }
                break;
            }

            case NT_end_assert:
                _Nx = 0;
                break;

            case NT_capture:
            {           // record current position
                NodeCapture* node = (NodeCapture*)_Nx;
                m_tgtState.m_groups[node->m_index].m_begin = m_tgtState.m_current;
                for (size_t index = m_tgtState.m_groupValid.size();
                     node->m_index < index; )
                {
                    m_tgtState.m_groupValid[--index] = false;
                }
                break;
            }

            case NT_end_capture:
            {           // record successful capture
                NodeEndGroup* node = (NodeEndGroup*)_Nx;
                NodeCapture* node0 = (NodeCapture*)node->m_back;
                if (_Cap || node0->m_index != 0)
                {           // update capture data
                    m_tgtState.m_groupValid[node0->m_index] = true;
                    m_tgtState.m_groups[node0->m_index].m_end = m_tgtState.m_current;
                }
                break;
            }

            case NT_back:
            {           // check back reference
                NodeBack* node = (NodeBack*)_Nx;
                if (m_tgtState.m_groupValid[node->m_index])
                {           // check for match
                    Iterator result = m_tgtState.m_current;
                    Iterator _Bx = m_tgtState.m_groups[node->m_index].m_begin;
                    Iterator _Ex = m_tgtState.m_groups[node->m_index].m_end;
                    if (_Bx != _Ex      // _Bx == _Ex for zero-length match
                        && (result = _Compare(m_tgtState.m_current, m_end,
                                    _Bx, _Ex, m_traits, m_sflags,
                                    (m_mflags& regex_constants::match_partial) != 0))
                        == m_tgtState.m_current)
                    {
                        isFailed = true;
                    }
                    else
                    {
                        m_tgtState.m_current = result;
                    }
                }
                break;
            }

            case NT_if:
                if (!_Do_if((NodeIf*)_Nx))
                {
                    isFailed = true;
                }
                _Nx = 0;
                break;

            case NT_endif:
                break;

            case NT_rep:
                if (!_Do_rep((NodeRepetition*)_Nx,
                        (_Nx->m_flags & NFLG_greedy) != 0, 0))
                {
                    isFailed = true;
                }
                _Nx = 0;
                break;

            case NT_end_rep:
            {           // return at end of loop
                NodeRepetition* _Nr = ((NodeEndRepetition*)_Nx)->_Begin_rep;
                LoopValues* _Psav = &m_loopVals[_Nr->m_loopNumber];

                if (_Nr->m_sampleLoop == 0 && !_Do_rep(_Nr,
                        (_Nr->m_flags & NFLG_greedy) != 0, _Psav->_Loop_idx))
                {
                    isFailed = true;        // recurse only if loop contains if/do
                }
                _Nx = 0;
                break;
            }

            case NT_begin:
                break;

            case NT_end:
                if (((m_mflags & (regex_constants::match_not_null | regex_constants::match_not_null1)) && m_begin == m_tgtState.m_current) || (m_isFull && m_tgtState.m_current != m_end))
                {
                    isFailed = true;
                }
                else if (!m_isMatched || _Better_match())
                {       // record successful match
                    m_result = m_tgtState;
                    m_isMatched = true;
                }
                _Nx = 0;
                break;

            default:
                Internal::MatchError(regex_constants::error_parse);
                return false;
            }

            if (isFailed)
            {
                _Nx = 0;
            }
            else if (_Nx)
            {
                _Nx = _Nx->m_next;
            }
        }

        if (0 < _Max_stack_count)
        {
            ++_Max_stack_count;
        }
        return !isFailed;
    }

    template<class BidirectionalIterator, class Element, class RegExTraits, class Iterator>
    inline BidirectionalIterator Matcher<BidirectionalIterator, Element, RegExTraits, Iterator>::Skip(BidirectionalIterator first, BidirectionalIterator last, NodeBase* _Node_arg)
    {   // skip until possible match
        NodeBase* _Nx = _Node_arg != 0 ? _Node_arg : m_rootNode;

        while (first != last && _Nx != 0)
        {   // check current node
            switch (_Nx->m_kind)
            {   // handle current node's type
            case NT_nop:
                break;

            case NT_bol:
            {       // check for embedded newline
                if (m_mflags & regex_constants::match_not_bol)
                {
                    return (last);
                }
                for (; first != last; ++first)
                {       // look for character following meta newline
                    Iterator previous = first;
                    if (*--previous == Meta_nl)
                    {
                        break;
                    }
                }
                return first;
            }

            case NT_eol:
                if (m_mflags & regex_constants::match_not_eol)
                {
                    return (last);
                }
                for (; first != last; ++first)
                {
                    if (*first == Meta_nl)
                    {
                        break;
                    }
                }
                return first;

            //          case NT_wbound:
            //          case NT_dot:

            case NT_str:
            {       // check for string match
                NodeString<Element>* node = (NodeString<Element>*)_Nx;
                for (; first != last; ++first)
                {           // look for starting match
                    BidirectionalIterator next = first;
                    if (_Compare(first, ++next,
                            node->m_data.String(),
                            node->m_data.String() + 1,
                            m_traits, m_sflags,
                            (m_mflags& regex_constants::match_partial) != 0)
                        != first)
                    {
                        break;
                    }
                }
                return first;
            }

            case NT_class:
            {       // check for string match
                for (; first != last; ++first)
                {           // look for starting match
                    static int m_max = BITMAP_max;      // to quiet diagnostics
                    static int m_min = 0;

                    bool _Found;
                    Element ch = *first;
                    NodeClass<Element, RegExTraits>* node =
                        (NodeClass<Element, RegExTraits>*)_Nx;
                    Iterator next = first;
                    ++next;

                    if (node->m_coll
                        && _Lookup_coll(first, next, node->m_coll)
                        != first)
                    {
                        _Found = true;
                    }
                    else if (node->m_ranges
                             && (_Lookup_range(
                                     (Element)(m_sflags & regex_constants::collate
                                               ? (int)m_traits.translate(ch)
                                               : (int)ch), node->m_ranges)))
                    {
                        _Found = true;
                    }
                    else if (m_min <= ch && ch < m_max)
                    {
                        _Found = node->m_small && node->m_small->Find(ch);
                    }
                    else if (node->m_large
                             && AZStd::find(node->m_large->String(), node->m_large->String() + node->m_large->Size(), ch)
                             != node->m_large->String() + node->m_large->Size())
                    {
                        _Found = true;
                    }
                    else if (node->_Classes
                             && m_traits.isctype(ch, node->_Classes))
                    {
                        _Found = true;
                    }
                    else if (node->m_equiv
                             && _Lookup_equiv(ch, node->m_equiv, m_traits))
                    {
                        _Found = true;
                    }
                    else
                    {
                        _Found = false;
                    }

                    if (_Found != (node->m_flags & NFLG_negate))
                    {
                        return first;
                    }
                }
            }
                return first;

            case NT_group:
                break;

            case NT_end_group:
                break;

            //          case NT_neg_assert:
            //          case NT_assert:

            case NT_end_assert:
                _Nx = 0;
                break;

            case NT_capture:
                break;

            case NT_end_capture:
                break;

            //          case NT_back:

            case NT_if:
            {       // check for soonest string match
                NodeIf* node = (NodeIf*)_Nx;

                for (; first != last && node != 0; node = node->m_child)
                {
                    last = Skip(first, last, node->m_next);
                }
                return last;
            }

            //          case NT_endif:
            //          case NT_rep:
            //          case NT_end_rep:

            case NT_begin:
                break;

            case NT_end:
                _Nx = 0;
                break;

            default:
                return (first);
            }
            if (_Nx)
            {
                _Nx = _Nx->m_next;
            }
        }
        return first;
    }

    // IMPLEMENTATION OF Parser
    // PARSER LANGUAGE FLAGS
    static const unsigned int _ECMA_flags = LF_ext_rep | LF_alt_pipe | LF_nex_grp | LF_nex_rep | LF_nc_grp
        | LF_asrt_gen | LF_asrt_wrd | LF_bckr | LF_ngr_rep
        | LF_esc_uni | LF_esc_hex | LF_esc_bsl | LF_esc_ffn | LF_esc_wsd
        | LF_esc_ctrl | LF_bzr_chr | LF_grp_esc | LF_ident_ECMA
        | LF_empty_grp;

    static const unsigned int _Basic_flags = LF_bckr | LF_lim_bckr | LF_anch_rstr
        | LF_star_beg | LF_empty_grp | LF_brk_rstr | LF_mtch_long;

    static const unsigned int _Grep_flags = _Basic_flags | LF_alt_nl | LF_no_nl;

    static const unsigned int _Extended_flags = LF_ext_rep | LF_alt_pipe | LF_nex_grp | LF_nex_rep | LF_ident_ERE
        | LF_paren_bal | LF_brk_rstr | LF_mtch_long;

    static const unsigned int _Awk_flags = _Extended_flags | LF_esc_oct | LF_esc_ffn | LF_esc_ffnx | LF_ident_awk;

    static const unsigned int _Egrep_flags = _Extended_flags | LF_alt_nl | LF_no_nl;

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::Error(regex_constants::error_type code)
    {   // handle errors
        if (m_errors)
        {
            m_errors->RegexError(code);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::IsEsc() const
    {   // assumes m_pat != m_end
        ForwardIterator _Ch0 = m_pat;
        return (++_Ch0 != m_end
                && ((!(m_lflags & LF_nex_grp)
                     && (*_Ch0 == Meta_lpar || *_Ch0 == Meta_rpar))
                    || (!(m_lflags & LF_nex_rep)
                        && (*_Ch0 == Meta_lbr || *_Ch0 == Meta_rbr))));
    }

    static const char Meta_map[] =
    {   // array of meta chars
        Meta_lpar, Meta_rpar, Meta_dlr, Meta_caret,
        Meta_dot, Meta_star, Meta_plus, Meta_query,
        Meta_lsq, Meta_rsq, Meta_bar, Meta_esc,
        Meta_dash, Meta_lbr, Meta_rbr, Meta_comma,
        Meta_colon, Meta_equal, Meta_exc, Meta_nl,
        Meta_cr, Meta_bsp,
        0
    };

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::Trans()
    {   // map character to meta-character
        if (m_pat == m_end)
        {
            m_metaChar = Meta_eos, m_char = (Element)Meta_eos;
        }
        else
        {   // map current character
            m_char = *m_pat;
            m_metaChar = strchr(Meta_map, m_char) != 0 ? (MetaType)m_char : Meta_chr;
        }
        switch (m_char)
        {   // handle special cases
        case Meta_esc:
            if (IsEsc())
            {           // replace escape sequence
                ForwardIterator _Ch0 = m_pat;
                m_metaChar = MetaType(m_char = *++_Ch0);
            }
            break;

        case Meta_nl:
            if ((m_lflags & LF_alt_nl) && m_disjointIndex == 0)
            {
                m_metaChar = Meta_bar;
            }
            break;

        case Meta_lpar:
        case Meta_rpar:
            if (!(m_lflags & LF_nex_grp))
            {
                m_metaChar = Meta_chr;
            }
            break;

        case Meta_lbr:
        case Meta_rbr:
            if (!(m_lflags & LF_nex_rep))
            {
                m_metaChar = Meta_chr;
            }
            break;

        case Meta_star:
            if ((m_lflags & LF_star_beg)
                && m_builder.BeginExpression())
            {
                m_metaChar = Meta_chr;
            }
            break;

        case Meta_caret:
            if ((m_lflags & LF_anch_rstr)
                && !m_builder.BeginExpression())
            {
                m_metaChar = Meta_chr;
            }
            break;

        case Meta_dlr:
        {           // check if $ is special
            ForwardIterator _Ch0 = m_pat;
            if ((m_lflags & LF_anch_rstr)
                && ++_Ch0 != m_end && *_Ch0 != Meta_nl)
            {
                m_metaChar = Meta_chr;
            }
            break;
        }

        case Meta_plus:
        case Meta_query:
            if (!(m_lflags & LF_ext_rep))
            {
                m_metaChar = Meta_chr;
            }
            break;

        case Meta_bar:
            if (!(m_lflags & LF_alt_pipe))
            {
                m_metaChar = Meta_chr;
            }
            break;
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::Next()
    {   // advance to next input character
        if (m_pat != m_end)
        {   // advance
            if (*m_pat == Meta_esc && IsEsc())
            {
                ++m_pat;
            }
            ++m_pat;
        }
        Trans();
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::Expect(MetaType _St, regex_constants::error_type code)
    {   // check whether current meta-character is _St
        if (m_metaChar != _St)
        {
            Error(code);
        }
        Next();
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline int Parser<ForwardIterator, Element, RegExTraits>::DoDigits(int base, int count)
    {   // translate digits to numeric value
        int ch;
        m_value = 0;
        while (count != 0 && (ch = m_traits.value(m_char, base)) != -1)
        {   // append next digit
            --count;
            m_value *= base;
            m_value += ch;
            Next();
        }
        return count;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::DecimalDigits()
    {   // check for decimal value
        return (DoDigits(10, AZStd::numeric_limits<int>::max()) != AZStd::numeric_limits<int>::max());
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::HexDigits(int count)
    {   // check for count hex digits
        if (DoDigits(16, count) != 0)
        {
            Error(regex_constants::error_escape);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::_OctalDigits()
    {   // check for up to 3 octal digits
        return (DoDigits(8, 3) != 3);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_Do_ex_class(MetaType end)
    {   // handle delimited expressions within bracket expression
        regex_constants::error_type _Errtype =
            (end == Meta_colon ? regex_constants::error_ctype
             : end == Meta_equal ? regex_constants::error_collate
             : end == Meta_dot ? regex_constants::error_collate
             : regex_constants::error_syntax);
        ForwardIterator begin = m_pat;
        AZ_REGEX_DIFFT(ForwardIterator) diff = 0;

        while (m_metaChar != Meta_colon && m_metaChar != Meta_equal && m_metaChar != Meta_dot && m_metaChar != Meta_eos)
        {   // advance to end delimiter
            Next();
            ++diff;
        }
        if (m_metaChar != end)
        {
            Error(_Errtype);
        }
        else if (end == Meta_colon)
        {   // handle named character class
            typename RegExTraits::char_class_type _Cls =
                m_traits.lookup_classname(begin, m_pat,
                    (m_flags& regex_constants::icase) != 0);
            if (!_Cls)
            {
                Error(regex_constants::error_ctype);
            }
            m_builder.AddNamedClass(_Cls);
        }
        else if (end == Meta_equal)
        {
            if (begin == m_pat)
            {
                Error(regex_constants::error_collate);
            }
            else
            {
                m_builder.AddEquiv(begin, m_pat, diff);
            }
        }
        else if (end == Meta_dot)
        {
            if (begin == m_pat)
            {
                Error(regex_constants::error_collate);
            }
            else
            {
                m_builder.AddColl(begin, m_pat, diff);
            }
        }
        Next();
        Expect(Meta_rsq, _Errtype);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::CharacterClassEscape(bool isAdd)
    {   // check for character class escape
        typename RegExTraits::char_class_type _Cls;
        ForwardIterator _Ch0 = m_pat;
        if (_Ch0 == m_end || (_Cls = m_traits.lookup_classname(m_pat, ++_Ch0, (m_flags& regex_constants::icase) != 0)) == 0)
        {
            return false;
        }

        if (isAdd)
        {
            m_builder.AddClass();
        }
        m_builder.AddNamedClass(_Cls);
        if (m_traits.isctype(m_char, RegExTraits::Ch_upper))
        {
            m_builder.Negate();
        }
        Next();
        return true;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline ParserClass Parser<ForwardIterator, Element, RegExTraits>::_ClassEscape(bool isAdd)
    {   // check for class escape
        if ((m_lflags & LF_esc_bsl) && m_char == Esc_bsl)
        {   // handle escape backslash if allowed
            m_value = Esc_bsl;
            Next();
            return (_Prs_chr);
        }
        else if ((m_lflags & LF_esc_wsd) && CharacterClassEscape(isAdd))
        {
            return (_Prs_set);
        }
        else if (DecimalDigits())
        {       // check for invalid value
            if (m_value != 0)
            {
                Error(regex_constants::error_escape);
            }
            return (_Prs_chr);
        }
        return (_CharacterEscape() ? _Prs_chr : _Prs_none);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline ParserClass Parser<ForwardIterator, Element, RegExTraits>::_ClassAtom()
    {   // check for class atom
        if (m_metaChar == Meta_esc)
        {   // check for valid escape sequence
            Next();
            if (m_lflags & LF_grp_esc)
            {
                return (_ClassEscape(false));
            }
            else if (((m_lflags & LF_esc_ffn) && _Do_ffn(m_char)) || ((m_lflags & LF_esc_ffnx) && _Do_ffnx(m_char)))
            {   // advance to next character
                Next();
                return (_Prs_chr);
            }
            m_value = Meta_esc;
            return _Prs_chr;
        }
        else if (m_metaChar == Meta_lsq)
        {   // check for valid delimited expression
            Next();
            if (m_metaChar == Meta_colon || m_metaChar == Meta_equal || m_metaChar == Meta_dot)
            {   // handle delimited expression
                MetaType _St = m_metaChar;
                Next();
                _Do_ex_class(_St);
                return _Prs_set;
            }
            else
            {   // handle ordinary [
                m_value = Meta_lsq;
                return _Prs_chr;
            }
        }
        else if (m_metaChar == Meta_rsq || m_metaChar == Meta_eos)
        {
            return _Prs_none;
        }
        else
        {   // handle ordinary character
            m_value = m_char;
            Next();
            return _Prs_chr;
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_ClassRanges()
    {   // check for valid class ranges
        ParserClass _Ret;

        for (;; )
        {   // process characters through end of bracket expression
            if ((_Ret = _ClassAtom()) == _Prs_none)
            {
                return;
            }
            else if (_Ret == _Prs_set)
            {
                ;
            }
            else if (m_value == 0 && !(m_lflags & LF_bzr_chr))
            {
                Error(regex_constants::error_escape);
            }
            else if (m_metaChar == Meta_dash)
            {   // check for valid range
                Next();
                Element _Chr1 = (Element)m_value;
                if ((_Ret = _ClassAtom()) == _Prs_none)
                {   // treat - as ordinary character
                    m_builder.AddCharToClass((Element)m_value);
                    m_builder.AddCharToClass(Meta_dash);
                    return;
                }
                else if (_Ret == _Prs_set)
                {
                    Error(regex_constants::error_range);    // set follows dash
                }
                else if (m_flags & regex_constants::collate)
                {   // translate ends of range
                    m_value = m_traits.translate((Element)m_value);
                    _Chr1 = m_traits.translate(_Chr1);
                }
                if (static_cast<Element>(m_value) < _Chr1)
                {
                    Error(regex_constants::error_range);
                }
                m_builder.AddRange(_Chr1, (Element)m_value);
            }
            else
            {
                m_builder.AddCharToClass((Element)m_value);
            }
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_CharacterClass()
    {   // add bracket expression
        m_builder.AddClass();
        if (m_metaChar == Meta_caret)
        {       // negate bracket expression
            m_builder.Negate();
            Next();
        }
        if ((m_lflags & LF_brk_rstr) && m_metaChar == Meta_rsq)
        {       // insert initial ] when not special
            m_builder.AddCharToClass(Meta_rsq);
            Next();
        }
        _ClassRanges();
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline
    void Parser<ForwardIterator, Element, RegExTraits>::_Do_capture_group()
    {   // add capture group
        NodeBase* _Pos1 = m_builder.BeginCaptureGroup(++m_groupIndex);
        _Disjunction();
        m_builder.EndGroup(_Pos1);
        m_finishedGroups.resize(m_groupIndex + 1);
        m_finishedGroups[((NodeCapture*)_Pos1)->m_index] = true;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_Do_noncapture_group()
    {   // add non-capture group
        NodeBase* _Pos1 = m_builder.BeginGroup();
        _Disjunction();
        m_builder.EndGroup(_Pos1);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_Do_assert_group(bool _Neg)
    {   // add assert group
        NodeBase* _Pos1 = m_builder.BeginAssertGroup(_Neg);
        _Disjunction();
        m_builder.EndAssertGroup(_Pos1);
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::_Wrapped_disjunction()
    {   // add disjunction inside group
        ++m_disjointIndex;
        if (!(m_lflags & LF_empty_grp) && m_metaChar == Meta_rpar)
        {
            Error(regex_constants::error_paren);
        }
        else if ((m_lflags & LF_nc_grp) && m_metaChar == Meta_query)
        {       // check for valid ECMAScript (?x ... ) group
            Next();
            MetaType ch = m_metaChar;
            Next();
            if (ch == Meta_colon)
            {
                _Do_noncapture_group();
            }
            else if (ch == Meta_exc)
            {       // process assert group, negating
                _Do_assert_group(true);
                --m_disjointIndex;
                return false;
            }
            else if (ch == Meta_equal)
            {       // process assert group
                _Do_assert_group(false);
                --m_disjointIndex;
                return false;
            }
            else
            {
                Error(regex_constants::error_syntax);
            }
        }
        else if (m_flags & regex_constants::nosubs)
        {
            _Do_noncapture_group();
        }
        else
        {
            _Do_capture_group();
        }
        --m_disjointIndex;
        return true;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::_IdentityEscape()
    {   // check for valid identity escape
        if ((m_lflags & LF_ident_ECMA) &&
            // ECMAScript identity escape characters
            !m_traits.isctype(m_char, RegExTraits::Ch_alnum)
            && m_char != '_')
        {
            ;
        }
        else if (!(m_lflags & LF_ident_ECMA) &&
                 // BRE, ERE, awk identity escape characters
                 (m_char == Meta_dot
                  || m_char == Meta_lsq
                  || m_char == Meta_esc
                  || m_char == Meta_star
                  || m_char == Meta_bar
                  || m_char == Meta_caret
                  || m_char == Meta_dlr))
        {
            ;
        }
        else if ((m_lflags & LF_ident_ERE) &&
                 // additional ERE identity escape characters
                 (m_char == Meta_lpar
                  || m_char == Meta_rpar
                  || m_char == Meta_plus
                  || m_char == Meta_query
                  || m_char == Meta_lbr
                  || m_char == Meta_rbr))
        {
            ;
        }
        else if ((m_lflags & LF_ident_awk) &&
                 // additional awk identity escape characters
                 (m_char == '"' || m_char == '/'))
        {
            ;
        }
        else
        {
            return false;
        }
        m_value = m_char;
        Next();
        return true;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::_Do_ffn(Element ch)
    {   // check for limited file format escape character
        if (ch == Esc_ctrl_b)
        {
            m_value = '\b';
        }
        else if (ch == Esc_ctrl_f)
        {
            m_value = '\f';
        }
        else if (ch == Esc_ctrl_n)
        {
            m_value = '\n';
        }
        else if (ch == Esc_ctrl_r)
        {
            m_value = '\r';
        }
        else if (ch == Esc_ctrl_t)
        {
            m_value = '\t';
        }
        else if (ch == Esc_ctrl_v)
        {
            m_value = '\v';
        }
        else
        {
            return false;
        }
        return true;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::_Do_ffnx(Element ch)
    {   // check for the remaining file format escape character
        if (ch == Esc_ctrl_a)
        {
            m_value = '\a';
        }
        else
        {
            return false;
        }
        return true;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::_CharacterEscape()
    {   // check for valid character escape
        if (((m_lflags & LF_esc_ffn) && _Do_ffn(m_char)) || ((m_lflags & LF_esc_ffnx) && _Do_ffnx(m_char)))
        {
            Next();
        }
        else if (m_char == Esc_ctrl && (m_lflags & LF_esc_ctrl))
        {   // handle control escape sequence
            Next();
            if (!m_traits.isctype(m_char, RegExTraits::Ch_alpha))
            {
                Error(regex_constants::error_escape);
            }
            m_value = (char)(m_char % 32);
            Next();
        }
        else if (m_char == Esc_hex && (m_lflags & LF_esc_hex))
        {   // handle hexadecimal escape sequence
            Next();
            HexDigits(2);
        }
        else if (m_char == Esc_uni && (m_lflags & LF_esc_uni))
        {   // handle unicode escape sequence
            Next();
            HexDigits(4);
        }
        else if ((m_lflags & LF_esc_oct) && _OctalDigits())
        {   // handle octal escape sequence
            if (m_value == 0)
            {
                Error(regex_constants::error_escape);
            }
        }
        else
        {
            return _IdentityEscape();
        }

        if (m_value < (int)(std::numeric_limits<Element>::min)() || (int)(std::numeric_limits<Element>::max)() < m_value)
        {
            Error(regex_constants::error_escape);
        }
        return true;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_AtomEscape()
    {   // check for valid atom escape
        if ((m_lflags & LF_bckr) && DecimalDigits())
        {       // check for valid back reference
            if (m_value == 0)
            {   // handle \0
                if (!(m_lflags & LF_bzr_chr))
                {
                    Error(regex_constants::error_escape);
                }
                else
                {
                    m_builder.AddChar((Element)m_value);
                }
            }
            else if (((m_lflags & LF_lim_bckr) && NFA_BRE_MAX_GRP < m_value) || m_groupIndex < m_value || !m_finishedGroups[m_value])
            {
                Error(regex_constants::error_backref);
            }
            else
            {
                m_builder.AddBackreference(m_value);
            }
        }
        else if (_CharacterEscape())
        {
            m_builder.AddChar((Element)m_value);
        }
        else if (!(m_lflags & LF_esc_wsd) || !CharacterClassEscape(true))
        {
            Error(regex_constants::error_escape);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_Quantifier()
    {   // check for quantifier following atom
        int m_min = 0;
        int m_max = -1;
        if (m_metaChar == Meta_star)
        {
            ;
        }
        else if (m_metaChar == Meta_plus)
        {
            m_min = 1;
        }
        else if (m_metaChar == Meta_query)
        {
            m_max = 1;
        }
        else if (m_metaChar == Meta_lbr)
        {       // check for valid bracketed value
            Next();
            if (!DecimalDigits())
            {
                Error(regex_constants::error_badbrace);
            }
            m_min = m_value;
            if (m_metaChar != Meta_comma)
            {
                m_max = m_min;
            }
            else
            {       // check for decimal constant following comma
                Next();
                if (m_metaChar == Meta_rbr)
                {
                    ;
                }
                else if (!DecimalDigits())
                {
                    Error(regex_constants::error_badbrace);
                }
                else
                {
                    m_max = m_value;
                }
            }
            if (m_metaChar != Meta_rbr || (m_max != -1 && m_max < m_min))
            {
                Error(regex_constants::error_badbrace);
            }
        }
        else
        {
            return;
        }
        m_builder.MarkFinal();
        Next();
        if ((m_lflags & LF_ngr_rep) && m_metaChar == Meta_query)
        {       // add non-greedy repeat node
            Next();
            m_builder.AddRep(m_min, m_max, false);
        }
        else
        {
            m_builder.AddRep(m_min, m_max, true);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline bool Parser<ForwardIterator, Element, RegExTraits>::_Alternative()
    {   // check for valid alternative
        bool _Found = false;
        for (;; )
        {   // concatenate valid elements
            bool _Quant = true;
            if (m_metaChar == Meta_eos || m_metaChar == Meta_bar || (m_metaChar == Meta_rpar && m_disjointIndex != 0))
            {
                return (_Found);
            }
            else if (m_metaChar == Meta_rpar && !(m_lflags & LF_paren_bal))
            {
                Error(regex_constants::error_paren);
                return false;
            }
            else if (m_metaChar == Meta_dot)
            {   // add dot node
                m_builder.AddDot();
                Next();
            }
            else if (m_metaChar == Meta_esc)
            {   // check for valid escape sequence
                Next();
                if ((m_lflags & LF_asrt_wrd) && m_char == Esc_word)
                {   // add word assert
                    m_builder.AddWbound();
                    Next();
                    _Quant = false;
                }
                else if ((m_lflags & LF_asrt_wrd) && m_char == Esc_not_word)
                {   // add not-word assert
                    m_builder.AddWbound();
                    m_builder.Negate();
                    Next();
                    _Quant = false;
                }
                else
                {
                    _AtomEscape();
                }
            }
            else if (m_metaChar == Meta_lsq)
            {   // add bracket expression
                Next();
                _CharacterClass();
                Expect(Meta_rsq, regex_constants::error_brack);
            }
            else if (m_metaChar == Meta_lpar)
            {   // check for valid group
                Next();
                _Quant = _Wrapped_disjunction();
                Expect(Meta_rpar, regex_constants::error_paren);
            }
            else if (m_metaChar == Meta_caret)
            {   // add bol node
                m_builder.AddBoolean();
                Next();
                _Quant = false;
            }
            else if (m_metaChar == Meta_dlr)
            {   // add eol node
                m_builder.AddEOL();
                Next();
                _Quant = false;
            }
            else if (m_metaChar == Meta_star || m_metaChar == Meta_plus || m_metaChar == Meta_query || m_metaChar == Meta_lbr)
            {
                Error(regex_constants::error_badrepeat);
            }
            else if (m_metaChar == Meta_rbr && !(m_lflags & LF_paren_bal))
            {
                Error(regex_constants::error_brace);
            }
            else if (m_metaChar == Meta_rsq && !(m_lflags & LF_paren_bal))
            {
                Error(regex_constants::error_brack);
            }
            else
            {   // add character
                m_builder.AddChar(m_char);
                Next();
            }
            if (_Quant)
            {
                _Quantifier();
            }
            _Found = true;
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline void Parser<ForwardIterator, Element, RegExTraits>::_Disjunction()
    {   // check for valid disjunction
        NodeBase* _Pos1 = m_builder.GetMark();
        if (_Alternative())
        {
            ;
        }
        else if (m_metaChar != Meta_bar)
        {
            return; // zero-length alternative not followed by '|'
        }
        else
        {   // zero-length leading alternative
            NodeBase* _Pos3 = m_builder.BeginGroup();
            m_builder.EndGroup(_Pos3);
        }

        NodeBase* _Pos2 = m_builder.BeginIf(_Pos1);
        while (m_metaChar == Meta_bar)
        {   // append terms as long as we keep finding | characters
            Next();
            if (!_Alternative())
            {   // zero-length trailing alternative
                NodeBase* _Pos3 = m_builder.BeginGroup();
                m_builder.EndGroup(_Pos3);
            }
            m_builder.ElseIf(_Pos1, _Pos2);
        }
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline RootNode* Parser<ForwardIterator, Element, RegExTraits>::Compile()
    {   // compile regular expression
        RootNode* result = 0;
        NodeBase* _Pos1 = m_builder.BeginCaptureGroup(0);
        _Disjunction();
        if (m_pat != m_end)
        {
            Error(regex_constants::error_syntax);
            return nullptr;
        }
        m_builder.EndGroup(_Pos1);
        result = m_builder.EndPattern();
        result->flags = m_flags;
        result->m_marks = MarkCount();
        //m_builder.DiscardPattern();
        return result;
    }

    template<class ForwardIterator, class Element, class RegExTraits>
    inline Parser<ForwardIterator, Element, RegExTraits>::Parser(const RegExTraits& traits, ForwardIterator first, ForwardIterator last, regex_constants::syntax_option_type flags, Internal::ErrorSink* errors)
        : m_pat(first)
        , m_begin(first)
        , m_end(last)
        , m_groupIndex(0)
        , m_disjointIndex(0)
        , m_finishedGroups(0)
        , m_builder(traits, flags)
        , m_traits(traits)
        , m_flags(flags)
        , m_errors(errors)
    {
        using namespace regex_constants;
        m_lflags = (m_flags & gmask) == ECMAScript || (m_flags & gmask) == 0 ? _ECMA_flags
            : (m_flags & gmask) == basic ? _Basic_flags
            : (m_flags & gmask) == extended ? _Extended_flags
            : (m_flags & gmask) == awk ? _Awk_flags
            : (m_flags & gmask) == grep ? _Grep_flags
            : (m_flags & gmask) == egrep ? _Egrep_flags
            : 0;
        if (m_lflags & LF_mtch_long)
        {
            m_builder.SetLong();
        }
        Trans();
    }
} // namespace AZStd

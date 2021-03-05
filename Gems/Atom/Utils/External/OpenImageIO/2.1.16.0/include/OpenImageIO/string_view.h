// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>

#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>


OIIO_NAMESPACE_BEGIN


/// A `string_view` is a non-owning, non-copying, non-allocating reference
/// to a sequence of characters.  It encapsulates both a character pointer
/// and a length.
///
/// A function that takes a string input (but does not need to alter the
/// string in place) may use a string_view parameter and accept input that
/// is any of char* (C string), string literal (constant char array), a
/// std::string (C++ string), or OIIO ustring.  For all of these cases, no
/// extra allocations are performed, and no extra copies of the string
/// contents are performed (as they would be, for example, if the function
/// took a const std::string& argument but was passed a char* or string
/// literal).
///
/// Furthermore, a function that returns a copy or a substring of one of its
/// inputs (for example, a substr()-like function) may return a string_view
/// rather than a std::string, and thus generate its return value without
/// any allocation or copying. Upon assignment to a std::string or ustring,
/// it will properly auto-convert.
///
/// There are two important caveats to using this class:
/// 1. The string_view merely refers to characters owned by another string,
///    so the string_view may not be used outside the lifetime of the string
///    it refers to. Thus, string_view is great for parameter passing, but
///    it's not a good idea to use a string_view to store strings in a data
///    structure (unless you are really sure you know what you're doing).
/// 2. Because the run of characters that the string_view refers to may not
///    be 0-terminated, it is important to distinguish between the data()
///    method, which returns the pointer to the characters, and the c_str()
///    method, which is guaranteed to return a valid C string that is
///    0-terminated. Thus, if you want to pass the contents of a string_view
///    to a function that expects a 0-terminated string (say, fopen), you
///    must call fopen(my_string_view.c_str()).  Note that the usual case
///    is that the string_view does refer to a 0-terminated string, and in
///    that case c_str() returns the same thing as data() without any extra
///    expense; but in the rare case that it is not 0-terminated, c_str()
///    will incur extra expense to internally allocate a valid C string.
///


class OIIO_API string_view {
public:
    typedef char charT;
    typedef charT value_type;
    typedef const charT* pointer;
    typedef const charT* const_pointer;
    typedef const charT& reference;
    typedef const charT& const_reference;
    typedef const_pointer const_iterator;
    typedef const_iterator iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef const_reverse_iterator reverse_iterator;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::char_traits<charT> traits;
    static const size_type npos = ~size_type(0);

    /// Default ctr
    string_view() noexcept { init(nullptr, 0); }
    /// Copy ctr
    string_view(const string_view& copy) noexcept { init(copy.m_chars, copy.m_len); }
    /// Construct from char* and length.
    string_view(const charT* chars, size_t len) { init(chars, len); }
    /// Construct from char*, use strlen to determine length.
    string_view(const charT* chars) { init(chars, chars ? strlen(chars) : 0); }
    /// Construct from std::string. Remember that a string_view doesn't have
    /// its own copy of the characters, so don't use the `string_view` after
    /// the original string has been destroyed or altered.
    string_view(const std::string& str) noexcept { init(str.data(), str.size()); }

    /// Convert a string_view to a `std::string`.
    std::string str() const
    {
        return (m_chars ? std::string(m_chars, m_len) : std::string());
    }

    /// Explicitly request a 0-terminated string. USUALLY, this turns out to
    /// be just data(), with no significant added expense (because most uses
    /// of string_view are simple wrappers of C strings, C++ std::string, or
    /// ustring -- all of which are 0-terminated). But in the more rare case
    /// that the string_view represetns a non-0-terminated substring, it
    /// will force an allocation and copy underneath.
    ///
    /// Caveats:
    /// 1. This is NOT going to be part of the C++17 std::string_view, so
    ///    it's probably best to avoid this method if you want to have 100%
    ///    drop-in compatibility with std::string_view.
    /// 2. It is NOT SAFE to use c_str() on a string_view whose last char
    ///    is the end of an allocation -- because that next char may only
    ///    *coincidentally* be a '\0', which will cause c_str() to return
    ///    the string start (thinking it's a valid C string, so why not just
    ///    return its address?), if there's any chance that the subsequent
    ///    char could change from 0 to non-zero during the use of the result
    ///    of c_str(), and thus break the assumption that it's a valid C str.
    const char* c_str() const;

    // Assignment
    string_view& operator=(const string_view& copy) noexcept
    {
        init(copy.data(), copy.length());
        return *this;
    }

    /// Convert a string_view to a `std::string`.
    operator std::string() const { return str(); }

    // iterators
    const_iterator begin() const noexcept { return m_chars; }
    const_iterator end() const noexcept { return m_chars + m_len; }
    const_iterator cbegin() const noexcept { return m_chars; }
    const_iterator cend() const noexcept { return m_chars + m_len; }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator (end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator (begin()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator (end()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator (begin()); }

    // capacity
    size_type size() const noexcept { return m_len; }
    size_type length() const noexcept { return m_len; }
    size_type max_size() const noexcept { return m_len; }
    /// Is the string_view empty, containing no characters?
    bool empty() const noexcept { return m_len == 0; }

    /// Element access of an individual chracter (beware: no bounds
    /// checking!).
    const charT& operator[](size_type pos) const { return m_chars[pos]; }
    /// Element access with bounds checking and exception if out of bounds.
    const charT& at(size_t pos) const
    {
        if (pos >= m_len)
            throw(std::out_of_range("OpenImageIO::string_view::at"));
        return m_chars[pos];
    }
    const charT& front() const { return m_chars[0]; }
    const charT& back() const { return m_chars[m_len - 1]; }
    const charT* data() const noexcept { return m_chars; }

    // modifiers
    void clear() noexcept { init(nullptr, 0); }
    void remove_prefix(size_type n) noexcept
    {
        if (n > m_len)
            n = m_len;
        m_chars += n;
        m_len -= n;
    }
    void remove_suffix(size_type n) noexcept
    {
        if (n > m_len)
            n = m_len;
        m_len -= n;
    }

    string_view substr(size_type pos, size_type n = npos) const noexcept
    {
        if (pos >= size())
            return string_view();  // start past end -> return empty
        if (n == npos || pos + n > size())
            n = size() - pos;
        return string_view(data() + pos, n);
    }

    int compare (string_view x) const noexcept {
        const int cmp = std::char_traits<char>::compare (m_chars, x.m_chars, (std::min)(m_len, x.m_len));
        return cmp != 0 ? cmp : int(m_len - x.m_len);
        // Equivalent to:
        //  cmp != 0 ? cmp : (m_len == x.m_len ? 0 : (m_len < x.m_len ? -1 : 1));
    }

#if 0
    // Do these later if anybody needs them
    bool starts_with(string_view x) const noexcept;
    bool ends_with(string_view x) const noexcept;
    size_type copy(CharT* dest, size_type count, size_type pos = 0) const;
#endif

    /// Find the first occurrence of substring s in *this, starting at
    /// position pos.
    size_type find(string_view s, size_t pos = 0) const noexcept
    {
        if (pos > size())
            pos = size();
        const_iterator i = std::search(this->cbegin() + pos, this->cend(),
                                       s.cbegin(), s.cend(), traits::eq);
        return i == this->cend() ? npos : std::distance(this->cbegin(), i);
    }

    /// Find the first occurrence of character c in *this, starting at
    /// position pos.
    size_type find (charT c, size_t pos=0) const noexcept {
        if (pos > size())
            pos = size();
        const_iterator i = std::find_if (this->cbegin()+pos, this->cend(),
                                         traits_eq(c));
        return i == this->cend() ? npos : std::distance (this->cbegin(), i);
    }

    /// Find the last occurrence of substring s *this, but only those
    /// occurrences earlier than position pos.
    size_type rfind (string_view s, size_t pos=npos) const noexcept {
        if (pos > size())
            pos = size();
        const_reverse_iterator b = this->crbegin()+(size()-pos);
        const_reverse_iterator e = this->crend();
        const_reverse_iterator i = std::search (b, e, s.crbegin(), s.crend(), traits::eq);
        return i == e ? npos : (reverse_distance(this->crbegin(),i) - s.size() + 1);
    }

    /// Find the last occurrence of character c in *this, but only those
    /// occurrences earlier than position pos.
    size_type rfind (charT c, size_t pos=npos) const noexcept {
        if (pos > size())
            pos = size();
        const_reverse_iterator b = this->crbegin()+(size()-pos);
        const_reverse_iterator e = this->crend();
        const_reverse_iterator i = std::find_if (b, e, traits_eq(c));
        return i == e ? npos : reverse_distance (this->crbegin(),i);
    }

    size_type find_first_of (charT c, size_t pos=0) const noexcept { return find (c, pos); }

    size_type find_last_of (charT c, size_t pos=npos) const noexcept { return rfind (c, pos); }

    size_type find_first_of (string_view s, size_t pos=0) const noexcept {
        if (pos >= size())
            return npos;
        const_iterator i = std::find_first_of (this->cbegin()+pos, this->cend(),
                                               s.cbegin(), s.cend(), traits::eq);
        return i == this->cend() ? npos : std::distance (this->cbegin(), i);
    }

    size_type find_last_of (string_view s, size_t pos=npos) const noexcept {
        if (pos > size())
            pos = size();
        size_t off = size()-pos;
        const_reverse_iterator i = std::find_first_of (this->crbegin()+off, this->crend(),
                                                       s.cbegin(), s.cend(), traits::eq);
        return i == this->crend() ? npos : reverse_distance (this->crbegin(), i);
    }

    size_type find_first_not_of (string_view s, size_t pos=0) const noexcept {
        if (pos >= size())
            return npos;
        const_iterator i = find_not_of (this->cbegin()+pos, this->cend(), s);
        return i == this->cend() ? npos : std::distance (this->cbegin(), i);
    }

    size_type find_first_not_of (charT c, size_t pos=0) const noexcept {
        if (pos >= size())
            return npos;
        for (const_iterator i = this->cbegin()+pos; i != this->cend(); ++i)
            if (! traits::eq (c, *i))
                return std::distance (this->cbegin(), i);
        return npos;
    }

    size_type find_last_not_of (string_view s, size_t pos=npos) const noexcept {
        if (pos > size())
            pos = size();
        size_t off = size()-pos;
        const_reverse_iterator i = find_not_of (this->crbegin()+off, this->crend(), s);
        return i == this->crend() ? npos : reverse_distance (this->crbegin(), i);
    }

    size_type find_last_not_of (charT c, size_t pos=npos) const noexcept {
        if (pos > size())
            pos = size();
        size_t off = size()-pos;
        for (const_reverse_iterator i = this->crbegin()+off; i != this->crend(); ++i)
            if (! traits::eq (c, *i))
                return reverse_distance (this->crbegin(), i);
        return npos;
    }

private:
    const charT* m_chars;
    size_t m_len;

    void init(const charT* chars, size_t len) noexcept
    {
        m_chars = chars;
        m_len   = len;
    }

    template<typename r_iter>
    size_type reverse_distance(r_iter first, r_iter last) const noexcept
    {
        return m_len - 1 - std::distance(first, last);
    }

    template<typename iter>
    iter find_not_of(iter first, iter last, string_view s) const noexcept
    {
        for (; first != last; ++first)
            if (!traits::find(s.data(), s.length(), *first))
                return first;
        return last;
    }

    class traits_eq {
    public:
        traits_eq (charT ch) noexcept : ch(ch) {}
        bool operator () (charT val) const noexcept { return traits::eq (ch, val); }
        charT ch;
    };
};



inline bool
operator==(string_view x, string_view y) noexcept
{
    return x.size() == y.size() ? (x.compare(y) == 0) : false;
}

inline bool
operator!=(string_view x, string_view y) noexcept
{
    return x.size() == y.size() ? (x.compare(y) != 0) : true;
}

inline bool
operator<(string_view x, string_view y) noexcept
{
    return x.compare(y) < 0;
}

inline bool
operator>(string_view x, string_view y) noexcept
{
    return x.compare(y) > 0;
}

inline bool
operator<=(string_view x, string_view y) noexcept
{
    return x.compare(y) <= 0;
}

inline bool
operator>=(string_view x, string_view y) noexcept
{
    return x.compare(y) >= 0;
}



// Inserter
inline std::ostream&
operator<<(std::ostream& out, const string_view& str)
{
    if (out.good())
        out.write(str.data(), str.size());
    return out;
}



// Temporary name equivalence
typedef string_view string_ref;


OIIO_NAMESPACE_END

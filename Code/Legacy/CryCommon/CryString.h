/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Custom reference counted string class.
//               Can easily be substituted instead of string


#ifndef CRYINCLUDE_CRYCOMMON_CRYSTRING_H
#define CRYINCLUDE_CRYCOMMON_CRYSTRING_H
#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/hash.h>

#if !defined(NOT_USE_CRY_STRING)

#include <string.h>
#include <wchar.h>
#include <limits.h>
#include <stdarg.h>
#include <ctype.h>
#include "LegacyAllocator.h"

#define CRY_STRING

// forward declaration of CryStackString
template<class T, size_t S>
class CryStackStringT;


class CConstCharWrapper;    //forward declaration for special const char * without memory allocations

//extern void CryDebugStr( const char *format,... );
//#define CRY_STRING_DEBUG(s) { if (*s) CryDebugStr( "[%6d] %s",_usedMemory(0),(s) );}
#define CRY_STRING_DEBUG(s)

class CryStringAllocator
    : public AZ::SimpleSchemaAllocator<AZ::HphaSchema>
{
public:
    AZ_TYPE_INFO(CryStringAllocator, "{763DFC83-8A6E-4FD9-B6BC-BBF56E93E4EE}");

    using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchema>;
    using Descriptor = Base::Descriptor;

    CryStringAllocator()
        : Base("CryStringAllocator", "Allocator for CryString<T>")
    {
        Descriptor desc;
        desc.m_systemChunkSize = 4 * 1024 * 1024;     // grow by 4 MB at a time
        desc.m_subAllocator = &AZ::AllocatorInstance<AZ::LegacyAllocator>::Get();
        Create(desc);
    }
};

//////////////////////////////////////////////////////////////////////////
// CryStringT class.
//////////////////////////////////////////////////////////////////////////
template <class T>
class CryStringT
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Types compatible with STL string.
    //////////////////////////////////////////////////////////////////////////
    typedef CryStringT<T> _Self;
    typedef size_t size_type;
    typedef T value_type;
    typedef const value_type* const_str;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef pointer iterator;
    typedef const_pointer const_iterator;

    enum _npos_type
    {
        npos = (size_type) ~0
    };

    //////////////////////////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////////////////////////
    CryStringT();

protected:
    CryStringT(const CConstCharWrapper& str);   //ctor for strings without memory allocations
    friend class CConstCharWrapper;
public:

    CryStringT(const _Self& str);
    CryStringT(const _Self& str, size_type nOff, size_type nCount);

    // Before September 2012 this constructor looked like this:
    //    explicit CryStringT( value_type ch, size_type nRepeat = 1 );
    // It was very error-prone, because the matching constructor
    // std::string constructor is different:
    //    std::string( size_type nRepeat, value_type ch );
    //
    // To prevent hard-to-catch bugs, we removed all calls of this
    // constructor in the existing CryEngine code (in September 2012)
    // and started using proper order of parameters (matching std::).
    //
    // To catch calls using the reversed arguments in other projects,
    // we retain the previous function with reversed arguments,
    // and declare it private.
    CryStringT(size_type nRepeat, value_type ch);
private:
    CryStringT(value_type ch, size_type nRepeat);

public:

    CryStringT(const_str str);
    CryStringT(const_str str, size_type nLength);
    CryStringT(const_iterator _First, const_iterator _Last);
    ~CryStringT();

    //////////////////////////////////////////////////////////////////////////
    // STL string like interface.
    //////////////////////////////////////////////////////////////////////////
    //! Operators.
    size_type length() const;
    size_type size() const;
    bool empty() const;
    void clear();  // free up the data

    //! Returns the storage currently allocated to hold the string, a value at least as large as length().
    size_type capacity() const;

    // Sets the capacity of the string to a number at least as great as a specified number.
    // nCount = 0 is shrinking string to fit number of characters in it.
    void reserve(size_type nCount = 0);

    _Self& append(const value_type* _Ptr);
    _Self& append(const value_type* _Ptr, size_type nCount);
    _Self& append(const _Self& _Str, size_type nOff, size_type nCount);
    _Self& append(const _Self& _Str);
    _Self& append(size_type nCount, value_type _Ch);
    _Self& append(const_iterator _First, const_iterator _Last);

    _Self& assign(const_str _Ptr);
    _Self& assign(const_str _Ptr, size_type nCount);
    _Self& assign(const _Self& _Str, size_type off, size_type nCount);
    _Self& assign(const _Self& _Str);
    _Self& assign(size_type nCount, value_type _Ch);
    _Self& assign(const_iterator _First, const_iterator _Last);

    value_type at(size_type index) const;

    const_iterator begin() const { return m_str; };
    const_iterator end() const { return m_str + length(); };

    iterator begin() { return m_str; };
    iterator end() { return m_str + length(); };

    // Following functions are commented out because they provide direct write access to
    // the string and such access doesn't work properly with our current reference-count
    // implementation.
    // If you really need write access to your string's elements, please consider
    // using CryStackStringT<> instead of CryString<>. Alternatively, you can modify
    // your string by multiple calls of erase() and append().
    // Note: If you need *linear memory read* access to your string's elements, use data()
    // or c_str(). If you need *linear memory write* access to your string's elements,
    // use a non-string class (std::vector<>, DynArray<>, etc.) instead of CryString<>.
    //value_type& at( size_type index );
    //iterator begin() { return m_str; };
    //iterator end() { return m_str+length(); };

    //! cast to C string operator.
    operator const_str() const {
        return m_str;
    }

    //! cast to C string.
    const value_type* c_str() const { return m_str; }
    const value_type* data() const { return m_str; };

    //////////////////////////////////////////////////////////////////////////
    // string comparison.
    //////////////////////////////////////////////////////////////////////////
    int compare(const _Self& _Str) const;
    int compare(size_type _Pos1, size_type _Num1, const _Self& _Str) const;
    int compare(size_type _Pos1, size_type _Num1, const _Self& _Str, size_type nOff, size_type nCount) const;
    int compare(const char* _Ptr) const;
    int compare(const wchar_t* _Ptr) const;
    int compare(size_type _Pos1, size_type _Num1, const value_type* _Ptr, size_type _Num2 = npos) const;

    // Case insensitive comparison
    int compareNoCase(const _Self& _Str) const;
    int compareNoCase(size_type _Pos1, size_type _Num1, const _Self& _Str) const;
    int compareNoCase(size_type _Pos1, size_type _Num1, const _Self& _Str, size_type nOff, size_type nCount) const;
    int compareNoCase(const value_type* _Ptr) const;
    int compareNoCase(size_type _Pos1, size_type _Num1, const value_type* _Ptr, size_type _Num2 = npos) const;

    // Copies at most a specified number of characters from an indexed position in a source string to a target character array.
    size_type copy(value_type* _Ptr, size_type nCount, size_type nOff = 0) const;

    void push_back(value_type _Ch) { _ConcatenateInPlace(&_Ch, 1); }
    void resize(size_type nCount, value_type _Ch = ' ');

    //! simple sub-string extraction
    _Self substr(size_type pos, size_type count = npos) const;

    // replace part of string.
    _Self& replace(value_type chOld, value_type chNew);
    _Self& replace(const_str strOld, const_str strNew);
    _Self& replace(size_type pos, size_type count, const_str strNew);
    _Self& replace(size_type pos, size_type count, const_str strNew, size_type count2);
    _Self& replace(size_type pos, size_type count, size_type nNumChars, value_type chNew);

    // insert new elements to string.
    _Self& insert(size_type nIndex, value_type ch);
    _Self& insert(size_type nIndex, size_type nCount, value_type ch);
    _Self& insert(size_type nIndex, const_str pstr);
    _Self& insert(size_type nIndex, const_str pstr, size_type nCount);

    //! delete count characters starting at zero-based index
    _Self& erase(size_type nIndex, size_type count = npos);

    //! searching (return starting index, or -1 if not found)
    //! look for a single character match
    //! like "C" strchr
    size_type find(value_type ch, size_type pos = 0) const;
    //! look for a specific sub-string
    //! like "C" strstr
    size_type find(const_str subs, size_type pos = 0) const;
    size_type rfind(value_type ch, size_type pos = npos) const;
    size_type rfind(const _Self& subs, size_type pos = 0) const;

    size_type find_first_of(value_type _Ch, size_type nOff = 0) const;
    size_type find_first_of(const_str charSet, size_type nOff = 0) const;
    //size_type find_first_of( const value_type* _Ptr,size_type _Off,size_type _Count ) const;
    size_type find_first_of(const _Self& _Str, size_type _Off = 0) const;

    size_type find_first_not_of(value_type _Ch, size_type _Off = 0) const;
    size_type find_first_not_of(const value_type* _Ptr, size_type _Off = 0) const;
    size_type find_first_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const;
    size_type find_first_not_of(const _Self& _Str, size_type _Off = 0) const;

    size_type find_last_of(value_type _Ch, size_type _Off = npos) const;
    size_type find_last_of(const value_type* _Ptr, size_type _Off = npos) const;
    size_type find_last_of(const value_type* _Ptr, size_type _Off, size_type _Count) const;
    size_type find_last_of(const _Self& _Str, size_type _Off = npos) const;

    size_type find_last_not_of(value_type _Ch, size_type _Off = npos) const;
    size_type find_last_not_of(const value_type* _Ptr, size_type _Off = npos) const;
    size_type find_last_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const;
    size_type find_last_not_of(const _Self& _Str, size_type _Off = npos) const;


    void swap(_Self& _Str);

    //////////////////////////////////////////////////////////////////////////
    // overloaded operators.
    //////////////////////////////////////////////////////////////////////////
    // overloaded indexing.
    //value_type operator[]( size_type index ) const; // same as at()
    //  value_type& operator[]( size_type index ); // same as at()

    // overloaded assignment
    _Self& operator=(const _Self& str);
    _Self& operator=(value_type ch);
    _Self& operator=(const_str str);

    template<size_t AnySize>
    CryStringT(const CryStackStringT<T, AnySize>& str);

protected:
    // we prohibit an implicit conversion from CryStackString to make user aware of allocation!
    // -> use string(stackedString) instead
    // as the private statement seems to be ignored (VS C++), we add a compile time error, see below
    template<size_t AnySize>
    _Self& operator=(const CryStackStringT<T, AnySize>& str)
    {
        // we add a compile-time error as the Visual C++ compiler seems to ignore the private statement?
#if defined(__clang__)
        //CLANG_TODO: clang verifies things differently
#else
        static_assert(false, "Use explicit string assignment when assigning from_StackString");
#endif
        // not reached, as above will generate a compile time error
        _Assign(str.c_str(), str.length());
        return *this;
    }

public:
    // string concatenation
    _Self& operator+=(const _Self& str);
    _Self& operator+=(value_type ch);
    _Self& operator+=(const_str str);

    //template <class TT> friend CryStringT<TT> operator+( const CryStringT<TT>& str1, const CryStringT<TT>& str2 );
    //template <class TT> friend CryStringT<TT> operator+( const CryStringT<TT>& str, value_type ch );
    //template <class TT> friend CryStringT<TT> operator+( value_type ch, const CryStringT<TT>& str );
    //template <class TT> friend CryStringT<TT> operator+( const CryStringT<TT>& str1, const_str str2 );
    //template <class TT> friend CryStringT<TT> operator+( const_str str1, const CryStringT<TT>& str2 );

    size_t GetAllocatedMemory() const
    {
        StrHeader* header = _header();
        if (header == _emptyHeader())
        {
            return 0;
        }
        return sizeof(StrHeader) + (header->nAllocSize + 1) * sizeof(value_type);
    }

    //////////////////////////////////////////////////////////////////////////
    // Extended functions.
    // This functions are not in the STL string.
    // They have an ATL CString interface.
    //////////////////////////////////////////////////////////////////////////
    //! Format string, use (sprintf)
    _Self& Format(const value_type* format, ...);

    //! Converts the string to lower-case
    //  This function uses the "C" locale for case-conversion (ie, A-Z only)
    _Self& MakeLower();

    //! Converts the string to upper-case
    //  This function uses the "C" locale for case-conversion (ie, A-Z only)
    _Self& MakeUpper();

    _Self& Trim();
    _Self& Trim(value_type ch);
    _Self& Trim(const value_type* sCharSet);

    _Self& TrimLeft();
    _Self& TrimLeft(value_type ch);
    _Self& TrimLeft(const value_type* sCharSet);

    _Self& TrimRight();
    _Self& TrimRight(value_type ch);
    _Self& TrimRight(const value_type* sCharSet);

    _Self SpanIncluding(const_str charSet) const;
    _Self SpanExcluding(const_str charSet) const;
    _Self Tokenize(const_str charSet, int& nStart) const;
    _Self Mid(size_type nFirst, size_type nCount = npos) const {  return substr(nFirst, nCount); };

    _Self Left(size_type count) const;
    _Self Right(size_type count) const;
    //////////////////////////////////////////////////////////////////////////

    // public utilities.
    static size_type _strlen(const_str str);
    static size_type _strnlen(const_str str, size_type maxLen);
    static const_str _strchr(const_str str, value_type c);
    static const_str _strrchr(const_str str, value_type c);
    static value_type* _strstr(value_type* str, const_str strSearch);
    static bool _IsValidString(const_str str);

#if defined(WIN32) || defined(WIN64)
    static int _vscpf(const_str format, va_list args);
#endif
    static int _vsnpf(value_type* buf, int cnt, const_str format, va_list args);


public:
    //////////////////////////////////////////////////////////////////////////
    // Only used for debugging statistics.
    //////////////////////////////////////////////////////////////////////////
    static size_t _usedMemory(ptrdiff_t size)
    {
        static size_t s_used_memory = 0;
        s_used_memory += size;
        return s_used_memory;
    }

protected:
    value_type* m_str; // pointer to ref counted string data

    // String header. Immediately after this header in memory starts actual string data.
    struct StrHeader
    {
        int nRefCount;
        int nLength;
        int nAllocSize;     // Size of memory allocated at the end of this class.

        value_type* GetChars() { return (value_type*)(this + 1); }
        void  AddRef() { nRefCount++; /*InterlockedIncrement(&_header()->nRefCount);*/};
        int   Release() { return --nRefCount; };
    };
    static StrHeader* _emptyHeader()
    {
        // Define 2 static buffers in a row. The 2nd is a dummy object to hold a single empty char string.
        static StrHeader sEmptyStringBuffer[2] = {
            {-1, 0, 0}, {0, 0, 0}
        };
        return &sEmptyStringBuffer[0];
    }

    // implementation helpers
    StrHeader* _header() const;

    void _AllocData(size_type nLen);
    static void _FreeData(StrHeader* pData);
    void _Free();
    void _Initialize();

    void _Concatenate(const_str sStr1, size_type nLen1, const_str sStr2, size_type nLen2);
    void _ConcatenateInPlace(const_str sStr, size_type nLen);
    void _Assign(const_str sStr, size_type nLen);
    void _MakeUnique();

    static void _copy(value_type* dest, const value_type* src, size_type count);
    static void _move(value_type* dest, const value_type* src, size_type count);
    static void _set(value_type* dest, value_type ch, size_type count);
};

// Variant of CryStringT<T> which does not share memory with other strings.
template<class T>
class CryStringLocalT
    : public CryStringT<T>
{
public:
    typedef CryStringT<T> BaseType;
    typedef typename BaseType::const_str const_str;
    typedef typename BaseType::value_type value_type;
    typedef typename BaseType::size_type size_type;
    typedef typename BaseType::iterator iterator;

    CryStringLocalT()
    {}
    CryStringLocalT(const CryStringLocalT& str)
        : BaseType(str.c_str())
    {}
    CryStringLocalT(const BaseType& str)
        : BaseType(str.c_str())
    {}
    template<size_t Size>
    CryStringLocalT(const CryStackStringT<T, Size>& str)
        : BaseType(str.c_str())
    {}
    CryStringLocalT(const_str str)
        : BaseType(str)
    {}
    CryStringLocalT(const_str str, size_t len)
        : BaseType(str, len)
    {}
    CryStringLocalT(const_str begin, const_str end)
        : BaseType(begin, end)
    {}
    CryStringLocalT(size_type nRepeat, value_type ch)
        : BaseType(nRepeat, ch)
    {}

    CryStringLocalT(const typename CryStringT<T>::_Self& str, size_type nOff, size_type nCount)
        : BaseType(str, nOff, nCount)
    {}

    CryStringLocalT& operator=(const BaseType& str)
    {
        BaseType::operator=(str.c_str());
        return *this;
    }
    CryStringLocalT& operator=(const CryStringLocalT& str)
    {
        BaseType::operator=(str.c_str());
        return *this;
    }
    CryStringLocalT& operator=(const_str str)
    {
        BaseType::operator=(str);
        return *this;
    }
    iterator begin() { return BaseType::m_str; }
    using BaseType::begin; // const version
    iterator end() { return BaseType::m_str + BaseType::length(); }
    using BaseType::end; // const version
};

typedef CryStringLocalT<char> CryStringLocal;


// wrapper class for creation of strings without memory allocation
// it creates a string with pointer pointing to const char* location
// destructor sets the string to empty
// NOTE: never copy a string from it, just use it as function parameters instead of const char* itself
class CConstCharWrapper
{
public:
    //passing *this is safe since the char pointer is already set and therefore is the this-ptr constructed complete enough
#pragma warning (push)
#pragma warning (disable : 4355)
    CConstCharWrapper(const char* const cpString)
        : cpChar(cpString)
        , str(*this){AZ_Assert(cpString, "c-string parameter cannot be nullptr"); } //create stack string
#pragma warning (pop)
    ~CConstCharWrapper(){str.m_str = CryStringT<char>::_emptyHeader()->GetChars(); }//reset string
    operator const CryStringT<char>&() const {
        return str;
    }                                                     //cast operator to const string reference
private:
    const char* const cpChar;
    CryStringT<char> str;

    char* GetCharPointer() const {return const_cast<char*>(cpChar); }    //access function for string ctor

    friend class CryStringT<char>;  //both are bidirectional friends to avoid any other accesses
};


//macro needed because compiler somehow cannot find the cast operator when not invoked directly
#define CONST_TEMP_STRING(a) ((const string&)CConstCharWrapper(a))

/////////////////////////////////////////////////////////////////////////////
// CryStringT<T> Implementation
//////////////////////////////////////////////////////////////////////////

template <class T>
inline typename CryStringT<T>::StrHeader * CryStringT<T>::_header() const
{
    AZ_Assert(m_str != nullptr, "string header is nullptr");
    return ((StrHeader*)m_str) - 1;
}

template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::_strlen(const_str str)
{
    return (str == NULL) ? 0 : (size_type)::strlen(str);
}

template <>
inline CryStringT<wchar_t>::size_type CryStringT<wchar_t>::_strlen(const_str str)
{
    return (str == NULL) ? 0 : (size_type)::wcslen(str);
}

template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::_strnlen(const_str str, size_type maxLen)
{
    size_type len = 0;
    if (str)
    {
        while (len < maxLen && *str != '\0')
        {
            len++;
            str++;
        }
    }
    return len;
}

template <class T>
inline typename CryStringT<T>::const_str CryStringT<T>::_strchr(const_str str, value_type c)
{
    return (str == NULL) ? 0 : ::strchr(str, c);
}

template <>
inline CryStringT<wchar_t>::const_str CryStringT<wchar_t>::_strchr(const_str str, value_type c)
{
    return (str == NULL) ? 0 : ::wcschr(str, c);
}

template <class T>
inline typename CryStringT<T>::const_str CryStringT<T>::_strrchr(const_str str, value_type c)
{
    return (str == NULL) ? 0 : ::strrchr(str, c);
}

template <>
inline CryStringT<wchar_t>::const_str CryStringT<wchar_t>::_strrchr(const_str str, value_type c)
{
    return (str == NULL) ? 0 : ::wcsrchr(str, c);
}

template <class T>
inline typename CryStringT<T>::value_type * CryStringT<T>::_strstr(value_type * str, const_str strSearch)
{
    return (str == NULL) ? 0 : (value_type*)::strstr(str, strSearch);
}

template <>
inline CryStringT<wchar_t>::value_type * CryStringT<wchar_t>::_strstr(value_type * str, const_str strSearch)
{
    return (str == NULL) ? 0 : ::wcsstr(str, strSearch);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_copy(value_type* dest, const value_type* src, size_type count)
{
    if (dest != src)
    {
        memcpy(dest, src, count * sizeof(value_type));
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_move(value_type* dest, const value_type* src, size_type count)
{
    memmove(dest, src, count * sizeof(value_type));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_set(value_type* dest, value_type ch, size_type count)
{
    static_assert(sizeof(value_type) == sizeof(T));
    static_assert(sizeof(value_type) == 1);
    memset(dest, ch, count);
}

//////////////////////////////////////////////////////////////////////////
template <>
inline void CryStringT<wchar_t>::_set(value_type* dest, value_type ch, size_type count)
{
    static_assert(sizeof(value_type) == sizeof(wchar_t));
    wmemset(dest, ch, count);
}

#if defined(WIN32) || defined(WIN64)

template<>
inline int CryStringT<char>::_vscpf(const_str format, va_list args)
{
    return _vscprintf(format, args);
}

template<>
inline int CryStringT<wchar_t>::_vscpf(const_str format, va_list args)
{
    return _vscwprintf(format, args);
}

template<>
inline int CryStringT<char>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
    return azvsnprintf(buf, cnt, format, args);
}

template<>
inline int CryStringT<wchar_t>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
    return azvsnwprintf(buf, cnt, format, args);
}

#else

template<>
inline int CryStringT<char>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
    return vsnprintf(buf, cnt, format, args);
}

template<>
inline int CryStringT<wchar_t>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
    return vswprintf(buf, cnt, format, args);
}

#endif

//////////////////////////////////////////////////////////////////////////
template <class T>
inline bool CryStringT<T>::_IsValidString(const_str)
{
    /*
    if (str == NULL)
        return false;
    int nLength = _strlen(str);
    return !::IsBadStringPtrA(str, nLength);
    */
    return true;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_Assign(const_str sStr, size_type nLen)
{
    // Check if this string is shared (reference count greater then 1) or not enough capacity to store new string.
    // Then allocate new string buffer.
    if (_header()->nRefCount > 1 || nLen > capacity())
    {
        _Free();
        _AllocData(nLen);
    }
    // Copy characters from new string to this buffer.
    _copy(m_str, sStr, nLen);
    // Set new length.
    _header()->nLength = aznumeric_cast<int>(nLen);
    // Make null terminated string.
    m_str[nLen] = 0;
    CRY_STRING_DEBUG(m_str)
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_Concatenate(const_str sStr1, size_type nLen1, const_str sStr2, size_type nLen2)
{
    size_type nLen = nLen1 + nLen2;

    if (nLen1 * 2 > nLen)
    {
        nLen = nLen1 * 2;
    }

    if (nLen != 0)
    {
        if (nLen < 8)
        {
            nLen = 8;
        }

        _AllocData(nLen);
        _copy(m_str, sStr1, nLen1);
        _copy(m_str + nLen1, sStr2, nLen2);
        _header()->nLength = aznumeric_cast<int>(nLen1 + nLen2);
        m_str[nLen1 + nLen2] = 0;
    }
    CRY_STRING_DEBUG(m_str)
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_ConcatenateInPlace(const_str sStr, size_type nLen)
{
    if (nLen != 0)
    {
        // Check if this string is shared (reference count greater then 1) or not enough capacity to store new string.
        // Then allocate new string buffer.
        if (_header()->nRefCount > 1 || length() + nLen > capacity())
        {
            StrHeader* pOldData = _header();
            _Concatenate(m_str, length(), sStr, nLen);
            _FreeData(pOldData);
        }
        else
        {
            _copy(m_str + length(), sStr, nLen);
            _header()->nLength = aznumeric_cast<int>(_header()->nLength + nLen);
            m_str[_header()->nLength] = 0; // Make null terminated string.
        }
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_MakeUnique()
{
    if (_header()->nRefCount > 1)
    {
        // If string is shared, make a copy of string buffer.
        StrHeader* pOldData = _header();
        // This will not free header because reference count is greater then 1.
        _Free();
        // Allocate a new string buffer.
        _AllocData(pOldData->nLength);
        // Full copy of null terminated string.
        _copy(m_str, pOldData->GetChars(), pOldData->nLength + 1);
        CRY_STRING_DEBUG(m_str)
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_Initialize()
{
    m_str = _emptyHeader()->GetChars();
}

// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
template <class T>
inline void CryStringT<T>::_AllocData(size_type nLen)
{
    AZ_Assert(nLen <= (std::numeric_limits<int>::max)() - 1, "New string allocation size %zu is greater than the max allowed size of %d",
        nLen, (std::numeric_limits<int>::max)() - 1);    // max size (enough room for 1 extra)

    if (nLen == 0)
    {
        _Initialize();
    }
    else
    {
        size_type allocLen = sizeof(StrHeader) + (nLen + 1) * sizeof(value_type);

        StrHeader* pData = (StrHeader*)azmalloc(allocLen, 32, CryStringAllocator);

        _usedMemory(allocLen);   // For statistics.
        pData->nRefCount = 1;
        m_str = pData->GetChars();
        pData->nLength = aznumeric_cast<int>(nLen);
        pData->nAllocSize = aznumeric_cast<int>(nLen);
        m_str[nLen] = 0; // null terminated string.
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_Free()
{
    if (_header()->nRefCount >= 0) // Not empty string.
    {
        _FreeData(_header());
        _Initialize();
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::_FreeData(StrHeader* pData)
{
    //Cry uses -1 to represent strings on the stack. 
    if (pData->nRefCount < 0)
    {
        return;
    }

    if (pData->nRefCount == 0 || pData->Release() <= 0)
    {
        azfree((void*)pData, CryStringAllocator);
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::CryStringT()
{
    _Initialize();
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::CryStringT(const CryStringT<T>& str)
{
    AZ_Assert(str._header()->nRefCount != 0, "Reference count input cry string should be greater than 0");
    if (str._header()->nRefCount >= 0)
    {
        m_str = str.m_str;
        _header()->AddRef();
    }
    else
    {
        _Initialize();
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::CryStringT(const CryStringT<T>& str, size_type nOff, size_type nCount)
{
    _Initialize();
    assign(str, nOff, nCount);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::CryStringT(const_str str)
{
    _Initialize();
    // Make a copy of C string.
    size_type nLen = _strlen(str);
    if (nLen != 0)
    {
        _AllocData(nLen);
        _copy(m_str, str, nLen);
        CRY_STRING_DEBUG(m_str)
    }
}

template <class T>
inline CryStringT<T>::CryStringT(const CConstCharWrapper& str)
{
    _Initialize();
    m_str = const_cast<pointer>(str.GetCharPointer());
}

template<class T>
template<size_t AnySize>
inline CryStringT<T>::CryStringT(const CryStackStringT<T, AnySize>& str)
{
    _Initialize();
    const size_type nLength = str.length();
    if (nLength > 0)
    {
        _AllocData(nLength);
        _copy(m_str, str, nLength);
        CRY_STRING_DEBUG(m_str)
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::CryStringT(const_str str, size_type nLength)
{
    _Initialize();
    if (nLength > 0)
    {
        _AllocData(nLength);
        _copy(m_str, str, nLength);
        CRY_STRING_DEBUG(m_str)
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::CryStringT(size_type nRepeat, value_type ch)
{
    _Initialize();
    if (nRepeat > 0)
    {
        _AllocData(nRepeat);
        _set(m_str, ch, nRepeat);
        CRY_STRING_DEBUG(m_str)
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::CryStringT(const_iterator _First, const_iterator _Last)
{
    _Initialize();
    size_type nLength = (size_type)(_Last - _First);
    if (nLength > 0)
    {
        _AllocData(nLength);
        _copy(m_str, _First, nLength);
        CRY_STRING_DEBUG(m_str)
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>::~CryStringT()
{
    _FreeData(_header());
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::length() const
{
    return _header()->nLength;
}
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::size() const
{
    return _header()->nLength;
}
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::capacity() const
{
    return _header()->nAllocSize;
}

template <class T>
inline bool CryStringT<T>::empty() const
{
    return length() == 0;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::clear()
{
    if (length() == 0)
    {
        return;
    }
    if (_header()->nRefCount >= 0)
    {
        _Free();
    }
    else
    {
        resize(0);
    }
    AZ_Assert(length() == 0, "Cleared string should have a length of 0");
    AZ_Assert(_header()->nRefCount < 0 || capacity() == 0,
        "Cleared string should have a reference count or capacity of 0");
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::reserve(size_type nCount)
{
    // Reserve of 0 is shrinking container to fit number of characters in it..
    if (nCount > capacity())
    {
        StrHeader* pOldData = _header();
        _AllocData(nCount);
        _copy(m_str, pOldData->GetChars(), pOldData->nLength);
        _header()->nLength = pOldData->nLength;
        m_str[pOldData->nLength] = 0;
        _FreeData(pOldData);
    }
    else if (nCount == 0)
    {
        if (length() != capacity())
        {
            StrHeader* pOldData = _header();
            _AllocData(length());
            _copy(m_str, pOldData->GetChars(), pOldData->nLength);
            _FreeData(pOldData);
        }
    }
    CRY_STRING_DEBUG(m_str)
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::append(const_str _Ptr)
{
    *this += _Ptr;
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::append(const_str _Ptr, size_type nCount)
{
    _ConcatenateInPlace(_Ptr, nCount);
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::append(const CryStringT<T>& _Str, size_type off, size_type nCount)
{
    size_type len = _Str.length();
    if (off > len)
    {
        return *this;
    }
    if (off + nCount > len)
    {
        nCount = len - off;
    }
    _ConcatenateInPlace(_Str.m_str + off, nCount);
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::append(const CryStringT<T>& _Str)
{
    *this += _Str;
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::append(size_type nCount, value_type _Ch)
{
    if (nCount > 0)
    {
        if (_header()->nRefCount > 1 || length() + nCount > capacity())
        {
            StrHeader* pOldData = _header();
            _AllocData(length() + nCount);
            _copy(m_str, pOldData->GetChars(), pOldData->nLength);
            _set(m_str + pOldData->nLength, _Ch, nCount);
            _FreeData(pOldData);
        }
        else
        {
            size_type nOldLength = length();
            _set(m_str + nOldLength, _Ch, nCount);
            _header()->nLength = aznumeric_cast<int>(nOldLength + nCount);
            m_str[length()] = 0; // Make null terminated string.
        }
    }
    CRY_STRING_DEBUG(m_str)
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::append(const_iterator _First, const_iterator _Last)
{
    append(_First, (size_type)(_Last - _First));
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::assign(const_str _Ptr)
{
    *this = _Ptr;
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::assign(const_str _Ptr, size_type nCount)
{
    size_type len = _strnlen(_Ptr, nCount);
    _Assign(_Ptr, len);
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::assign(const CryStringT<T>& _Str, size_type off, size_type nCount)
{
    size_type len = _Str.length();
    if (off > len)
    {
        return *this;
    }
    if (off + nCount > len)
    {
        nCount = len - off;
    }
    _Assign(_Str.m_str + off, nCount);
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::assign(const CryStringT<T>& _Str)
{
    *this = _Str;
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::assign(size_type nCount, value_type _Ch)
{
    if (nCount >= 1)
    {
        _AllocData(nCount);
        _set(m_str, _Ch, nCount);
        CRY_STRING_DEBUG(m_str)
    }
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::assign(const_iterator _First, const_iterator _Last)
{
    assign(_First, (size_type)(_Last - _First));
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::value_type CryStringT<T>::at(size_type index) const
{
    AZ_Assert(index >= 0 && index < length(), "index into CryString is out of range");
    return m_str[index];
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline int CryStringT<T>::compare(const CryStringT<T>& _Str) const
{
    return compare(_Str.m_str);
}

template <class T>
inline int CryStringT<T>::compare(size_type _Pos1, size_type _Num1, const CryStringT<T>& _Str) const
{
    return compare(_Pos1, _Num1, _Str.m_str, npos);
}

template <class T>
inline int CryStringT<T>::compare(size_type _Pos1, size_type _Num1, const CryStringT<T>& _Str, size_type nOff, size_type nCount) const
{
    AZ_Assert(nOff < _Str.length(), "Offset into input string is larger than the index range");
    return compare(_Pos1, _Num1, _Str.m_str + nOff, nCount);
}

template <class T>
inline int CryStringT<T>::compare(const char* _Ptr) const
{
    return strcmp(m_str, _Ptr);
}

template <class T>
inline int CryStringT<T>::compare(const wchar_t* _Ptr) const
{
    return wcscmp(m_str, _Ptr);
}

template <class T>
inline int CryStringT<T>::compare(size_type _Pos1, size_type _Num1, const value_type* _Ptr, size_type _Num2) const
{
    AZ_Assert(_Pos1 < length(), "position index to compare is larger than the length of this string");
    if (length() - _Pos1 < _Num1)
    {
        _Num1 = length() - _Pos1; // trim to size
    }
    int res = _Num1 == 0 ? 0 : strncmp(m_str + _Pos1, _Ptr, (_Num1 < _Num2) ? _Num1 : _Num2);
    return (res != 0 ? res : _Num2 == npos && _Ptr[_Num1] == 0 ? 0 : _Num1 < _Num2 ? -1 : _Num1 == _Num2 ? 0 : +1);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline int CryStringT<T>::compareNoCase(const CryStringT<T>& _Str) const
{
    return _stricmp(m_str, _Str.m_str);
}

template <class T>
inline int CryStringT<T>::compareNoCase(size_type _Pos1, size_type _Num1, const CryStringT<T>& _Str) const
{
    return compareNoCase(_Pos1, _Num1, _Str.m_str, npos);
}

template <class T>
inline int CryStringT<T>::compareNoCase(size_type _Pos1, size_type _Num1, const CryStringT<T>& _Str, size_type nOff, size_type nCount) const
{
    AZ_Assert(nOff < _Str.length(), "offset to start comparison within input string is larger than the string index range");
    return compareNoCase(_Pos1, _Num1, _Str.m_str + nOff, nCount);
}

template <class T>
inline int CryStringT<T>::compareNoCase(const value_type* _Ptr) const
{
    return _stricmp(m_str, _Ptr);
}

template <class T>
inline int CryStringT<T>::compareNoCase(size_type _Pos1, size_type _Num1, const value_type* _Ptr, size_type _Num2) const
{
    AZ_Assert(_Pos1 < length(), "Position to start case-insensitive search is larger the indexable range");
    if (length() - _Pos1 < _Num1)
    {
        _Num1 = length() - _Pos1; // trim to size
    }
    int res = _Num1 == 0 ? 0 : _strnicmp(m_str + _Pos1, _Ptr, (_Num1 < _Num2) ? _Num1 : _Num2);
    return (res != 0 ? res : _Num2 == npos && _Ptr[_Num1] == 0 ? 0 : _Num1 < _Num2 ? -1 : _Num1 == _Num2 ? 0 : +1);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::copy(value_type* _Ptr, size_type nCount, size_type nOff) const
{
    AZ_Assert(nOff < length(), "Offset to copy from string to output address is outside the indexable range");
    if (nCount < 0)
    {
        nCount = 0;
    }
    if (nOff + nCount > length()) // trim to offset.
    {
        nCount = length() - nOff;
    }

    _copy(_Ptr, m_str + nOff, nCount);
    return nCount;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::resize(size_type nCount, value_type _Ch)
{
    _MakeUnique();
    if (nCount > length())
    {
        size_type numToAdd = nCount - length();
        append(numToAdd, _Ch);
    }
    else if (nCount < length())
    {
        _header()->nLength = nCount;
        m_str[length()] = 0; // Make null terminated string.
    }
}

//////////////////////////////////////////////////////////////////////////
//! compare helpers
template <class T>
inline bool operator==(const CryStringT<T>& s1, const CryStringT<T>& s2)
{ return s1.compare(s2) == 0; }
template <class T>
inline bool operator==(const CryStringT<T>& s1, const typename CryStringT<T>::value_type* s2)
{ return s1.compare(s2) == 0; }
template <class T>
inline bool operator==(const typename CryStringT<T>::value_type* s1, const CryStringT<T>& s2)
{ return s2.compare(s1) == 0; }
template <class T>
inline bool operator!=(const CryStringT<T>& s1, const CryStringT<T>& s2)
{ return s1.compare(s2) != 0; }
template <class T>
inline bool operator!=(const CryStringT<T>& s1, const typename CryStringT<T>::value_type* s2)
{ return s1.compare(s2) != 0; }
template <class T>
inline bool operator!=(const typename CryStringT<T>::value_type* s1, const CryStringT<T>& s2)
{ return s2.compare(s1) != 0; }
template <class T>
inline bool operator<(const CryStringT<T>& s1, const CryStringT<T>& s2)
{ return s1.compare(s2) < 0; }
template <class T>
inline bool operator<(const CryStringT<T>& s1, const typename CryStringT<T>::value_type* s2)
{ return s1.compare(s2) < 0; }
template <class T>
inline bool operator<(const typename CryStringT<T>::value_type* s1, const CryStringT<T>& s2)
{ return s2.compare(s1) > 0; }
template <class T>
inline bool operator>(const CryStringT<T>& s1, const CryStringT<T>& s2)
{ return s1.compare(s2) > 0; }
template <class T>
inline bool operator>(const CryStringT<T>& s1, const typename CryStringT<T>::value_type* s2)
{ return s1.compare(s2) > 0; }
template <class T>
inline bool operator>(const typename CryStringT<T>::value_type* s1, const CryStringT<T>& s2)
{ return s2.compare(s1) < 0; }
template <class T>
inline bool operator<=(const CryStringT<T>& s1, const CryStringT<T>& s2)
{ return s1.compare(s2) <= 0; }
template <class T>
inline bool operator<=(const CryStringT<T>& s1, const typename CryStringT<T>::value_type* s2)
{ return s1.compare(s2) <= 0; }
template <class T>
inline bool operator<=(const typename CryStringT<T>::value_type* s1, const CryStringT<T>& s2)
{ return s2.compare(s1) >= 0; }
template <class T>
inline bool operator>=(const CryStringT<T>& s1, const CryStringT<T>& s2)
{ return s1.compare(s2) >= 0; }
template <class T>
inline bool operator>=(const CryStringT<T>& s1, const typename CryStringT<T>::value_type* s2)
{ return s1.compare(s2) >= 0; }
template <class T>
inline bool operator>=(const typename CryStringT<T>::value_type* s1, const CryStringT<T>& s2)
{ return s2.compare(s1) <= 0; }

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::operator=(value_type ch)
{
    _Assign(&ch, 1);
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T> operator+(const CryStringT<T>& string1, typename CryStringT<T>::value_type ch)
{
    CryStringT<T> s(string1);
    s.append(1, ch);
    return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T> operator+(typename CryStringT<T>::value_type ch, const CryStringT<T>& str)
{
    CryStringT<T> s;
    s.reserve(str.size() + 1);
    s.append(1, ch);
    s.append(str);
    return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T> operator+(const CryStringT<T>& string1, const CryStringT<T>& string2)
{
    CryStringT<T> s(string1);
    s.append(string2);
    return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T> operator+(const CryStringT<T>& str1, const typename CryStringT<T>::value_type* str2)
{
    CryStringT<T> s(str1);
    s.append(str2);
    return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T> operator+(const typename CryStringT<T>::value_type* str1, const CryStringT<T>& str2)
{
    AZ_Assert(str1 == nullptr || CryStringT<T>::_IsValidString(str1), "Input string is not valid");
    CryStringT<T> s;
    s.reserve(CryStringT<T>::_strlen(str1) + str2.size());
    s.append(str1);
    s.append(str2);
    return s;
}

template <class T>
inline CryStringT<T>& CryStringT<T>::operator=(const CryStringT<T>& str)
{
    if (m_str != str.m_str)
    {
        if (_header()->nRefCount < 0)
        {
            // Empty string.
            //          _Assign( str.m_str,str.length() );
            if (str._header()->nRefCount < 0)
            {
                ; // two empty strings...
            }
            else
            {
                m_str = str.m_str;
                _header()->AddRef();
            }
        }
        else if (str._header()->nRefCount < 0)
        {
            // If source string is empty.
            _Free();
            m_str = str.m_str;
        }
        else
        {
            // Copy string reference.
            _Free();
            m_str = str.m_str;
            _header()->AddRef();
        }
    }
    return *this;
}

template <class T>
inline CryStringT<T>& CryStringT<T>::operator=(const_str str)
{
    AZ_Assert(str == nullptr || _IsValidString(str), "Input string is not valid");
    _Assign(str, _strlen(str));
    return *this;
}

template <class T>
inline CryStringT<T>& CryStringT<T>::operator+=(const_str str)
{
    AZ_Assert(str == nullptr || _IsValidString(str), "Input string is not valid");
    _ConcatenateInPlace(str, _strlen(str));
    return *this;
}

template <class T>
inline CryStringT<T>& CryStringT<T>::operator+=(value_type ch)
{
    _ConcatenateInPlace(&ch, 1);
    return *this;
}

template <class T>
inline CryStringT<T>& CryStringT<T>::operator+=(const CryStringT<T>& str)
{
    _ConcatenateInPlace(str.m_str, str.length());
    return *this;
}

//! find first single character
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find(value_type ch, size_type pos) const
{
    if (!(pos >= 0 && pos <= length()))
    {
        return (typename CryStringT<T>::size_type)npos;
    }
    const_str str = _strchr(m_str + pos, ch);
    // return npos if not found and index otherwise
    return (str == NULL) ? npos : (size_type)(str - m_str);
}

//! find a sub-string (like strstr)
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find(const_str subs, size_type pos) const
{
    AZ_Assert(_IsValidString(subs), "Input string is not valid");
    if (!(pos >= 0 && pos <= length()))
    {
        return npos;
    }

    // find first matching substring
    const_str str = _strstr(m_str + pos, subs);

    // return npos for not found, distance from beginning otherwise
    return (str == NULL) ? npos : (size_type)(str - m_str);
}

//! find last single character
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::rfind(value_type ch, size_type pos) const
{
    const_str str;
    if (pos == npos)
    {
        // find last single character
        str = _strrchr(m_str, ch);
        // return -1 if not found, distance from beginning otherwise
        return (str == NULL) ? (size_type) - 1 : (size_type)(str - m_str);
    }
    else
    {
        if (pos == npos)
        {
            pos = length();
        }
        if (!(pos >= 0 && pos <= length()))
        {
            return npos;
        }

        value_type tmp = m_str[pos + 1];
        m_str[pos + 1] = 0;
        str = _strrchr(m_str, ch);
        m_str[pos + 1] = tmp;
    }
    // return -1 if not found, distance from beginning otherwise
    return (str == NULL) ? (size_type) - 1 : (size_type)(str - m_str);
}

template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::rfind(const CryStringT<T>& subs, size_type pos) const
{
    size_type res = npos;
    for (int i = (int)size(); i >= (int)pos; --i)
    {
        size_type findRes = find(subs, i);
        if (findRes != npos)
        {
            res = findRes;
            break;
        }
    }
    return res;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_first_of(const CryStringT<T>& _Str, size_type _Off) const
{
    return find_first_of(_Str.m_str, _Off);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_first_of(value_type _Ch, size_type nOff) const
{
    if (!(nOff >= 0 && nOff <= length()))
    {
        return npos;
    }
    value_type charSet[2] = { _Ch, 0 };
    const_str str = strpbrk(m_str + nOff, charSet);
    return (str == NULL) ? (size_type) - 1 : (size_type)(str - m_str);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_first_of(const_str charSet, size_type nOff) const
{
    AZ_Assert(_IsValidString(charSet), "input c-string must not be nullptr");
    if (!(nOff >= 0 && nOff <= length()))
    {
        return npos;
    }
    const_str str = strpbrk(m_str + nOff, charSet);
    return (str == NULL) ? (size_type) - 1 : (size_type)(str - m_str);
}

template <>
inline CryStringT<wchar_t>::size_type CryStringT<wchar_t>::find_first_of(const_str charSet, size_type nOff) const
{
    AZ_Assert(_IsValidString(charSet), "input c-string must not be nullptr");
    if (!(nOff >= 0 && nOff <= length()))
    {
        return npos;
    }
    const_str str = wcspbrk(m_str + nOff, charSet);
    return (str == NULL) ? (size_type) - 1 : (size_type)(str - m_str);
}

//size_type find_first_not_of(const _Self& __s, size_type __pos = 0) const
//{ return find_first_not_of(__s._M_start, __pos, __s.size()); }

//size_type find_first_not_of(const _CharT* __s, size_type __pos = 0) const
//{ _STLP_FIX_LITERAL_BUG(__s) return find_first_not_of(__s, __pos, _Traits::length(__s)); }

//size_type find_first_not_of(const _CharT* __s, size_type __pos,   size_type __n) const;

//size_type find_first_not_of(_CharT __c, size_type __pos = 0) const;


//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_first_not_of(const value_type* _Ptr, size_type _Off) const
{ return find_first_not_of(_Ptr, _Off, _strlen(_Ptr)); }

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_first_not_of(const CryStringT<T>& _Str, size_type _Off) const
{ return find_first_not_of(_Str.m_str, _Off); }

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_first_not_of(value_type _Ch, size_type _Off) const
{
    if (_Off > length())
    {
        return npos;
    }
    else
    {
        for (const value_type* str = begin() + _Off; str != end(); ++str)
        {
            if (*str != _Ch)
            {
                return size_type(str - begin()); // Character found!
            }
        }
        return npos;
    }
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_first_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const
{
    if (_Off > length())
    {
        return npos;
    }
    else
    {
        const value_type* charsFirst = _Ptr, * charsLast = _Ptr + _Count;
        for (const value_type* str = begin() + _Off; str != end(); ++str)
        {
            const value_type* c;
            for (c = charsFirst; c != charsLast; ++c)
            {
                if (*c == *str)
                {
                    break;
                }
            }
            if (c == charsLast)
            {
                return size_type(str - begin());// Current character not in char set.
            }
        }
        return npos;
    }
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_of(value_type _Ch, size_type _Off) const
{
    size_type nLenght(length());
    // Empty strings, always return npos (same semantic as std::string).
    if (nLenght == 0)
    {
        return npos;
    }

    // If the offset is is bigger than the size of the string
    if (_Off >= nLenght)
    {
        // We set it to the size of the string -1, so we will not do bad pointer operations nor we will
        // test the null terminating character.
        _Off = nLenght - 1;
    }

    // From the character at the offset position, going to to the direction of the first character.
    for (const value_type* str = begin() + _Off; true; --str)
    {
        // We found a character in the string which matches the input character.
        if (*str == _Ch)
        {
            return size_type(str - begin()); // Character found!
        }
        // If the next element will be begin()-1, then we should stop.
        if (str == begin())
        {
            break;
        }
    }

    // We found nothing.
    return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_of(const value_type* _Ptr, size_type _Off) const
{
    // This function is actually a convenience alias...
    // BTW: what will happeb if wchar_t is used here?
    return find_last_of(_Ptr, _Off, _strlen(_Ptr));
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_of(const value_type* _Ptr, size_type _Off, size_type _Count) const
{
    size_type nLenght(length());
    // Empty strings, always return npos (same semantic as std::string).
    if (nLenght == 0)
    {
        return npos;
    }

    // If the offset is is bigger than the size of the string
    if (_Off >= nLenght)
    {
        // We set it to the size of the string -1, so we will not do bad pointer operations nor we will
        // test the null terminating character.
        _Off = nLenght - 1;
    }

    // From the character at the offset position, going to to the direction of the first character.
    const value_type* charsFirst = _Ptr, * charsLast = _Ptr + _Count;
    for (const value_type* str = begin() + _Off; true; --str)
    {
        const value_type* c;
        // For every character in the character set.
        for (c = charsFirst; c != charsLast; ++c)
        {
            // If the current character matches any of the charcaters in the input string...
            if (*c == *str)
            {
                // This is the value we must return.
                return size_type(str - begin());
            }
        }

        // If the next element will be begin()-1, then we should stop.
        if (str == begin())
        {
            break;
        }
    }
    // We couldn't find any character of the input string in the current string.
    return npos;
}
/////////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_of(const _Self& strCharSet, size_type _Off) const
{
    size_type nLenght(length());
    // Empty strings, always return npos (same semantic as std::string).
    if (nLenght == 0)
    {
        return npos;
    }

    // If the offset is is bigger than the size of the string
    if (_Off >= nLenght)
    {
        // We set it to the size of the string -1, so we will not do bad pointer operations nor we will
        // test the null terminating character.
        _Off = nLenght - 1;
    }


    // From the character at the offset position, going to to the direction of the first character.
    for (const value_type* str = begin() + _Off; true; --str)
    {
        // We check every character of the input string.
        for (const value_type* strInputCharacter = strCharSet.begin(); strInputCharacter != strCharSet.end(); ++strInputCharacter)
        {
            // If any character matches.
            if (*str == *strInputCharacter)
            {
                // We return the position where we found it.
                return size_type(str - begin()); // Character found!
            }
        }
        // If the next element will be begin()-1, then we should stop.
        if (str == begin())
        {
            break;
        }
    }

    // As we couldn't find any matching character...we return the appropriate value.
    return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_not_of(value_type _Ch, size_type _Off) const
{
    size_type nLenght(length());
    // Empty strings, always return npos (same semantic as std::string).
    if (nLenght == 0)
    {
        return npos;
    }

    // If the offset is is bigger than the size of the string
    if (_Off >= nLenght)
    {
        // We set it to the size of the string -1, so we will not do bad pointer operations nor we will
        // test the null terminating character.
        _Off = nLenght - 1;
    }


    // From the character at the offset position, going to to the direction of the first character.
    for (const value_type* str = begin() + _Off; true; --str)
    {
        // If the current character being analyzed is different of the input character.
        if (*str != _Ch)
        {
            // We found the last item which is not the input character before the given offset.
            return size_type(str - begin()); // Character found!
        }
        // If the next element will be begin()-1, then we should stop.
        if (str == begin())
        {
            break;
        }
    }

    // As we couldn't find any matching character...we return the appropriate value.
    return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_not_of(const value_type* _Ptr, size_type _Off) const
{
    // This function is actually a convenience alias...
    // BTW: what will happeb if wchar_t is used here?
    return find_last_not_of(_Ptr, _Off, _strlen(_Ptr));
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const
{
    size_type nLenght(length());
    // Empty strings, always return npos (same semantic as std::string).
    if (nLenght == 0)
    {
        return npos;
    }

    // If the offset is is bigger than the size of the string
    if (_Off >= nLenght)
    {
        // We set it to the size of the string -1, so we will not do bad pointer operations nor we will
        // test the null terminating character.
        _Off = nLenght - 1;
    }


    // From the character at the offset position, going to to the direction of the first character.
    const value_type* charsFirst = _Ptr, * charsLast = _Ptr + _Count;
    for (const value_type* str = begin() + _Off; true; --str)
    {
        bool boFoundAny(false);
        const value_type* c;
        // For every character in the character set.
        for (c = charsFirst; c != charsLast; ++c)
        {
            // If the current character matches any of the charcaters in the input string...
            if (*c == *str)
            {
                // So we signal it was found and stop this search.
                boFoundAny = true;
                break;
            }
        }

        // Using a different solution of the other similar methods
        // to make it easier to read.
        // If the character being analyzed is not in the set...
        if (!boFoundAny)
        {
            //.. we return the position where we found it.
            return size_type(str - begin());
        }

        // If the next element will be begin()-1, then we should stop.
        if (str == begin())
        {
            break;
        }
    }
    // We couldn't find any character of the input string not in the character set.
    return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename CryStringT<T>::size_type CryStringT<T>::find_last_not_of(const _Self& _Str, size_type _Off) const
{
    size_type nLenght(length());
    // Empty strings, always return npos (same semantic as std::string).
    if (nLenght == 0)
    {
        return npos;
    }

    // If the offset is is bigger than the size of the string
    if (_Off >= nLenght)
    {
        // We set it to the size of the string -1, so we will not do bad pointer operations nor we will
        // test the null terminating character.
        _Off = nLenght - 1;
    }


    // From the character at the offset position, going to to the direction of the first character.
    for (const value_type* str = begin() + _Off; true; --str)
    {
        bool boFoundAny(false);
        for (const value_type* strInputCharacter = _Str.begin(); strInputCharacter != _Str.end(); ++strInputCharacter)
        {
            // The character matched one of the character set...
            if (*strInputCharacter == *str)
            {
                // So we signal it was found and stop this search.
                boFoundAny = true;
                break;
            }
        }

        // Using a different solution of the other similar methods
        // to make it easier to read.
        // If the character being analyzed is not in the set...
        if (!boFoundAny)
        {
            //.. we return the position where we found it.
            return size_type(str - begin());
        }

        // If the next element will be begin()-1, then we should stop.
        if (str == begin())
        {
            break;
        }
    }

    // As we couldn't find any matching character...we return the appropriate value.
    return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T> CryStringT<T>::substr(size_type pos, size_type count) const
{
    if (pos >= length())
    {
        return CryStringT<T>();
    }
    if (count == npos)
    {
        count = length() - pos;
    }
    if (pos + count > length())
    {
        count = length() - pos;
    }
    return CryStringT<T>(m_str + pos, count);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::erase(size_type nIndex, size_type nCount)
{
    if (nIndex < 0)
    {
        nIndex = 0;
    }
    if (nCount < 0 || nCount > length() - nIndex)
    {
        nCount = length() - nIndex;
    }
    if (nCount > 0 && nIndex < length())
    {
        _MakeUnique();
        size_type nNumToCopy = length() - (nIndex + nCount) + 1;
        _move(m_str + nIndex, m_str + nIndex + nCount, nNumToCopy);
        _header()->nLength = length() - nCount;
    }

    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::insert(size_type nIndex, value_type ch)
{
    return insert(nIndex, 1, ch);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::insert(size_type nIndex, size_type nCount, value_type ch)
{
    _MakeUnique();

    if (nIndex < 0)
    {
        nIndex = 0;
    }

    size_type nNewLength = length();
    if (nIndex > nNewLength)
    {
        nIndex = nNewLength;
    }
    nNewLength += nCount;

    if (capacity() < nNewLength)
    {
        StrHeader* pOldData = _header();
        const_str pstr = m_str;
        _AllocData(nNewLength);
        _copy(m_str, pstr, pOldData->nLength + 1);
        _FreeData(pOldData);
    }

    _move(m_str + nIndex + nCount, m_str + nIndex, (nNewLength - nIndex - nCount) + 1);
    _set(m_str + nIndex, ch, nCount);
    _header()->nLength = nNewLength;
    CRY_STRING_DEBUG(m_str)

    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::insert(size_type nIndex, const_str pstr, size_type nCount)
{
    if (nIndex < 0)
    {
        nIndex = 0;
    }

    size_type nInsertLength = nCount;
    size_type nNewLength = length();
    if (nInsertLength > 0)
    {
        _MakeUnique();
        if (nIndex > nNewLength)
        {
            nIndex = nNewLength;
        }
        nNewLength += nInsertLength;

        if (capacity() < nNewLength)
        {
            StrHeader* pOldData = _header();
            const_str pOldStr = m_str;
            _AllocData(nNewLength);
            _copy(m_str, pOldStr, (pOldData->nLength + 1));
            _FreeData(pOldData);
        }

        _move(m_str + nIndex + nInsertLength, m_str + nIndex, (nNewLength - nIndex - nInsertLength + 1));
        _copy(m_str + nIndex, pstr, nInsertLength);
        _header()->nLength = nNewLength;
        m_str[length()] = 0;
    }
    CRY_STRING_DEBUG(m_str)

    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::insert(size_type nIndex, const_str pstr)
{
    return insert(nIndex, pstr, _strlen(pstr));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::replace(size_type pos, size_type count, const_str strNew)
{
    return replace(pos, count, strNew, _strlen(strNew));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::replace(size_type pos, size_type count, const_str strNew, size_type count2)
{
    erase(pos, count);
    insert(pos, strNew, count2);
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::replace(size_type pos, size_type count, size_type nNumChars, value_type chNew)
{
    erase(pos, count);
    insert(pos, nNumChars, chNew);
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::replace(value_type chOld, value_type chNew)
{
    if (chOld != chNew)
    {
        _MakeUnique();
        value_type* strend = m_str + length();
        for (value_type* str = m_str; str != strend; ++str)
        {
            if (*str == chOld)
            {
                *str = chNew;
            }
        }
    }
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::replace(const_str strOld, const_str strNew)
{
    size_type nSourceLen = _strlen(strOld);
    if (nSourceLen == 0)
    {
        return *this;
    }
    size_type nReplacementLen = _strlen(strNew);

    size_type nCount = 0;
    value_type* strStart = m_str;
    value_type* strEnd = m_str + length();
    value_type* strTarget;
    while (strStart < strEnd)
    {
        while ((strTarget = _strstr(strStart, strOld)) != NULL)
        {
            nCount++;
            strStart = strTarget + nSourceLen;
        }
        strStart += _strlen(strStart) + 1;
    }

    if (nCount > 0)
    {
        _MakeUnique();

        size_type nOldLength = length();
        size_type nNewLength =  nOldLength + (nReplacementLen - nSourceLen) * nCount;
        if (capacity() < nNewLength || _header()->nRefCount > 1)
        {
            StrHeader* pOldData = _header();
            const_str pstr = m_str;
            _AllocData(nNewLength);
            _copy(m_str, pstr, pOldData->nLength);
            _FreeData(pOldData);
        }
        strStart = m_str;
        strEnd = m_str + length();

        while (strStart < strEnd)
        {
            while ((strTarget = _strstr(strStart, strOld)) != NULL)
            {
                size_type nBalance = nOldLength - ((size_type)(strTarget - m_str) + nSourceLen);
                _move(strTarget + nReplacementLen, strTarget + nSourceLen, nBalance);
                _copy(strTarget, strNew, nReplacementLen);
                strStart = strTarget + nReplacementLen;
                strStart[nBalance] = 0;
                nOldLength += (nReplacementLen - nSourceLen);
            }
            strStart += _strlen(strStart) + 1;
        }
        _header()->nLength = nNewLength;
    }
    CRY_STRING_DEBUG(m_str)

    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void CryStringT<T>::swap(CryStringT<T>& _Str)
{
    value_type* temp = _Str.m_str;
    _Str.m_str = m_str;
    m_str = temp;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::Format(const_str format, ...)
{
    AZ_Assert(_IsValidString(format), "format c-string must not be nullptr");

#if defined(WIN32) || defined(WIN64)
    va_list argList;
    va_start(argList, format);
    int n = _vscpf(format, argList);
    if (n < 0)
    {
        n = 0;
    }
    resize(n);  //this will actually allocate n+1 elements to accommodate the null terminator
    _vsnpf(m_str, n, format, argList);
    va_end(argList);
    return *this;
#else
    value_type temp[4096]; // Limited to 4096 characters!
    va_list argList;
    va_start(argList, format);
    _vsnpf(temp, 4096, format, argList);
    temp[4095] = '\0';
    va_end(argList);
    *this = temp;
    return *this;
#endif
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::MakeLower()
{
    _MakeUnique();
    for (value_type* s = m_str; *s != 0; s++)
    {
        const value_type c = *s;
        *s = (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c; // ASCII only, standard "C" locale
    }
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::MakeUpper()
{
    _MakeUnique();
    for (value_type* s = m_str; *s != 0; s++)
    {
        const value_type c = *s;
        *s = (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c; // ASCII only, standard "C" locale
    }
    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::Trim()
{
    return TrimRight().TrimLeft();
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::Trim(value_type ch)
{
    _MakeUnique();
    const value_type chset[2] = { ch, 0 };
    return TrimRight(chset).TrimLeft(chset);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::Trim(const value_type* sCharSet)
{
    _MakeUnique();
    return TrimRight(sCharSet).TrimLeft(sCharSet);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::TrimRight(value_type ch)
{
    const value_type chset[2] = { ch, 0 };
    return TrimRight(chset);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::TrimRight(const value_type* sCharSet)
{
    if (!sCharSet || !(*sCharSet) || length() < 1)
    {
        return *this;
    }

    const value_type* last = m_str + length() - 1;
    const value_type* str = last;
    while ((str != m_str) && (_strchr(sCharSet, *str) != 0))
    {
        str--;
    }

    if (str != last)
    {
        // Just shrink length of the string.
        size_type nNewLength = (size_type)(str - m_str) + 1; // m_str can change in _MakeUnique
        _MakeUnique();
        _header()->nLength = nNewLength;
        m_str[nNewLength] = 0;
    }

    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::TrimRight()
{
    if (length() < 1)
    {
        return *this;
    }

    const value_type* last = m_str + length() - 1;
    const value_type* str = last;
    while ((str != m_str) && (isspace((unsigned char)*str) != 0))
    {
        str--;
    }

    if (str != last)     // something changed?
    {
        // Just shrink length of the string.
        size_type nNewLength = (size_type)(str - m_str) + 1; // m_str can change in _MakeUnique
        _MakeUnique();
        _header()->nLength = nNewLength;
        m_str[nNewLength] = 0;
    }

    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::TrimLeft(value_type ch)
{
    const value_type chset[2] = { ch, 0 };
    return TrimLeft(chset);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::TrimLeft(const value_type* sCharSet)
{
    if (!sCharSet || !(*sCharSet))
    {
        return *this;
    }

    const value_type* str = m_str;
    while ((*str != 0) && (_strchr(sCharSet, *str) != 0))
    {
        str++;
    }

    if (str != m_str)
    {
        size_type nOff = (size_type)(str - m_str); // m_str can change in _MakeUnique
        _MakeUnique();
        size_type nNewLength = length() - nOff;
        _move(m_str, m_str + nOff, nNewLength + 1);
        _header()->nLength = nNewLength;
        m_str[nNewLength] = 0;
    }

    return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T>& CryStringT<T>::TrimLeft()
{
    const value_type* str = m_str;
    while ((*str != 0) && (isspace((unsigned char)*str) != 0))
    {
        str++;
    }

    if (str != m_str)
    {
        size_type nOff = (size_type)(str - m_str); // m_str can change in _MakeUnique
        _MakeUnique();
        size_type nNewLength = length() - nOff;
        _move(m_str, m_str + nOff, nNewLength + 1);
        _header()->nLength = nNewLength;
        m_str[nNewLength] = 0;
    }

    return *this;
}

template <class T>
inline CryStringT<T> CryStringT<T>::Right(size_type count) const
{
    if (count == npos)
    {
        return CryStringT<T>();
    }
    else if (count > length())
    {
        return *this;
    }

    return CryStringT<T>(m_str + length() - count, count);
}

template <class T>
inline CryStringT<T> CryStringT<T>::Left(size_type count) const
{
    if (count == npos)
    {
        return CryStringT<T>();
    }
    else if (count > length())
    {
        count = length();
    }

    return CryStringT<T>(m_str, count);
}

// strspn equivalent
template <class T>
inline CryStringT<T> CryStringT<T>::SpanIncluding(const_str charSet) const
{
    AZ_Assert(_IsValidString(charSet), "input c-string must not be nullptr");
    return Left((size_type)strspn(m_str, charSet));
}

// strcspn equivalent
template <class T>
inline CryStringT<T> CryStringT<T>::SpanExcluding(const_str charSet) const
{
    AZ_Assert(_IsValidString(charSet), "input c-string must not be nullptr");
    return Left((size_type)strcspn(m_str, charSet));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline CryStringT<T> CryStringT<T>::Tokenize(const_str charSet, int& nStart) const
{
    if (nStart < 0)
    {
        return CryStringT<T>();
    }

    if (!charSet)
    {
        return *this;
    }

    const_str sPlace = m_str + nStart;
    const_str sEnd = m_str + length();
    if (sPlace < sEnd)
    {
        int nIncluding = (int)strspn(sPlace, charSet);

        if ((sPlace + nIncluding) < sEnd)
        {
            sPlace += nIncluding;
            int nExcluding = (int)strcspn(sPlace, charSet);
            int nFrom = nStart + nIncluding;
            nStart = nFrom + nExcluding + 1;

            return substr(nFrom, nExcluding);
        }
    }
    // Return empty string.
    nStart = -1;
    return CryStringT<T>();
}

//////////////////////////////////////////////////////////////////////////
// This code prevents std::string compiling in Win32 with STLPort
//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) && !defined(WIN64) && defined(_STLP_BEGIN_NAMESPACE) && !defined(DONT_BAN_STD_STRING)
#define CRYINCLUDE_STLP_STRING_FWD_H
#define CRYINCLUDE_STLP_INTERNAL_STRING_H
namespace std
{
    //const char* __get_c_string( const CryStringT<char> &str ) { return str.c_str(); };
    class string
    {
        // std::string must not be used if CryString included.
        // Use string instead.
    };
}
//////////////////////////////////////////////////////////////////////////
#endif // WIN64

typedef CryStringT<char> string;
typedef CryStringT<wchar_t> wstring;

#else // !defined(NOT_USE_CRY_STRING)

#include <string>  // STL string
typedef std::string string;
typedef std::wstring wstring;

#endif // !defined(NOT_USE_CRY_STRING)

namespace AZStd
{
    template <>
    struct hash<::string>
    {
        typedef ::string   argument_type;
        typedef size_t     result_type;
        inline result_type operator()(const argument_type& value) const 
        { 
            return hash_string(value.c_str(), value.length());
        }

        static size_t hash_string(const char* str, size_t length)
        {
            size_t hash = 14695981039346656037ULL;
            const size_t fnvPrime = 1099511628211ULL;
            const char* cptr = str;
            for (; length; --length)
            {
                hash ^= static_cast<size_t>(*cptr++);
                hash *= fnvPrime;
            }
            return hash;
        }
    };
}

#endif // CRYINCLUDE_CRYCOMMON_CRYSTRING_H

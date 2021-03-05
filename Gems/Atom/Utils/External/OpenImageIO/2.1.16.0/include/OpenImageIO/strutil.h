// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

/////////////////////////////////////////////////////////////////////////
/// @file  strutil.h
///
/// @brief String-related utilities, all in namespace Strutil.
/////////////////////////////////////////////////////////////////////////



#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <OpenImageIO/export.h>
#include <OpenImageIO/hash.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/platform.h>
#include <OpenImageIO/string_view.h>

// For now, let a prior set of OIIO_USE_FMT=0 cause us to fall back to
// tinyformat and/or disable its functionality. Use with caution!
#ifndef OIIO_USE_FMT
#    define OIIO_USE_FMT 1
#endif

#if OIIO_GNUC_VERSION >= 70000
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#ifndef FMT_HEADER_ONLY
#    define FMT_HEADER_ONLY
#endif
#ifndef FMT_EXCEPTIONS
#    define FMT_EXCEPTIONS 0
#endif
#ifndef FMT_USE_GRISU
#    define FMT_USE_GRISU 1
#endif
#if OIIO_USE_FMT
#    include "fmt/ostream.h"
#    include "fmt/format.h"
#    include "fmt/printf.h"
#endif
#if OIIO_GNUC_VERSION >= 70000
#    pragma GCC diagnostic pop
#endif

// Allow client software to know if this version of OIIO as Strutil::sprintf
#define OIIO_HAS_SPRINTF 1

// Allow client software to know if this version of OIIO has Strutil::format
// behave like sprintf (OIIO_FORMAT_IS_FMT==0) or like python / {fmt} /
// C++20ish std::format (OIIO_FORMAT_IS_FMT==1).
#define OIIO_FORMAT_IS_FMT 0

// Allow client software to know that at this moment, the fmt-based string
// formatting is locale-independent. This was 0 in older versions when fmt
// was locale dependent.
#define OIIO_FMT_LOCALE_INDEPENDENT 1

// Use fmt rather than tinyformat, even for printf-style formatting
#ifndef OIIO_USE_FMT_FOR_SPRINTF
#    define OIIO_USE_FMT_FOR_SPRINTF 0
#endif

#if !OIIO_USE_FMT_FOR_SPRINTF
#    ifndef TINYFORMAT_USE_VARIADIC_TEMPLATES
#        define TINYFORMAT_USE_VARIADIC_TEMPLATES
#    endif
#    include <OpenImageIO/tinyformat.h>
#endif

#ifndef OPENIMAGEIO_PRINTF_ARGS
#   ifndef __GNUC__
#       define __attribute__(x)
#   endif
    // Enable printf-like warnings with gcc by attaching
    // OPENIMAGEIO_PRINTF_ARGS to printf-like functions.  Eg:
    //
    // void foo (const char* fmt, ...) OPENIMAGEIO_PRINTF_ARGS(1,2);
    //
    // The arguments specify the positions of the format string and the the
    // beginning of the varargs parameter list respectively.
    //
    // For member functions with arguments like the example above, you need
    // OPENIMAGEIO_PRINTF_ARGS(2,3) instead.  (gcc includes the implicit this
    // pointer when it counts member function arguments.)
#   define OPENIMAGEIO_PRINTF_ARGS(fmtarg_pos, vararg_pos) \
        __attribute__ ((format (printf, fmtarg_pos, vararg_pos) ))
#endif


OIIO_NAMESPACE_BEGIN
/// @namespace Strutil
///
/// @brief     String-related utilities.
namespace Strutil {

/// Output the string to the file/stream in a synchronized fashion, so that
/// buffers are flushed and internal mutex is used to prevent threads from
/// clobbering each other -- output strings coming from concurrent threads
/// may be interleaved, but each string is "atomic" and will never splice
/// each other character-by-character.
void OIIO_API sync_output (FILE *file, string_view str);
void OIIO_API sync_output (std::ostream &file, string_view str);


/// Construct a std::string in a printf-like fashion.  For example:
///
///    std::string s = Strutil::sprintf ("blah %d %g", (int)foo, (float)bar);
///
/// Uses the tinyformat or fmt library underneath, so it's fully type-safe, and
/// works with any types that understand stream output via '<<'.
/// The formatting of the string will always use the classic "C" locale
/// conventions (in particular, '.' as decimal separator for float values).
template<typename... Args>
inline std::string sprintf (const char* fmt, const Args&... args)
{
#if OIIO_USE_FMT_FOR_SPRINTF
    return ::fmt::sprintf (fmt, args...);
#else
    return tinyformat::format (fmt, args...);
#endif
}



/// format() constructs formatted strings. Note that this is in transition!
///
/// Strutil::old::format() uses printf conventions and matches format() used
/// in OIIO 1.x. It is equivalent to Strutil::sprintf().
///
///    std::string s = Strutil::old::sprintf ("blah %d %g", (int)foo, (float)bar);
///
/// Strutil::fmt::format() uses "Python" conventions, in the style of string
/// formatting used by C++20 std::format and implemented today in the {fmt}
/// package (https://github.com/fmtlib/fmt). For example:
///
///    std::string s = Strutil::format ("blah {}  {}", (int)foo, (float)bar);
///
/// Straight-up Strutil::format is today aliased to old::format for the sake
/// of back-compatibility, but will someday be switched to fmt::format.
///
/// Recommended strategy for users:
/// * If you want printf conventions, switch to Strutil::sprintf().
/// * If you want to use the python conventions prior to the big switch,
///   use Strutil::fmt::format() explicitly (but see the caveat below).
/// * Use of unspecified Strutil::format() is, for back compatibility,
///   currently equivalent to sprintf, but beware that some point it will
///   switch to the future-standard formatting rules.
///

namespace fmt {
template<typename... Args>
inline std::string format (const char* fmt, const Args&... args)
{
#if OIIO_USE_FMT
    return ::fmt::format (fmt, args...);
#else
    // Disabled for some reason
    return std::string(fmt);
#endif
}
} // namespace fmt

namespace old {
template<typename... Args>
inline std::string format (const char* fmt, const Args&... args)
{
    return Strutil::sprintf (fmt, args...);
}

// DEPRECATED(2.0) string_view version. Phasing this out because
// std::string_view won't have a c_str() method.
template<typename... Args>
inline std::string format (string_view fmt, const Args&... args)
{
    return format (fmt.c_str(), args...);
}
} // namespace old



using old::format;



/// Strutil::print (fmt, ...)
/// Strutil::fprint (FILE*, fmt, ...)
/// Strutil::fprint (ostream& fmt, ...)
///
/// Output formatted strings to stdout, a FILE*, or a stream, respectively.
/// All use printf-like formatting rules, are type-safe, are thread-safe
/// (the outputs are "atomic", at least versus other calls to
/// Strutil::*printf), and automatically flush their outputs. They are all
/// locale-independent (forcing classic "C" locale).

template<typename... Args>
inline void printf (const char* fmt, const Args&... args)
{
    sync_output (stdout, Strutil::sprintf(fmt, args...));
}

template<typename... Args>
inline void fprintf (FILE *file, const char* fmt, const Args&... args)
{
    sync_output (file, Strutil::sprintf(fmt, args...));
}

template<typename... Args>
inline void fprintf (std::ostream &file, const char* fmt, const Args&... args)
{
    sync_output (file, Strutil::sprintf(fmt, args...));
}



/// Strutil::print (fmt, ...)
/// Strutil::print (FILE*, fmt, ...)
/// Strutil::print (ostream& fmt, ...)
///
/// Output formatted strings to stdout, a FILE*, or a stream, respectively.
/// All use "Python-like" formatting description (as {fmt} does, and some
/// day, std::format), are type-safe, are thread-safe (the outputs are
/// "atomic", at least versus other calls to Strutil::*printf), and
/// automatically flush their outputs. They are all locale-independent by
/// default (use {:n} for locale-aware formatting).

template<typename... Args>
inline void print (const char* fmt, const Args&... args)
{
    sync_output (stdout, Strutil::fmt::format(fmt, args...));
}

template<typename... Args>
inline void print (FILE *file, const char* fmt, const Args&... args)
{
    sync_output (file, Strutil::fmt::format(fmt, args...));
}

template<typename... Args>
inline void print (std::ostream &file, const char* fmt, const Args&... args)
{
    sync_output (file, Strutil::fmt::format(fmt, args...));
}




/// Return a std::string formatted from printf-like arguments -- passed
/// already as a va_list.  This is not guaranteed type-safe and is not
/// extensible like format(). Use with caution!
std::string OIIO_API vsprintf (const char *fmt, va_list ap)
                                         OPENIMAGEIO_PRINTF_ARGS(1,0);

/// Return a std::string formatted like Strutil::format, but passed
/// already as a va_list.  This is not guaranteed type-safe and is not
/// extensible like format(). Use with caution!
std::string OIIO_API vformat (const char *fmt, va_list ap)
                                         OPENIMAGEIO_PRINTF_ARGS(1,0);

/// Return a string expressing a number of bytes, in human readable form.
///  - memformat(153)           -> "153 B"
///  - memformat(15300)         -> "14.9 KB"
///  - memformat(15300000)      -> "14.6 MB"
///  - memformat(15300000000LL) -> "14.2 GB"
std::string OIIO_API memformat (long long bytes, int digits=1);

/// Return a string expressing an elapsed time, in human readable form.
/// e.g. "0:35.2"
std::string OIIO_API timeintervalformat (double secs, int digits=1);


/// Get a map with RESTful arguments extracted from the given string 'str'.
/// Add it into the 'result' argument (Warning: the 'result' argument may
/// be changed even if 'get_rest_arguments ()' return an error!).
/// Return true on success, false on error.
/// Acceptable forms:
///  - text?arg1=val1&arg2=val2...
///  - ?arg1=val1&arg2=val2...
/// Everything before question mark will be saved into the 'base' argument.
bool OIIO_API get_rest_arguments (const std::string &str, std::string &base,
                                   std::map<std::string, std::string> &result);

/// Take a string that may have embedded newlines, tabs, etc., and turn
/// those characters into escape sequences like `\n`, `\t`, `\v`, `\b`,
/// `\r`, `\f`, `\a`, `\\`, `\"`.
std::string OIIO_API escape_chars (string_view unescaped);

/// Take a string that has embedded escape sequences (`\\`, `\"`, `\n`,
/// etc.) and collapse them into the 'real' characters.
std::string OIIO_API unescape_chars (string_view escaped);

/// Word-wrap string `src` to no more than `columns` width, starting with an
/// assumed position of `prefix` on the first line and intending by `prefix`
/// blanks before all lines other than the first.
///
/// Words may be split AT any characters in `sep` or immediately AFTER any
/// characters in `presep`. After the break, any extra `sep` characters will
/// be deleted.
///
/// By illustration,
///     wordwrap("0 1 2 3 4 5 6 7 8", 10, 4)
/// should return:
///     "0 1 2\n    3 4 5\n    6 7 8"
std::string OIIO_API wordwrap (string_view src, int columns = 80,
                               int prefix = 0, string_view sep = " ",
                               string_view presep = "");

/// Hash a string_view.
inline size_t
strhash (string_view s)
{
    return s.length() ? farmhash::Hash (s) : 0;
}



/// Case-insensitive comparison of strings.  For speed, this always uses
/// a static locale that doesn't require a mutex.
bool OIIO_API iequals (string_view a, string_view b);

/// Case-insensitive ordered comparison of strings.  For speed, this always
/// uses a static locale that doesn't require a mutex.
bool OIIO_API iless (string_view a, string_view b);

/// Does 'a' start with the string 'b', with a case-sensitive comparison?
bool OIIO_API starts_with (string_view a, string_view b);

/// Does 'a' start with the string 'b', with a case-insensitive comparison?
/// For speed, this always uses a static locale that doesn't require a mutex.
bool OIIO_API istarts_with (string_view a, string_view b);

/// Does 'a' end with the string 'b', with a case-sensitive comparison?
bool OIIO_API ends_with (string_view a, string_view b);

/// Does 'a' end with the string 'b', with a case-insensitive comparison?
/// For speed, this always uses a static locale that doesn't require a mutex.
bool OIIO_API iends_with (string_view a, string_view b);

/// Does 'a' contain the string 'b' within it?
bool OIIO_API contains (string_view a, string_view b);

/// Does 'a' contain the string 'b' within it, using a case-insensitive
/// comparison?
bool OIIO_API icontains (string_view a, string_view b);

/// Convert to upper case in place, faster than std::toupper because we use
/// a static locale that doesn't require a mutex lock.
void OIIO_API to_lower (std::string &a);

/// Convert to upper case in place, faster than std::toupper because we use
/// a static locale that doesn't require a mutex lock.
void OIIO_API to_upper (std::string &a);

/// Return an all-upper case version of `a` (locale-independent).
inline std::string OIIO_API lower (string_view a) {
    std::string result(a);
    to_lower(result);
    return result;
}

/// Return an all-upper case version of `a` (locale-independent).
inline std::string OIIO_API upper (string_view a) {
    std::string result(a);
    to_upper(result);
    return result;
}



/// Return a reference to the section of str that has all consecutive
/// characters in chars removed from the beginning and ending.  If chars is
/// empty, it will be interpreted as " \t\n\r\f\v" (whitespace).
string_view OIIO_API strip (string_view str, string_view chars=string_view());

/// Return a reference to the section of str that has all consecutive
/// characters in chars removed from the beginning (left side).  If chars is
/// empty, it will be interpreted as " \t\n\r\f\v" (whitespace).
string_view OIIO_API lstrip (string_view str, string_view chars=string_view());

/// Return a reference to the section of str that has all consecutive
/// characters in chars removed from the ending (right side).  If chars is
/// empty, it will be interpreted as " \t\n\r\f\v" (whitespace).
string_view OIIO_API rstrip (string_view str, string_view chars=string_view());


/// Fills the "result" list with the words in the string, using sep as
/// the delimiter string.  If maxsplit is > -1, at most maxsplit splits
/// are done. If sep is "", any whitespace string is a separator.
void OIIO_API split (string_view str, std::vector<string_view> &result,
                     string_view sep = string_view(), int maxsplit = -1);
void OIIO_API split (string_view str, std::vector<std::string> &result,
                     string_view sep = string_view(), int maxsplit = -1);

/// Split the contents of `str` using `sep` as the delimiter string. If
/// `sep` is "", any whitespace string is a separator. If `maxsplit > -1`,
/// at most `maxsplit` split fragments will be produced (for example,
/// maxsplit=2 will split at only the first separator, yielding at most two
/// fragments). The result is returned as a vector of std::string (for
/// `splits()`) or a vector of string_view (for `splitsv()`).
OIIO_API std::vector<std::string>
splits (string_view str, string_view sep = "", int maxsplit = -1);
OIIO_API std::vector<string_view>
splitsv (string_view str, string_view sep = "", int maxsplit = -1);

/// Join all the strings in 'seq' into one big string, separated by the
/// 'sep' string. The Sequence can be any iterable collection of items that
/// are able to convert to string via stream output. Examples include:
/// std::vector<string_view>, std::vector<std::string>, std::set<ustring>,
/// std::vector<int>, etc.
template<class Sequence>
std::string join (const Sequence& seq, string_view sep="")
{
    std::ostringstream out;
    out.imbue(std::locale::classic());  // Force "C" locale
    bool first = true;
    for (auto&& s : seq) {
        if (! first && sep.size())
            out << sep;
        out << s;
        first = false;
    }
    return out.str();
}

/// Join all the strings in 'seq' into one big string, separated by the
/// 'sep' string. The Sequence can be any iterable collection of items that
/// are able to convert to string via stream output. Examples include:
/// std::vector<string_view>, std::vector<std::string>, std::set<ustring>,
/// std::vector<int>, etc. Values will be rendered into the string in a
/// locale-independent manner (i.e., '.' for decimal in floats). If the
/// optional `len` is nonzero, exactly that number of elements will be
/// output (truncating or default-value-padding the sequence).
template<class Sequence>
std::string join (const Sequence& seq, string_view sep /*= ""*/, size_t len)
{
    using E = typename std::remove_reference<decltype(*std::begin(seq))>::type;
    std::ostringstream out;
    out.imbue(std::locale::classic());  // Force "C" locale
    bool first = true;
    for (auto&& s : seq) {
        if (! first)
            out << sep;
        out << s;
        first = false;
        if (len && (--len == 0))
            break;
    }
    while (len--) {
        if (! first)
            out << sep;
        out << E();
        first = false;
    }
    return out.str();
}

/// Concatenate two strings, returning a std::string, implemented carefully
/// to not perform any redundant copies or allocations. This is
/// semantically equivalent to `Strutil::sprintf("%s%s", s, t)`, but is
/// more efficient.
std::string OIIO_API concat(string_view s, string_view t);

/// Repeat a string formed by concatenating str n times.
std::string OIIO_API repeat (string_view str, int n);

/// Replace a pattern inside a string and return the result. If global is
/// true, replace all instances of the pattern, otherwise just the first.
std::string OIIO_API replace (string_view str, string_view pattern,
                              string_view replacement, bool global=false);


/// strtod/strtof equivalents that are "locale-independent", always using
/// '.' as the decimal separator. This should be preferred for I/O and other
/// situations where you want the same standard formatting regardless of
/// locale.
float OIIO_API strtof (const char *nptr, char **endptr = nullptr) noexcept;
double OIIO_API strtod (const char *nptr, char **endptr = nullptr) noexcept;


// stoi() returns the int conversion of text from a string.
// No exceptions or errors -- parsing errors just return 0, over/underflow
// gets clamped to int range. No locale consideration.
OIIO_API int stoi (string_view s, size_t* pos=0, int base=10);

// stoui() returns the unsigned int conversion of text from a string.
// No exceptions or errors -- parsing errors just return 0. Negative
// values are cast, overflow is clamped. No locale considerations.
inline unsigned int stoui (string_view s, size_t* pos=0, int base=10) {
    return static_cast<unsigned int>(stoi (s, pos, base));
}

/// stof() returns the float conversion of text from several string types.
/// No exceptions or errors -- parsing errors just return 0.0. These always
/// use '.' for the decimal mark (versus atof and std::strtof, which are
/// locale-dependent).
OIIO_API float stof (string_view s, size_t* pos=0);
#define OIIO_STRUTIL_HAS_STOF 1  /* be able to test this */

// Temporary fix: allow separate std::string and char* versions, to avoid
// string_view allocation on some platforms. This will be deprecated once
// we can count on all supported compilers using short string optimization.
OIIO_API float stof (const std::string& s, size_t* pos=0);
OIIO_API float stof (const char* s, size_t* pos=0);
// N.B. For users of ustring, there's a stof(ustring) defined in ustring.h.

OIIO_API double stod (string_view s, size_t* pos=0);
OIIO_API double stod (const std::string& s, size_t* pos=0);
OIIO_API double stod (const char* s, size_t* pos=0);



/// Return true if the string is exactly (other than leading and trailing
/// whitespace) a valid int.
OIIO_API bool string_is_int (string_view s);

/// Return true if the string is exactly (other than leading or trailing
/// whitespace) a valid float. This operations in a locale-independent
/// manner, i.e., it assumes '.' as the decimal mark.
OIIO_API bool string_is_float (string_view s);



// Helper template to convert from generic type to string. Used when you
// want stoX but you're in a template. Rigged to use "C" locale.
template<typename T>
inline T from_string (string_view s) {
    return T(s); // Generic: assume there is an explicit converter
}
// Special case for int
template<> inline int from_string<int> (string_view s) {
    return Strutil::stoi(s);
}
// Special case for uint
template<> inline unsigned int from_string<unsigned int> (string_view s) {
    return Strutil::stoui(s);
}
// Special case for float -- note that by using Strutil::strtof, this
// always treats '.' as the decimal mark.
template<> inline float from_string<float> (string_view s) {
    return Strutil::stof(s);
}



// Template function to convert any type to a string. The default
// implementation is just to use sprintf. The template can be
// overloaded if there is a better method for particular types.
// Eventually, we want this to use fmt::to_string, but for now that doesn't
// work because {fmt} doesn't correctly support locale-independent
// formatting of floating-point types.
template<typename T>
inline std::string to_string (const T& value) {
    return Strutil::sprintf("%s",value);
}

template<> inline std::string to_string (const std::string& value) { return value; }
template<> inline std::string to_string (const string_view& value) { return value; }
inline std::string to_string (const char* value) { return value; }


#if !OIIO_USE_FMT_FOR_SPRINTF && OIIO_USE_FMT
// When not using fmt, nonetheless fmt::to_string is incredibly faster than
// tinyformat for ints, so speciaize to use the fast one.
inline std::string to_string (int value) { return ::fmt::to_string(value); }
inline std::string to_string (size_t value) { return ::fmt::to_string(value); }
#endif



// Helper template to test if a string is a generic type. Used instead of
// string_is_X, but when you're inside templated code.
template<typename T>
inline bool string_is (string_view /*s*/) {
    return false; // Generic: assume there is an explicit specialization
}
// Special case for int
template <> inline bool string_is<int> (string_view s) {
    return string_is_int (s);
}
// Special case for float. Note that by using Strutil::stof, this always
// treats '.' as the decimal character.
template <> inline bool string_is<float> (string_view s) {
    return string_is_float (s);
}




/// Given a string containing values separated by a comma (or optionally
/// another separator), extract the individual values, placing them into
/// vals[] which is presumed to already contain defaults.  If only a single
/// value was in the list, replace all elements of vals[] with the value.
/// Otherwise, replace them in the same order.  A missing value will simply
/// not be replaced. Return the number of values found in the list
/// (including blank or malformed ones). If the vals vector was empty
/// initially, grow it as necessary.
///
/// For example, if T=float, suppose initially, vals[] = {0, 1, 2}, then
///   "3.14"       results in vals[] = {3.14, 3.14, 3.14}
///   "3.14,,-2.0" results in vals[] = {3.14, 1, -2.0}
///
/// This can work for type T = int, float, or any type for that has
/// an explicit constructor from a std::string.
template<class T, class Allocator>
int extract_from_list_string (std::vector<T, Allocator> &vals,
                              string_view list,
                              string_view sep = ",")
{
    size_t nvals = vals.size();
    std::vector<string_view> valuestrings;
    Strutil::split (list, valuestrings, sep);
    for (size_t i = 0, e = valuestrings.size(); i < e; ++i) {
        T v = from_string<T> (valuestrings[i]);
        if (nvals == 0)
            vals.push_back (v);
        else if (valuestrings[i].size()) {
            if (vals.size() > i)  // don't replace non-existnt entries
                vals[i] = from_string<T> (valuestrings[i]);
        }
        /* Otherwise, empty space between commas, so leave default alone */
    }
    if (valuestrings.size() == 1 && nvals > 0) {
        vals.resize (1);
        vals.resize (nvals, vals[0]);
    }
    return list.size() ? (int) valuestrings.size() : 0;
}


/// Given a string containing values separated by a comma (or optionally
/// another separator), extract the individual values, returning them as a
/// std::vector<T>. The vector will be initialized with `nvals` elements
/// with default value `val`. If only a single value was in the list,
/// replace all elements of vals[] with the value. Otherwise, replace them
/// in the same order.  A missing value will simply not be replaced and
/// will retain the initialized default value. If the string contains more
/// then `nvals` values, they will append to grow the vector.
///
/// For example, if T=float,
///   extract_from_list_string ("", 3, 42.0f)
///       --> {42.0, 42.0, 42.0}
///   extract_from_list_string ("3.14", 3, 42.0f)
///       --> {3.14, 3.14, 3.14}
///   extract_from_list_string ("3.14,,-2.0", 3, 42.0f)
///       --> {3.14, 42.0, -2.0}
///   extract_from_list_string ("1,2,3,4", 3, 42.0f)
///       --> {1.0, 2.0, 3.0, 4.0}
///
/// This can work for type T = int, float, or any type for that has
/// an explicit constructor from a std::string.
template<class T>
std::vector<T>
extract_from_list_string (string_view list, size_t nvals=0, T val=T(),
                          string_view sep = ",")
{
    std::vector<T> vals (nvals, val);
    extract_from_list_string (vals, list, sep);
    return vals;
}




/// C++ functor wrapper class for using strhash for unordered_map or
/// unordered_set.  The way this is used, in conjunction with
/// StringEqual, to build an efficient hash map for char*'s or
/// std::string's is as follows:
/// \code
///    unordered_map <const char *, Key, Strutil::StringHash, Strutil::StringEqual>
/// \endcode
class StringHash {
public:
    size_t operator() (string_view s) const {
        return (size_t)Strutil::strhash(s);
    }
};



/// C++ functor for comparing two strings for equality of their characters.
struct OIIO_API StringEqual {
    bool operator() (const char *a, const char *b) const noexcept { return strcmp (a, b) == 0; }
    bool operator() (string_view a, string_view b) const noexcept { return a == b; }
};


/// C++ functor for comparing two strings for equality of their characters
/// in a case-insensitive and locale-insensitive way.
struct OIIO_API StringIEqual {
    bool operator() (const char *a, const char *b) const noexcept;
    bool operator() (string_view a, string_view b) const noexcept { return iequals (a, b); }
};


/// C++ functor for comparing the ordering of two strings.
struct OIIO_API StringLess {
    bool operator() (const char *a, const char *b) const noexcept { return strcmp (a, b) < 0; }
    bool operator() (string_view a, string_view b) const noexcept { return a < b; }
};


/// C++ functor for comparing the ordering of two strings in a
/// case-insensitive and locale-insensitive way.
struct OIIO_API StringILess {
    bool operator() (const char *a, const char *b) const noexcept;
    bool operator() (string_view a, string_view b) const noexcept { return a < b; }
};



#ifdef _WIN32
/// Conversion functions between UTF-8 and UTF-16 for windows.
///
/// For historical reasons, the standard encoding for strings on windows is
/// UTF-16, whereas the unix world seems to have settled on UTF-8.  These two
/// encodings can be stored in std::string and std::wstring respectively, with
/// the caveat that they're both variable-width encodings, so not all the
/// standard string methods will make sense (for example std::string::size()
/// won't return the number of glyphs in a UTF-8 string, unless it happens to
/// be made up of only the 7-bit ASCII subset).
///
/// The standard windows API functions usually have two versions, a UTF-16
/// version with a 'W' suffix (using wchar_t* strings), and an ANSI version
/// with a 'A' suffix (using char* strings) which uses the current windows
/// code page to define the encoding.  (To make matters more confusing there is
/// also a further "TCHAR" version which is #defined to the UTF-16 or ANSI
/// version, depending on whether UNICODE is defined during compilation.
/// This is meant to make it possible to support compiling libraries in
/// either unicode or ansi mode from the same codebase.)
///
/// Using std::string as the string container (as in OIIO) implies that we
/// can't use UTF-16.  It also means we need a variable-width encoding to
/// represent characters in non-Latin alphabets in an unambiguous way; the
/// obvious candidate is UTF-8.  File paths in OIIO are considered to be
/// represented in UTF-8, and must be converted to UTF-16 before passing to
/// windows API file opening functions.

/// On the other hand, the encoding used for the ANSI versions of the windows
/// API is the current windows code page.  This is more compatible with the
/// default setup of the standard windows command prompt, and may be more
/// appropriate for error messages.

// Conversion to wide char
//
std::wstring OIIO_API utf8_to_utf16 (string_view utf8str) noexcept;

// Conversion from wide char
//
std::string OIIO_API utf16_to_utf8(const std::wstring& utf16str) noexcept;
#endif


/// Copy at most size characters (including terminating 0 character) from
/// src into dst[], filling any remaining characters with 0 values. Returns
/// dst. Note that this behavior is identical to strncpy, except that it
/// guarantees that there will be a termining 0 character.
OIIO_API char * safe_strcpy (char *dst, string_view src, size_t size) noexcept;


/// Modify str to trim any leading whitespace (space, tab, linefeed, cr)
/// from the front.
void OIIO_API skip_whitespace (string_view &str) noexcept;

/// Modify str to trim any trailing whitespace (space, tab, linefeed, cr)
/// from the back.
void OIIO_API remove_trailing_whitespace (string_view &str) noexcept;

/// Modify str to trim any whitespace (space, tab, linefeed, cr) from both
/// the front and back.
inline void trim_whitespace (string_view &str) noexcept {
    skip_whitespace(str);
    remove_trailing_whitespace(str);
}

/// If str's first character is c (or first non-whitespace char is c, if
/// skip_whitespace is true), return true and additionally modify str to
/// skip over that first character if eat is also true. Otherwise, if str
/// does not begin with character c, return false and don't modify str.
bool OIIO_API parse_char (string_view &str, char c,
                          bool skip_whitespace = true, bool eat=true) noexcept;

/// Modify str to trim all characters up to (but not including) the first
/// occurrence of c, and return true if c was found or false if the whole
/// string was trimmed without ever finding c. But if eat is false, then
/// don't modify str, just return true if any c is found, false if no c
/// is found.
bool OIIO_API parse_until_char (string_view &str, char c, bool eat=true) noexcept;

/// If str's first non-whitespace characters are the prefix, return true and
/// additionally modify str to skip over that prefix if eat is also true.
/// Otherwise, if str doesn't start with optional whitespace and the prefix,
/// return false and don't modify str.
bool OIIO_API parse_prefix (string_view &str, string_view prefix, bool eat=true) noexcept;

/// If str's first non-whitespace characters form a valid integer, return
/// true, place the integer's value in val, and additionally modify str to
/// skip over the parsed integer if eat is also true. Otherwise, if no
/// integer is found at the beginning of str, return false and don't modify
/// val or str.
bool OIIO_API parse_int (string_view &str, int &val, bool eat=true) noexcept;

/// If str's first non-whitespace characters form a valid float, return
/// true, place the float's value in val, and additionally modify str to
/// skip over the parsed float if eat is also true. Otherwise, if no float
/// is found at the beginning of str, return false and don't modify val or
/// str.
bool OIIO_API parse_float (string_view &str, float &val, bool eat=true) noexcept;

enum QuoteBehavior { DeleteQuotes, KeepQuotes };
/// If str's first non-whitespace characters form a valid string (either a
/// single word separated by whitespace or anything inside a double-quoted
/// ("") or single-quoted ('') string, return true, place the string's value
/// (not including surrounding double quotes) in val, and additionally
/// modify str to skip over the parsed string if eat is also true.
/// Otherwise, if no string is found at the beginning of str, return false
/// and don't modify val or str. If keep_quotes is true, the surrounding
/// double quotes (if present) will be kept in val.
bool OIIO_API parse_string (string_view &str, string_view &val, bool eat=true,
                            QuoteBehavior keep_quotes=DeleteQuotes) noexcept;

/// Return the first "word" (set of contiguous alphabetical characters) in
/// str, and additionally modify str to skip over the parsed word if eat is
/// also true. Otherwise, if no word is found at the beginning of str,
/// return an empty string_view and don't modify str.
string_view OIIO_API parse_word (string_view &str, bool eat=true) noexcept;

/// If str's first non-whitespace characters form a valid C-like identifier,
/// return the identifier, and additionally modify str to skip over the
/// parsed identifier if eat is also true. Otherwise, if no identifier is
/// found at the beginning of str, return an empty string_view and don't
/// modify str.
string_view OIIO_API parse_identifier (string_view &str, bool eat=true) noexcept;

/// If str's first non-whitespace characters form a valid C-like identifier,
/// return the identifier, and additionally modify str to skip over the
/// parsed identifier if eat is also true. Otherwise, if no identifier is
/// found at the beginning of str, return an empty string_view and don't
/// modify str. The 'allowed' parameter may specify a additional characters
/// accepted that would not ordinarily be allowed in C identifiers, for
/// example, parse_identifier (blah, "$:") would allow "identifiers"
/// containing dollar signs and colons as well as the usual alphanumeric and
/// underscore characters.
string_view OIIO_API parse_identifier (string_view &str,
                                       string_view allowed, bool eat = true) noexcept;

/// If the C-like identifier at the head of str exactly matches id,
/// return true, and also advance str if eat is true. If it is not a match
/// for id, return false and do not alter str.
bool OIIO_API parse_identifier_if (string_view &str, string_view id,
                                   bool eat=true) noexcept;

/// Return the characters until any character in sep is found, storing it in
/// str, and additionally modify str to skip over the parsed section if eat
/// is also true. Otherwise, if no word is found at the beginning of str,
/// return an empty string_view and don't modify str.
string_view OIIO_API parse_until (string_view &str,
                                  string_view sep=" \t\r\n", bool eat=true) noexcept;

/// Return the characters at the head of the string that match any in set,
/// and additionally modify str to skip over the parsed section if eat is
/// also true. Otherwise, if no `set` characters are found at the beginning
/// of str, return an empty string_view and don't modify str.
string_view OIIO_API parse_while (string_view &str,
                                  string_view set, bool eat=true) noexcept;

/// Assuming the string str starts with either '(', '[', or '{', return the
/// head, up to and including the corresponding closing character (')', ']',
/// or '}', respectively), recognizing nesting structures. For example,
/// parse_nested("(a(b)c)d") should return "(a(b)c)", NOT "(a(b)". Return an
/// empty string if str doesn't start with one of those characters, or
/// doesn't contain a correctly matching nested pair. If eat==true, str will
/// be modified to trim off the part of the string that is returned as the
/// match.
string_view OIIO_API parse_nested (string_view &str, bool eat=true) noexcept;


/// Look within `str` for the pattern:
///     head nonwhitespace_chars whitespace
/// Remove that full pattern from `str` and return the nonwhitespace
/// part that followed the head (or return the empty string and leave `str`
/// unmodified, if the head was never found).
OIIO_API std::string
excise_string_after_head (std::string& str, string_view head);


/// Converts utf-8 string to vector of unicode codepoints. This function
/// will not stop on invalid sequences. It will let through some invalid
/// utf-8 sequences like: 0xfdd0-0xfdef, 0x??fffe/0x??ffff. It does not
/// support 5-6 bytes long utf-8 sequences. Will skip trailing character if
/// there are not enough bytes for decoding a codepoint.
///
/// N.B. Following should probably return u32string instead of taking
/// vector, but C++11 support is not yet stabilized across compilers.
/// We will eventually add that and deprecate this one, after everybody
/// is caught up to C++11.
void OIIO_API utf8_to_unicode (string_view str, std::vector<uint32_t> &uvec);

/// Encode the string in base64.
/// https://en.wikipedia.org/wiki/Base64
std::string OIIO_API base64_encode (string_view str);

}  // namespace Strutil

OIIO_NAMESPACE_END

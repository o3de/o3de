// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <OpenImageIO/strutil.h>
#include <OpenImageIO/sysutil.h>


OIIO_NAMESPACE_BEGIN

namespace simd {
// Force a float-based abs and max to appear in namespace simd
inline float
abs(float x)
{
    return std::abs(x);
}
inline float
max(float x, float y)
{
    return std::max(x, y);
}
}  // namespace simd

namespace pvt {

class UnitTestFailureCounter {
public:
    UnitTestFailureCounter() noexcept
        : m_failures(0)
    {
    }
    ~UnitTestFailureCounter()
    {
        if (m_failures) {
            std::cout << Sysutil::Term(std::cout).ansi("red", "ERRORS!\n");
            std::exit(m_failures != 0);
        } else {
            std::cout << Sysutil::Term(std::cout).ansi("green", "OK\n");
        }
    }
    const UnitTestFailureCounter& operator++() noexcept  // prefix
    {
        ++m_failures;
        return *this;
    }
    int operator++(int) noexcept { return m_failures++; }  // postfix
    UnitTestFailureCounter operator+=(int i) noexcept
    {
        m_failures += i;
        return *this;
    }
    operator int() const noexcept { return m_failures; }

private:
    int m_failures = 0;
};


template<typename X, typename Y>
inline bool
equal_approx(const X& x, const Y& y)
{
    using namespace simd;
    return all(abs((x) - (y)) <= 0.001f * max(abs(x), abs(y)));
}


}  // end namespace pvt

OIIO_NAMESPACE_END

// Helper: print entire vectors. This makes the OIIO_CHECK_EQUAL macro
// work for std::vector!
template<typename T>
inline std::ostream&
operator<<(std::ostream& out, const std::vector<T>& v)
{
    out << '{' << OIIO::Strutil::join(v, ",") << '}';
    return out;
}

static OIIO::pvt::UnitTestFailureCounter unit_test_failures;



/// OIIO_CHECK_* macros checks if the conditions is met, and if not,
/// prints an error message indicating the module and line where the
/// error occurred, but does NOT abort.  This is helpful for unit tests
/// where we do not want one failure.
#define OIIO_CHECK_ASSERT(x)                                                   \
    ((x) ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << "\n"),                                               \
            (void)++unit_test_failures))

#define OIIO_CHECK_EQUAL(x, y)                                                 \
    (((x) == (y))                                                              \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " == " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))

#define OIIO_CHECK_EQUAL_THRESH(x, y, eps)                                     \
    ((std::abs((x) - (y)) <= eps)                                              \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " == " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y) << "'"  \
                       << ", diff was " << std::abs((x) - (y)) << "\n"),       \
            (void)++unit_test_failures))

#define OIIO_CHECK_EQUAL_APPROX(x, y)                                          \
    (OIIO::pvt::equal_approx(x, y)                                             \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " == " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y) << "'"  \
                       << ", diff was " << ((x) - (y)) << "\n"),               \
            (void)++unit_test_failures))

#define OIIO_CHECK_NE(x, y)                                                    \
    (((x) != (y))                                                              \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " != " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))

#define OIIO_CHECK_LT(x, y)                                                    \
    (((x) < (y))                                                               \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " < " << #y << "\n"                                  \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))

#define OIIO_CHECK_GT(x, y)                                                    \
    (((x) > (y))                                                               \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " > " << #y << "\n"                                  \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))

#define OIIO_CHECK_LE(x, y)                                                    \
    (((x) <= (y))                                                              \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " <= " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))

#define OIIO_CHECK_GE(x, y)                                                    \
    (((x) >= (y))                                                              \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " >= " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))


// Special SIMD related equality checks that use all()
#define OIIO_CHECK_SIMD_EQUAL(x, y)                                            \
    (all((x) == (y))                                                           \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " == " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))

#define OIIO_CHECK_SIMD_EQUAL_THRESH(x, y, eps)                                \
    (all(abs((x) - (y)) < (eps))                                               \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << " == " << #y << "\n"                                 \
                       << "\tvalues were '" << (x) << "' and '" << (y)         \
                       << "'\n"),                                              \
            (void)++unit_test_failures))


// Test if ImageBuf operation got an error. It's a lot like simply testing
// OIIO_CHECK_ASSERT(x), but if x is false, it will get an error message
// from the buffer and incorporate it into the failure message.
// Call like this, for example:
//    ImageBuf buf;
//    OIIO_CHECK_IMAGEBUF_STATUS(buf,
//        ImageBufAlgo::Func (buf, ...)
//    );
#define OIIO_CHECK_IMAGEBUF_STATUS(buf, x)                                     \
    ((x && !buf.has_error())                                                   \
         ? ((void)0)                                                           \
         : ((std::cout << OIIO::Sysutil::Term(std::cout).ansi("red,bold")      \
                       << __FILE__ << ":" << __LINE__ << ":\n"                 \
                       << "FAILED: "                                           \
                       << OIIO::Sysutil::Term(std::cout).ansi("normal") << #x  \
                       << ": " << buf.geterror() << "\n"),                     \
            (void)++unit_test_failures))

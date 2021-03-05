/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
// Inspired by the Boost library's BOOST_STATIC_ASSERT(),
// see http://www.boost.org/doc/libs/1_49_0/doc/html/boost_staticassert/how.html
// or http://www.boost.org/libs/static_assert

#ifndef CRYINCLUDE_CRYCOMMON_COMPILETIMEASSERT_H
#define CRYINCLUDE_CRYCOMMON_COMPILETIMEASSERT_H
#pragma once

#if defined(__cplusplus)
/*
template <bool b>
struct COMPILE_TIME_ASSERT_FAIL;

template <>
struct COMPILE_TIME_ASSERT_FAIL<true>
{
};

template <int i>
struct COMPILE_TIME_ASSERT_TEST
{
    enum { dummy = i };
};

#define COMPILE_TIME_ASSERT_BUILD_NAME2(x, y) x##y
#define COMPILE_TIME_ASSERT_BUILD_NAME1(x, y) COMPILE_TIME_ASSERT_BUILD_NAME2(x, y)
#define COMPILE_TIME_ASSERT_BUILD_NAME(x, y) COMPILE_TIME_ASSERT_BUILD_NAME1(x, y)

#ifndef __RECODE__
    #define COMPILE_TIME_ASSERT(expr) \
        typedef COMPILE_TIME_ASSERT_TEST<sizeof(COMPILE_TIME_ASSERT_FAIL<(bool)(expr)>)> \
        COMPILE_TIME_ASSERT_BUILD_NAME(compile_time_assert_test_, __LINE__)
    // note: for MS Visual Studio we could use __COUNTER__ instead of __LINE__
#else
    #define COMPILE_TIME_ASSERT(expr)
#endif  // __RECODE__

#else

#define COMPILE_TIME_ASSERT(expr)
*/
#endif

#define COMPILE_TIME_ASSERT_MSG(expr, msg) static_assert(expr, msg)
#define COMPILE_TIME_ASSERT(expr) COMPILE_TIME_ASSERT_MSG(expr, "Compile Time Assert")


#endif // CRYINCLUDE_CRYCOMMON_COMPILETIMEASSERT_H

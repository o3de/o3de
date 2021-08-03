/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Assert dialog box

#pragma once

#include <AzCore/base.h>

//-----------------------------------------------------------------------------------------------------
// Just undef this if you want to use the standard assert function
//-----------------------------------------------------------------------------------------------------

// if AZ_ENABLE_TRACING is enabled, then calls to AZ_Assert(...) will flow in.  This is the case
// even in Profile mode - thus if you want to manage what happens, USE_CRY_ASSERT also needs to be enabled in those cases.
// if USE_CRY_ASSERT is not enabled, but AZ_ENABLE_TRACING is enabled, then the default behavior for assets will occur instead
// which is to throw the DEBUG BREAK exception / signal, which tends to end with application shutdown.
#if defined(AZ_ENABLE_TRACE_ASSERTS)
#define USE_AZ_ASSERT
#endif

#if !defined (USE_AZ_ASSERT) && defined(AZ_ENABLE_TRACING)
#undef USE_CRY_ASSERT
#define USE_CRY_ASSERT
#endif

// you can undefine this.  It will cause the assert message box to appear anywhere that USE_CRY_ASSERT is enabled
// instead of it only appearing in debug. 
// if this is DEFINED then only in debug builds will you see the message box.  In other builds, CRY_ASSERTS become CryWarning instead of
// instead (showing no message box, only a warning).
#define CRY_ASSERT_DIALOG_ONLY_IN_DEBUG

#if defined(FORCE_STANDARD_ASSERT) || defined(USE_AZ_ASSERT)
#undef USE_CRY_ASSERT
#undef CRY_ASSERT_DIALOG_ONLY_IN_DEBUG
#endif

// Using AZ_Assert for all assert kinds (assert =, CRY_ASSERT, AZ_Assert).
// see Trace::Assert for implementation
#if defined(USE_AZ_ASSERT)
    #undef assert
    #if !defined(NDEBUG)
        #define assert(condition) AZ_Assert(condition, "%s", #condition)
    #else
        #define assert(condition)
    #endif
#endif //defined(USE_AZ_ASSERT)

//-----------------------------------------------------------------------------------------------------
// Use like this:
// CRY_ASSERT(expression);
// CRY_ASSERT_MESSAGE(expression,"Useful message");
// CRY_ASSERT_TRACE(expression,("This should never happen because parameter n%d named %s is %f",iParameter,szParam,fValue));
//-----------------------------------------------------------------------------------------------------

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryAssert_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32) || defined(APPLE) || defined(LINUX)
    #define CRYASSERT_H_TRAIT_USE_CRY_ASSERT_MESSAGE 1
#endif

#if defined(USE_CRY_ASSERT) && CRYASSERT_H_TRAIT_USE_CRY_ASSERT_MESSAGE
void CryAssertTrace(const char*, ...);
bool CryAssert(const char*, const char*, unsigned int, bool*);
void CryDebugBreak();

    #define CRY_ASSERT(condition) CRY_ASSERT_MESSAGE(condition, NULL)

    #define CRY_ASSERT_MESSAGE(condition, message) CRY_ASSERT_TRACE(condition, (message))

    #define CRY_ASSERT_TRACE(condition, parenthese_message)                  \
    do                                                                       \
    {                                                                        \
        static bool s_bIgnoreAssert = false;                                 \
        if (!s_bIgnoreAssert && !(condition))                                \
        {                                                                    \
            CryAssertTrace parenthese_message;                               \
            if (CryAssert(#condition, __FILE__, __LINE__, &s_bIgnoreAssert)) \
            {                                                                \
                DEBUG_BREAK;                                                 \
            }                                                                \
        }                                                                    \
    } while (0)

    #undef assert
    #define assert CRY_ASSERT
#elif !defined(CRY_ASSERT)
#ifndef USE_AZ_ASSERT
    #include <assert.h>
#endif //USE_AZ_ASSERT
    #define CRY_ASSERT(condition) assert(condition)
    #define CRY_ASSERT_MESSAGE(condition, message) assert(condition)
    #define CRY_ASSERT_TRACE(condition, parenthese_message) assert(condition)
#endif

//-----------------------------------------------------------------------------------------------------

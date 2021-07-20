/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_CONFIG_H
#define AZSTD_CONFIG_H 1

//////////////////////////////////////////////////////////////////////////
//
// Control defines user can control most of them if needed.

/**
 * \defgroup ConfigDefines Configuration Defines
 * Here is the list of defines you can used to control AZStd behavior.
 * \n\b IMPORTANT: all of these should be defined before the first include of \e AZStd/base.h
 * @{
 */
#ifndef AZSTD_CONTAINER_ASSERT
    #define AZSTD_CONTAINER_ASSERT  AZ_Assert
#endif

/**
 * \def AZSTD_CHECKED_ITERATORS
 * States: always disabled = 0 (default), enabled in debug = 1, = 2 always enabled
 * \b IMPORTANT: if you change this flag you need to make sure you re-compile of your dependent libs. They must have the the same AZSTD_CHECKED_ITERATORS
 * value. This flags changes the size of the containers and iterators, so changing it just for the headers will cause bad crashes with libs using different state.
 * Normally you don't using this define often, it this is the case. It is considered that in near future we will add configuration policy to all containers, so
 * this define will become obsolete.
 */
#ifndef AZSTD_CHECKED_ITERATORS
    #define AZSTD_CHECKED_ITERATORS 0
#endif

#if ((defined(AZ_DEBUG_BUILD) && (AZSTD_CHECKED_ITERATORS == 1)) || (AZSTD_CHECKED_ITERATORS == 2))
    #define AZSTD_HAS_CHECKED_ITERATORS
#endif

/**
 * \def AZSTD_NO_DEFAULT_STD_INCLUDES
 * AZStd uses \cond <new> and <string.h> \endcond files for in place new, memcopy, memset, etc. If you don't want to include this files
 * make sure you have your included (in advance) and define this macro.
 */
#ifndef AZSTD_NO_DEFAULT_STD_INCLUDES
    #include <new>
    #include <string.h>
    #include <stdlib.h>
#endif

/**
 * \def AZSTD_NO_CHECKED_ITERATORS_IN_MULTI_THREADS
 * This define must be defined if you do not want to use containers in different threads and have AZSTD_CHECKED_ITERATORS enabled.
 * By default this is enabled for Windows when AZSTD_HAS_CHECKED_ITERATORS is enabled. This is used for debugging only, you still need to do the container access
 * synchronization yourself or use provided sync classes like concurent_queue, concurent_prioriry_queue, etc.
 */
#if !defined(AZSTD_NO_CHECKED_ITERATORS_IN_MULTI_THREADS) && defined(AZSTD_HAS_CHECKED_ITERATORS)
#define AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
#endif

#ifdef AZSTD_DOXYGEN_INVOKED
// for documentation only
    #define AZStd::allocator
    #define AZSTD_NO_DEFAULT_STD_INCLUDES
    #define AZSTD_NO_PARALLEL_SUPPORT
    #define AZSTD_NO_CHECKED_ITERATORS_IN_MULTI_THREADS
#endif

/// @}

#ifndef AZSTD_STL
    #if defined(AZ_COMPILER_MSVC)
        #define AZSTD_STL std
    #else
        #define AZSTD_STL
    #endif
#endif

/**
 *  By default we don't use RTTI. You can define AZ_USE_RTTI if you compile your code with rtti.
 *  We highly recommend that you rebuild all libs when you enable rtti because unexpected behavior
 *  may occur
 */
#if !defined(AZ_USE_RTTI)
#   define AZ_NO_RTTI
#endif

#if !defined(AZ_USE_AUTO_PTR)
#   define AZ_NO_AUTO_PTR
#endif

//////////////////////////////////////////////////////////////////////////
// Utils - \todo move to utils

/**
 * \defgroup ADLHelperMacros Multiple STL implementation helper macros - ADL issues
 * These helper macros can be used to specialize a function for a specific type, to fix \ref ADL issues.
 * For example in the standard the iter_swap function takes template arguments. If we use another STL
 * implementation and we call one of its functions which is not ADL safe there will be ambiguity.
 * \code
 *  AZStd { template<typename A, typename B> void iter_swap(A& a, B& b) {...} }
 *  OtherStl { template<typename A, typename B> void iter_swap(A& a, B& b) {...} }
 * \endcode
 *  Then we have a iterator from AZStd, let's say AZStd::vector<int>::iterator and we call
 *  a sort function from OtherStl...
 * \code
 *  OtherStl::sort(azstdVectorInt.begin(),azstdVectorInt.end(),std::less<int>);
 * \endcode
 * Now this code is all fine and will work fine, but if OtherStl::sort looks like
 * \code
 * namespace OtherStl
 * {
 *  template<Iterator>
 *  sort(Iterator first, Iterator last, ...) {
 *  ...
 *  iter_swap(first,last);  <- this is a problem for ADL, it can match AZStd::iter_swap or OtherStl::iter_swap. For this code to be safe we should have OtherStl::iter_swap(first,last),
 *                             but this usually is third party code, so we can't fix it. That is why we need to specialize for the current iterators (function).
 *  ...
 *  }
 * }
 * \endcode
 *
 * To fix that problem we can use the following macros which will specialize the particular type (AZStd::vector<int>::iterator from the example)
 * to use the AZStd::iter_swap.
 * So what we need to do is:
 * \code
 *   AZSTD_ADL_FIX_FUNCTION_SPEC_1_2(iter_swap,AZStd::vector<int>::iterator&);
 * \endcode
 * This line will resolve the issue.
 * For instance, this is a typical issue when you use AZStd with STLport.
 * @{
 */
#define AZSTD_ADL_FIX_FUNCTION_SPEC_1(_Function, _Param) \
    AZ_FORCE_INLINE void _Function(_Param a)        { AZStd::_Function(a); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_1_RET(_Function, _Return, _Param) \
    AZ_FORCE_INLINE _Return _Function(_Param a)             { return AZStd::_Function(a); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_1_2(_Function, _Param) \
    AZ_FORCE_INLINE void _Function(_Param a, _Param b)       { AZStd::_Function(a, b); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_1_2_RET(_Function, _Return, _Param) \
    AZ_FORCE_INLINE _Return _Function(_Param a, _Param b)        { return AZStd::_Function(a, b); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_2(_Function, _Param1, _Param2) \
    AZ_FORCE_INLINE void _Function(_Param1 a, _Param2 b)     { AZStd::_Function(a, b); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_2_RET(_Function, _Return, _Param1, _Param2) \
    AZ_FORCE_INLINE _Return _Function(_Param1 a, _Param2 b)  { return AZStd::_Function(a, b); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_2_3(_Function, _Param1, _Param2) \
    AZ_FORCE_INLINE void _Function(_Param1 a, _Param1 b, _Param2 c)   { AZStd::_Function(a, b, c); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_2_3_RET(_Function, _Return, _Param1, _Param2) \
    AZ_FORCE_INLINE _Return _Function(_Param1 a, _Param1 b, _Param2 c)    { return AZStd::_Function(a, b, c); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_2_4(_Function, _Param1, _Param2) \
    AZ_FORCE_INLINE void _Function(_Param1 a, _Param1 b, _Param1 c, _Param2 d) { AZStd::_Function(a, b, c, d); }
#define AZSTD_ADL_FIX_FUNCTION_SPEC_2_4_RET(_Function, _Return, _Param1, _Param2) \
    AZ_FORCE_INLINE _Return _Function(_Param1 a, _Param1 b, _Param1 c, _Param2 d)  { return AZStd::_Function(a, b, c, d); }
/// @}

#endif // AZSTD_CONFIG_H




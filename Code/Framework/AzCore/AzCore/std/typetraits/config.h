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
#ifndef AZSTD_TYPE_TRAITS_CONFIG_INCLUDED
#define AZSTD_TYPE_TRAITS_CONFIG_INCLUDED

#include <AzCore/std/base.h>

// Compilers are more consistent with the traits, since AZStd doesn't have the goal or design need
// to control that we are pulling std one into AZStd till we have all tests pass and have consisten syntax
// across compilers
#include <type_traits>

//
// define AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
// when we want to test __stdcall etc function types with is_function etc
//
#if defined(_MSC_EXTENSIONS)
    #define AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
#else
    #define AZSTD_DISABLE_WIN32
#endif

//
// whenever we have a conversion function with elipses
// it needs to be declared __cdecl to suppress compiler
// warnings from MS
#if defined(AZ_COMPILER_MSVC) && !defined(AZSTD_DISABLE_WIN32)
    #define AZSTD_TYPE_TRAITS_DECL __cdecl
#else
    #define AZSTD_TYPE_TRAITS_DECL /**/
#endif

namespace AZStd
{
    using std::is_constructible;
    using std::is_default_constructible;
    using std::is_assignable;
    //using std::is_trivially_constructible; still not implemented in GCC 4.9 Android
    //using std::is_trivially_assignable; still not implemented in GCC 4.9 Android

    using std::is_copy_constructible;
    using std::is_copy_assignable;
    //using std::is_trivially_copy_constructible; still not implemented in GCC 4.9 Android
    //using std::is_trivially_copy_assignable; still not implemented in GCC 4.9 Android

    using std::is_move_constructible;
    using std::is_move_assignable;
    //using std::is_trivially_move_constructible; still not implemented in GCC 4.9 Android
    //using std::is_trivially_move_assignable; still not implemented in GCC 4.9 Android

    using std::is_destructible;
    //using std::is_trivially_destructible; still not implemented in GCC 4.9 Android

} // namespace AZStd


#endif // AZSTD_TYPE_TRAITS_CONFIG_INCLUDED
#pragma once


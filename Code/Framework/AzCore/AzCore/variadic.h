/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


// Variadic MACROS util functions

/**
 * AZ_VA_NUM_ARGS
 * counts number of parameters (up to 10).
 * example. AZ_VA_NUM_ARGS(x,y,z) -> expands to 3
 */
#ifndef AZ_VA_NUM_ARGS

#   define AZ_VA_HAS_ARGS(...) ""#__VA_ARGS__[0] != 0

// we add the zero to avoid the case when we require at least 1 param at the end...
#   define AZ_VA_NUM_ARGS(...) AZ_VA_NUM_ARGS_IMPL_((__VA_ARGS__, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define AZ_VA_NUM_ARGS_IMPL_(tuple) AZ_VA_NUM_ARGS_IMPL tuple
#   define AZ_VA_NUM_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, _123, _124, _125, N, ...) N
// Expands a macro and calls a different macro based on the number of arguments.
//
// Example: We need to specialize a macro for 1 and 2 arguments
// #define AZ_MY_MACRO(...)   AZ_MACRO_SPECIALIZE(AZ_MY_MACRO_,AZ_VA_NUM_ARGS(__VA_ARGS__),(__VA_ARGS__))
//
// #define AZ_MY_MACRO_1(_1)    /* code for 1 param */
// #define AZ_MY_MACRO_2(_1,_2) /* code for 2 params */
// ... etc.
//
//
// We have 3 levels of macro expansion...
#   define AZ_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)    MACRO_NAME##NPARAMS PARAMS
#   define AZ_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)     AZ_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
#   define AZ_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS)       AZ_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

#endif // AZ_VA_NUM_ARGS


// Out of all supported compilers, mwerks is the only one
// that requires variadic macros to have at least 1 param.
// This is a pain they we use macros to call functions (with no params).

// we implement functions for up to 10 params
#define AZ_FUNCTION_CALL_1(_1)                                          _1()
#define AZ_FUNCTION_CALL_2(_1, _2)                                      _1(_2)
#define AZ_FUNCTION_CALL_3(_1, _2, _3)                                  _1(_2, _3)
#define AZ_FUNCTION_CALL_4(_1, _2, _3, _4)                              _1(_2, _3, _4)
#define AZ_FUNCTION_CALL_5(_1, _2, _3, _4, _5)                          _1(_2, _3, _4, _5)
#define AZ_FUNCTION_CALL_6(_1, _2, _3, _4, _5, _6)                      _1(_2, _3, _4, _5, _6)
#define AZ_FUNCTION_CALL_7(_1, _2, _3, _4, _5, _6, _7)                  _1(_2, _3, _4, _5, _6, _7)
#define AZ_FUNCTION_CALL_8(_1, _2, _3, _4, _5, _6, _7, _8)              _1(_2, _3, _4, _5, _6, _7, _8)
#define AZ_FUNCTION_CALL_9(_1, _2, _3, _4, _5, _6, _7, _8, _9)          _1(_2, _3, _4, _5, _6, _7, _8, _9)
#define AZ_FUNCTION_CALL_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10)    _1(_2, _3, _4, _5, _6, _7, _8, _9, _10)

// We require at least 1 param FunctionName
#define AZ_FUNCTION_CALL(...)           AZ_MACRO_SPECIALIZE(AZ_FUNCTION_CALL_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

// Based on boost macro expansion fix...
#define AZ_PREVENT_MACRO_SUBSTITUTION

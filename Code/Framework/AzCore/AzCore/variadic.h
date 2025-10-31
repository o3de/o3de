/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


// Variadic MACROS util functions

// Detect C++20 __VA_OPT__ macro support
// If it is not support then __VA_OPT__(,) will not expand
// Therefore it will be supplied as three arguments as the following
// AZ_VA_OPT_SUPPORTED_CHECK(__VA_OPT__(,), true, false)
// 1. __VA_OPT__(,)
// 2. true
// 3. false
//
// If it is supported then it will expand to a comma
// This results in two empty arguments before the true and false argument
// AZ_VA_OPT_SUPPORTED_CHECK(,, true, false)
// 1.
// 2.
// 3. true
// 4. false
#define AZ_VA_OPT_SUPPORTED_CHECK(first, second, va_opt_supported, ...) va_opt_supported
#define AZ_VA_OPT_SUPPORTED_IMPL(...)  AZ_VA_OPT_SUPPORTED_CHECK(__VA_OPT__(,), true, false)

// WARNING: __VA_OPT__ is not available in MSVC C++17 even when using the new preprocessor via the `/Zc:preprocessor` option,
// despite intellisense not diming the block. The actual value is false,
#define AZ_HAS_VA_OPT AZ_VA_OPT_SUPPORTED_IMPL(TestArg)

/**
 * AZ_VA_NUM_ARGS
 * counts number of parameters (up to 10).
 * example. AZ_VA_NUM_ARGS(x,y,z) -> expands to 3
 */
#ifndef AZ_VA_NUM_ARGS

#   define AZ_VA_HAS_ARGS(...) ""#__VA_ARGS__[0] != 0

#   define AZ_VA_NUM_ARGS(...) AZ_VA_NUM_ARGS_SUB(__VA_ARGS__)

// NOTE: Windows clang preprocessor has an issue with the AZ_VA_NUM_ARGS(,) case, so the non-VA_OPT case is used
// Also using the __clang__ macro instead of AZ_COMPILER_CLANG as this file cannot contain any includes
#if AZ_HAS_VA_OPT && (!defined(__clang__) || !defined(_MSC_VER ))
// Use the __VA_OPT__macro to optionally add a comma if the __VA_ARGS__ list is not empty
#   define AZ_VA_NUM_ARGS_SUB(...) AZ_VA_NUM_ARGS_IMPL(__VA_ARGS__ __VA_OPT__(,) 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define AZ_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, _123, _124, N, ...) N
#else
// we add the zero to avoid the case when we require at least 1 param at the end...
#   define AZ_VA_NUM_ARGS_SUB(...) AZ_VA_NUM_ARGS_IMPL(0, ## __VA_ARGS__, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define AZ_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, _123, _124, _125, N, ...) N
#endif

// Validated the num argument macro expands to the correct value
static_assert(AZ_VA_NUM_ARGS() == 0);
static_assert(AZ_VA_NUM_ARGS((1, 2)) == 1);
static_assert(AZ_VA_NUM_ARGS(5A) == 1);
static_assert(AZ_VA_NUM_ARGS(,) == 2);
static_assert(AZ_VA_NUM_ARGS(6B,) == 2);
static_assert(AZ_VA_NUM_ARGS(7C,8D) == 2);
static_assert(AZ_VA_NUM_ARGS(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92,
    93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
    114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125) == 125);


#define AZ_VA_OPT_CALL_II(MACRO_NAME, NPARAMS, separator_macro) MACRO_NAME##NPARAMS (separator_macro)
#define AZ_VA_OPT_CALL_I(MACRO_NAME, NPARAMS, separator_macro) AZ_VA_OPT_CALL_II(MACRO_NAME, NPARAMS, separator_macro)
#define AZ_VA_OPT_CALL(MACRO_NAME, NPARAMS, separator_macro) AZ_VA_OPT_CALL_I(MACRO_NAME, NPARAMS, separator_macro)
#define AZ_VA_OPT_0(separator_macro)
#define AZ_VA_OPT_1(separator_macro) separator_macro()
#define AZ_VA_OPT_2(separator_macro) separator_macro()
#define AZ_VA_OPT_3(separator_macro) separator_macro()
#define AZ_VA_OPT_4(separator_macro) separator_macro()
#define AZ_VA_OPT_5(separator_macro) separator_macro()
#define AZ_VA_OPT_6(separator_macro) separator_macro()
#define AZ_VA_OPT_7(separator_macro) separator_macro()
#define AZ_VA_OPT_8(separator_macro) separator_macro()
#define AZ_VA_OPT_9(separator_macro) separator_macro()
#define AZ_VA_OPT_10(separator_macro) separator_macro()
//! This method optionally adds the separator if the __VA_ARGS__ list is not empty
//! The caller should pass the __VA_ARGS__ from an outer macro call to this method
//! The way to use this macro is to add it between tokens separated by spaces
//! ex. `define MACRO(Token1, ...) Token1 AZ_VA_OPT(COMMA_SEPARATOR_MACRO, __VA_ARGS__) __VA_ARGS__`
//! When no macro arguments are supplied, the separator_macro is not called
//! When 1 or more macro arguments are supplied, the separator_macro is called
#define AZ_VA_OPT_HELPER(separator_macro, ...) AZ_VA_OPT_CALL(AZ_VA_OPT_, AZ_VA_NUM_ARGS(__VA_ARGS__), separator_macro)
#define AZ_VA_OPT(separator_macro, ...) AZ_VA_OPT_HELPER(separator_macro, __VA_ARGS__)

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
//
// NOTE: For this macro, it can only expand once per chain of macro calls
// For example given the following set of macros
// #define AZ_EXPAND_1(X) X
// #define AZ_TYPE_INFO_INTERNAL_1(params) AZ_MACRO_SPECIALIZE(AZ_EXPAND_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))
//
// If the AZ_MACRO_SPEICALIZE is used as follows with a single argument of FOO:
// `AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_, 1, (FOO))`,
// then the inner AZ_EXPAND_1 will NOT expand because the preprocessor suppresses expansion of the `AZ_MACRO_SPECIALIZE`
// with even if it is called with a different set of arguments.
// So the result of the macro is  `AZ_MACRO_SPECIALIZE(AZ_EXPAND_, 1, (FOO))`, not `FOO`
// https://godbolt.org/z/GM6f7drh1
//
// See the following thread for information about the C Preprocessor works in this manner:
// https://stackoverflow.com/questions/66593868/understanding-the-behavior-of-cs-preprocessor-when-a-macro-indirectly-expands-i
// Since the C preprocessor is not turning complete, the only workaround is to provide a macro with the same content of
// `AZ_MACRO_SPECIALIZE` macros but with a different name for every use of it.
#   define AZ_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)    MACRO_NAME##NPARAMS PARAMS
#   define AZ_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)     AZ_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
#   define AZ_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS)       AZ_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

#endif // AZ_VA_NUM_ARGS

// Macro which does nothing with its argument
// It can be used as a unary predicate for the AZ_FOR_EACH* macros
#define AZ_IDENITY_MACRO(X) X

// The following is used to join the AZ_JOIN_EACH_<N> function in order to implement
// the AZ_JOIN function which can join 2 or more parameters
#define AZ_JOIN_TWO_INTERNAL_II(X, Y) X##Y
#define AZ_JOIN_TWO_INTERNAL_I(X, Y) AZ_JOIN_TWO_INTERNAL_II(X, Y)
#define AZ_JOIN_TWO_INTERNAL(X, Y) AZ_JOIN_TWO_INTERNAL_I(X, Y)

// For each macro where with no bound arguments
#define AZ_JOIN_EACH_0(  param)
#define AZ_JOIN_EACH_1(  param)      param
#define AZ_JOIN_EACH_2(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_1(__VA_ARGS__))
#define AZ_JOIN_EACH_3(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_2(__VA_ARGS__))
#define AZ_JOIN_EACH_4(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_3(__VA_ARGS__))
#define AZ_JOIN_EACH_5(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_4(__VA_ARGS__))
#define AZ_JOIN_EACH_6(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_5(__VA_ARGS__))
#define AZ_JOIN_EACH_7(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_6(__VA_ARGS__))
#define AZ_JOIN_EACH_8(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_7(__VA_ARGS__))
#define AZ_JOIN_EACH_9(  param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_8(__VA_ARGS__))
#define AZ_JOIN_EACH_10( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_9(__VA_ARGS__))
#define AZ_JOIN_EACH_11( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_10(__VA_ARGS__))
#define AZ_JOIN_EACH_12( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_11(__VA_ARGS__))
#define AZ_JOIN_EACH_13( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_12(__VA_ARGS__))
#define AZ_JOIN_EACH_14( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_13(__VA_ARGS__))
#define AZ_JOIN_EACH_15( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_14(__VA_ARGS__))
#define AZ_JOIN_EACH_16( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_15(__VA_ARGS__))
#define AZ_JOIN_EACH_17( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_16(__VA_ARGS__))
#define AZ_JOIN_EACH_18( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_17(__VA_ARGS__))
#define AZ_JOIN_EACH_19( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_18(__VA_ARGS__))
#define AZ_JOIN_EACH_20( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_19(__VA_ARGS__))
#define AZ_JOIN_EACH_21( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_20(__VA_ARGS__))
#define AZ_JOIN_EACH_22( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_21(__VA_ARGS__))
#define AZ_JOIN_EACH_23( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_22(__VA_ARGS__))
#define AZ_JOIN_EACH_24( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_23(__VA_ARGS__))
#define AZ_JOIN_EACH_25( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_24(__VA_ARGS__))
#define AZ_JOIN_EACH_26( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_25(__VA_ARGS__))
#define AZ_JOIN_EACH_27( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_26(__VA_ARGS__))
#define AZ_JOIN_EACH_28( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_27(__VA_ARGS__))
#define AZ_JOIN_EACH_29( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_28(__VA_ARGS__))
#define AZ_JOIN_EACH_30( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_29(__VA_ARGS__))
#define AZ_JOIN_EACH_31( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_30(__VA_ARGS__))
#define AZ_JOIN_EACH_32( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_31(__VA_ARGS__))
#define AZ_JOIN_EACH_33( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_32(__VA_ARGS__))
#define AZ_JOIN_EACH_34( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_33(__VA_ARGS__))
#define AZ_JOIN_EACH_35( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_34(__VA_ARGS__))
#define AZ_JOIN_EACH_36( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_35(__VA_ARGS__))
#define AZ_JOIN_EACH_37( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_36(__VA_ARGS__))
#define AZ_JOIN_EACH_38( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_37(__VA_ARGS__))
#define AZ_JOIN_EACH_39( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_38(__VA_ARGS__))
#define AZ_JOIN_EACH_40( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_39(__VA_ARGS__))
#define AZ_JOIN_EACH_41( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_40(__VA_ARGS__))
#define AZ_JOIN_EACH_42( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_41(__VA_ARGS__))
#define AZ_JOIN_EACH_43( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_42(__VA_ARGS__))
#define AZ_JOIN_EACH_44( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_43(__VA_ARGS__))
#define AZ_JOIN_EACH_45( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_44(__VA_ARGS__))
#define AZ_JOIN_EACH_46( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_45(__VA_ARGS__))
#define AZ_JOIN_EACH_47( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_46(__VA_ARGS__))
#define AZ_JOIN_EACH_48( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_47(__VA_ARGS__))
#define AZ_JOIN_EACH_49( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_48(__VA_ARGS__))
#define AZ_JOIN_EACH_50( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_49(__VA_ARGS__))
#define AZ_JOIN_EACH_51( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_50(__VA_ARGS__))
#define AZ_JOIN_EACH_52( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_51(__VA_ARGS__))
#define AZ_JOIN_EACH_53( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_52(__VA_ARGS__))
#define AZ_JOIN_EACH_54( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_53(__VA_ARGS__))
#define AZ_JOIN_EACH_55( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_54(__VA_ARGS__))
#define AZ_JOIN_EACH_56( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_55(__VA_ARGS__))
#define AZ_JOIN_EACH_57( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_56(__VA_ARGS__))
#define AZ_JOIN_EACH_58( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_57(__VA_ARGS__))
#define AZ_JOIN_EACH_59( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_58(__VA_ARGS__))
#define AZ_JOIN_EACH_60( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_59(__VA_ARGS__))
#define AZ_JOIN_EACH_61( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_60(__VA_ARGS__))
#define AZ_JOIN_EACH_62( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_61(__VA_ARGS__))
#define AZ_JOIN_EACH_63( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_62(__VA_ARGS__))
#define AZ_JOIN_EACH_64( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_63(__VA_ARGS__))
#define AZ_JOIN_EACH_65( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_64(__VA_ARGS__))
#define AZ_JOIN_EACH_66( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_65(__VA_ARGS__))
#define AZ_JOIN_EACH_67( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_66(__VA_ARGS__))
#define AZ_JOIN_EACH_68( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_67(__VA_ARGS__))
#define AZ_JOIN_EACH_69( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_68(__VA_ARGS__))
#define AZ_JOIN_EACH_70( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_69(__VA_ARGS__))
#define AZ_JOIN_EACH_71( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_70(__VA_ARGS__))
#define AZ_JOIN_EACH_72( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_71(__VA_ARGS__))
#define AZ_JOIN_EACH_73( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_72(__VA_ARGS__))
#define AZ_JOIN_EACH_74( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_73(__VA_ARGS__))
#define AZ_JOIN_EACH_75( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_74(__VA_ARGS__))
#define AZ_JOIN_EACH_76( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_75(__VA_ARGS__))
#define AZ_JOIN_EACH_77( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_76(__VA_ARGS__))
#define AZ_JOIN_EACH_78( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_77(__VA_ARGS__))
#define AZ_JOIN_EACH_79( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_78(__VA_ARGS__))
#define AZ_JOIN_EACH_80( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_79(__VA_ARGS__))
#define AZ_JOIN_EACH_81( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_80(__VA_ARGS__))
#define AZ_JOIN_EACH_82( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_81(__VA_ARGS__))
#define AZ_JOIN_EACH_83( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_82(__VA_ARGS__))
#define AZ_JOIN_EACH_84( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_83(__VA_ARGS__))
#define AZ_JOIN_EACH_85( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_84(__VA_ARGS__))
#define AZ_JOIN_EACH_86( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_85(__VA_ARGS__))
#define AZ_JOIN_EACH_87( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_86(__VA_ARGS__))
#define AZ_JOIN_EACH_88( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_87(__VA_ARGS__))
#define AZ_JOIN_EACH_89( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_88(__VA_ARGS__))
#define AZ_JOIN_EACH_90( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_89(__VA_ARGS__))
#define AZ_JOIN_EACH_91( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_90(__VA_ARGS__))
#define AZ_JOIN_EACH_92( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_91(__VA_ARGS__))
#define AZ_JOIN_EACH_93( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_92(__VA_ARGS__))
#define AZ_JOIN_EACH_94( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_93(__VA_ARGS__))
#define AZ_JOIN_EACH_95( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_94(__VA_ARGS__))
#define AZ_JOIN_EACH_96( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_95(__VA_ARGS__))
#define AZ_JOIN_EACH_97( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_96(__VA_ARGS__))
#define AZ_JOIN_EACH_98( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_97(__VA_ARGS__))
#define AZ_JOIN_EACH_99( param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_98(__VA_ARGS__))
#define AZ_JOIN_EACH_100(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_99(__VA_ARGS__))
#define AZ_JOIN_EACH_101(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_100(__VA_ARGS__))
#define AZ_JOIN_EACH_102(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_101(__VA_ARGS__))
#define AZ_JOIN_EACH_103(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_102(__VA_ARGS__))
#define AZ_JOIN_EACH_104(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_103(__VA_ARGS__))
#define AZ_JOIN_EACH_105(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_104(__VA_ARGS__))
#define AZ_JOIN_EACH_106(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_105(__VA_ARGS__))
#define AZ_JOIN_EACH_107(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_106(__VA_ARGS__))
#define AZ_JOIN_EACH_108(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_107(__VA_ARGS__))
#define AZ_JOIN_EACH_109(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_108(__VA_ARGS__))
#define AZ_JOIN_EACH_110(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_109(__VA_ARGS__))
#define AZ_JOIN_EACH_111(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_110(__VA_ARGS__))
#define AZ_JOIN_EACH_112(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_111(__VA_ARGS__))
#define AZ_JOIN_EACH_113(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_112(__VA_ARGS__))
#define AZ_JOIN_EACH_114(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_113(__VA_ARGS__))
#define AZ_JOIN_EACH_115(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_114(__VA_ARGS__))
#define AZ_JOIN_EACH_116(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_115(__VA_ARGS__))
#define AZ_JOIN_EACH_117(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_116(__VA_ARGS__))
#define AZ_JOIN_EACH_118(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_117(__VA_ARGS__))
#define AZ_JOIN_EACH_119(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_118(__VA_ARGS__))
#define AZ_JOIN_EACH_120(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_119(__VA_ARGS__))
#define AZ_JOIN_EACH_121(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_120(__VA_ARGS__))
#define AZ_JOIN_EACH_122(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_121(__VA_ARGS__))
#define AZ_JOIN_EACH_123(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_122(__VA_ARGS__))
#define AZ_JOIN_EACH_124(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_123(__VA_ARGS__))
#define AZ_JOIN_EACH_125(param, ...) AZ_JOIN_TWO_INTERNAL(param, AZ_JOIN_EACH_124(__VA_ARGS__))


// Joins 2 or more macro parameters together
#define AZ_JOIN_EACH_CALL_II(macro, ...) macro(__VA_ARGS__)
#define AZ_JOIN_EACH_CALL_I(macro, ...) AZ_JOIN_EACH_CALL_II(macro, __VA_ARGS__)
#define AZ_JOIN_EACH_CALL(macro, ...) AZ_JOIN_EACH_CALL_I(macro, __VA_ARGS__)
#define AZ_JOIN(...) AZ_JOIN_EACH_CALL(AZ_JOIN_TWO_INTERNAL(AZ_JOIN_EACH_, AZ_VA_NUM_ARGS(__VA_ARGS__)),__VA_ARGS__)

// Based on boost macro expansion fix...
#define AZ_PREVENT_MACRO_SUBSTITUTION


// For each macro where a fixed first argument is supplied to the supplied macro
#define AZ_FOR_EACH_BIND1ST_0(pppredicate,  boundarg, param)
#define AZ_FOR_EACH_BIND1ST_1(pppredicate,  boundarg, param)      pppredicate(boundarg, param)
#define AZ_FOR_EACH_BIND1ST_2(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_1(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_3(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_2(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_4(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_3(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_5(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_4(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_6(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_5(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_7(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_6(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_8(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_7(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_9(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_8(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_10(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_9(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_11(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_10(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_12(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_11(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_13(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_12(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_14(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_13(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_15(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_14(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_16(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_15(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_17(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_16(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_18(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_17(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_19(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_18(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_20(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_19(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_21(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_20(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_22(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_21(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_23(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_22(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_24(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_23(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_25(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_24(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_26(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_25(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_27(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_26(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_28(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_27(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_29(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_28(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_30(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_29(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_31(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_30(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_32(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_31(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_33(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_32(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_34(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_33(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_35(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_34(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_36(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_35(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_37(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_36(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_38(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_37(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_39(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_38(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_40(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_39(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_41(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_40(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_42(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_41(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_43(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_42(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_44(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_43(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_45(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_44(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_46(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_45(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_47(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_46(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_48(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_47(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_49(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_48(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_50(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_49(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_51(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_50(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_52(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_51(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_53(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_52(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_54(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_53(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_55(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_54(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_56(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_55(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_57(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_56(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_58(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_57(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_59(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_58(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_60(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_59(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_61(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_60(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_62(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_61(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_63(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_62(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_64(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_63(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_65(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_64(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_66(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_65(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_67(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_66(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_68(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_67(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_69(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_68(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_70(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_69(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_71(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_70(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_72(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_71(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_73(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_72(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_74(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_73(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_75(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_74(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_76(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_75(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_77(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_76(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_78(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_77(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_79(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_78(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_80(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_79(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_81(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_80(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_82(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_81(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_83(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_82(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_84(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_83(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_85(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_84(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_86(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_85(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_87(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_86(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_88(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_87(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_89(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_88(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_90(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_89(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_91(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_90(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_92(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_91(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_93(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_92(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_94(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_93(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_95(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_94(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_96(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_95(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_97(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_96(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_98(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_97(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_99(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_98(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_100(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_99(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_101(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_100(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_102(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_101(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_103(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_102(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_104(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_103(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_105(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_104(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_106(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_105(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_107(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_106(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_108(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_107(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_109(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_108(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_110(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_109(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_111(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_110(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_112(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_111(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_113(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_112(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_114(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_113(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_115(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_114(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_116(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_115(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_117(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_116(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_118(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_117(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_119(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_118(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_120(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_119(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_121(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_120(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_122(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_121(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_123(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_122(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_124(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_123(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_125(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST_124(pppredicate, boundarg, __VA_ARGS__)

#define AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, ...) AZ_FOR_EACH_CALL(AZ_JOIN(AZ_FOR_EACH_BIND1ST_, AZ_VA_NUM_ARGS(__VA_ARGS__)), pppredicate, boundarg, __VA_ARGS__)

// For each macro where with no bound arguments
#define AZ_FOR_EACH_0(pppredicate,   separator, param)
#define AZ_FOR_EACH_1(pppredicate,   separator, param)      pppredicate(param)
#define AZ_FOR_EACH_2(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_1(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_3(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_2(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_4(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_3(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_5(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_4(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_6(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_5(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_7(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_6(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_8(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_7(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_9(pppredicate,   separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_8(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_10(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_9(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_11(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_10(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_12(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_11(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_13(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_12(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_14(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_13(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_15(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_14(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_16(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_15(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_17(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_16(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_18(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_17(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_19(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_18(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_20(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_19(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_21(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_20(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_22(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_21(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_23(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_22(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_24(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_23(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_25(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_24(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_26(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_25(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_27(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_26(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_28(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_27(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_29(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_28(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_30(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_29(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_31(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_30(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_32(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_31(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_33(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_32(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_34(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_33(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_35(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_34(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_36(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_35(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_37(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_36(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_38(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_37(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_39(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_38(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_40(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_39(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_41(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_40(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_42(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_41(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_43(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_42(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_44(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_43(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_45(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_44(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_46(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_45(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_47(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_46(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_48(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_47(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_49(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_48(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_50(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_49(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_51(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_50(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_52(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_51(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_53(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_52(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_54(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_53(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_55(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_54(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_56(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_55(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_57(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_56(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_58(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_57(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_59(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_58(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_60(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_59(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_61(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_60(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_62(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_61(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_63(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_62(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_64(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_63(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_65(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_64(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_66(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_65(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_67(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_66(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_68(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_67(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_69(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_68(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_70(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_69(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_71(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_70(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_72(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_71(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_73(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_72(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_74(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_73(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_75(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_74(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_76(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_75(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_77(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_76(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_78(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_77(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_79(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_78(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_80(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_79(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_81(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_80(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_82(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_81(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_83(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_82(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_84(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_83(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_85(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_84(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_86(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_85(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_87(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_86(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_88(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_87(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_89(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_88(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_90(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_89(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_91(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_90(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_92(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_91(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_93(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_92(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_94(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_93(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_95(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_94(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_96(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_95(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_97(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_96(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_98(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_97(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_99(pppredicate,  separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_98(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_100(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_99(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_101(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_100(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_102(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_101(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_103(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_102(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_104(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_103(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_105(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_104(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_106(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_105(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_107(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_106(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_108(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_107(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_109(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_108(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_110(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_109(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_111(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_110(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_112(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_111(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_113(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_112(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_114(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_113(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_115(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_114(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_116(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_115(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_117(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_116(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_118(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_117(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_119(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_118(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_120(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_119(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_121(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_120(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_122(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_121(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_123(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_122(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_124(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_123(pppredicate, separator, __VA_ARGS__)
#define AZ_FOR_EACH_125(pppredicate, separator, param, ...) pppredicate(param) separator() AZ_FOR_EACH_124(pppredicate, separator, __VA_ARGS__)

// Invokes unary predicate macro on each variadic argument, without any separator between calls to unary predicate
#define AZ_FOR_EACH(pp_unary_predicate, ...) AZ_FOR_EACH_CALL(AZ_JOIN(AZ_FOR_EACH_, AZ_VA_NUM_ARGS(__VA_ARGS__)),pp_unary_predicate,AZ_EMPTY_SEPARATOR,__VA_ARGS__)
// Invokes unary predicate macro on each variadic argument with a separator_macro() macro that adds a separator between each argument
#define AZ_FOR_EACH_WITH_SEPARATOR(pp_unary_predicate, separator_macro, ...) AZ_FOR_EACH_CALL(AZ_JOIN(AZ_FOR_EACH_, AZ_VA_NUM_ARGS(__VA_ARGS__)),pp_unary_predicate,separator_macro,__VA_ARGS__)

// Macro used only for expanding macros arguments for the AZ_FOR_EACH_BIND1ST, AZ_FOR_EACH macro
// This should not be used directly
#define AZ_FOR_EACH_CALL_II(macro, ...) macro(__VA_ARGS__)
#define AZ_FOR_EACH_CALL_I(macro, ...) AZ_FOR_EACH_CALL_II(macro, __VA_ARGS__)
#define AZ_FOR_EACH_CALL(macro, ...) AZ_FOR_EACH_CALL_I(macro, __VA_ARGS__)

// Used for the normal AZ_FOR_EACH macro to resolve macro to expand separator() to an empty expansion
#define AZ_EMPTY_SEPARATOR()

// Allows passing a comma has a separator to AZ_FOR_EACH_WITH_SEPARATOR / AZ_FOR_EACH_UNWRAP_WITH_SEPARATOR
// It has to be a macro call via (), so that the comma isn't immediately substituted
#define AZ_COMMA_SEPARATOR() ,

// Unpacks a (...) parameter into variable arguments
// Match case when a "()" __VA_ARGS__ is unwrapped
#define AZ_FOR_EACH_UNWRAP_UNPACKED(pp_unary_predicate, ...) AZ_FOR_EACH(pp_unary_predicate,__VA_ARGS__)
#define AZ_FOR_EACH_UNWRAP(pp_unary_predicate, parenthesized_args) AZ_FOR_EACH_UNWRAP_UNPACKED(pp_unary_predicate,AZ_UNWRAP(parenthesized_args))
// Unwraps a parenthesize expression and invokes AZ_FOR_EACH_WITH_SEPARATOR
#define AZ_FOR_EACH_UNWRAP_UNPACKED_WITH_SEPARATOR(pp_unary_predicate, separator_macro, ...) AZ_FOR_EACH_WITH_SEPARATOR(pp_unary_predicate,separator_macro,__VA_ARGS__)
#define AZ_FOR_EACH_UNWRAP_WITH_SEPARATOR(pp_unary_predicate, separator_macro, parenthesized_args) AZ_FOR_EACH_UNWRAP_UNPACKED_WITH_SEPARATOR(pp_unary_predicate,separator_macro,AZ_UNWRAP(parenthesized_args))

#define AZ_REMOVE_PARENTHESIS(...) AZ_REMOVE(AZ_PARENTHESIS __VA_ARGS__)
#define AZ_PARENTHESIS(...) AZ_PARENTHESIS __VA_ARGS__
#define AZ_REMOVE(...) AZ_REMOVE_I(__VA_ARGS__)
#define AZ_REMOVE_I(...) O3DE_ ## __VA_ARGS__
#define O3DE_AZ_PARENTHESIS
#define AZ_UNWRAP(...) AZ_REMOVE(AZ_PARENTHESIS __VA_ARGS__)
// Wraps variadic arguments inside parenthesis
#define AZ_WRAP_I(...) (__VA_ARGS__)
#define AZ_WRAP(...) AZ_WRAP_I(__VA_ARGS__)

// Extracts the first argument from a list of variadic arguments
#define AZ_USE_FIRST_ARG_1(_1, ...) _1
#define AZ_USE_FIRST_ARG(...) AZ_USE_FIRST_ARG_1(__VA_ARGS__)

// Removes the first arguments from a list of variardic arguments
#define AZ_SKIP_FIRST_ARG_I(_1, ...)  __VA_ARGS__
#define AZ_SKIP_FIRST_ARG(...) AZ_SKIP_FIRST_ARG_I(__VA_ARGS__)

// Provides a level of indirection for a macro function like call to allow expansion to occur
// This will invoke the macro in the second pass over the macro arguments
#define AZ_MACRO_CALL_II(macro, ...) macro(__VA_ARGS__)
#define AZ_MACRO_CALL_I(macro, ...) AZ_MACRO_CALL_II(macro, __VA_ARGS__)
#define AZ_MACRO_CALL(macro, ...) AZ_MACRO_CALL_I(macro, __VA_ARGS__)

// Does not use a wrapper macro to expand the __VA_ARGS__ first
// This should be used for triggering macros in the first pass over the macro before it is suppressed for recursive calls
#define AZ_MACRO_CALL_FIRST_PASS(macro, ...) macro(__VA_ARGS__)
#define AZ_MACRO_CALL_INDEX(prefix, ...) AZ_MACRO_CALL(AZ_JOIN(prefix, AZ_VA_NUM_ARGS(AZ_UNWRAP(__VA_ARGS__))), AZ_UNWRAP(__VA_ARGS__))
#define AZ_MACRO_CALL_WRAP(macro, ...)  AZ_MACRO_CALL(macro, __VA_ARGS__)


// Provides macro call expansion solely for the AZ_SIMPLE_TEMPLATE_ID_<N> and AZ_TEMPLATE_ARGUMENT_LIST_<N> macros below
#define AZ_TEMPLATE_MACRO_CALL_II(macro, nparams, params) macro##nparams params
#define AZ_TEMPLATE_MACRO_CALL_I(macro, nparams, params) AZ_TEMPLATE_MACRO_CALL_II(macro, nparams, params)
#define AZ_TEMPLATE_MACRO_CALL(macro, nparams, params) AZ_TEMPLATE_MACRO_CALL_I(macro, nparams, params)

#define AZ_TEMPLATE_MACRO_EXPANSION_CALL_II(macro, nparams, params) macro##nparams params
#define AZ_TEMPLATE_MACRO_EXPANSION_CALL_I(macro, nparams, params) AZ_TEMPLATE_MACRO_EXPANSION_CALL_II(macro, nparams, params)
#define AZ_TEMPLATE_MACRO_EXPANSION_CALL(macro, nparams, params) AZ_TEMPLATE_MACRO_EXPANSION_CALL_I(macro, nparams, params)

#define AZ_CLASS_TYPE class
#define AZ_CLASS_ARG(A) A

#define AZ_CLASS_VARARGS_TYPE class...
#define AZ_CLASS_VARARGS_ARG(A) A...

#define AZ_AUTO_TYPE auto
#define AZ_AUTO_ARG(A) A

#define AZ_TEMPLATE_TYPE_EXPANSION_0(_1)
#define AZ_TEMPLATE_TYPE_EXPANSION_1(_1) AZ_JOIN(_1, _TYPE) T1
#define AZ_TEMPLATE_TYPE_EXPANSION_2(_1, _2) AZ_TEMPLATE_TYPE_EXPANSION_1(_1),             AZ_JOIN(_2, _TYPE) T2
#define AZ_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3) AZ_TEMPLATE_TYPE_EXPANSION_2(_1, _2),         AZ_JOIN(_3, _TYPE) T3
#define AZ_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4) AZ_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3),     AZ_JOIN(_4, _TYPE) T4
#define AZ_TEMPLATE_TYPE_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4), AZ_JOIN(_5, _TYPE) T5
#define AZ_TEMPLATE_TYPE_EXPANSION(...) AZ_TEMPLATE_MACRO_EXPANSION_CALL(AZ_TEMPLATE_TYPE_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_TEMPLATE_ARGUMENT_EXPANSION_0(_1)
#define AZ_TEMPLATE_ARGUMENT_EXPANSION_1(_1) AZ_JOIN(_1, _ARG)(T1)
#define AZ_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2) AZ_TEMPLATE_ARGUMENT_EXPANSION_1(_1),             AZ_JOIN(_2, _ARG)(T2)
#define AZ_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3) AZ_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2),         AZ_JOIN(_3, _ARG)(T3)
#define AZ_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4) AZ_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3),     AZ_JOIN(_4, _ARG)(T4)
#define AZ_TEMPLATE_ARGUMENT_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4), AZ_JOIN(_5, _ARG)(T5)
#define AZ_TEMPLATE_ARGUMENT_EXPANSION(...) AZ_TEMPLATE_MACRO_EXPANSION_CALL(AZ_TEMPLATE_ARGUMENT_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_SIMPLE_TEMPLATE_ID_0(...)
#define AZ_SIMPLE_TEMPLATE_ID_1(...) template< AZ_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__) >
#define AZ_SIMPLE_TEMPLATE_ID_2(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_3(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_4(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_5(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_6(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_7(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_8(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_9(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_10(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_11(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_12(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_13(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_14(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID_15(...) AZ_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_SIMPLE_TEMPLATE_ID(...) AZ_TEMPLATE_MACRO_CALL(AZ_SIMPLE_TEMPLATE_ID_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))


#define AZ_TEMPLATE_ARGUMENT_LIST_0(...)
#define AZ_TEMPLATE_ARGUMENT_LIST_1(...) < AZ_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__) >
#define AZ_TEMPLATE_ARGUMENT_LIST_2(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_3(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_4(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_5(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_6(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_7(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_8(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_9(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_10(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_11(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_12(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_13(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_14(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST_15(...) AZ_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TEMPLATE_ARGUMENT_LIST(...) AZ_TEMPLATE_MACRO_CALL(AZ_TEMPLATE_ARGUMENT_LIST_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// === API ===

//! Generate an enumeration. Decorated with string conversion, and count capabilities.
//! example:
//!     AZ_ENUM(MyCoolEnum, A, B);
//! is equivalent to
//!     enum MyCoolEnum { A, B };
//! plus, will introduce in your scope:
//!     a MyCoolEnumNamespace   namespace
//!     a MyCoolEnumCount       inline size_t with the number of enumerators
//!     a MyCoolEnumMembers     AZStd::array of elements describing the enumerators value and string
//!     two string conversion functions:
//!         ToString(MyCoolEnum) -> string_view
//!     and
//!         FromStringToMyCoolEnum(string_view) -> optional< MyCoolEnum >
//!     (also accessible through
//!         MyCoolEnumNamespace::FromString)
//! note that you can specify values to your enumerators this way:
//!     AZ_ENUM(E,  (a, 1),  (b, 2),  (c, 4) );
//!
//! limitation: The ToString function will only return an integral value with the first option associated with that value.
//!             Ex. AZ_CLASS_ENUM(E, (a, 1), (b, 2), (c, 2) )
//!             Invoking either ToString(E::b) or ToString(E::c) will result in the string "b" being returned
//! limitation: maximum of 125 enumerators
#define AZ_ENUM(EnumTypeName, ...)                                                      MAKE_REFLECTABLE_ENUM_UNSCOPED(EnumTypeName, __VA_ARGS__)

//! Generate a decorated enumeration as AZ_ENUM, but with a specific underlying type.
#define AZ_ENUM_WITH_UNDERLYING_TYPE(EnumTypeName, EnumUnderlyingType, ...)             MAKE_REFLECTABLE_ENUM_UNSCOPED_WITH_UNDERLYING_TYPE(EnumTypeName, EnumUnderlyingType, __VA_ARGS__)

//! Generate a decorated enumeration as AZ_ENUM, but with the class qualification (scoped enumeration).
#define AZ_ENUM_CLASS(EnumTypeName, ...)                                                MAKE_REFLECTABLE_ENUM_SCOPED(EnumTypeName, __VA_ARGS__)

//! Generate a decorated enumeration as AZ_ENUM, but with class qualification and a specific underlying type.
#define AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(EnumTypeName, EnumUnderlyingType, ...)       MAKE_REFLECTABLE_ENUM_SCOPED_WITH_UNDERLYING_TYPE(EnumTypeName, EnumUnderlyingType, __VA_ARGS__)



// === implementation ===

#include <AzCore/base.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/optional.h>

// For each macro where a fixed first argument is supplied to the supplied macro
#define AZ_FOR_EACH_BIND1ST_1(pppredicate,  boundarg, param)      pppredicate(boundarg, param)
#define AZ_FOR_EACH_BIND1ST_2(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_3(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_4(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_5(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_6(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_7(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_8(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_9(pppredicate,  boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_10(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_11(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_12(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_13(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_14(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_15(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_16(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_17(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_18(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_19(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_20(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_21(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_22(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_23(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_24(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_25(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_26(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_27(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_28(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_29(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_30(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_31(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_32(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_33(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_34(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_35(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_36(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_37(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_38(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_39(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_40(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_41(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_42(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_43(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_44(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_45(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_46(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_47(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_48(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_49(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_50(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_51(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_52(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_53(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_54(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_55(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_56(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_57(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_58(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_59(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_60(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_61(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_62(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_63(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_64(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_65(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_66(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_67(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_68(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_69(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_70(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_71(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_72(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_73(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_74(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_75(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_76(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_77(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_78(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_79(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_80(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_81(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_82(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_83(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_84(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_85(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_86(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_87(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_88(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_89(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_90(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_91(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_92(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_93(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_94(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_95(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_96(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_97(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_98(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_99(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_100(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_101(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_102(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_103(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_104(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_105(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_106(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_107(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_108(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_109(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_110(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_111(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_112(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_113(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_114(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_115(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_116(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_117(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_118(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_119(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_120(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_121(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_122(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_123(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_124(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)
#define AZ_FOR_EACH_BIND1ST_125(pppredicate, boundarg, param, ...) pppredicate(boundarg, param) AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, __VA_ARGS__)

#define AZ_FOR_EACH_BIND1ST(pppredicate, boundarg, ...) AZ_MACRO_CALL(AZ_JOIN(AZ_FOR_EACH_BIND1ST_, AZ_VA_NUM_ARGS(__VA_ARGS__)), pppredicate, boundarg, __VA_ARGS__)

#define AZ_REMOVE_PARENTHESIS(...) AZ_REMOVE_PARENTHESIS __VA_ARGS__
#define AAZ_REMOVE_PARENTHESIS
#define AZ_JOIN_VA_ARGS_IMPL(X, ...) X ## __VA_ARGS__
#define AZ_JOIN_VA_ARGS(X, ...) AZ_JOIN_VA_ARGS_IMPL(X, __VA_ARGS__)
#define AZ_UNWRAP(...)  AZ_JOIN_VA_ARGS(A, AZ_REMOVE_PARENTHESIS __VA_ARGS__)
#define AZ_WRAP(...) (__VA_ARGS__)


#define AZ_MACRO_CALL(macro, ...)  macro AZ_WRAP(__VA_ARGS__)
#define AZ_MACRO_CALL_INDEX(prefix, ...) AZ_MACRO_CALL(AZ_JOIN(prefix, AZ_VA_NUM_ARGS(AZ_UNWRAP(__VA_ARGS__))), AZ_UNWRAP(__VA_ARGS__))
#define AZ_MACRO_CALL_WRAP(macro, ...)  AZ_MACRO_CALL(macro, __VA_ARGS__)
// Needed for clang preprocessor to argument prescan to expand arguments subs
// https://gcc.gnu.org/onlinedocs/gcc-9.2.0/cpp/Argument-Prescan.html#Argument-Prescan
#define AZ_IDENTITY_128(...) AZ_IDENTITY_64(AZ_IDENTITY_64(__VA_ARGS__))
#define AZ_IDENTITY_64(...) AZ_IDENTITY_32(AZ_IDENTITY_32(__VA_ARGS__))
#define AZ_IDENTITY_32(...) AZ_IDENTITY_16(AZ_IDENTITY_16(__VA_ARGS__))
#define AZ_IDENTITY_16(...) AZ_IDENTITY_8(AZ_IDENTITY_8(__VA_ARGS__))
#define AZ_IDENTITY_8(...) AZ_IDENTITY_4(AZ_IDENTITY_4(__VA_ARGS__))
#define AZ_IDENTITY_4(...) AZ_IDENTITY_2(AZ_IDENTITY_2(__VA_ARGS__))
#define AZ_IDENTITY_2(...) AZ_IDENTITY(AZ_IDENTITY(__VA_ARGS__))
#define AZ_IDENTITY(...) __VA_ARGS__

// 1. as a plain `EnumTypeName` argument in which case the enum has no underlying type
// 2. surrounded by parenthesis `(EnumTypeName, AZ::u8)` in which case the enum underlying type is the second element
// The underlying type of the enum is the second argument
#define GET_ENUM_OPTION_NAME_1(_name) _name
#define GET_ENUM_OPTION_NAME_2(_name, _initializer) _name
#define GET_ENUM_OPTION_NAME(_name) AZ_MACRO_CALL_INDEX(GET_ENUM_OPTION_NAME_, _name)
#define GET_ENUM_OPTION_INITIALIZER_1(_name) _name,
#define GET_ENUM_OPTION_INITIALIZER_2(_name, _initializer) _name = _initializer,
#define GET_ENUM_OPTION_INITIALIZER(_name) AZ_MACRO_CALL_INDEX(GET_ENUM_OPTION_INITIALIZER_, _name)

#define INIT_ENUM_STRING_PAIR_IMPL(_enumname, _optionname) { _enumname::_optionname, AZ_STRINGIZE(_optionname) },
#define INIT_ENUM_STRING_PAIR(_enumname, _optionname) INIT_ENUM_STRING_PAIR_IMPL(_enumname, GET_ENUM_OPTION_NAME(_optionname))

#define MAKE_REFLECTABLE_ENUM_UNSCOPED(EnumTypeName, ...) MAKE_REFLECTABLE_ENUM_(, EnumTypeName,, __VA_ARGS__)
#define MAKE_REFLECTABLE_ENUM_UNSCOPED_WITH_UNDERLYING_TYPE(EnumTypeName, EnumUnderlyingType, ...) MAKE_REFLECTABLE_ENUM_(, EnumTypeName, : EnumUnderlyingType, __VA_ARGS__)
#define MAKE_REFLECTABLE_ENUM_SCOPED(EnumTypeName, ...) MAKE_REFLECTABLE_ENUM_(class, EnumTypeName,, __VA_ARGS__)
#define MAKE_REFLECTABLE_ENUM_SCOPED_WITH_UNDERLYING_TYPE(EnumTypeName, EnumUnderlyingType, ...)   MAKE_REFLECTABLE_ENUM_(class, EnumTypeName, : EnumUnderlyingType, __VA_ARGS__)
#define MAKE_REFLECTABLE_ENUM_(SCOPE_QUAL, EnumTypeName, EnumUnderlyingType, ...) \
    MAKE_REFLECTABLE_ENUM_ARGS(SCOPE_QUAL, EnumTypeName \
    , EnumUnderlyingType, AZ_VA_NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#define MAKE_REFLECTABLE_ENUM_ARGS(SCOPE_QUAL, EnumTypeName, EnumUnderlyingType, EnumTypeCount, ...) \
inline namespace AZ_JOIN(EnumTypeName, Namespace) \
{ \
    enum SCOPE_QUAL EnumTypeName EnumUnderlyingType \
    { \
        AZ_IDENTITY_128(AZ_FOR_EACH_BIND1ST(AZ_MACRO_CALL_WRAP, GET_ENUM_OPTION_INITIALIZER, __VA_ARGS__)) \
    };\
    inline constexpr size_t AZ_JOIN(EnumTypeName, Count) = EnumTypeCount; \
    struct AZ_JOIN(EnumTypeName, EnumeratorValueAndString) \
    { \
        EnumTypeName m_value; \
        AZStd::string_view m_string; \
    }; \
    constexpr AZStd::array<AZ_JOIN(EnumTypeName, EnumeratorValueAndString), AZ_JOIN(EnumTypeName, Count)> AZ_JOIN(EnumTypeName, Members) = \
    {{ \
        AZ_IDENTITY_128(AZ_FOR_EACH_BIND1ST(INIT_ENUM_STRING_PAIR, EnumTypeName, __VA_ARGS__)) \
    }}; \
    constexpr AZStd::optional<EnumTypeName> FromString(AZStd::string_view stringifiedEnumerator)\
    { \
        auto cbegin = AZ_JOIN(EnumTypeName, Members).cbegin();\
        auto cend = AZ_JOIN(EnumTypeName, Members).cend();\
        auto iterator = AZStd::find_if(cbegin, cend, \
                                       [&](auto&& memberPair)constexpr{ return memberPair.m_string == stringifiedEnumerator; }); \
        return iterator == cend ? AZStd::optional<EnumTypeName>{} : iterator->m_value; \
    } \
    constexpr AZStd::optional<EnumTypeName> AZ_JOIN(FromStringTo, EnumTypeName)(AZStd::string_view stringifiedEnumerator) \
    { \
        return FromString(stringifiedEnumerator); \
    } \
    constexpr AZStd::string_view ToString(EnumTypeName enumerator) \
    { \
        auto cbegin = AZ_JOIN(EnumTypeName, Members).cbegin();\
        auto cend = AZ_JOIN(EnumTypeName, Members).cend();\
        auto iterator = AZStd::find_if(cbegin, cend, \
                                       [&](auto&& memberPair)constexpr{ return memberPair.m_value == enumerator; }); \
        return iterator == cend ? AZStd::string_view{} : iterator->m_string; \
    } \
} \
\
template< typename T > struct AzEnumTraits; \
\
template<> \
struct AzEnumTraits< AZ_JOIN(EnumTypeName, Namespace)::EnumTypeName > \
{ \
    using MembersArrayType                              = decltype( AZ_JOIN(EnumTypeName, Namespace)::AZ_JOIN(EnumTypeName, Members) ); \
    inline static constexpr MembersArrayType& Members   = AZ_JOIN(EnumTypeName, Namespace)::AZ_JOIN(EnumTypeName, Members); \
    inline static constexpr size_t Count                = AZ_JOIN(EnumTypeName, Namespace)::AZ_JOIN(EnumTypeName, Count); \
    inline static constexpr AZStd::string_view EnumName = AZ_STRINGIZE(EnumTypeName); \
}; 

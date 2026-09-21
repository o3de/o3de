/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <string_view>

// preprocessor sadness. would not have to do that if we had BOOST.PP
#define MSVCFIX_EXPAND(x) x
#define AZSLC_FOR_EACH_1(pppredicate,  val, x)      pppredicate(x, val)
#define AZSLC_FOR_EACH_2(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_1 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_3(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_2 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_4(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_3 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_5(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_4 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_6(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_5 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_7(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_6 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_8(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_7 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_9(pppredicate,  val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_8 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_10(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_9 (pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_11(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_10(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_12(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_11(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_13(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_12(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_14(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_13(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_15(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_14(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_16(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_15(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_17(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_16(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_18(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_17(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_19(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_18(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_20(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_19(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_21(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_20(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_22(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_21(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_23(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_22(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_24(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_23(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_25(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_24(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_26(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_25(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_27(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_26(pppredicate, val + 1, __VA_ARGS__))
#define AZSLC_FOR_EACH_28(pppredicate, val, x, ...) pppredicate(x, val) MSVCFIX_EXPAND(AZSLC_FOR_EACH_27(pppredicate, val + 1, __VA_ARGS__))
#define AZ_FIRST_ARG_(N, ...) N
#define AZ_FIRST_ARG(args) AZ_FIRST_ARG_ args  // hack needed for MSVC bug
// Laurent Deniau's method
#define AZSLC_FOR_EACH_NARG(...) AZSLC_FOR_EACH_NARG_(__VA_ARGS__, AZSLC_FOR_EACH_RSEQ_N())
#define AZSLC_FOR_EACH_NARG_(...) MSVCFIX_EXPAND(AZSLC_FOR_EACH_ARG_N(__VA_ARGS__))
#define AZSLC_FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, N, ...) N
#define AZSLC_FOR_EACH_RSEQ_N() 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define AZ_CONCATENATE(x,y) x##y
#define AZSLC_FOR_EACH_(N, pppredicate, val, ...) MSVCFIX_EXPAND(AZ_CONCATENATE(AZSLC_FOR_EACH_, N)(pppredicate, val, __VA_ARGS__))
// Final API:
// use as such:  AZSLC_FOR_EACH( LAMBDAMACRO_1ARG, LIST_OF_PARAMETERS* )
#define AZSLC_FOR_EACH(pppredicate, ...)\
    AZSLC_FOR_EACH_(AZSLC_FOR_EACH_NARG(__VA_ARGS__), pppredicate, 0, __VA_ARGS__)


#define GEN_ONE_ENUMERATOR_LINE(X, val)          X,
#define GEN_ONE_ENUMERATOR_LINE_INIT(X, val)     X = val,
#define GEN_ONE_ENUMERATOR_LINE_INIT_PWR(X, val) X = 0x00000001 << (val),

#define GEN_ONE_CASE(X, val) case X: return #X;

#define GEN_TO_STRING_FUNCTION(...)\
static constexpr string_view ToStr(EnumType enumValue)\
{\
  switch (enumValue)\
  {\
    AZSLC_FOR_EACH( GEN_ONE_CASE, __VA_ARGS__ )\
    default: break;\
  }\
  return "<error>";\
}

#define GEN_ONE_IF_STR_EQ(X, val) if (str == #X) return X;

#define GEN_FROM_STRING_FUNCTION(...)\
static constexpr EnumType FromStr(string_view str)\
{\
  AZSLC_FOR_EACH( GEN_ONE_IF_STR_EQ, __VA_ARGS__ )\
  return EndEnumeratorSentinel_;\
}

// public API here
#define MAKE_REFLECTABLE_ENUM_(EnumTypeName, EnumeratorInit, EnumeratorNextOp, ...)\
struct EnumTypeName\
{\
    /** the real internal enum type */\
    enum EnumType\
    {\
        AZSLC_FOR_EACH( EnumeratorInit, __VA_ARGS__ )\
        EnumeratorInit(EndEnumeratorSentinel_, AZSLC_FOR_EACH_NARG(__VA_ARGS__))\
    };\
    \
    template <auto... Values> struct MetaVals{};\
    /** meta programming access of enumerators in the form of a template value list */\
    using MetaValueList = MetaVals<\
        AZSLC_FOR_EACH( GEN_ONE_ENUMERATOR_LINE, __VA_ARGS__ )\
        EndEnumeratorSentinel_\
    >;\
    /** construction by default and by implicit conversion from the internal enum type */\
    EnumTypeName() = default;\
    EnumTypeName(EnumType val_) : m_value{val_} {}\
    /** stringifiers/serializer */\
    GEN_TO_STRING_FUNCTION(__VA_ARGS__)\
    GEN_FROM_STRING_FUNCTION(__VA_ARGS__)\
    /** currently held enumerator value. defaults constructs to end */\
    EnumType m_value = EndEnumeratorSentinel_;\
    /** conversion operator to avoid to have to leak .m_value everywhere */\
    operator EnumType() const { return m_value; }\
    struct Iterator\
    {\
        EnumType m_i;\
        using iterator_category = std::forward_iterator_tag;\
        using difference_type   = int;\
        using value_type        = EnumType;\
        using reference         = EnumType&;\
        constexpr EnumType operator*() { return m_i; }\
        constexpr Iterator operator++() { m_i = static_cast<EnumType>(EnumeratorNextOp); return {m_i}; }\
        constexpr bool operator!=(const Iterator& rhs) const { return m_i != rhs.m_i; }\
    };\
    /** sub-type to make it clear on client sites */ \
    struct Enumerate\
    {\
        /** ranges API to list enumerators */ \
        constexpr Iterator begin() const { return Iterator{EnumType( AZ_FIRST_ARG((__VA_ARGS__)) )}; }\
        constexpr Iterator end()   const { return Iterator{EndEnumeratorSentinel_}; }\
    };\
    /** query: "is current value any of these ... ?" */\
    bool IsOneOf(EnumType rhs) const { return m_value == rhs; }\
    /** query: "is current value any of these ... ?" */\
    template <typename... Args> bool IsOneOf(EnumType rhsHead, Args... packTail) const\
    {\
        return m_value == rhsHead || IsOneOf(packTail...);\
    }\
    /** direct value checks: avoid to need to access .m_value (to look/behave like a native enum) */\
    friend bool operator==(EnumTypeName lhs,           EnumTypeName::EnumType rhs) { return lhs.m_value == rhs;         }\
    friend bool operator==(EnumTypeName::EnumType lhs, EnumTypeName rhs          ) { return lhs         == rhs.m_value; }\
    friend bool operator==(EnumTypeName lhs,           EnumTypeName rhs          ) { return lhs.m_value == rhs.m_value; }\
    friend bool operator!=(EnumTypeName lhs,           EnumTypeName::EnumType rhs) { return lhs.m_value != rhs;         }\
    friend bool operator!=(EnumTypeName::EnumType lhs, EnumTypeName rhs          ) { return lhs         != rhs.m_value; }\
    friend bool operator!=(EnumTypeName lhs,           EnumTypeName rhs          ) { return lhs.m_value != rhs.m_value; }\
}

#define MAKE_REFLECTABLE_ENUM(EnumTypeName, ...)          MAKE_REFLECTABLE_ENUM_(EnumTypeName, GEN_ONE_ENUMERATOR_LINE_INIT, m_i + 1, __VA_ARGS__)
#define MAKE_REFLECTABLE_ENUM_POWER(EnumTypeName, ...)    MAKE_REFLECTABLE_ENUM_(EnumTypeName, GEN_ONE_ENUMERATOR_LINE_INIT_PWR, (m_i == 0 ? 1 : m_i << 1), __VA_ARGS__)

// can't use an "enum class" because scopes are not flexible (can't `using`).
//  free functions becomes un-implementable because of the need for a "closure" predicate in az_foreach

// test:
#ifndef NDEBUG
#include <cassert>

namespace AZ::Tests
{
    MAKE_REFLECTABLE_ENUM(MyEnum,
        Enumerand1, Enumerand2, Enumerand3);

    static_assert(MyEnum::Enumerand3 == 2);
    static_assert(MyEnum::ToStr(MyEnum::Enumerand3) == "Enumerand3");
    static_assert(MyEnum::FromStr("Enumerand3") == MyEnum::Enumerand3);

    // POWER version
    MAKE_REFLECTABLE_ENUM_POWER(MyEnumFlaggable,
        Flag1, Flag2, Flag3);

    // test executed:
    inline void DoAsserts4()
    {
        int i = 0;
        for (auto e : MyEnum::Enumerate{})
        {
            if (i == 0) assert(e == MyEnum::Enumerand1);
            if (i == 1) assert(e == MyEnum::Enumerand2);
            if (i == 2) assert(e == MyEnum::Enumerand3);
            ++i;
        }

        MyEnum e;
        e = MyEnum::Enumerand2;

        assert(e.IsOneOf(MyEnum::Enumerand1, MyEnum::Enumerand2));
        assert(!e.IsOneOf(MyEnum::Enumerand3));
        assert(!e.IsOneOf(MyEnum::Enumerand1, MyEnum::Enumerand3));

        i = 0;
        for (auto e : MyEnumFlaggable::Enumerate{})
        {
            assert(e == 1 << i);
            ++i;
        }

        static_assert(MyEnumFlaggable::ToStr(MyEnumFlaggable::Flag1) == "Flag1");
        static_assert(MyEnumFlaggable::FromStr("Flag2") == MyEnumFlaggable::Flag2);

    } // builds cleanly with gcc 8 --pedantic, clang 7 --pedantic, msvc 2017 /permissive- /Wall
}
#endif
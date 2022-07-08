/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <AzCore/variadic.h>


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
    struct AZ_JOIN(EnumTypeName, EnumTraits)                                                                                               \
    {                                                                                                                                      \
        using MembersArrayType = decltype(AZ_JOIN(EnumTypeName, Namespace)::AZ_JOIN(EnumTypeName, Members));                               \
        inline static constexpr MembersArrayType& Members = AZ_JOIN(EnumTypeName, Namespace)::AZ_JOIN(EnumTypeName, Members);              \
        inline static constexpr size_t Count = AZ_JOIN(EnumTypeName, Namespace)::AZ_JOIN(EnumTypeName, Count);                             \
        inline static constexpr AZStd::string_view EnumName = AZ_STRINGIZE(EnumTypeName);                                                  \
        /* Visitor must accept a type convertible from EnumTypeName as the first parameter and an AZStd::string_view as the second */      \
        template<class Visitor>                                                                                                            \
        static constexpr void Visit(Visitor&& enumVisitor)                                                                                 \
        {                                                                                                                                  \
            for (const auto& member : Members)                                                                                             \
            {                                                                                                                              \
                enumVisitor(member.m_value, member.m_string);                                                                              \
            }                                                                                                                              \
        }                                                                                                                                  \
    };                                                                                                                                     \
                                                                                                                                           \
    inline auto GetAzEnumTraits(AZ_JOIN(EnumTypeName, Namespace)::EnumTypeName)                                                            \
    {                                                                                                                                      \
        return AZ_JOIN(EnumTypeName, EnumTraits){};                                                                                        \
    };

namespace AZ::Internal
{
    template <class T, class = void>
    inline constexpr bool HasGetAzEnumTraits = false;
    template <class T>
    inline constexpr bool HasGetAzEnumTraits<T, AZStd::enable_if_t<!AZStd::is_void_v<decltype(GetAzEnumTraits(AZStd::declval<T>()))>>> = true;

    // Deleted function to provide the AZ::Internal namespace with the GetAzEnumTraits symbol
    // This is to make an overload which the AzEnumTraitsImpl would use with its
    // template parameter to use Argument Dependent Lookup to find the GetAzEnumTraits
    // function for the Enum Type in the correct namespace.
    template<typename T>
    void GetAzEnumTraits(T&&) = delete;

    template <class T, class = void>
    struct GetAzEnumTraits_Impl {};
    template <class T>
    struct GetAzEnumTraits_Impl<T, AZStd::enable_if_t<HasGetAzEnumTraits<T>>>
    {
        // The AZ_ENUM macros contain a function which returns a struct
        // with the enum traits within it
        // We use decltype here to get the type of that function
        using type = decltype(GetAzEnumTraits(AZStd::declval<T>()));
    };

    template<class T, class = void>
    inline constexpr bool HasAzEnumTraitsImpl = false;

    template<class T>
    inline constexpr bool HasAzEnumTraitsImpl<T, AZStd::enable_if_t<!AZStd::is_void_v<typename GetAzEnumTraits_Impl<T>::type>>> = true;
} // namespace AZ::Internal

namespace AZ
{
    template<class T>
    using AzEnumTraits = typename Internal::GetAzEnumTraits_Impl<AZStd::remove_cvref_t<T>>::type;

    template<class T>
    using HasAzEnumTraits = AZStd::bool_constant<Internal::HasAzEnumTraitsImpl<AZStd::remove_cvref_t<T>>>;

    template<class T>
    /*concept*/ inline constexpr bool HasAzEnumTraits_v = HasAzEnumTraits<T>::value;
}

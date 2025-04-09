/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>

 /**
 * AzGenericTypeInfo allows retrieving the Uuid for templates that are registered for type info, such as
 * AzGenericTypeInfo::Uuid<AZStd::pair>();
 */
 // Due to the various non-types that templates require, it's not possible to wrap these functions in a structure. There
 // also needs to be an overload for every version because there isn't a way in C++ to accept a template that takes
 // accepts a variadic number of arguments, that can bind type, non-type or template parameters in a single signature.
 // i.e there is no such signature such as `template<template T> void Func();`
 // A template parameter must specify an argument list
namespace AZ::AzGenericTypeInfo::Internal
{
    // TemplateIdentityX is a set of placeholder types that is used to specialize GetO3deTemplateId
    // function for the following templates which accept the following parameters
    template<template<class...> class T>
    struct TemplateIdentityTypes {};

    template<template<auto> class T>
    struct TemplateIdentityAuto {};

    template<template<auto, auto> class T>
    struct TemplateIdentityAutoAuto {};

    template<template<class, class, auto> class T>
    struct TemplateIdentityTypeTypeAuto {};

    template<template<class, class, class, auto> class T>
    struct TemplateIdentityTypeTypeTypeAuto {};

    template<template<class, auto> class T>
    struct TemplateIdentityTypeAuto {};
    template<template<class, auto, class> class T>
    struct TemplateIdentityTypeAutoType {};

    // GetTemplateIdentity is used to map to the TemplateIdentityX
    // type the use of decltype for an explicit template calls
    // i.e `decltype(GetTemplateIdentity<AZStd::vector>())` = TemplateIdentity1
    // i.e `decltype(GetTemplateIdentity<AZStd::bitset>())` = TemplateIdentity2
    template<template<class...> class T>
    auto GetTemplateIdentity() -> TemplateIdentityTypes<T>;

    template<template<auto> class T>
    auto GetTemplateIdentity() -> TemplateIdentityAuto<T>;

    template<template<auto, auto> class T>
    auto GetTemplateIdentity() -> TemplateIdentityAutoAuto<T>;

    template<template<class, class, auto> class T>
    auto GetTemplateIdentity() -> TemplateIdentityTypeTypeAuto<T>;

    template<template<class, class, class, auto> class T>
    auto GetTemplateIdentity() -> TemplateIdentityTypeTypeTypeAuto<T>;

    //! NOTE: Avoid an MSVC issue
    //! which causes ambiguity between passing a template template paramter
    //! which has a default parameter
    //! https://godbolt.org/z/bYEfo7jch
    //!
    //! For example a template TT,with parameters `<class, size_t, class = default>`
    //! Can bind to both a `template<template<class, size_t> class>`
    //! and a `template<template<class, size_t, class> class>`
    //! MSVC has the both overloaded functions as ambiguous
    //!
    //! One particular case is AZStd::basic_fixed_string
    //! It is template parameters are
    //! `<class Element, size_t MaxElementCount, class Traits = char_traits<Element>>`
    //!
    //! The way to avoid the is to make sure that none of the template template parameters
    //! are prefixes of each other.
    //! i.e `template<template<class> class>` is a prefix of `template<template<class, size_t> class>`
    //! However variadic template parameters have a lower binding level, so it OK to have overloads
    //! with the following two template parameters
    //! `template<template<class...> class>` and `template<template<class, size_t> class>`

    // MSVC 14.34 has fixed the issue with specifying a variadic pack after auto,
    // not being able to bind non-type arguments to the auto parameter
    // As MSVC 14.33 is still supported, GetTemplateIdentity has been updated
    // to have overload for an <class, auto> and a <class, auto, class> overload
    // which leads to another MSVC defect ...
    // Where if a class template with a default parameter
    // is supplied as a template argument of a function overload, that accepts both the
    // class template without the default and the class template with the default parameter
    // the compilation will cause an ambiguity error
    // https://godbolt.org/z/cK1G99KGr
    //
    // The following is workaround for this issue and has been prototyped at https://godbolt.org/z/463Toox9r
    template<template<class, auto> class T, class = void>
    inline constexpr bool ExactClassAutoMatch = true;

#if defined(AZ_COMPILER_MSVC)
    template<template<class, auto, class> class T>
    using ClassAutoHasDefaultParam = void;
    template<template<class, auto> class T>
    inline constexpr bool ExactClassAutoMatch<T, decltype(ClassAutoHasDefaultParam<T>())> = false;
#endif

    template<template<class, auto> class T>
    auto GetTemplateIdentity() ->
        AZStd::enable_if_t<Internal::ExactClassAutoMatch<T>, TemplateIdentityTypeAuto<T>>;
    template<template<class, auto, class> class T>
    auto GetTemplateIdentity() -> TemplateIdentityTypeAutoType<T>;
}

namespace AZ::AzGenericTypeInfo
{
    // User facing
    template<class T>
    inline constexpr AZ::TypeId Uuid()
    {
        return AzTypeInfo<T>::GetTemplateId();
    }

    template<template<class...> class T>
    constexpr AZ::TypeId Uuid()
    {
        using Internal::GetTemplateIdentity;
        using TemplateIdentity = decltype(GetTemplateIdentity<T>());
        return GetO3deTemplateId(AZ::Adl{}, TemplateIdentity{});
    }
    template<template<auto> class T>
    inline constexpr AZ::TypeId Uuid()
    {
        using Internal::GetTemplateIdentity;
        using TemplateIdentity = decltype(GetTemplateIdentity<T>());
        return GetO3deTemplateId(AZ::Adl{}, TemplateIdentity{});
    }

    template<template<auto, auto> class T>
    inline constexpr AZ::TypeId Uuid()
    {
        using Internal::GetTemplateIdentity;
        using TemplateIdentity = decltype(GetTemplateIdentity<T>());
        return GetO3deTemplateId(AZ::Adl{}, TemplateIdentity{});
    }

    template<template<class, class, auto> class T>
    inline constexpr AZ::TypeId Uuid()
    {
        using Internal::GetTemplateIdentity;
        using TemplateIdentity = decltype(GetTemplateIdentity<T>());
        return GetO3deTemplateId(AZ::Adl{}, TemplateIdentity{});
    }

    template<template<class, class, class, auto> class T>
    inline constexpr AZ::TypeId Uuid()
    {
        using Internal::GetTemplateIdentity;
        using TemplateIdentity = decltype(GetTemplateIdentity<T>());
        return GetO3deTemplateId(AZ::Adl{}, TemplateIdentity{});
    }

    template<template<class, auto> class T>
    inline constexpr AZ::TypeId Uuid()
    {
        using Internal::GetTemplateIdentity;
        using TemplateIdentity = decltype(GetTemplateIdentity<T>());
        return GetO3deTemplateId(AZ::Adl{}, TemplateIdentity{});
    }

    template<template<class, auto, class> class T>
    inline constexpr AZ::TypeId Uuid()
    {
        using Internal::GetTemplateIdentity;
        using TemplateIdentity = decltype(GetTemplateIdentity<T>());
        return GetO3deTemplateId(AZ::Adl{}, TemplateIdentity{});
    }
}

#define AZ_TYPE_INFO_INTERNAL_TYPENAME_TYPE typename
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_ARG(A) A
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_UUID(A) AZ::Internal::GetCanonicalTypeId< A >()
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_NAME(A) AZ::Internal::GetTypeName< A >()

#define AZ_TYPE_INFO_INTERNAL_CLASS_TYPE class
#define AZ_TYPE_INFO_INTERNAL_CLASS_ARG(A) A
#define AZ_TYPE_INFO_INTERNAL_CLASS_UUID(A) AZ::Internal::GetCanonicalTypeId< A >()
#define AZ_TYPE_INFO_INTERNAL_CLASS_NAME(A) AZ::Internal::GetTypeName< A >()

#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS_TYPE typename...
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS_ARG(A) A...
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS_UUID(A) AZ::Internal::AggregateTypes< A... >::GetCanonicalTypeId()
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS_NAME(A) AZ::Internal::AggregateTypes< A... >::TypeName()

#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS_TYPE class...
#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS_ARG(A) A...
#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS_UUID(A) AZ::Internal::AggregateTypes< A... >::GetCanonicalTypeId()
#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS_NAME(A) AZ::Internal::AggregateTypes< A... >::TypeName()

#define AZ_TYPE_INFO_INTERNAL_AUTO_TYPE auto
#define AZ_TYPE_INFO_INTERNAL_AUTO_ARG(A) A
#define AZ_TYPE_INFO_INTERNAL_AUTO_UUID(A) AZ::Internal::GetCanonicalTypeId< A >()
#define AZ_TYPE_INFO_INTERNAL_AUTO_NAME(A) AZ::Internal::GetTypeName< A >()


// NOTE: This the same macro logic as the `AZ_MACRO_SPECIALIZE`, but it must used instead of
// `AZ_MACRO_SPECIALIZE` macro as the C preprocessor macro expansion will not expand a macro with the same
// name again during second scan of the macro during expansion
// For example given the following set of macros
// #define AZ_EXPAND_1(X) X
// #define AZ_TYPE_INFO_INTERNAL_1(params) AZ_MACRO_SPECIALIZE(AZ_EXPAND_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))
//
// If the AZ_MACRO_SPEICALIZE is used as follows with a single argument of FOO:
// `AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_, 1, (FOO))`,
// then the inner AZ_EXPAND_1 will NOT expand because the preprocessor suppresses expansion of the `AZ_MACRO_SPECIALIZE`
// with even if it is called with a different set of arguments.
// So the result of the macro is `AZ_MACRO_SPECIALIZE(AZ_EXPAND_, 1, (FOO))`, not `FOO`
// https://godbolt.org/z/GM6f7drh1
//
// See the following thread for information about the C Preprocessor works in this manner:
// https://stackoverflow.com/questions/66593868/understanding-the-behavior-of-cs-preprocessor-when-a-macro-indirectly-expands-i
// Using the name AZ_TYPE_INFO_INTERNAL_MACRO_CALL
#   define AZ_TYPE_INFO_INTERNAL_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS)    MACRO_NAME##NPARAMS PARAMS
#   define AZ_TYPE_INFO_INTERNAL_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS)     AZ_TYPE_INFO_INTERNAL_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS)
#   define AZ_TYPE_INFO_INTERNAL_MACRO_CALL(MACRO_NAME, NPARAMS, PARAMS)       AZ_TYPE_INFO_INTERNAL_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS)

#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_1(_1)                                                                                  AZ_JOIN(_1, _TYPE) T1
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_2(_1, _2)             AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_1(_1),             AZ_JOIN(_2, _TYPE) T2
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3)         AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_2(_1, _2),         AZ_JOIN(_3, _TYPE) T3
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4)     AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3),     AZ_JOIN(_4, _TYPE) T4
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4), AZ_JOIN(_5, _TYPE) T5
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(...) AZ_TYPE_INFO_INTERNAL_MACRO_CALL(AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_1(_1)                                                                                      AZ_JOIN(_1, _ARG)(T1)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2)             AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_1(_1),             AZ_JOIN(_2, _ARG)(T2)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3)         AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2),         AZ_JOIN(_3, _ARG)(T3)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4)     AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3),     AZ_JOIN(_4, _ARG)(T4)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4), AZ_JOIN(_5, _ARG)(T5)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(...) AZ_TYPE_INFO_INTERNAL_MACRO_CALL(AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_1(_1)                                                                                  AZ_JOIN(_1, _NAME)(T1)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_2(_1, _2)             AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_1(_1),             AZ_JOIN(_2, _NAME)(T2)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_3(_1, _2, _3)         AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_2(_1, _2),         AZ_JOIN(_3, _NAME)(T3)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_4(_1, _2, _3, _4)     AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_3(_1, _2, _3),     AZ_JOIN(_4, _NAME)(T4)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_4(_1, _2, _3, _4), AZ_JOIN(_5, _NAME)(T5)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION(...) AZ_TYPE_INFO_INTERNAL_MACRO_CALL(AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_1(_1)                                                                                    AZ_JOIN(_1, _UUID)(T1)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_2(_1, _2)             (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_1(_1) +             AZ_JOIN(_2, _UUID)(T2))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_3(_1, _2, _3)         (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_2(_1, _2) +         AZ_JOIN(_3, _UUID)(T3))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_4(_1, _2, _3, _4)     (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_3(_1, _2, _3) +     AZ_JOIN(_4, _UUID)(T4))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_5(_1, _2, _3, _4, _5) (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_4(_1, _2, _3, _4) + AZ_JOIN(_5, _UUID)(T5))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(...) AZ_TYPE_INFO_INTERNAL_MACRO_CALL(AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))


    //! Helper Macro for adding GetO3deTypeName/GetO3deTypeId/GetO3deClassTemplateId and GetO3deTemplateId overloads for use with AZ::TypeInfo
    //! Supported namespaces where there overloads can be added are the namespaces where the type is declared within
    //! and the AZ namespace. For example the overloads for a class PhysX::WorldSystem, can be placed in either the PhysX
    //! namespace or the AZ namespace

    // Declares but does not define the GetO3de* type info overloads
#define AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID_DECL(_TemplateName) \
    AZ::TemplateId GetO3deTemplateId(AZ::Adl, \
        decltype(AZ::AzGenericTypeInfo::Internal::GetTemplateIdentity<_TemplateName>()));

#define AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID_DECL(_TemplateName, ...) \
    template <AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)> \
    AZ::TypeNameString GetO3deTypeName(AZ::Adl, \
        AZStd::type_identity<_TemplateName<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>); \
    template <AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)> \
    AZ::TypeId GetO3deTypeId(AZ::Adl, \
        AZStd::type_identity<_TemplateName<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>); \
    template <AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)> \
    AZ::TemplateId GetO3deClassTemplateId(AZ::Adl, \
        AZStd::type_identity<_TemplateName<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>);

#define AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID_DECL(_TemplateName, ...) \
    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID_DECL(_TemplateName) \
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID_DECL(_TemplateName, __VA_ARGS__)

#define AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _PrefixUuid, _PostfixUuid, _FunctionSpecifier) \
    _FunctionSpecifier AZ::TemplateId GetO3deTemplateId(AZ::Adl, \
        decltype(AZ::AzGenericTypeInfo::Internal::GetTemplateIdentity<_TemplateName>())) \
    { \
        constexpr AZ::TypeId prefixUuid{_PrefixUuid}; \
        constexpr AZ::TypeId postfixUuid{_PostfixUuid}; \
        if constexpr (!prefixUuid.IsNull()) \
        { \
            return prefixUuid; \
        } \
        else if constexpr (!postfixUuid.IsNull()) \
        { \
            return postfixUuid; \
        } \
        else \
        { \
            return AZ::TemplateId{}; \
        } \
    }

#define AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _PrefixUuid, _PostfixUuid, _Inline, ...) \
    template <AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)> \
    _Inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, \
        AZStd::type_identity<_TemplateName<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>) \
    { \
        AZ::TypeNameString s_canonicalTypeName; \
        if (s_canonicalTypeName.empty()) \
        { \
            AZStd::fixed_string<512> typeName { _DisplayName "<" }; \
            bool prependSeparator = false; \
            for (AZStd::string_view templateParamName : { AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION(__VA_ARGS__) }) \
            { \
                typeName += prependSeparator ? AZ::Internal::TypeNameSeparator : ""; \
                typeName += templateParamName; \
                prependSeparator = true; \
            } \
            typeName += '>'; \
            s_canonicalTypeName = typeName; \
        } \
        return s_canonicalTypeName; \
    } \
    template <AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)> \
    _Inline AZ::TypeId GetO3deTypeId(AZ::Adl, \
        AZStd::type_identity<_TemplateName<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>) \
    { \
        AZ::TypeId s_canonicalTypeId; \
        if (s_canonicalTypeId.IsNull()) \
        { \
            constexpr AZ::TypeId prefixUuid{_PrefixUuid}; \
            constexpr AZ::TypeId postfixUuid{_PostfixUuid}; \
            if constexpr (!prefixUuid.IsNull()) \
            { \
                s_canonicalTypeId = prefixUuid + AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(__VA_ARGS__); \
            } \
            else if constexpr (!postfixUuid.IsNull()) \
            { \
                s_canonicalTypeId = AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(__VA_ARGS__) + postfixUuid; \
            } \
            else \
            { \
                AZ_Assert(false, "Call to macro with template name %s, requires either a valid prefix or postfix uuid", #_DisplayName); \
            } \
        } \
        return s_canonicalTypeId; \
    } \
    template <AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)> \
    _Inline AZ::TemplateId GetO3deClassTemplateId(AZ::Adl, \
        AZStd::type_identity<_TemplateName<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>) \
    { \
        constexpr AZ::TypeId prefixUuid{_PrefixUuid}; \
        constexpr AZ::TypeId postfixUuid{_PostfixUuid}; \
        if constexpr (!prefixUuid.IsNull()) \
        { \
            return prefixUuid; \
        } \
        else if constexpr (!postfixUuid.IsNull()) \
        { \
            return postfixUuid; \
        } \
        else \
        { \
            return AZ::TemplateId{}; \
        } \
    }

#define AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID(_TemplateName, _DisplayName, _PrefixUuid, _PostfixUuid, _Inline, ...) \
    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _PrefixUuid, _PostfixUuid, inline constexpr) \
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _PrefixUuid, _PostfixUuid, _Inline, __VA_ARGS__)

#define AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(_TemplateName, _DisplayName, _TemplateUuid, ...) \
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID(_TemplateName, _DisplayName, _TemplateUuid,,, __VA_ARGS__)

#define AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(_TemplateName, _DisplayName, _TemplateUuid, ...) \
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID(_TemplateName, _DisplayName, , _TemplateUuid,, __VA_ARGS__)


// Adds the static member function of GetO3deTypeName and GetO3deTypeId to the specified class template
// which allows instantiations of the class template to opt-in to AzTypeInfo
// This macro requires the same kind of placeholder arguments as the AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID macro
// eg. AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME(Foo, "Foo", "{XXXXXXX-...}", AZ_TYPE_INFO_CLASS)
#define AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME(_TemplateName, _DisplayName, _TemplateUuid, ...) \
    friend AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<_TemplateName>) \
    { \
        static AZ::TypeNameString s_canonicalTypeName; \
        if (s_canonicalTypeName.empty()) \
        { \
            AZ::TypeNameString typeName{ _DisplayName "<" }; \
            bool prependSeparator = false; \
            for (AZStd::string_view templateParamName : { AZ::Internal::AggregateTypes< __VA_ARGS__ >::TypeName() }) \
            { \
                typeName += prependSeparator ? AZ::Internal::TypeNameSeparator : ""; \
                typeName += templateParamName; \
                prependSeparator = true; \
            } \
            typeName += '>'; \
            s_canonicalTypeName = typeName; \
        } \
        return s_canonicalTypeName; \
    } \
    friend AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<_TemplateName>) \
    { \
        static AZ::TypeId s_canonicalTypeId; \
        if (s_canonicalTypeId.IsNull()) \
        { \
            constexpr AZ::TypeId templateUuid{_TemplateUuid}; \
            if constexpr (!templateUuid.IsNull()) \
            { \
                s_canonicalTypeId = templateUuid + AZ::Internal::AggregateTypes< __VA_ARGS__ >::GetCanonicalTypeId(); \
            } \
            else \
            { \
                AZ_Assert(false, "Call to macro with template name %s, requires either a valid template uuid", #_DisplayName); \
            } \
        } \
        return s_canonicalTypeId; \
    } \
    friend constexpr AZ::TemplateId GetO3deClassTemplateId(AZ::Adl, AZStd::type_identity<_TemplateName>) \
    { \
        constexpr AZ::TypeId templateUuid{_TemplateUuid}; \
        if constexpr (!templateUuid.IsNull()) \
        { \
            return templateUuid; \
        } \
        else \
        { \
            return AZ::TemplateId{}; \
        } \
    } \
    static const char* TYPEINFO_Name() \
    { \
        static const AZ::TypeNameString s_typeName = GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<_TemplateName>{}); \
        return s_typeName.c_str(); \
    } \
    static AZ::TypeId TYPEINFO_Uuid() \
    { \
        return GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<_TemplateName>{}); \
    }

// Allows a Class Template to opt-in to providing an overload for GetO3deTemplateId
// that associates a Uuid with the Class Template itself.
// NOTE: This only associates the Uuid with the Template, not instances of the class instantiated from the template
// i.e It can associated a "template id" with `AZStd::vector`
// It cannot associate a "template id" with `AZStd::vector<T>` as that refers to a class instance
// NOTE: The final argument to AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID is empty for the postfix UUID parameter
#define AZ_TEMPLATE_INFO_WITH_NAME(_TemplateName, _DisplayName, _TemplateUuid) \
    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, #_TemplateName, _TemplateUuid,)

// all template versions are handled by a variadic template handler
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_1 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_2 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_3 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_4 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_5 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_6 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_7 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_8 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_9 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_10 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_11 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_12 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_13 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_14 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_15 AZ_CLASS_TYPE_INFO_INTRUSIVE_TEMPLATE_WITH_NAME

#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1(_TemplateName, _DisplayName, _TemplateUuid, _Inline, _TemplateParamsInParen) \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline AZ::TypeNameString GetO3deTypeName( \
        AZ::Adl, AZStd::type_identity<_TemplateName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen>) \
    { \
        static AZ::TypeNameString s_canonicalTypeName; \
        if (s_canonicalTypeName.empty()) \
        { \
            AZ::TypeNameString typeName{ _DisplayName "<" }; \
            bool prependSeparator = false; \
            for (AZStd::string_view templateParamName : { AZ::Internal::AggregateTypes< AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION _TemplateParamsInParen >::TypeName() }) \
            { \
                typeName += prependSeparator ? AZ::Internal::TypeNameSeparator : ""; \
                typeName += templateParamName; \
                prependSeparator = true; \
            } \
            typeName += '>'; \
            s_canonicalTypeName = typeName; \
        } \
        return s_canonicalTypeName; \
    } \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline AZ::TypeId GetO3deTypeId( \
        AZ::Adl, AZStd::type_identity<_TemplateName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen>) \
    { \
        static AZ::TypeId s_canonicalTypeId; \
        if (s_canonicalTypeId.IsNull()) \
        { \
            constexpr AZ::TypeId templateUuid{_TemplateUuid}; \
            if constexpr (!templateUuid.IsNull()) \
            { \
                s_canonicalTypeId = templateUuid + AZ::Internal::AggregateTypes< AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION _TemplateParamsInParen >::GetCanonicalTypeId(); \
            } \
            else \
            { \
                AZ_Assert(false, "Call to macro with template name %s, requires either a valid template uuid", #_DisplayName); \
            } \
        } \
        return s_canonicalTypeId; \
    } \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline AZ::TypeId GetO3deClassTemplateId( \
        AZ::Adl, AZStd::type_identity<_TemplateName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen>) \
    { \
        constexpr AZ::TypeId templateUuid{_TemplateUuid}; \
        if constexpr (!templateUuid.IsNull()) \
        { \
            return templateUuid; \
        } \
        else \
        { \
            return AZ::TemplateId{}; \
        } \
    } \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline const char* _TemplateName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen ::TYPEINFO_Name() \
    { \
        static const AZ::TypeNameString s_typeName = GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<_TemplateName>{}); \
        return s_typeName.c_str(); \
    } \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline AZ::TypeId _TemplateName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen ::TYPEINFO_Uuid() \
    { \
        return GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<_TemplateName>{}); \
    }

#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_2 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_3 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_4 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_5 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_6 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_7 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_8 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_9 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_10 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_11 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_12 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_13 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_14 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_15 AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_1

// Used to declare that a template argument is a "typename" with AZ_TYPE_INFO_TEMPLATE.
#define AZ_TYPE_INFO_TYPENAME AZ_TYPE_INFO_INTERNAL_TYPENAME
// Used to declare that a template argument is a "typename..." with AZ_TYPE_INFO_TEMPLATE.
#define AZ_TYPE_INFO_TYPENAME_VARARGS AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS
// Used to declare that a template argument is a "class" with AZ_TYPE_INFO_TEMPLATE.
#define AZ_TYPE_INFO_CLASS AZ_TYPE_INFO_INTERNAL_CLASS
// Used to declare that a template argument is a "class..." with AZ_TYPE_INFO_TEMPLATE.
#define AZ_TYPE_INFO_CLASS_VARARGS AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS
// Used to declare that a template argument is a number such as size_t with AZ_TYPE_INFO_TEMPLATE.
#define AZ_TYPE_INFO_AUTO AZ_TYPE_INFO_INTERNAL_AUTO

/**
* The same as AZ_TYPE_INFO_TEMPLATE, but allows the name to explicitly set. This was added for backwards compatibility.
*/
#define AZ_TYPE_INFO_TEMPLATE_WITH_NAME(_Template, _Name, _Uuid, ...) \
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(_Template, _Name, _Uuid, __VA_ARGS__)
/**
* Use this macro outside a template class to allow it to be identified across modules and serialized (in different contexts).
* The expected inputs are:
*   - The template
*   - The assigned uuid as a string or an instance of a uuid
*   - One or more template arguments.
* Note that the AZ_TYPE_INFO_TEMPLATE always has be declared in "namespace AZ".
* Example:
*   template<typename T1, size_t N, class T2>
*   class MyTemplateClass
*   {
*   public:
*       ...
*   };
*
*   AZ_TYPE_INFO_TEMPLATE(MyTemplateClass, "{6BE82E8C-BC3D-4DA3-835E-644864A56405}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_AUTO, AZ_TYPE_INFO_CLASS);
*/
#define AZ_TYPE_INFO_TEMPLATE(_Template, _Uuid, ...) AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(_Template, #_Template, _Uuid, __VA_ARGS__)

/**
* Provides declarations for type info overloads that support class templates
* Does not add the definitions for those overloads
* This pairs with AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL and AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL_INLINE to define the body
* @param _TemplateName name of the Class template
* @params __VA_ARGS__ list of AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_AUTO or AZ_TYPE_INFO_CLASS_VARARGS placeholders
*         to substitute for the template arguments
*/
#define AZ_TYPE_INFO_TEMPLATE_WITH_NAME_DECL(_TemplateName, ...) \
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID_DECL(_TemplateName, __VA_ARGS__)

/**
* Provides definitions for type info overloads that support class templates
* Suitable for use in a translation unit(.cpp) with a regular class(class AZ::Entity)
* or an explicit class template instantiations(template class AZStd::vector<int>)
* This pairs with the AZ_TYPE_INFO_TEMPLATE_WITH_NAME_DECL
* @param _TemplateName name of the Class template
* @params __VA_ARGS__ list of AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_AUTO or AZ_TYPE_INFO_CLASS_VARARGS placeholders
*         to substitute for the template arguments
* NOTE: The GetO3deTemplateId function has the inline specifier so that this macro can be used in multiple
*       translation units when working with a template specialization
*       As GetO3deTypeName/GetO3deTypeId/GetO3deClassTemplateId is templated it would have a different signature
*       for each specialization, while GetO3deTemplate shares the same function signature
*/
#define AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL(_TemplateName, _DisplayName, _TemplateUuid, ...) \
    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _TemplateUuid,,inline) \
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _TemplateUuid,,, __VA_ARGS__)

/**
* Version of AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL which prefixes functions with the inline specifier
* This can be used add definitions of the TypeInfo overloads in header or inline file
*/
#define AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL_INLINE(_TemplateName, _DisplayName, _TemplateUuid, ...) \
    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _TemplateUuid,, inline) \
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(_TemplateName, _DisplayName, _TemplateUuid,, inline, __VA_ARGS__)

// Helper macro for explicitly instantiating the GetO3deTypeName, GetO3deTypeId and GetO3deClassTemplateId functions
// for a full template specialization
// @param TemplateName the name of the template to add an explicit instantiation for
// @params __VA_ARGS__ The actual types that are being explicitly instantiated(i.e char, AZ::Entity, etc..)
// Any arguments that contains commas in them can be wrapped in parentheses.
// Any wrapped arguments are unwrapped as single argument
// ex. Template with zero arguments that contains commas
// namespace AZStd
// {
//     AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(char_traits, char);
// }
// ex. Template with an argument that contains commas
// namespace AZStd
// {
//     AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(vector, (pair<char, char>), allocator);
// }
#define AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(TemplateName, ...) \
        template AZ::TypeNameString GetO3deTypeName( \
            AZ::Adl, AZStd::type_identity<TemplateName<AZ_FOR_EACH_WITH_SEPARATOR(AZ_UNWRAP, AZ_COMMA_SEPARATOR, __VA_ARGS__)>>); \
        template AZ::TypeId GetO3deTypeId( \
            AZ::Adl, AZStd::type_identity<TemplateName<AZ_FOR_EACH_WITH_SEPARATOR(AZ_UNWRAP, AZ_COMMA_SEPARATOR, __VA_ARGS__)>>); \
        template AZ::TemplateId GetO3deClassTemplateId( \
            AZ::Adl, AZStd::type_identity<TemplateName<AZ_FOR_EACH_WITH_SEPARATOR(AZ_UNWRAP, AZ_COMMA_SEPARATOR, __VA_ARGS__)>>);

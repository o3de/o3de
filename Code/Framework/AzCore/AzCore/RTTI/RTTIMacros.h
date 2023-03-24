/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/typetraits/has_member_function.h>

namespace AZ
{
    struct Uuid;
    using TypeId = Uuid;

    /// RTTI Visitor
    using RTTI_EnumCallback = void (*)(const AZ::TypeId& /*typeId*/, void* /*userData*/);

    template<class T>
    inline constexpr bool HasAZRttiExternal_v = false;

    enum class RttiKind
    {
        None,
        Intrusive,
        External
    };

    // Check if a class has Rtti support - HasAZRtti<ClassType>....
    AZ_HAS_MEMBER_QUAL(AZRttiIntrusive, RTTI_GetType, AZ::TypeId, (), const);

    template<class T>
    struct HasAZRtti
        : AZStd::bool_constant<HasAZRttiExternal_v<T> || HasAZRttiIntrusive_v<T>>
    {
        using kind_type = AZStd::integral_constant<RttiKind, HasAZRttiIntrusive<T>::value ? RttiKind::Intrusive : (HasAZRttiExternal_v<T> ? RttiKind::External : RttiKind::None)>;
    };

    template<class T>
    inline constexpr bool HasAZRtti_v = HasAZRtti<T>::value;
} // namespace AZ

/**
 * AZ RTTI is a lightweight user defined RTTI system that
 * allows dynamic_cast like functionality on your object.
 * As C++ rtti, azrtti is implemented using the virtual table
 * so it will work only on polymorphic types.
 * Unlike C++ rtti, it will be assigned to the classes manually,
 * so you keep your memory footprint in check. In addition
 * azdynamic_cast is generally faster, and offers a few extensions
 * that are used by the serialization, script, etc. systems.
 * Limitations: As we add RTTI manually, we can NOT cast from
 * a void pointer. Because of this we can't catch unrelated pointer
 * cast at compile time.
 */
#define azdynamic_cast  AZ::RttiCast
#define azrtti_cast     AZ::RttiCast
#define azrtti_istypeof AZ::RttiIsTypeOf
/// For instances azrtti_typeid(instance) or azrtti_typeid<Type>() for types
#define azrtti_typeid   AZ::RttiTypeId


// Require AZ_TYPE_INFO to be declared to be declared for the type

#define AZ_RTTI_TEMPLATE_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
#define AZ_RTTI_TEMPLATE_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_TEMPLATE_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS)
#define AZ_RTTI_TEMPLATE_MACRO_CALL(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_TEMPLATE_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS)

// Expands template placeholder arguments for substituting template parameters
// and wraps it in `template< ... > ` suitable for use as a simple template id
// http://eel.is/c++draft/temp.names#nt:simple-template-id
// ex. A single template placeholder argument for type parameter
// AZ_RTTI_SIMPLE_TEMPLATE_ID(AZ_TYPE_INFO_CLASS) -> template<class T1>
// ex. A single template placeholder argument for non-type parameter
// AZ_RTTI_SIMPLE_TEMPLATE_ID(AZ_TYPE_INFO_CLASS) -> template<auto T1>
// ex. Multiple template placeholder argumetns
// AZ_RTTI_SIMPLE_TEMPLATE_ID(AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_AUTO) -> template<class T1, auto T2>
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_0(...)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_1(...) template< AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__) >
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_2(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_3(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_4(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_5(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_6(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_7(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_8(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_9(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_10(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_11(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_12(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_13(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_14(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID_15(...) AZ_RTTI_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_RTTI_SIMPLE_TEMPLATE_ID(...) AZ_RTTI_TEMPLATE_MACRO_CALL(AZ_RTTI_SIMPLE_TEMPLATE_ID_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

// Expands template placeholder arguments for substituting template arguments
// and wraps it an `< ... >` suitable for use as a template argument list for a class template
// http://eel.is/c++draft/temp.names#nt:template-argument-list
// ex. A single template placeholder argument for type parameter
// AZ_RTTI_TEMPLATE_ARGUMENT_LIST(AZ_TYPE_INFO_CLASS) -> <T1>
// ex. A single template placeholder argument for non-type parameter
// AZ_RTTI_TEMPLATE_ARGUMENT_LIST(AZ_TYPE_INFO_CLASS) -> <T1>
// ex. Multiple template placeholder argumetns
// AZ_RTTI_TEMPLATE_ARGUMENT_LIST(AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_AUTO) -> <T1, T2>
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_0(...)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(...) < AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__) >
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_2(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_3(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_4(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_5(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_6(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_7(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_8(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_9(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_10(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_11(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_12(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_13(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_14(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST_15(...) AZ_RTTI_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_RTTI_TEMPLATE_ARGUMENT_LIST(...) AZ_RTTI_TEMPLATE_MACRO_CALL(AZ_RTTI_TEMPLATE_ARGUMENT_LIST_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

// The virtual functions are forward declared and the static functions are defined inline
// Even the constexpr functions are only defined at the implementation location
// Due to this, the Base Classes do not need to be specifed to the this macro
#define AZ_RTTI_NO_TYPE_INFO_DECL_() \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    virtual AZ::TypeId RTTI_GetType() const; \
    virtual const char* RTTI_GetTypeName() const; \
    virtual bool RTTI_IsTypeOf(const AZ::TypeId & typeId) const; \
    virtual void RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData); \
    virtual const void* RTTI_AddressOf(const AZ::TypeId& id) const;\
    virtual void* RTTI_AddressOf(const AZ::TypeId& id);\
AZ_POP_DISABLE_WARNING \
    static bool RTTI_IsContainType(const AZ::TypeId& id); \
    static void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData); \
    static AZ::TypeId RTTI_Type(); \
    static const char* RTTI_TypeName();

// Common implementation functions for any class with RTTI
// The virtual functions are defined here
#define AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ AZ::TypeId ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_GetType() const \
    { return RTTI_Type(); } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const char* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_GetTypeName() const \
    { return RTTI_TypeName(); } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_IsTypeOf(const AZ::TypeId& typeId) const \
    { return RTTI_IsContainType(typeId); } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData) \
    { return RTTI_EnumHierarchy(cb, userData); } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ AZ::TypeId ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_Type() \
    { \
        return GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen>{}); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const char* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_TypeName() \
    { \
        static const AZ::TypeNameString s_typeName = GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen>{}); \
        return s_typeName.c_str(); \
    }


#define AZ_RTTI_COMMON(ClassName_, TemplateParamsInParen)                                                                      \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                                              \
virtual inline        AZ::TypeId RTTI_GetType() const { return RTTI_Type(); }                                                  \
virtual inline const char* RTTI_GetTypeName() const { return RTTI_TypeName(); }                                                \
virtual inline bool             RTTI_IsTypeOf(const AZ::TypeId & typeId) const { return RTTI_IsContainType(typeId); }          \
virtual void                    RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData) { RTTI_EnumHierarchy(cb, userData); } \
static inline AZ::TypeId RTTI_Type() \
{ \
    return TYPEINFO_Uuid(); \
} \
static inline const char* RTTI_TypeName() \
{ \
    return TYPEINFO_Name(); \
} \
AZ_POP_DISABLE_WARNING

/// AZ_RTTI()
#define AZ_RTTI_0(ClassName_, TemplateParamsInParen)             AZ_RTTI_COMMON(ClassName_, TemplateParamsInParen)                \
static bool                 RTTI_IsContainType(const AZ::TypeId& id) { return id == RTTI_Type(); }                      \
static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { cb(RTTI_Type(), userData); } \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                                                 \
virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const { return (id == RTTI_Type()) ? this : nullptr; }           \
virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) { return (id == RTTI_Type()) ? this : nullptr; }                 \
AZ_POP_DISABLE_WARNING

//! Function definitions to add RTTI function implementation for a class with no base classes
#define AZ_RTTI_NO_TYPE_INFO_IMPL_0(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) const { return id == RTTI_Type() ? this : nullptr; } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) { return id == RTTI_Type() ? this : nullptr; } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_IsContainType(const AZ::TypeId& id) { return id == RTTI_Type(); } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { cb(RTTI_Type(), userData); }

/// AZ_RTTI(BaseClass)
#define AZ_RTTI_1(ClassName_, TemplateParamsInParen, _1)           AZ_RTTI_COMMON(ClassName_, TemplateParamsInParen) \
static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                     \
    if (id == RTTI_Type()) { return true; }                                                \
    return AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id); }                         \
static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { \
    cb(RTTI_Type(), userData);                                                             \
    AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData); }                      \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                          \
virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                   \
    if (id == RTTI_Type()) { return this; }                                                \
    return AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); }                       \
virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                         \
    if (id == RTTI_Type()) { return this; }                                                \
    return AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); }                       \
AZ_POP_DISABLE_WARNING

//! Function definitions to add RTTI function implementation for a class with base class(es)
#define AZ_RTTI_NO_TYPE_INFO_IMPL_1(ClassName_, TemplateParamsInParen, Inline_, _1) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) const \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        if (id == RTTI_Type()) { return this; } \
        return AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        if (id == RTTI_Type()) { return this; } \
        return AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_IsContainType(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        return id == RTTI_Type() || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        cb(RTTI_Type(), userData); \
        AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
    }

/// AZ_RTTI(BaseClass1,BaseClass2)
#define AZ_RTTI_2(ClassName_, TemplateParamsInParen, _1, _2)                                     \
    AZ_RTTI_COMMON(ClassName_, TemplateParamsInParen)                                            \
static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
    if (id == RTTI_Type()) { return true; }                                                      \
    if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
    return AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id); }                               \
static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
    cb(RTTI_Type(), userData);                                                                   \
    AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData); }                            \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                \
virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
    if (id == RTTI_Type()) { return this; }                                                      \
    const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
    return AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); }                             \
virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
    if (id == RTTI_Type()) { return this; }                                                      \
    void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
    return AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); }                             \
AZ_POP_DISABLE_WARNING

#define AZ_RTTI_NO_TYPE_INFO_IMPL_2(ClassName_, TemplateParamsInParen, Inline_, _1, _2) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) const \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        if (id == RTTI_Type()) { return this; } \
        const auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        if (id == RTTI_Type()) { return this; } \
        auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_IsContainType(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        return id == RTTI_Type() || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType2>::RTTI_IsContainType(id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        cb(RTTI_Type(), userData); \
        AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType2>::RTTI_EnumHierarchy(cb, userData); \
    }

/// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3)
#define AZ_RTTI_3(ClassName_, TemplateParamsInParen, _1, _2, _3)                                 \
    AZ_RTTI_COMMON(ClassName_, TemplateParamsInParen)                                             \
static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
    if (id == RTTI_Type()) { return true; }                                                      \
    if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
    if (AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id)) { return true; }                   \
    return AZ::Internal::RttiCaller<_3>::RTTI_IsContainType(id); }                               \
static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
    cb(RTTI_Type(), userData);                                                                   \
    AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_3>::RTTI_EnumHierarchy(cb, userData); }                            \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                \
virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
    if (id == RTTI_Type()) { return this; }                                                      \
    const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
    r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    return AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); }                             \
virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
    if (id == RTTI_Type()) { return this; }                                                      \
    void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
    r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    return AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); }                             \
AZ_POP_DISABLE_WARNING

#define AZ_RTTI_NO_TYPE_INFO_IMPL_3(ClassName_, TemplateParamsInParen, Inline_, _1, _2, _3) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) const \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        if (id == RTTI_Type()) { return this; } \
        const auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        if (id == RTTI_Type()) { return this; } \
        auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_IsContainType(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        return id == RTTI_Type() \
            || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType2>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType3>::RTTI_IsContainType(id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        cb(RTTI_Type(), userData); \
        AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType2>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType3>::RTTI_EnumHierarchy(cb, userData); \
    }

/// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3,BaseClass4)
#define AZ_RTTI_4(ClassName_, TemplateParamsInParen, _1, _2, _3, _4)                             \
    AZ_RTTI_COMMON(ClassName_, TemplateParamsInParen)                                            \
static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
    if (id == RTTI_Type()) { return true; }                                                      \
    if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
    if (AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id)) { return true; }                   \
    if (AZ::Internal::RttiCaller<_3>::RTTI_IsContainType(id)) { return true; }                   \
    return AZ::Internal::RttiCaller<_4>::RTTI_IsContainType(id); }                               \
static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
    cb(RTTI_Type(), userData);                                                                   \
    AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_3>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_4>::RTTI_EnumHierarchy(cb, userData); }                            \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                \
virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
    if (id == RTTI_Type()) { return this; }                                                      \
    const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
    r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    r = AZ::Internal::RttiCaller< _3>::RTTI_AddressOf(this, id); if (r) { return r; }            \
    return AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); }                             \
virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
    if (id == RTTI_Type()) { return this; }                                                      \
    void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
    r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    r = AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    return AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); }                             \
AZ_POP_DISABLE_WARNING

#define AZ_RTTI_NO_TYPE_INFO_IMPL_4(ClassName_, TemplateParamsInParen, Inline_, _1, _2, _3, _4) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) const \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        if (id == RTTI_Type()) { return this; } \
        const auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType4>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        if (id == RTTI_Type()) { return this; } \
        auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType4>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_IsContainType(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        return id == RTTI_Type() \
            || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType2>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType3>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType4>::RTTI_IsContainType(id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        cb(RTTI_Type(), userData); \
        AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType2>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType3>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType4>::RTTI_EnumHierarchy(cb, userData); \
    }

/// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3,BaseClass4,BaseClass5)
#define AZ_RTTI_5(ClassName_, TemplateParamsInParen, _1, _2, _3, _4, _5)                         \
    AZ_RTTI_COMMON(ClassName_, TemplateParamsInParen)                                            \
static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
    if (id == RTTI_Type()) { return true; }                                                      \
    if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
    if (AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id)) { return true; }                   \
    if (AZ::Internal::RttiCaller<_3>::RTTI_IsContainType(id)) { return true; }                   \
    if (AZ::Internal::RttiCaller<_4>::RTTI_IsContainType(id)) { return true; }                   \
    return AZ::Internal::RttiCaller<_5>::RTTI_IsContainType(id); }                               \
static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
    cb(RTTI_Type(), userData);                                                                   \
    AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_3>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_4>::RTTI_EnumHierarchy(cb, userData);                              \
    AZ::Internal::RttiCaller<_5>::RTTI_EnumHierarchy(cb, userData); }                            \
AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                \
virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
    if (id == RTTI_Type()) { return this; }                                                      \
    const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
    r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    r = AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    r = AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    return AZ::Internal::RttiCaller<_5>::RTTI_AddressOf(this, id); }                             \
virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
    if (id == RTTI_Type()) { return this; }                                                      \
    void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
    r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    r = AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    r = AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); if (r) { return r; }             \
    return AZ::Internal::RttiCaller<_5>::RTTI_AddressOf(this, id); }                             \
AZ_POP_DISABLE_WARNING

#define AZ_RTTI_NO_TYPE_INFO_IMPL_5(ClassName_, TemplateParamsInParen, Inline_, _1, _2, _3, _4, _5) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, TemplateParamsInParen, Inline_) \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) const \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        using BaseType5 = AZ_REMOVE_PARENTHESIS(_5); \
        if (id == RTTI_Type()) { return this; } \
        const auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType4>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType5>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_AddressOf(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        using BaseType5 = AZ_REMOVE_PARENTHESIS(_5); \
        if (id == RTTI_Type()) { return this; } \
        auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); if (address) { return address; } \
        address = AZ::Internal::RttiCaller<BaseType4>::RTTI_AddressOf(this, id); if (address) { return address; } \
        return AZ::Internal::RttiCaller<BaseType5>::RTTI_AddressOf(this, id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_IsContainType(const AZ::TypeId& id) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        using BaseType5 = AZ_REMOVE_PARENTHESIS(_5); \
        return id == RTTI_Type() \
            || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType2>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType3>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType4>::RTTI_IsContainType(id) \
            || AZ::Internal::RttiCaller<BaseType5>::RTTI_IsContainType(id); \
    } \
    AZ_RTTI_SIMPLE_TEMPLATE_ID TemplateParamsInParen \
    Inline_ void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST TemplateParamsInParen ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
    { \
        using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
        using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
        using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
        using BaseType4 = AZ_REMOVE_PARENTHESIS(_4); \
        using BaseType5 = AZ_REMOVE_PARENTHESIS(_5); \
        cb(RTTI_Type(), userData); \
        AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType2>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType3>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType4>::RTTI_EnumHierarchy(cb, userData); \
        AZ::Internal::RttiCaller<BaseType5>::RTTI_EnumHierarchy(cb, userData); \
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACRO specialization to allow optional parameters for template version of AZ_RTTI
#define AZ_INTERNAL_EXTRACT(...) AZ_INTERNAL_EXTRACT __VA_ARGS__
#define AZ_INTERNAL_NOTHING_AZ_INTERNAL_EXTRACT
#define AZ_INTERNAL_PASTE(_x, ...)  _x ## __VA_ARGS__
#define AZ_INTERNAL_EVALUATING_PASTE(_x, ...) AZ_INTERNAL_PASTE(_x, __VA_ARGS__)
#define AZ_INTERNAL_REMOVE_PARENTHESIS(_x) AZ_INTERNAL_EVALUATING_PASTE(AZ_INTERNAL_NOTHING_, AZ_INTERNAL_EXTRACT _x)

#define AZ_INTERNAL_USE_FIRST_ELEMENT_1(_1, ...) _1
#define AZ_INTERNAL_USE_FIRST_ELEMENT(...) AZ_INTERNAL_USE_FIRST_ELEMENT_1(__VA_ARGS__)

#define AZ_INTERNAL_SKIP_FIRST_I(_1, ...)  __VA_ARGS__
#define AZ_INTERNAL_SKIP_FIRST(...) AZ_INTERNAL_SKIP_FIRST_I(__VA_ARGS__)

#define AZ_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
#define AZ_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
#define AZ_RTTI_I_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

#define AZ_RTTI_I_MACRO_CALL_II(MACRO_NAME, ...) MACRO_NAME(__VA_ARGS__)
#define AZ_RTTI_I_MACRO_CALL_I(MACRO_NAME, ...) AZ_RTTI_I_MACRO_CALL_II(MACRO_NAME, __VA_ARGS__)
#define AZ_RTTI_I_MACRO_CALL(MACRO_NAME, ...) AZ_RTTI_I_MACRO_CALL_I(MACRO_NAME, __VA_ARGS__)

#define AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT_WITH_NAME(_BaseClassesInParen, _Name, _DisplayName, _Uuid, ...) \
    AZ_RTTI_I_MACRO_CALL(AZ_TYPE_INFO_WITH_NAME, _Name, _DisplayName, _Uuid AZ_VA_OPT(AZ_COMMA_SEPARATOR, __VA_ARGS__) __VA_ARGS__) \
    AZ_RTTI_I_MACRO_SPECIALIZE(AZ_RTTI_, AZ_VA_NUM_ARGS(AZ_INTERNAL_REMOVE_PARENTHESIS(_BaseClassesInParen)), AZ_WRAP(_Name, (__VA_ARGS__) \
        AZ_VA_OPT(AZ_COMMA_SEPARATOR, AZ_UNWRAP(_BaseClassesInParen)) AZ_UNWRAP(_BaseClassesInParen)))

#define AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT(_BaseClassesInParen, _Name, _Uuid, ...) \
    AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT_WITH_NAME(_BaseClassesInParen, _Name, #_Name, _Uuid, __VA_ARGS__)

// Handles the case of AZ_RTTI_WITH_NAME(Name, "DisplayName", "{Uuid}", BaseClasses ...)
// Invokes AZ_TYPE_INFO_WITH_NAME(Name, "DisplayName", "{Uuid}")
#define AZ_RTTI_HELPER_WITH_NAME_1(_Name, _DisplayName, _Uuid, ...) AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT_WITH_NAME((__VA_ARGS__), _Name, _DisplayName, _Uuid)

// Handles the case of AZ_RTTI_WITH_NAME((Name, "DisplayName", "{Uuid}", TemplateParamPlaceholder...), BaseClasses ...);
// Notice the 3 or more parameters in the inner parentheses which represents the first argument to macro
// Invoke a helper macro based that wraps the base class parameters in parentheses and removes parenthesis from all other parameters
// i.e AZ_RTTI_WITH_NAME((Name, "DisplayName", "{Uuid}", TemplateParamPlaceholder...), BaseClasses ...)
// ->  AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT((BaseClasses ...), Name, "DisplayName", "{Uuid}", TemplateParamPlaceholder...)
// Afterwards a call to the AZ_TYPE_INFO_WITH_NAME macro is used to bind the C++ identifier to the "DisplayName" and "{Uuid}":
// and a call to AZ_RTTI_N macro is used to bind implement the virtual functions for adding base class support to the IRttiHelper
// 1. AZ_TYPE_INFO_WITH_NAME(Name, "DisplayName", "{Uuid}", TemplateParamPlaceholder...)
// 2. AZ_RTTI_N(BaseClassesN...)
#define AZ_RTTI_HELPER_WITH_NAME_3(...) AZ_MACRO_CALL_FIRST_PASS(AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT_WITH_NAME, \
    (AZ_INTERNAL_SKIP_FIRST(__VA_ARGS__)), \
    AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__)))

#define AZ_RTTI_HELPER_WITH_NAME_4(...)  AZ_RTTI_HELPER_WITH_NAME_3(__VA_ARGS__)
#define AZ_RTTI_HELPER_WITH_NAME_5(...)  AZ_RTTI_HELPER_WITH_NAME_3(__VA_ARGS__)
#define AZ_RTTI_HELPER_WITH_NAME_6(...)  AZ_RTTI_HELPER_WITH_NAME_3(__VA_ARGS__)
#define AZ_RTTI_HELPER_WITH_NAME_7(...)  AZ_RTTI_HELPER_WITH_NAME_3(__VA_ARGS__)
#define AZ_RTTI_HELPER_WITH_NAME_8(...)  AZ_RTTI_HELPER_WITH_NAME_3(__VA_ARGS__)

// Handles the case of AZ_RTTI(Name, "{Uuid}", BaseClasses ...)
// Invokes AZ_TYPE_INFO_WITH_NAME(Name, #Name, "{Uuid}")
#define AZ_RTTI_HELPER_1(_Name, _Uuid, ...) AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT((__VA_ARGS__), _Name, _Uuid)

// Handles the case of AZ_RTTI((Name, "{Uuid}", TemplateParamPlaceholder...), BaseClasses ...);
// Notice the 2 or more parameters in the inner parentheses which represents the first argument to macro
#define AZ_RTTI_HELPER_2(...) AZ_MACRO_CALL_FIRST_PASS(AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT, \
    (AZ_INTERNAL_SKIP_FIRST(__VA_ARGS__)), \
    AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__)))

#define AZ_RTTI_HELPER_3(...)  AZ_RTTI_HELPER_2(__VA_ARGS__)
#define AZ_RTTI_HELPER_4(...)  AZ_RTTI_HELPER_2(__VA_ARGS__)
#define AZ_RTTI_HELPER_5(...)  AZ_RTTI_HELPER_2(__VA_ARGS__)
#define AZ_RTTI_HELPER_6(...)  AZ_RTTI_HELPER_2(__VA_ARGS__)
#define AZ_RTTI_HELPER_7(...)  AZ_RTTI_HELPER_2(__VA_ARGS__)
#define AZ_RTTI_HELPER_8(...)  AZ_RTTI_HELPER_2(__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Macro which adds the declarations of the virtual functions required for RTTI to the class declaration
 * This does add overloads required for TypeInfo to the class
 * The AZ_TYPE_INFO_WITH_NAME macro should be used separately
 * Useful for class which supplies itself as a template argument to one of it's base classes
 * as part of CRTP
 */
#define AZ_RTTI_NO_TYPE_INFO_DECL() AZ_RTTI_NO_TYPE_INFO_DECL_()

/*
 * Adds implementation of RTTI functions to the current file
 * Pairs with AZ_RTTI_NO_TYPE_INFO_DECL() macro
 * @param ClassOrTemplateName_ A class name or simple-template-id (http://eel.is/c++draft/temp.names#nt:simple-template-id) specifier for the class
 * surround in parenthesis with the any AZ_TYPE_INFO_* template placeholder parameters
 * @param __VA_ARGS__ Up to 5 direct base classes to be associated with the class for rtti casting to and from
 *
 * The AZ_RTTI_NO_TYPE_INFO_IMPL macro can be used after the class definition to implement only the RTTI functions
 * Wrapping the first argument in parenthesis allows template placeholder arguments to be supplied
 * which allows adding RTTI to class template
 * The Template placeholders such as AZ_TYPE_INFO_CLASS and AZ_TYPE_INFO_AUTO are detailed in the bottom of TemplateInfo.h
 *
 * The first argument should be either the class name for a regular class or when specifying the arguments for a class template
 * the simple-template-name (with no brackets) inside of an inner set of parenthesis, followed by 0 or template placeholder
 * arguments
 * ex. class -> `AZ_RTTI_NO_TYPE_INFO_IMPL(EditorComponentBase, Component)`
 * ex. class template -> `AZ_RTTI_NO_TYPE_INFO_IMPL((EditorComponentAdapter, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS), EditorComponentBase);
 *
 * INFO: Any argument can be surrounded in parenthesis to suppress commas within the argument.
 * For example specifying a base class template specialization with a comma in it, can be wrapped in parenthesis
 * ex. `AZ_RTTI_NO_TYPE_INFO_IMPL(MyVector, (AZStd::vector<int, AZStd::allocator), OtherBase)`
 *
 * First unpack the base classes to a AZ_RTTI_NO_TYPE_INFO_IMPL_N macro call where 'N'
 * represents the number of base class parameters
 * AZ_RTTI_NO_TYPE_INFO_IMPL((TemplateName, TemplateParam1, ..., TemplateParamN), BaseClass1, ..., BaseClassN)
 * -> AZ_RTTI_NO_TYPE_INFO_IMPL_N(TemplateName,  BaseClass1, ..., BaseClassN, TemplateParam1, TemplateParam2...)
 * This allow using the __VA_ARGS__ to only bind to the template parameters, with names binding to each base class
 *
 * Explanation of macro
 * `AZ_RTTI_MACRO_SPECIALIZE` -> Wrapper macro used to call the combined AZ_RTTI_NO_TYPE_INFO_IMPL_<N>
 * `AZ_RTTI_NO_TYPE_INFO_IMPL_` -> Prefix to combine with the number of base class arguments
 * `AZ_VA_NUM_ARGS(__VA_ARGS__))` -> counts the number of base class arguments
 * `AZ_INTERNAL_USE_FIRST_ELEMENT(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_))` ->
 *  removes any parenthesis from the first argument and then grab any inner first arguments
 *    If ClassOrTemplateName_ is class argument of `ClassName`, unpacks to `ClassName`
 *    If ClassOrTemplateName_ is a template argument of `(ClassName, TemplArg1)`, also unpacks to `ClassName`
 * `AZ_WRAP(AZ_INTERNAL_SKIP_FIRST(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_)))` ->
 *   removes any parenthesis from the first argument and then grabs all the arguments after the inner first argument.
 *   Afterwards wraps those remaining arguments in parenthesis.
 *   The result is the template placeholder parameters are passed (TemplateParam1, ..., TemplateParamN)
 *
 * `__VA_ARGS__` -> Passes the base classes as the 1st through Nth index arguments to the `AZ_RTTI_NO_TYPE_INFO_IMPL_N` macro
 */
#define AZ_RTTI_NO_TYPE_INFO_IMPL_HELPER(ClassOrTemplateName_, FunctionSpecifier, ...) \
    AZ_RTTI_MACRO_SPECIALIZE(AZ_RTTI_NO_TYPE_INFO_IMPL_, \
    AZ_VA_NUM_ARGS(__VA_ARGS__), \
    ( \
        AZ_INTERNAL_USE_FIRST_ELEMENT(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_)), \
        AZ_WRAP(AZ_INTERNAL_SKIP_FIRST(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_))), \
        FunctionSpecifier \
        AZ_VA_OPT(AZ_COMMA_SEPARATOR, __VA_ARGS__) __VA_ARGS__ \
    )) \

#define AZ_RTTI_NO_TYPE_INFO_IMPL(ClassOrTemplateName_, ...) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_HELPER(ClassOrTemplateName_,, __VA_ARGS__)

//! Version of AZ_RTTI_NO_TYPE_INFO_IMPL which adds the `inline` specifier to the RTTI function definitions
//! This can be used to define the RTTI functions in a .h or .inl such as when adding RTTI support to a class template
#define AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(ClassOrTemplateName_, ...) \
    AZ_RTTI_NO_TYPE_INFO_IMPL_HELPER(ClassOrTemplateName_,inline, __VA_ARGS__)

/* Steps to Add AZ_RTTI support to a derived class which inherits from a base class that used the
 * CRTP
 * 1. Forward declare the derived class
 * 2. Add external TypeInfo overload for the class using either the AZ_TYPE_INFO_SPECIALIZE_WITH_NAME for a class type
 *    or AZ_TYPE_INFO_TEMPLATE_WITH_NAME for a class template
 * 3. Add class definition for the derived class and inherit from the template base class, while supplying the derived class
 *    as a template argument
 * 4. Inside the class definition add a call to the AZ_RTTI_NO_TYPE_INFO_DECL() macro to forward declare the Rtti functions
 * 5. After the class definition add a call to the AZ_RTTI_NO_TYPE_INFO_IMPL() with the first parameter being a template-name
 *    without any arguments(correct: AZStd::vector, incorrect: AZStd::vector<T, Alloc>)
 * ex. Given a class which supplies itself as base class template argument as follows
 * template <class ValueType, class Derived>
 class RangedValueParameter
     : public DefaultValueParameter<ValueType, RangedValueParameter<ValueType, Derived>>
 {
     using BaseType = DefaultValueParameter<ValueType, RangedValueParameter<ValueType, Derived>>;
     AZ_RTTI_NO_TYPE_INFO_DECL
     // ...

 * `AZ_RTTI_NO_TYPE_INFO_IMPL((RangedValueParameter, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS), BaseType);
 */

// NOTE: This the same macro logic as the `AZ_MACRO_SPECIALIZE`, but it must used instead of
// `AZ_MACRO_SPECIALIZE` macro as the C preprocessor macro expansion will not expand a macro with the same
// name again during second scan of the macro during expansion
// For example given the following set of macros
// #define AZ_EXPAND_1(X) X
// #define AZ_TYPE_INFO_INTERNAL_1(params) AZ_MACRO_SPECIALIZE(AZ_EXPAND_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))
//
// If the AZ_MACRO_SPECIALIZE is used as follows with a single argument of FOO:
// `AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_, 1, (FOO))`,
// then the inner AZ_EXPAND_1 will NOT expand because the preprocessor suppresses expansion of the `AZ_MACRO_SPECIALIZE`
// with even if it is called with a different set of arguments.
// So the result of the macro is `AZ_MACRO_SPECIALIZE(AZ_EXPAND_, 1, (FOO))`, not `FOO`
// https://godbolt.org/z/GM6f7drh1
//
// See the following thread for information about the C Preprocessor works in this manner:
// https://stackoverflow.com/questions/66593868/understanding-the-behavior-of-cs-preprocessor-when-a-macro-indirectly-expands-i
// Using the name AZ_RTTI_MACRO_SPECIALIZE
#define AZ_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
#define AZ_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
#define AZ_RTTI_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

/*
 * The AZ_RTTI_WITH_NAME macro allows specifying a display name to associated with the type name being reflected
 */
#define AZ_RTTI_WITH_NAME(...) AZ_RTTI_MACRO_SPECIALIZE(AZ_RTTI_HELPER_WITH_NAME_, \
    AZ_VA_NUM_ARGS(AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))), \
    (__VA_ARGS__))

/*
 * Use AZ_RTTI macro to enable RTTI for the specific class. Keep in mind AZ_RTTI uses virtual function
 * so make sure the class has a virtual destructor.
 * AZ_RTTI includes the functionality of AZ_TYPE_INFO, so users should only use the AZ_RTTI macro in that case
 * The syntax is AZ_RTTI(ClassName,ClassUuid,(BaseClass1..N)) Between have 0 to N(currently 5) base classes is allowed, which enables
 * the IRttiHelper to perform dynamic casts and query about types.
 *
 *  \note A more complex use case is when class template is associated with RTTI.
         In that case the first argument must be wrapped in parentheses and any template arguments that are desired
         to be combined with the class template must be specified using the template placholders in TypeInfo.h
         The following placeholders are supported:
         AZ_TYPE_INFO_CLASS / AZ_TYPE_INFO_TYPENAME -> For use with a type template argument(i.e `class` or `typename`)
         AZ_TYPE_INFO_CLASS_VARARGS / AZ_TYPE_INFO_TYPENAME_VARARGS -> For use with a variadic pack of type template argument(i.e `class...` or `typename...`)
         AZ_TYPE_INFO_AUTO -> For use with a non-type template argument(i.e `auto`, `size_t`, `int`, `bool`)

 * ex. AZ_RTTI( (TemplateName, ClassUuid, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS, ...), BaseClass1...)
 *
 * Here is a second example for using AZ_RTTI as part of the AZStd::array class
 * ex. AZ_RTTI_WITH_NAME( (AZStd::array, "AZStd::array", "{911B2EA8-CCB1-4F0C-A535-540AD00173AE}", AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_AUTO) );
 *
 * Take care with the parentheses, excruciatingly explicitly delineated here: AZ_RTTI (2 (1 template-name, Uuid, template placholder macros... 1), base classes... 2)
 * Note that using this approach will not expose the AzGenericTypeInfo for the template.
 */
#define AZ_RTTI(...) AZ_RTTI_MACRO_SPECIALIZE(AZ_RTTI_HELPER_, \
    AZ_VA_NUM_ARGS(AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))), \
    (__VA_ARGS__))

// Adds RTTI support for <Type> without modifying the class
// Variadic parameters should be the base class(es) if there are any
#define AZ_EXTERNAL_RTTI_SPECIALIZE(Type, ...)                                  \
    template<>                                                                  \
    inline constexpr bool HasAZRttiExternal_v<Type> = true;                     \
                                                                                \
    template<>                                                                  \
        struct AZ::Internal::RttiHelper<Type>                                   \
        : public AZ::Internal::ExternalVariadicRttiHelper<Type, ##__VA_ARGS__>  \
    {                                                                           \
    };


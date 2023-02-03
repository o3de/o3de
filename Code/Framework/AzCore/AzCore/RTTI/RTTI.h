/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/typetraits/has_member_function.h>
#include <AzCore/std/typetraits/is_abstract.h>

namespace AZStd
{
    template<class T>
    class shared_ptr;
    template<class T>
    class intrusive_ptr;
}

namespace AZ
{
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

    /// RTTI typeId
    using RTTI_EnumCallback = void (*)(const AZ::TypeId& /*typeId*/, void* /*userData*/);

    // Helper template to retrieve Class Name from class with a `void RTTI_Enable()`
    // without needing to specify the class name
    template<class T>
    struct RttiGetClassFromMember;
    template<class T>
    struct RttiGetClassFromMember<void (T::*)()>
    {
        using Type = T;
    };


    // Require AZ_TYPE_INFO to be declared to be declared for the type

    #define AZ_RTTI_TEMPLATE_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
    #define AZ_RTTI_TEMPLATE_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_TEMPLATE_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS)
    #define AZ_RTTI_TEMPLATE_MACRO_CALL(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_TEMPLATE_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS)

    // Expands Template placeholder arguments for substituting template parameters
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

    // Expands Template placholder arguments for substituting template arguments
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

    // The virtual functions are forward declared and the static constexpr functions are defined inline
    // The non-constexpr static functions are also forward declared
    // Due to this, the Base Classes do not need to be specifed to the this macro
    #define AZ_RTTI_NO_TYPE_INFO_DECL_() \
        void RTTI_Enable();\
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
        static inline constexpr AZ::TypeId RTTI_Type() \
        { \
            using ClassType = typename AZ::RttiGetClassFromMember<decltype(&RTTI_Enable)>::Type; \
            return AZ::AzTypeInfo<ClassType>::GetCanonicalTypeId(); \
        } \
        static inline constexpr const char* RTTI_TypeName() \
        { \
            using ClassType = typename AZ::RttiGetClassFromMember<decltype(&RTTI_Enable)>::Type; \
            return AZ::AzTypeInfo<ClassType>::Name(); \
        }

    // Common implementation functions for any class with RTTI
    // The virtual functions are defined here
    #define AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, ...) \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_Enable() {} \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline AZ::TypeId ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_GetType() const { return RTTI_Type(); } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline const char* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_GetTypeName() const { return RTTI_TypeName(); } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_IsTypeOf(const AZ::TypeId& typeId) const { return RTTI_IsContainType(typeId); } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData) { return RTTI_EnumHierarchy(cb, userData); }


    #define AZ_RTTI_COMMON()                                                                                                       \
    AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                                              \
    void RTTI_Enable() {}                                                                                                          \
    virtual inline        AZ::TypeId RTTI_GetType() const { return RTTI_Type(); }                                                  \
    virtual inline const char* RTTI_GetTypeName() const { return RTTI_TypeName(); }                                      \
    virtual inline bool             RTTI_IsTypeOf(const AZ::TypeId & typeId) const { return RTTI_IsContainType(typeId); }          \
    virtual void                    RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData) { RTTI_EnumHierarchy(cb, userData); } \
    static inline constexpr AZ::TypeId RTTI_Type() \
    { \
        using ClassType = typename AZ::RttiGetClassFromMember<decltype(&RTTI_Enable)>::Type; \
        return AZ::AzTypeInfo<ClassType>::GetCanonicalTypeId(); \
    } \
    static inline constexpr const char* RTTI_TypeName() \
    { \
        using ClassType = typename AZ::RttiGetClassFromMember<decltype(&RTTI_Enable)>::Type; \
        return AZ::AzTypeInfo<ClassType>::Name(); \
    } \
    AZ_POP_DISABLE_WARNING

    /// AZ_RTTI()
    #define AZ_RTTI_0()             AZ_RTTI_COMMON()                                                                                  \
    static constexpr bool                 RTTI_IsContainType(const AZ::TypeId& id) { return id == RTTI_Type(); }                      \
    static constexpr void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { cb(RTTI_Type(), userData); } \
    AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                                                 \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const { return (id == RTTI_Type()) ? this : nullptr; }           \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) { return (id == RTTI_Type()) ? this : nullptr; }                 \
    AZ_POP_DISABLE_WARNING

    //! Function definitions to add RTTI function implementation for a class with no bases
    #define AZ_RTTI_NO_TYPE_INFO_IMPL_0(ClassName_, ...) \
        AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, __VA_ARGS__) \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) const { return id == RTTI_Type() ? this : nullptr; } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) { return id == RTTI_Type() ? this : nullptr; } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_IsContainType(const AZ::TypeId& id) { return id == RTTI_Type(); } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { cb(RTTI_Type(), userData); }

    /// AZ_RTTI(BaseClass)
    #define AZ_RTTI_1(_1)           AZ_RTTI_COMMON()                                           \
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
    #define AZ_RTTI_NO_TYPE_INFO_IMPL_1(ClassName_, _1, ...) \
        AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, __VA_ARGS__) \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) const \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            if (id == RTTI_Type()) { return this; } \
            return AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            if (id == RTTI_Type()) { return this; } \
            return AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_IsContainType(const AZ::TypeId& id) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            return id == RTTI_Type() || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            cb(RTTI_Type(), userData); \
            AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
        }

    /// AZ_RTTI(BaseClass1,BaseClass2)
    #define AZ_RTTI_2(_1, _2)        AZ_RTTI_COMMON()                                                \
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

    #define AZ_RTTI_NO_TYPE_INFO_IMPL_2(ClassName_, _1, _2, ...) \
        AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, __VA_ARGS__) \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) const \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            if (id == RTTI_Type()) { return this; } \
            const auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
            return AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            if (id == RTTI_Type()) { return this; } \
            auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
            return AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_IsContainType(const AZ::TypeId& id) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            return id == RTTI_Type() || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id) \
                || AZ::Internal::RttiCaller<BaseType2>::RTTI_IsContainType(id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            cb(RTTI_Type(), userData); \
            AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
            AZ::Internal::RttiCaller<BaseType2>::RTTI_EnumHierarchy(cb, userData); \
        }

    /// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3)
    #define AZ_RTTI_3(_1, _2, _3)    AZ_RTTI_COMMON()                                                \
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

    /// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3,BaseClass4)
    #define AZ_RTTI_4(_1, _2, _3, _4)  AZ_RTTI_COMMON()                                              \
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

    #define AZ_RTTI_NO_TYPE_INFO_IMPL_3(ClassName_, _1, _2, _3, ...) \
        AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, __VA_ARGS__) \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) const \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
            if (id == RTTI_Type()) { return this; } \
            const auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
            address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
            return AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
            if (id == RTTI_Type()) { return this; } \
            auto address = AZ::Internal::RttiCaller<BaseType1>::RTTI_AddressOf(this, id); if (address) { return address; } \
            address = AZ::Internal::RttiCaller<BaseType2>::RTTI_AddressOf(this, id); if (address) { return address; } \
            return AZ::Internal::RttiCaller<BaseType3>::RTTI_AddressOf(this, id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_IsContainType(const AZ::TypeId& id) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
            return id == RTTI_Type() \
                || AZ::Internal::RttiCaller<BaseType1>::RTTI_IsContainType(id) \
                || AZ::Internal::RttiCaller<BaseType2>::RTTI_IsContainType(id) \
                || AZ::Internal::RttiCaller<BaseType3>::RTTI_IsContainType(id); \
        } \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
        { \
            using BaseType1 = AZ_REMOVE_PARENTHESIS(_1); \
            using BaseType2 = AZ_REMOVE_PARENTHESIS(_2); \
            using BaseType3 = AZ_REMOVE_PARENTHESIS(_3); \
            cb(RTTI_Type(), userData); \
            AZ::Internal::RttiCaller<BaseType1>::RTTI_EnumHierarchy(cb, userData); \
            AZ::Internal::RttiCaller<BaseType2>::RTTI_EnumHierarchy(cb, userData); \
            AZ::Internal::RttiCaller<BaseType3>::RTTI_EnumHierarchy(cb, userData); \
        }

#define AZ_RTTI_NO_TYPE_INFO_IMPL_4(ClassName_, _1, _2, _3, _4, ...) \
        AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, __VA_ARGS__) \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) const \
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
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) \
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
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_IsContainType(const AZ::TypeId& id) \
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
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
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
    #define AZ_RTTI_5( _1, _2, _3, _4, _5)  AZ_RTTI_COMMON()                                         \
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

    #define AZ_RTTI_NO_TYPE_INFO_IMPL_5(ClassName_, _1, _2, _3, _4, _5, ...) \
        AZ_RTTI_NO_TYPE_INFO_IMPL_COMMON(ClassName_, __VA_ARGS__) \
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline const void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) const \
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
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void* ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_AddressOf(const AZ::TypeId& id) \
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
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline bool ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_IsContainType(const AZ::TypeId& id) \
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
        AZ_RTTI_SIMPLE_TEMPLATE_ID(__VA_ARGS__) \
        inline void ClassName_ AZ_RTTI_TEMPLATE_ARGUMENT_LIST(__VA_ARGS__) ::RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) \
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

    #define AZ_RTTI_HELPER_METADATA_BASECLASS_SPLIT_WITH_NAME(_BaseClassesInParen, _Name, _DisplayName, _Uuid, ...) \
        AZ_TYPE_INFO_WITH_NAME(_Name, _DisplayName, _Uuid AZ_VA_OPT(AZ_COMMA_SEPARATOR, __VA_ARGS__) __VA_ARGS__); \
        AZ_RTTI_I_MACRO_SPECIALIZE(AZ_RTTI_, AZ_VA_NUM_ARGS(AZ_INTERNAL_REMOVE_PARENTHESIS(_BaseClassesInParen)), _BaseClassesInParen)

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

    // Check if a class has Rtti support - HasAZRtti<ClassType>....
    AZ_HAS_MEMBER(AZRttiIntrusive, RTTI_Enable, void, ());

    enum class RttiKind
    {
        None,
        Intrusive,
        External
    };

    template<typename T>
    struct HasAZRttiExternal : AZStd::false_type {};

    template<typename T>
    struct HasAZRtti
    {
        static constexpr bool value = HasAZRttiExternal<T>::value || HasAZRttiIntrusive<T>::value;
        using type = std::integral_constant<bool, value>;
        using kind_type = AZStd::integral_constant<RttiKind, HasAZRttiIntrusive<T>::value ? RttiKind::Intrusive : (HasAZRttiExternal<T>::value ? RttiKind::External : RttiKind::None)>;
    };

    /*
     * Macro which adds the declarations of the virtual functions required for RTTI to the class declaration
     * This does add overloads required for GetAzTypeInfo to the class
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
     * The Template placholders such as AZ_TYPE_INFO_CLASS and AZ_TYPE_INFO_AUTO are detailed in the bottom of TemplateInfo.h
     *
     * The first argument should be either the class name for a regular class or when specifying the arguments for a class template
     * the simple-template-name (with no brackets) inside of an inner set of parenthesis, followed by 0 or template placholder
     * arguments
     * ex. class -> `AZ_RTTI_NO_TYPE_INFO_IMPL(EditorComponentBase, Component)`
     * ex. class template -> `AZ_RTTI_NO_TYPE_INFO_IMPL((EditorComponentAdapter, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS), EditorComponentBase);
     *
     * INFO: Any argument can be surrouned in parenthesis to supress commas within the argument.
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
     * `AZ_VA_NUM_ARGS(__VA_ARGS__))` -> counts the number of base class aguments
     * `AZ_INTERNAL_USE_FIRST_ELEMENT(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_))` ->
     *  removes any parenthesis from the first argument and then grab any inner first arguments
     *    If ClassOrTemplateName_ is class argument of `ClassName`, unpacks to `ClassName`
     *    If ClassOrTemplateName_ is a template argument of `(ClassName, TemplArg1)`, also unpacks to `ClassName`
     * `__VA_ARGS__` -> Passes the base classes as the 1st through Nth index arguments to the `AZ_RTTI_NO_TYPE_INFO_IMPL_N` macro
     * `AZ_INTERNAL_SKIP_FIRST(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_))` ->
     *   removes any parenthesis from the first argument and then grabs all the arguments after the inner first argument
     *   This result in passing in all the template placeholder parameters as __VA_ARGS__
     */
    #define AZ_RTTI_NO_TYPE_INFO_IMPL(ClassOrTemplateName_, ...) \
        AZ_RTTI_MACRO_SPECIALIZE(AZ_RTTI_NO_TYPE_INFO_IMPL_, \
        AZ_VA_NUM_ARGS(__VA_ARGS__), \
        ( \
            AZ_INTERNAL_USE_FIRST_ELEMENT(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_)), \
            __VA_ARGS__ \
            AZ_VA_OPT(AZ_COMMA_SEPARATOR, AZ_INTERNAL_SKIP_FIRST(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_))) AZ_INTERNAL_SKIP_FIRST(AZ_INTERNAL_REMOVE_PARENTHESIS(ClassOrTemplateName_)) \
        )) \

    /* Steps to Add AZ_RTTI support to a derived class which inherits from a base class that used the
     * CRTP
     * 1. Forward declare the derived class
     * 2. Add external GetAzTypeInfo overload for the class using either the AZ_TYPE_INFO_SPECIALIZE_WITH_NAME for a class type
     *    or AZ_TYPE_INFO_TEMPLATE_WITH_NAME for a class template
     * 3. Add class definition for the derived class and inherit from the template base class, while supplying the derived class
     *    as a template argument
     * 4. Inside the class defintion add a call to the AZ_RTTI_NO_TYPE_INFO_DECL() macro to forward declare the Rtti functoins
     * 5. After the class defintion add a call to the AZ_RTTI_NO_TYPE_INFO_IMPL() with the first parameter being a template-name
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

    /**
     * The AZ_RTTI_WITH_NAME macro allows specifying a display name to associated with the type name being reflected
     */
    #define AZ_RTTI_WITH_NAME(...) AZ_RTTI_MACRO_SPECIALIZE(AZ_RTTI_HELPER_WITH_NAME_, \
        AZ_VA_NUM_ARGS(AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))), \
        (__VA_ARGS__))

    /**
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
        struct HasAZRttiExternal<Type> : AZStd::true_type{};                        \
                                                                                    \
        template<>                                                                  \
            struct AZ::Internal::RttiHelper<Type>                                   \
            : public AZ::Internal::ExternalVariadicRttiHelper<Type, ##__VA_ARGS__>  \
        {                                                                           \
        };

     /**
      * Interface for resolving RTTI information when no compile-time type information is available.
      * The serializer retrieves an instance of the helper during type registration (when type information
      * is still available) and uses it to get RTTI information during serialization.
      */
    class IRttiHelper
    {
    public:
        virtual ~IRttiHelper() = default;
        virtual AZ::TypeId        GetActualUuid(const void* instance) const = 0;
        virtual const char* GetActualTypeName(const void* instance) const = 0;
        virtual const void*       Cast(const void* instance, const AZ::TypeId& asType) const = 0;
        virtual void*             Cast(void* instance, const AZ::TypeId& asType) const = 0;
        virtual AZ::TypeId        GetTypeId() const = 0;
        // If the type is an instance of a template this will return the type id of the template instead of the instance.
        // otherwise it return the regular type id.
        virtual AZ::TypeId        GetGenericTypeId() const = 0;
        virtual bool              IsTypeOf(const AZ::TypeId& id) const = 0;
        virtual bool              IsAbstract() const = 0;
        virtual bool              ProvidesFullRtti() const = 0;
        virtual void              EnumHierarchy(RTTI_EnumCallback callback, void* userData = nullptr) const = 0;
        virtual TypeTraits        GetTypeTraits() const = 0;
        virtual size_t            GetTypeSize() const = 0;

        // Template helpers
        template <typename TargetType>
        AZ_FORCE_INLINE const TargetType* Cast(const void* instance)
        {
            return reinterpret_cast<const TargetType*>(Cast(instance, AZ::AzTypeInfo<TargetType>::Uuid()()));
        }

        template <typename TargetType>
        AZ_FORCE_INLINE TargetType* Cast(void* instance)
        {
            return reinterpret_cast<TargetType*>(Cast(instance, AZ::AzTypeInfo<TargetType>::Uuid()));
        }

        template <typename QueryType>
        AZ_FORCE_INLINE bool IsTypeOf()
        {
            return IsTypeOf(AZ::AzTypeInfo<QueryType>::Uuid());
        }
    };

    namespace Internal
    {
        /*
         * Helper class to retrieve AZ::TypeIds and perform AZRtti queries.
         * This helper uses AZRtti when available, and does what it can when not.
         * It automatically resolves pointer-to-pointers to their value types
         * and supports queries without type information through the IRttiHelper
         * interface.
         * Call GetRttiHelper<T>() to retrieve an IRttiHelper interface
         * for AZRtti-enabled types while type info is still available, so it
         * can be used for queries after type info is lost.
         */
        template<typename T>
        struct RttiHelper
            : public IRttiHelper
        {
        public:
            using ValueType = T;
            static_assert(HasAZRtti<ValueType>::value, "Type parameter for RttiHelper must support AZ RTTI.");

            //////////////////////////////////////////////////////////////////////////
            // IRttiHelper
            AZ::TypeId GetActualUuid(const void* instance) const override
            {
                return instance
                    ? reinterpret_cast<const T*>(instance)->RTTI_GetType()
                    : T::RTTI_Type();
            }
            const char* GetActualTypeName(const void* instance) const override
            {
                return instance
                    ? reinterpret_cast<const T*>(instance)->RTTI_GetTypeName()
                    : T::RTTI_TypeName();
            }
            const void* Cast(const void* instance, const AZ::TypeId& asType) const override
            {
                return instance
                    ? reinterpret_cast<const T*>(instance)->RTTI_AddressOf(asType)
                    : nullptr;
            }
            void* Cast(void* instance, const AZ::TypeId& asType) const override
            {
                return instance
                    ? reinterpret_cast<T*>(instance)->RTTI_AddressOf(asType)
                    : nullptr;
            }
            AZ::TypeId GetTypeId() const override
            {
                return T::RTTI_Type();
            }
            AZ::TypeId GetGenericTypeId() const override
            {
                return AzTypeInfo<T>::GetTemplateId();
            }
            bool IsTypeOf(const AZ::TypeId& id) const override
            {
                return T::RTTI_IsContainType(id);
            }
            bool IsAbstract() const override
            {
                return AZStd::is_abstract<T>::value;
            }
            bool ProvidesFullRtti() const override
            {
                return true;
            }
            void EnumHierarchy(RTTI_EnumCallback callback, void* userData = nullptr) const override
            {
                T::RTTI_EnumHierarchy(callback, userData);
            }
            TypeTraits GetTypeTraits() const override
            {
                return AzTypeInfo<T>::GetTypeTraits();
            }
            size_t GetTypeSize() const override
            {
                return AzTypeInfo<T>::Size();
            }
            //////////////////////////////////////////////////////////////////////////
        };

        template<typename T>
        struct TypeInfoOnlyRttiHelper
            : public IRttiHelper
        {
        public:
            using ValueType = T;
            static_assert(HasAzTypeInfo_v<ValueType>, "Type parameter for RttiHelper must support AZ Type Info.");

            //////////////////////////////////////////////////////////////////////////
            // IRttiHelper
            AZ::TypeId GetActualUuid(const void*) const override
            {
                return AZ::AzTypeInfo<ValueType>::Uuid();
            }
            const char* GetActualTypeName(const void*) const override
            {
                return AZ::AzTypeInfo<ValueType>::Name();
            }
            const void* Cast(const void* instance, const AZ::TypeId& asType) const override
            {
                return asType == GetTypeId() ? instance : nullptr;
            }
            void* Cast(void* instance, const AZ::TypeId& asType) const override
            {
                return asType == GetTypeId() ? instance : nullptr;
            }
            AZ::TypeId GetTypeId() const override
            {
                return AZ::AzTypeInfo<ValueType>::Uuid();
            }
            AZ::TypeId GetGenericTypeId() const override
            {
                return AZ::AzTypeInfo<ValueType>::GetTemplateId();
            }
            bool IsTypeOf(const AZ::TypeId& id) const override
            {
                return id == GetTypeId();
            }
            bool IsAbstract() const override
            {
                return false;
            }
            bool ProvidesFullRtti() const override
            {
                return false;
            }
            void EnumHierarchy(RTTI_EnumCallback callback, void* instance) const override
            {
                callback(GetTypeId(), instance);
            }
            TypeTraits GetTypeTraits() const override
            {
                return AzTypeInfo<T>::GetTypeTraits();
            }
            size_t GetTypeSize() const override
            {
                return AzTypeInfo<T>::Size();
            }
            //////////////////////////////////////////////////////////////////////////
        };

        template<typename T, typename... TArgs>
        struct ExternalVariadicRttiHelper
            : public IRttiHelper
        {
            AZ::TypeId GetActualUuid(const void*) const override
            {
                return AZ::AzTypeInfo<T>::Uuid();
            }
            const char* GetActualTypeName(const void*) const override
            {
                return AZ::AzTypeInfo<T>::Name();
            }

            const void* Cast(const void* instance, const AZ::TypeId& asType) const override
            {
                const void* result = GetTypeId() == asType ? instance : nullptr;

                using dummy = bool[];
                [[maybe_unused]] dummy d { true, (CastInternal<TArgs>(result, instance, asType), true)... };
                return result;
            }

            void* Cast(void* instance, const AZ::TypeId& asType) const override
            {
                return const_cast<void*>(Cast(const_cast<const void*>(instance), asType));
            }

            AZ::TypeId GetTypeId() const override
            {
                return AZ::AzTypeInfo<T>::Uuid();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return AZ::AzTypeInfo<T>::GetTemplateId();
            }

            bool IsTypeOf(const AZ::TypeId& id) const override
            {
                bool result = GetTypeId() == id;

                using dummy = bool[];
                [[maybe_unused]] dummy d = { true, (IsTypeOfInternal<TArgs>(result, id), true)... };

                return result;
            }
            bool IsAbstract() const override
            {
                return AZStd::is_abstract<T>::value;
            }
            bool ProvidesFullRtti() const override
            {
                return true;
            }
            void EnumHierarchy(RTTI_EnumCallback callback, void* instance) const override
            {
                callback(GetActualUuid(instance), instance);

                using dummy = bool[];
                [[maybe_unused]] dummy d = { true, (RttiHelper<TArgs>{}.EnumHierarchy(callback, instance), true)... };
            }
            TypeTraits GetTypeTraits() const override
            {
                return AzTypeInfo<T>::GetTypeTraits();
            }
            size_t GetTypeSize() const override
            {
                return AzTypeInfo<T>::Size();
            }

        private:
            template<typename T2>
            static void CastInternal(const void*& outPointer, const void* instance, const AZ::TypeId& asType)
            {
                if (outPointer == nullptr)
                {
                    // For classes with Intrusive RTTI IsTypeOf is used instead as this External RTTI class does not have
                    // virtual RTTI_AddressOf functions for looking up the correct address
                    if (HasAZRttiExternal<T2>::value)
                    {
                        outPointer = RttiHelper<T2>{}.Cast(instance, asType);
                    }
                    else if(RttiHelper<T2>{}.IsTypeOf(asType))
                    {
                        outPointer = static_cast<const T2*>(instance);
                    }
                }
            }

            template<typename T2>
            static void IsTypeOfInternal(bool& result, const AZ::TypeId& id)
            {
                if (!result)
                {
                    result = RttiHelper<T2>{}.IsTypeOf(id);
                }
            }
        };

        namespace RttiHelperTags
        {
            struct Unavailable {};
            struct Standard {};
            struct TypeInfoOnly {};
        }

        // Overload for AZRtti
        // Named _Internal so that GetRttiHelper() calls from inside namespace Internal don't resolve here
        template<typename T>
        IRttiHelper* GetRttiHelper_Internal(RttiHelperTags::Standard)
        {
            static RttiHelper<T> s_instance;
            return &s_instance;
        }

        template<typename T>
        IRttiHelper* GetRttiHelper_Internal(RttiHelperTags::TypeInfoOnly)
        {
            static TypeInfoOnlyRttiHelper<T> s_instance;
            return &s_instance;
        }
        // Overload for no typeinfo available
        template<typename>
        IRttiHelper* GetRttiHelper_Internal(RttiHelperTags::Unavailable)
        {
            return nullptr;
        }
    } // namespace Internal

    template<typename T>
    IRttiHelper* GetRttiHelper()
    {
        using ValueType = AZStd::remove_cvref_t<AZStd::remove_pointer_t<T>>;
        static constexpr bool hasRtti =  HasAZRtti<ValueType>::value;
        static constexpr bool hasTypeInfo = Internal::HasAzTypeInfo_v<ValueType>;

        using TypeInfoTag = AZStd::conditional_t<hasTypeInfo, AZ::Internal::RttiHelperTags::TypeInfoOnly, AZ::Internal::RttiHelperTags::Unavailable>;
        using Tag = AZStd::conditional_t<hasRtti, AZ::Internal::RttiHelperTags::Standard, TypeInfoTag>;

        return AZ::Internal::GetRttiHelper_Internal<ValueType>(Tag{});
    }

    namespace Internal
    {
        template<class T, class U>
        inline T        RttiCastHelper(U ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
        {
            typedef typename AZStd::remove_pointer<T>::type CastType;
            return ptr ? reinterpret_cast<T>(ptr->RTTI_AddressOf(AzTypeInfo<CastType>::Uuid())) : nullptr;
        }

        template<class T, class U>
        inline T        RttiCastHelper(U ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
        {
            typedef typename AZStd::remove_pointer<T>::type CastType;
            return ptr ? reinterpret_cast<T>(RttiHelper<AZStd::remove_pointer_t<U>>().Cast(ptr, AzTypeInfo<CastType>::Uuid())) : nullptr;
        }

        template<class T, class U>
        struct RttiIsSameCast
        {
            static inline T Cast(U)         { return nullptr; }
        };

        template<class T>
        struct RttiIsSameCast<T, T>
        {
            static inline T Cast(T ptr)     { return ptr; }
        };

        template<class T, class U>
        inline T        RttiCastHelper(U ptr, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
        {
            static_assert(HasAzTypeInfo_v<U>, "AZ_TYPE_INFO is required to perform an azrtti_cast");
            return RttiIsSameCast<T, U>::Cast(ptr);
        }

        template<class T, bool IsConst = AZStd::is_const<AZStd::remove_pointer_t<T>>::value >
        struct AddressTypeHelper
        {
            typedef const void* type;
        };

        template<class T>
        struct AddressTypeHelper<T, false>
        {
            typedef void* type;
        };

        template<class U>
        inline typename AddressTypeHelper<U>::type RttiAddressOfHelper(U ptr, const AZ::TypeId& id, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
        {
            return ptr ? ptr->RTTI_AddressOf(id) : nullptr;
        }

        template<class U>
        inline typename AddressTypeHelper<U>::type RttiAddressOfHelper(U ptr, const AZ::TypeId& id, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
        {
            return ptr ? RttiHelper<AZStd::remove_pointer_t<U>>().Cast(ptr, id) : nullptr;
        }

        template<class U>
        inline typename AddressTypeHelper<U>::type  RttiAddressOfHelper(U ptr, const AZ::TypeId& id, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
        {
            typedef typename AZStd::remove_pointer<U>::type CastType;
            return (id == AzTypeInfo<CastType>::Uuid()) ? ptr : nullptr;
        }

        template<class T>
        using RttiRemoveQualifiers = AZStd::remove_cvref_t<AZStd::remove_pointer_t<T>>;

        template<class T, class U>
        struct RttiIsTypeOfHelper
        {
            static inline bool Check(const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ref.RTTI_IsTypeOf(AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid());
            }

            static inline bool Check(const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                return RttiHelper<U>().IsTypeOf(AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid());
            }

            static inline bool Check(const U&, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid() == AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }
        };
        template<class T, class U>
        struct RttiIsTypeOfHelper<T, U*>
        {
            static inline bool  Check(const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ptr && ptr->RTTI_IsTypeOf(AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid());
            }

            static inline bool  Check(const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                return ptr ? RttiHelper<U>().IsTypeOf(AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid()) : false;
            }

            static inline bool  Check(const U*, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid() == AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }
        };

        template<class U>
        struct RttiIsTypeOfIdHelper
        {
            static inline bool  Check(const AZ::TypeId& id, const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ref.RTTI_IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                return RttiHelper<U>().IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, const U&, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return id == AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }

            static inline AZ::TypeId Type(const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ref.RTTI_GetType();
            }

            static inline AZ::TypeId Type(const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                return RttiHelper<U>().GetTypeId();
            }

            static inline AZ::TypeId Type(const U&, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }
        };

        template<class U>
        struct RttiIsTypeOfIdHelper<U*>
        {
            static inline bool Check(const AZ::TypeId& id, U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ptr && ptr->RTTI_IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                return ptr && GetRttiHelper<U>()->IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, U*, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return id == AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }

            static inline AZ::TypeId Type(U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                static constexpr AZ::TypeId s_invalidUuid{};
                return ptr ? ptr->RTTI_GetType() : s_invalidUuid;
            }

            static inline AZ::TypeId Type(U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                static constexpr AZ::TypeId s_invalidUuid;
                return ptr ? RttiHelper<U>().GetTypeId() : s_invalidUuid;
            }

            static inline AZ::TypeId Type(U*, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }
        };

        template<class T>
        void RttiEnumHierarchyHelper(RTTI_EnumCallback cb, void* userData, const AZStd::true_type& /* HasAZRtti<T> */)
        {
            RttiHelper<T>().EnumHierarchy(cb, userData);
        }

        template<class T>
        constexpr void RttiEnumHierarchyHelper(RTTI_EnumCallback cb, void* userData, const AZStd::false_type& /* HasAZRtti<T> */)
        {
            cb(AzTypeInfo<T>::Uuid(), userData);
        }

        // Helper to prune types that are base class, but don't support RTTI
        template<class T, RttiKind rttiKind = HasAZRtti<T>::kind_type::value>
        struct RttiCaller
        {
            AZ_FORCE_INLINE static constexpr bool RTTI_IsContainType(const AZ::TypeId& id)
            {
                return T::RTTI_IsContainType(id);
            }

            AZ_FORCE_INLINE static constexpr void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData)
            {
                T::RTTI_EnumHierarchy(cb, userData);
            }

            AZ_FORCE_INLINE static const void* RTTI_AddressOf(const T* obj, const AZ::TypeId& id)
            {
                return obj->T::RTTI_AddressOf(id);
            }

            AZ_FORCE_INLINE static void* RTTI_AddressOf(T* obj, const AZ::TypeId& id)
            {
                return obj->T::RTTI_AddressOf(id);
            }
        };

        template<class T>
        struct RttiCaller<T, RttiKind::External>
        {
            AZ_FORCE_INLINE static bool RTTI_IsContainType(const AZ::TypeId& id)
            {
                return RttiHelper<T>().IsTypeOf(id);
            }

            AZ_FORCE_INLINE static void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData)
            {
                RttiHelper<T>().EnumHierarchy(cb, userData);
            }

            AZ_FORCE_INLINE static const void* RTTI_AddressOf(const T* obj, const AZ::TypeId& id)
            {
                return RttiHelper<T>().Cast(obj, id);
            }

            AZ_FORCE_INLINE static void* RTTI_AddressOf(T* obj, const AZ::TypeId& id)
            {
                return RttiHelper<T>().Cast(obj, id);
            }
        };

        // Specialization for classes that don't have intrusive RTTI implemented
        template<class T>
        struct RttiCaller<T, RttiKind::None>
        {
            AZ_FORCE_INLINE static constexpr bool RTTI_IsContainType(const AZ::TypeId&)
            {
                return false;
            }

            AZ_FORCE_INLINE static constexpr void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback, void*)
            {
            }

            AZ_FORCE_INLINE static constexpr const void* RTTI_AddressOf(const T*, const AZ::TypeId&)
            {
                return nullptr;
            }

            AZ_FORCE_INLINE static constexpr void* RTTI_AddressOf(T*, const AZ::TypeId&)
            {
                return nullptr;
            }
        };
    } // namespace Internal

    /// Enumerate class hierarchy (static) based on input type. Safe to call for type not supporting AZRtti returns the AzTypeInfo<T>::Uuid only.
    template<class T>
    inline void     RttiEnumHierarchy(RTTI_EnumCallback cb, void* userData)
    {
        AZ::Internal::RttiEnumHierarchyHelper<T>(cb, userData, typename HasAZRtti<AZStd::remove_pointer_t<T>>::type());
    }

    /// Performs a RttiCast when possible otherwise return NULL. Safe to call for type not supporting AZRtti (returns NULL unless type fully match).
    template<class T, class U>
    inline T        RttiCast(U ptr)
    {
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        static_assert(AZStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        return AZ::Internal::RttiCastHelper<T>(ptr, typename HasAZRtti<AZStd::remove_pointer_t<U>>::kind_type());
    }

    /// Specialization for nullptr_t, it's convertible to anything
    template <class T>
    inline T        RttiCast(AZStd::nullptr_t)
    {
        static_assert(AZStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        return nullptr;
    }

    /// RttiCast specialization for shared_ptr.
    template<class T, class U>
    inline AZStd::shared_ptr<typename AZStd::remove_pointer<T>::type>       RttiCast(const AZStd::shared_ptr<U>& ptr)
    {
        using DestType = typename AZStd::remove_pointer<T>::type;
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        static_assert(AZStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        T castPtr = AZ::Internal::RttiCastHelper<T>(ptr.get(), typename HasAZRtti<U>::kind_type());
        if (castPtr)
        {
            return AZStd::shared_ptr<DestType>(ptr, castPtr);
        }
        return AZStd::shared_ptr<DestType>();
    }

    // RttiCast specialization for intrusive_ptr.
    template<class T, class U>
    inline AZStd::intrusive_ptr<typename AZStd::remove_pointer<T>::type>    RttiCast(const AZStd::intrusive_ptr<U>& ptr)
    {
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        static_assert(AZStd::is_pointer<T>::value, "rtti_cast supports only pointer types");
        return AZ::Internal::RttiCastHelper<T>(ptr.get(), typename HasAZRtti<U>::kind_type());
    }

    // Gets address of a contained type or NULL. Safe to call for type not supporting AZRtti (returns 0 unless type fully match).
    template<class T>
    inline typename AZ::Internal::AddressTypeHelper<T>::type RttiAddressOf(T ptr, const AZ::TypeId& id)
    {
        // we can support references (as not exception is needed), but pointer should be sufficient when it comes to addresses!
        static_assert(AZStd::is_pointer<T>::value, "RttiAddressOf supports only pointer types");
        return AZ::Internal::RttiAddressOfHelper(ptr, id, typename HasAZRtti<AZStd::remove_pointer_t<T>>::kind_type());
    }

    template<class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AZ::AzTypeInfo<U>::Uuid();
    }

    template<template<class...> class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<auto> class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<auto, auto> class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, class, auto> class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, class, class, auto> class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, auto> class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, auto, class> class U>
    inline constexpr AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<class U>
    inline AZ::TypeId RttiTypeId(const U& data)
    {
        return AZ::Internal::RttiIsTypeOfIdHelper<U>::Type(data, typename HasAZRtti<AZStd::remove_pointer_t<U>>::kind_type());
    }


    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<class T, class U>
    inline bool     RttiIsTypeOf(const U& data)
    {
        return AZ::Internal::RttiIsTypeOfHelper<T, U>::Check(data, typename HasAZRtti<AZStd::remove_pointer_t<U>>::kind_type());
    }

    // Since it's not possible to create an instance of a non-type template, only the T has to be overloaded.

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<class...> class T, class U>
    inline bool RttiIsTypeOf(const U&)
    {
        return AzGenericTypeInfo::Uuid<T>() == AzGenericTypeInfo::Uuid<Internal::RttiRemoveQualifiers<U>>();
    }

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).

    template<template<auto> class T, class U>
    inline bool RttiIsTypeOf(const U&)
    {
        return AzGenericTypeInfo::Uuid<T>() == AzGenericTypeInfo::Uuid<Internal::RttiRemoveQualifiers<U>>();
    }

    template<template<auto, auto> class T, class U>
    inline bool RttiIsTypeOf(const U&)
    {
        return AzGenericTypeInfo::Uuid<T>() == AzGenericTypeInfo::Uuid<Internal::RttiRemoveQualifiers<U>>();
    }

    // Returns true if the type is contained, otherwise false.Safe to call for type not supporting AZRtti(returns false unless type fully match).
    template<template<class, class, auto> class T, class U>
    inline bool RttiIsTypeOf(const U&)
    {
        return AzGenericTypeInfo::Uuid<T>() == AzGenericTypeInfo::Uuid<Internal::RttiRemoveQualifiers<U>>();
    }

    // Returns true if the type is contained, otherwise false.Safe to call for type not supporting AZRtti(returns false unless type fully match).
    template<template<class, class, class, auto> class T, class U>
    inline bool RttiIsTypeOf(const U&)
    {
        return AzGenericTypeInfo::Uuid<T>() == AzGenericTypeInfo::Uuid<Internal::RttiRemoveQualifiers<U>>();
    }

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<class, auto> class T, class U>
    inline bool RttiIsTypeOf(const U&)
    {
        return AzGenericTypeInfo::Uuid<T>() == AzGenericTypeInfo::Uuid<Internal::RttiRemoveQualifiers<U>>();
    }

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<class, auto, class> class T, class U>
    inline bool RttiIsTypeOf(const U&)
    {
        return AzGenericTypeInfo::Uuid<T>() == AzGenericTypeInfo::Uuid<Internal::RttiRemoveQualifiers<U>>();
    }

    // Returns true if the type (by id) is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<class U>
    inline bool RttiIsTypeOf(const AZ::TypeId& id, U data)
    {
        return AZ::Internal::RttiIsTypeOfIdHelper<U>::Check(id, data, typename HasAZRtti<AZStd::remove_pointer_t<U>>::kind_type());
    }

} // namespace AZ


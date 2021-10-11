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
    typedef void (* RTTI_EnumCallback)(const AZ::TypeId& /*typeId*/, void* /*userData*/);

    // We require AZ_TYPE_INFO to be declared
    #define AZ_RTTI_COMMON()                                                                                                       \
    AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                                              \
    void RTTI_Enable();                                                                                                            \
    virtual inline const AZ::TypeId& RTTI_GetType() const { return RTTI_Type(); }                                                  \
    virtual inline const char*      RTTI_GetTypeName() const { return RTTI_TypeName(); }                                           \
    virtual inline bool             RTTI_IsTypeOf(const AZ::TypeId & typeId) const { return RTTI_IsContainType(typeId); }          \
    virtual void                    RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData) { RTTI_EnumHierarchy(cb, userData); } \
    static inline const AZ::TypeId& RTTI_Type() { return TYPEINFO_Uuid(); }                                                        \
    static inline const char*       RTTI_TypeName() { return TYPEINFO_Name(); }                                                    \
    AZ_POP_DISABLE_WARNING

    //#define AZ_RTTI_1(_1)           static_assert(false,"You must provide a valid classUuid!")

    /// AZ_RTTI()
    #define AZ_RTTI_1()             AZ_RTTI_COMMON()                                                                        \
    static bool                 RTTI_IsContainType(const AZ::TypeId& id) { return id == RTTI_Type(); }                      \
    static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { cb(RTTI_Type(), userData); } \
    AZ_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override")                                                       \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const { return (id == RTTI_Type()) ? this : nullptr; } \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) { return (id == RTTI_Type()) ? this : nullptr; }       \
    AZ_POP_DISABLE_WARNING

    /// AZ_RTTI(BaseClass)
    #define AZ_RTTI_2(_1)           AZ_RTTI_COMMON()                                           \
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

    /// AZ_RTTI(BaseClass1,BaseClass2)
    #define AZ_RTTI_3(_1, _2)        AZ_RTTI_COMMON()                                                \
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

    /// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3)
    #define AZ_RTTI_4(_1, _2, _3)    AZ_RTTI_COMMON()                                                \
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
    #define AZ_RTTI_5(_1, _2, _3, _4)  AZ_RTTI_COMMON()                                              \
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

    /// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3,BaseClass4,BaseClass5)
    #define AZ_RTTI_6(_1, _2, _3, _4, _5)  AZ_RTTI_COMMON()                                          \
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

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MACRO specialization to allow optional parameters for template version of AZ_RTTI
    #define AZ_INTERNAL_EXTRACT(...) AZ_INTERNAL_EXTRACT __VA_ARGS__
    #define AZ_INTERNAL_NOTHING_AZ_INTERNAL_EXTRACT
    #define AZ_INTERNAL_PASTE(_x, ...)  _x ## __VA_ARGS__
    #define AZ_INTERNAL_EVALUATING_PASTE(_x, ...) AZ_INTERNAL_PASTE(_x, __VA_ARGS__)
    #define AZ_INTERNAL_REMOVE_PARENTHESIS(_x) AZ_INTERNAL_EVALUATING_PASTE(AZ_INTERNAL_NOTHING_, AZ_INTERNAL_EXTRACT _x)

    #define AZ_INTERNAL_USE_FIRST_ELEMENT(_1, ...) _1
    #define AZ_INTERNAL_SKIP_FIRST(_1, ...)  __VA_ARGS__

    #define AZ_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
    #define AZ_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
    #define AZ_RTTI_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

    #define AZ_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
    #define AZ_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
    #define AZ_RTTI_I_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

    #define AZ_RTTI_HELPER_1(_Name, ...) AZ_TYPE_INFO_INTERNAL_2(_Name, AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__)); AZ_RTTI_I_MACRO_SPECIALIZE(AZ_RTTI_, AZ_VA_NUM_ARGS(__VA_ARGS__), (AZ_INTERNAL_SKIP_FIRST(__VA_ARGS__)))

    #define AZ_RTTI_HELPER_3(...)  AZ_TYPE_INFO_INTERNAL(AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))); AZ_RTTI_I_MACRO_SPECIALIZE(AZ_RTTI_, AZ_VA_NUM_ARGS(__VA_ARGS__), (AZ_INTERNAL_SKIP_FIRST(__VA_ARGS__)))
    #define AZ_RTTI_HELPER_2(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_4(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_5(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_6(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_7(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_8(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_9(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
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
    struct HasAZRttiExternal
    {
        static constexpr bool value = false;
        using type = AZStd::false_type;
    };

    template<typename T>
    struct HasAZRtti
    {
        static constexpr bool value = HasAZRttiExternal<T>::value || HasAZRttiIntrusive<T>::value;
        using type = std::integral_constant<bool, value>;
        using kind_type = AZStd::integral_constant<RttiKind, HasAZRttiIntrusive<T>::value ? RttiKind::Intrusive : (HasAZRttiExternal<T>::value ? RttiKind::External : RttiKind::None)>;
    };

    /**
     * Use AZ_RTTI macro to enable RTTI for the specific class. Keep in mind AZ_RTTI uses virtual function
     * so make sure you have virtual destructor.
     * AZ_RTTI includes the functionality of AZ_TYPE_INFO, so you don't have to declare TypeInfo it if you use AZ_RTTI
     * The syntax is AZ_RTTI(ClassName,ClassUuid,(BaseClass1..N)) you can have 0 to N base classes this will allow us
     * to perform dynamic casts and query about types.
     * 
     *  \note A more complex use case is when you use templates where you have to group the parameters for the TypeInfo call.
     * ex. AZ_RTTI( ( (ClassName<TemplateArg1, TemplateArg2, ...>), ClassUuid, TemplateArg1, TemplateArg2, ...), BaseClass1...)
     *
     * Take care with the parentheses, excruciatingly explicitly delineated here: AZ_RTTI (3 (2 (1 fully templated class name 1), Uuid, template args... 2), base classes... 3)
     * Note that using this approach will not expose the AzGenericTypeInfo for the template.
     */
    #define AZ_RTTI(...)  AZ_RTTI_MACRO_SPECIALIZE(AZ_RTTI_HELPER_, AZ_VA_NUM_ARGS(AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))), (__VA_ARGS__))

    // Adds RTTI support for <Type> without modifying the class
    // Variadic parameters should be the base class(es) if there are any
    #define AZ_EXTERNAL_RTTI_SPECIALIZE(Type, ...)                                  \
        template<>                                                                  \
        struct HasAZRttiExternal<Type>                                              \
        {                                                                           \
            static const bool value = true;                                         \
            using type = AZStd::true_type;                                          \
        };                                                                          \
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
        virtual const AZ::TypeId& GetActualUuid(const void* instance) const = 0;
        virtual const char*       GetActualTypeName(const void* instance) const = 0;
        virtual const void*       Cast(const void* instance, const AZ::TypeId& asType) const = 0;
        virtual void*             Cast(void* instance, const AZ::TypeId& asType) const = 0;
        virtual const AZ::TypeId& GetTypeId() const = 0;
        // If the type is an instance of a template this will return the type id of the template instead of the instance.
        // otherwise it return the regular type id.
        virtual const AZ::TypeId& GetGenericTypeId() const = 0;
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
            const AZ::TypeId& GetActualUuid(const void* instance) const override
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
            const AZ::TypeId& GetTypeId() const override
            {
                return T::RTTI_Type();
            }
            const AZ::TypeId& GetGenericTypeId() const override
            {
                return AzTypeInfo<T>::template Uuid<AZ::GenericTypeIdTag>();
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
            static_assert(AZ::Internal::HasAZTypeInfo<ValueType>::value, "Type parameter for RttiHelper must support AZ Type Info.");

            //////////////////////////////////////////////////////////////////////////
            // IRttiHelper
            const AZ::TypeId& GetActualUuid(const void*) const override
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
            const AZ::TypeId& GetTypeId() const override
            {
                return AZ::AzTypeInfo<ValueType>::Uuid();
            }
            const AZ::TypeId& GetGenericTypeId() const override
            {
                return AZ::AzTypeInfo<ValueType>::template Uuid<AZ::GenericTypeIdTag>();
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
            const AZ::TypeId& GetActualUuid(const void*) const override
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

            const AZ::TypeId& GetTypeId() const override
            {
                return AZ::AzTypeInfo<T>::Uuid();
            }

            const AZ::TypeId& GetGenericTypeId() const override
            {
                return AZ::AzTypeInfo<T>::template Uuid<AZ::GenericTypeIdTag>();
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
        using ValueType = std::remove_const_t<std::remove_pointer_t<T>>;
        static constexpr bool hasRtti =  HasAZRtti<ValueType>::value;
        static constexpr bool hasTypeInfo = AZ::Internal::HasAZTypeInfo<ValueType>::value;

        using TypeInfoTag = AZStd::conditional_t<hasTypeInfo, AZ::Internal::RttiHelperTags::TypeInfoOnly, AZ::Internal::RttiHelperTags::Unavailable>;
        using Tag = AZStd::conditional_t<hasRtti, AZ::Internal::RttiHelperTags::Standard, TypeInfoTag>;

        static_assert(!HasAZRttiIntrusive<ValueType>::value || !AZ::Internal::HasAZTypeInfoSpecialized<ValueType>::value,
            "Types cannot use AZ_TYPE_INFO_SPECIALIZE macro externally with the AZ_RTTI macro"
            " AZ_RTTI adds virtual functions to the type in order to find the concrete class when looking up a typeid"
            " Specializing AzTypeInfo for a type would cause it not be used");

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
            static_assert(HasAZTypeInfo<U>::value, "AZ_TYPE_INFO is required to perform an azrtti_cast");
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
        struct RttiRemoveQualifiers
        {
            typedef typename AZStd::remove_cv<typename AZStd::remove_reference<typename AZStd::remove_pointer<T>::type>::type>::type type;
        };

        template<class T, class U>
        struct RttiIsTypeOfHelper
        {
            static inline bool  Check(const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return ref.RTTI_IsTypeOf(AzTypeInfo<CheckType>::Uuid());
            }

            static inline bool  Check(const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return RttiHelper<U>().IsTypeOf(AzTypeInfo<CheckType>::Uuid());
            }

            static inline bool  Check(const U&, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return AzTypeInfo<CheckType>::Uuid() == AzTypeInfo<SrcType>::Uuid();
            }
        };
        template<class T, class U>
        struct RttiIsTypeOfHelper<T, U*>
        {
            static inline bool  Check(const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return ptr && ptr->RTTI_IsTypeOf(AzTypeInfo<CheckType>::Uuid());
            }

            static inline bool  Check(const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return ptr ? RttiHelper<U>().IsTypeOf(AzTypeInfo<CheckType>::Uuid()) : false;
            }

            static inline bool  Check(const U*, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return AzTypeInfo<CheckType>::Uuid() == AzTypeInfo<SrcType>::Uuid();
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
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return id == AzTypeInfo<SrcType>::Uuid();
            }

            static inline const AZ::TypeId& Type(const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ref.RTTI_GetType();
            }

            static inline const AZ::TypeId& Type(const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                return RttiHelper<U>().GetTypeId();
            }

            static inline const AZ::TypeId& Type(const U&, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return AzTypeInfo<SrcType>::Uuid();
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
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return id == AzTypeInfo<SrcType>::Uuid();
            }

            static inline const AZ::TypeId& Type(U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                static AZ::TypeId s_invalidUuid = AZ::TypeId::CreateNull();
                return ptr ? ptr->RTTI_GetType() : s_invalidUuid;
            }

            static inline const AZ::TypeId& Type(U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                static AZ::TypeId s_invalidUuid = AZ::TypeId::CreateNull();
                return ptr ? RttiHelper<U>().GetTypeId() : s_invalidUuid;
            }

            static inline const AZ::TypeId& Type(U*, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return AzTypeInfo<SrcType>::Uuid();
            }
        };

        template<class T>
        void RttiEnumHierarchyHelper(RTTI_EnumCallback cb, void* userData, const AZStd::true_type& /* HasAZRtti<T> */)
        {
            RttiHelper<T>().EnumHierarchy(cb, userData);
        }

        template<class T>
        void RttiEnumHierarchyHelper(RTTI_EnumCallback cb, void* userData, const AZStd::false_type& /* HasAZRtti<T> */)
        {
            cb(AzTypeInfo<T>::Uuid(), userData);
        }

        // Helper to prune types that are base class, but don't support RTTI
        template<class T, RttiKind rttiKind = HasAZRtti<T>::kind_type::value>
        struct RttiCaller
        {
            AZ_FORCE_INLINE static bool RTTI_IsContainType(const AZ::TypeId& id)
            {
                return T::RTTI_IsContainType(id);
            }

            AZ_FORCE_INLINE static void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData)
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
            AZ_FORCE_INLINE static bool RTTI_IsContainType(const AZ::TypeId&)
            {
                return false;
            }

            AZ_FORCE_INLINE static void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback, void*)
            {
            }

            AZ_FORCE_INLINE static const void* RTTI_AddressOf(const T*, const AZ::TypeId&)
            {
                return nullptr;
            }

            AZ_FORCE_INLINE static void* RTTI_AddressOf(T*, const AZ::TypeId&)
            {
                return nullptr;
            }
        };

        template<class U, typename TypeIdResolverTag, typename AZStd::enable_if_t<AZStd::is_same_v<TypeIdResolverTag, AZStd::false_type>>* = nullptr>
        inline const AZ::TypeId& RttiTypeId()
        {
            return AzTypeInfo<U>::Uuid();
        }

        template<class U, typename TypeIdResolverTag, typename AZStd::enable_if_t<!AZStd::is_same_v<TypeIdResolverTag, AZStd::false_type>>* = nullptr>
        inline const AZ::TypeId& RttiTypeId()
        {
            return AzTypeInfo<U>::template Uuid<TypeIdResolverTag>();
        }
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

    // TypeIdResolverTag is one of the TypeIdResolverTags such as CanonicalTypeIdTag. If false_type is provided the default on assigned
    // by the type will be used.
    template<class U, typename TypeIdResolverTag = AZStd::false_type>
    inline const AZ::TypeId& RttiTypeId()
    {
        return AZ::Internal::RttiTypeId<U, TypeIdResolverTag>();
    }
    
    template<template<typename...> class U, typename = void>
    inline const AZ::TypeId& RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }
    
    template<template<AZStd::size_t...> class U, typename = void>
    inline const AZ::TypeId& RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }
    
    template<template<typename, AZStd::size_t> class U, typename = void>
    inline const AZ::TypeId& RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }
    
    template<template<typename, typename, AZStd::size_t> class U, typename = void>
    inline const AZ::TypeId& RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }
    
    template<template<typename, typename, typename, AZStd::size_t> class U, typename = void>
    inline const AZ::TypeId& RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }
    
    template<class U>
    inline const AZ::TypeId& RttiTypeId(const U& data)
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
    template<template<typename...> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename AZ::Internal::RttiRemoveQualifiers<U>::type;
        return AzGenericTypeInfo::Uuid<T>() == RttiTypeId<CheckType, AZ::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<AZStd::size_t...> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename AZ::Internal::RttiRemoveQualifiers<U>::type;
        return AzGenericTypeInfo::Uuid<T>() == RttiTypeId<CheckType, AZ::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<typename, AZStd::size_t> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename AZ::Internal::RttiRemoveQualifiers<U>::type;
        return AzGenericTypeInfo::Uuid<T>() == RttiTypeId<CheckType, AZ::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false.Safe to call for type not supporting AZRtti(returns false unless type fully match).
    template<template<typename, typename, AZStd::size_t> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename AZ::Internal::RttiRemoveQualifiers<U>::type;
        return AzGenericTypeInfo::Uuid<T>() == RttiTypeId<CheckType, AZ::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false.Safe to call for type not supporting AZRtti(returns false unless type fully match).
    template<template<typename, typename, typename, AZStd::size_t> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename AZ::Internal::RttiRemoveQualifiers<U>::type;
        return AzGenericTypeInfo::Uuid<T>() == RttiTypeId<CheckType, AZ::GenericTypeIdTag>();
    }

    // Returns true if the type (by id) is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<class U>
    inline bool     RttiIsTypeOf(const AZ::TypeId& id, U data)
    {
        return AZ::Internal::RttiIsTypeOfIdHelper<U>::Check(id, data, typename HasAZRtti<AZStd::remove_pointer_t<U>>::kind_type());
    }

} // namespace AZ


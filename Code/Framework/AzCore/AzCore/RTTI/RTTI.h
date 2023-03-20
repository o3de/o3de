/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTIMacros.h>
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
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
                    : AZ::AzTypeInfo<T>::Uuid();
            }
            const char* GetActualTypeName(const void* instance) const override
            {
                return instance
                    ? reinterpret_cast<const T*>(instance)->RTTI_GetTypeName()
                    : AZ::AzTypeInfo<T>::Name();
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
                return AZ::AzTypeInfo<T>::Uuid();
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
                    if (HasAZRttiExternal_v<T2>)
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
        inline T        RttiCastHelper(U ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
        {
            typedef typename AZStd::remove_pointer<T>::type CastType;
            return ptr ? reinterpret_cast<T>(ptr->RTTI_AddressOf(AzTypeInfo<CastType>::Uuid())) : nullptr;
        }

        template<class T, class U>
        inline T        RttiCastHelper(U ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
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
        inline typename AddressTypeHelper<U>::type RttiAddressOfHelper(U ptr, const AZ::TypeId& id, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
        {
            return ptr ? ptr->RTTI_AddressOf(id) : nullptr;
        }

        template<class U>
        inline typename AddressTypeHelper<U>::type RttiAddressOfHelper(U ptr, const AZ::TypeId& id, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
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
            static inline bool Check(const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
            {
                return ref.RTTI_IsTypeOf(AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid());
            }

            static inline bool Check(const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
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
            static inline bool  Check(const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
            {
                return ptr && ptr->RTTI_IsTypeOf(AzTypeInfo<RttiRemoveQualifiers<T>>::Uuid());
            }

            static inline bool  Check(const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
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
            static inline bool  Check(const AZ::TypeId& id, const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
            {
                return ref.RTTI_IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
            {
                return RttiHelper<U>().IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, const U&, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return id == AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }

            static inline AZ::TypeId Type(const U& ref, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
            {
                return ref.RTTI_GetType();
            }

            static inline AZ::TypeId Type(const U&, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
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
            static inline bool Check(const AZ::TypeId& id, U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
            {
                return ptr && ptr->RTTI_IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, const U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
            {
                return ptr && GetRttiHelper<U>()->IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, U*, const AZStd::integral_constant<RttiKind, RttiKind::None>& /* !HasAZRtti<U> */)
            {
                return id == AzTypeInfo<RttiRemoveQualifiers<U>>::Uuid();
            }

            static inline AZ::TypeId Type(U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive_v<U> */)
            {
                static constexpr AZ::TypeId s_invalidUuid{};
                return ptr ? ptr->RTTI_GetType() : s_invalidUuid;
            }

            static inline AZ::TypeId Type(U* ptr, const AZStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal_v<U> */)
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
    inline AZ::TypeId RttiTypeId()
    {
        return AZ::AzTypeInfo<U>::Uuid();
    }

    template<template<class...> class U>
    inline AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<auto> class U>
    inline AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<auto, auto> class U>
    inline AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, class, auto> class U>
    inline AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, class, class, auto> class U>
    inline AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, auto> class U>
    inline AZ::TypeId RttiTypeId()
    {
        return AzGenericTypeInfo::Uuid<U>();
    }

    template<template<class, auto, class> class U>
    inline AZ::TypeId RttiTypeId()
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


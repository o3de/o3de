/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/static_storage.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/has_member_function.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/function/invoke.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Crc.h>

#include <cstdio> // for snprintf

namespace AZStd
{
    template <class T>
    struct less;
    template <class T>
    struct less_equal;
    template <class T>
    struct greater;
    template <class T>
    struct greater_equal;
    template <class T>
    struct equal_to;
    template <class T>
    struct hash;
    template< class T1, class T2>
    struct pair;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class vector;
    template< class T, AZStd::size_t N >
    class array;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class forward_list;
    template<class Key, class MappedType, class Compare /*= AZStd::less<Key>*/, class Allocator /*= AZStd::allocator*/>
    class map;
    template<class Key, class MappedType, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_map;
    template<class Key, class MappedType, class Hasher /* = AZStd::hash<Key>*/, class EqualKey /* = AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_multimap;
    template <class Key, class Compare, class Allocator>
    class set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_multiset;
    template<class T, size_t Capacity>
    class fixed_vector;
    template< class T, size_t NumberOfNodes>
    class fixed_list;
    template< class T, size_t NumberOfNodes>
    class fixed_forward_list;
    template<AZStd::size_t NumBits>
    class bitset;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;

    template<class Element, class Traits, class Allocator>
    class basic_string;

    template<class Element>
    struct char_traits;

    template <class Element, class Traits>
    class basic_string_view;

    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;

    template<class Signature>
    class function;

    template<class T>
    class optional;

    struct monostate;

    template<class... Types>
    class variant;
}

namespace AZ
{
    enum class PlatformID;
    /**
     * Used to reference unique types by their unique id.
     */
    using TypeId = AZ::Uuid;

    template <class T, bool IsEnum = AZStd::is_enum<T>::value>
    struct AzTypeInfo;

    inline namespace TypeIdResolverTags
    {
        /**
         * PointerRemovedTypeId is used to lookup TypeIds of template specializations created before the pointer typeid
         * typeid was taken into account when generating a templates typeid.
         * Previously an AZStd::vector<AZ::Entity> and AZStd::vector<AZ::Entity*> had the same typeid which causes incorrect
         * Serialization data to be generated for one of the types. For example the class element for the AZStd::vector<AZ::Entity*>
         * should have the ClassElement::FLG_POINTER set for it's elements, but would not if it was registered with the SerailizeContext
         * last.
         */
        struct PointerRemovedTypeIdTag
        {
        };
        /**
         * CanonicalTypeId is used to lookup TypeIds of template specializations while taking into account that T* and T are
         * different types and therefore requires separate TypeId values.
         */
        struct CanonicalTypeIdTag
        {
        };
        /**
         * For template classes that are registered with specialization support this version will return the uuid of the template class itself, otherwise the
         * canonical type will be returned.
         */
        struct GenericTypeIdTag
        {
        };
    }

    /**
    * AzGenericTypeInfo allows retrieving the Uuid for templates that are registered for type info, such as
    * AzGenericTypeInfo::Uuid<AZStd::pair>();
    */
    // Due to the various non-types that templates require, it's not possible to wrap these functions in a structure. There
    // also needs to be an overload for every version because they all represent overloads for different non-types.
    namespace AzGenericTypeInfo
    {
        /// Needs to match declared parameter type.
        template <template <typename...> class> constexpr bool false_v1 = false;
        template <template <AZStd::size_t...> class> constexpr bool false_v2 = false;
        template <template <typename, AZStd::size_t> class> constexpr bool false_v3 = false;
        template <template <typename, typename, AZStd::size_t> class> constexpr bool false_v4 = false;
        template <template <typename, typename, typename, AZStd::size_t> class> constexpr bool false_v5 = false;
        template <template <typename, AZStd::size_t, typename> class> constexpr bool false_v6 = false;

        template<typename T>
        inline const AZ::TypeId& Uuid()
        {
            return AzTypeInfo<T>::template Uuid<AZ::GenericTypeIdTag>();
        }

        template<template<typename...> class T>
        inline const AZ::TypeId& Uuid()
        {
            static_assert(false_v1<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const AZ::TypeId s_uuid = AZ::TypeId::CreateNull();
            return s_uuid;
        }

        template<template<AZStd::size_t...> class T>
        inline const AZ::TypeId& Uuid()
        {
            static_assert(false_v2<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const AZ::TypeId s_uuid = AZ::TypeId::CreateNull();
            return s_uuid;
        }

        template<template<typename, AZStd::size_t> class T>
        inline const AZ::TypeId& Uuid()
        {
            static_assert(false_v3<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const AZ::TypeId s_uuid = AZ::TypeId::CreateNull();
            return s_uuid;
        }

        template<template<typename, typename, AZStd::size_t> class T>
        inline const AZ::TypeId& Uuid()
        {
            static_assert(false_v4<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const AZ::TypeId s_uuid = AZ::TypeId::CreateNull();
            return s_uuid;
        }

        template<template<typename, typename, typename, AZStd::size_t> class T>
        inline const AZ::TypeId& Uuid()
        {
            static_assert(false_v5<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const AZ::TypeId s_uuid = AZ::TypeId::CreateNull();
            return s_uuid;
        }

        template<template<typename, AZStd::size_t, typename> class T>
        inline const AZ::TypeId& Uuid()
        {
            static_assert(false_v6<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const AZ::TypeId s_uuid = AZ::TypeId::CreateNull();
            return s_uuid;
        }
    }

    namespace Internal
    {
        /// Check if a class has TypeInfo declared via AZ_TYPE_INFO in its declaration
        AZ_HAS_MEMBER(AZTypeInfoIntrusive, TYPEINFO_Enable, void, ());

        /// Check if a class has an AzTypeInfo specialization
        template<typename T, typename = void>
        struct HasAZTypeInfoSpecialized
            : AZStd::false_type
        {
        };

        template<typename T>
        struct HasAZTypeInfoSpecialized<T, AZStd::enable_if_t<AZStd::is_invocable_r<bool, decltype(&AzTypeInfo<T>::Specialized)>::value>>
            : AZStd::true_type
        {
        };

        template <class T>
        struct HasAZTypeInfo
        {
            static constexpr bool value = HasAZTypeInfoSpecialized<T>::value || HasAZTypeInfoIntrusive<T>::value;
            using type = typename AZStd::Utils::if_c<value, AZStd::true_type, AZStd::false_type>::type;
        };

        inline void AzTypeInfoSafeCat(char* destination, size_t maxDestination, const char* source)
        {
            if (!source || !destination || maxDestination == 0)
            {
                return;
            }
            size_t destinationLen = strlen(destination);
            size_t sourceLen = strlen(source);
            if (sourceLen == 0)
            {
                return;
            }
            size_t maxToCopy = maxDestination - destinationLen - 1;
            if (maxToCopy == 0)
            {
                return;
            }
            // clamp with max length
            if (sourceLen > maxToCopy)
            {
                sourceLen = maxToCopy;
            }
            destination += destinationLen;
            memcpy(destination, source, sourceLen);
            destination += sourceLen;
            *destination = 0;
        }

        template<class... Tn>
        struct AggregateTypes;

        template<class T1, class... Tn>
        struct AggregateTypes<T1, Tn...>
        {
            static void TypeName(char* buffer, size_t bufferSize)
            {
                AzTypeInfoSafeCat(buffer, bufferSize, AzTypeInfo<T1>::Name());
                AzTypeInfoSafeCat(buffer, bufferSize, " ");
                AggregateTypes<Tn...>::TypeName(buffer, bufferSize);
            }

            template<typename TypeIdResolverTag = CanonicalTypeIdTag>
            static AZ::TypeId Uuid()
            {
                const AZ::TypeId tail = AggregateTypes<Tn...>::template Uuid<TypeIdResolverTag>();
                if (!tail.IsNull())
                {
                    // Avoid accumulating a null uuid, since it affects the result.
                    return AzTypeInfo<T1>::template Uuid<TypeIdResolverTag>() + tail;
                }
                else
                {
                    return AzTypeInfo<T1>::template Uuid<TypeIdResolverTag>();
                }
            }
        };

        template<>
        struct AggregateTypes<>
        {
            static void TypeName(char* buffer, size_t bufferSize)
            {
                (void)buffer; (void)bufferSize;
            }

            template<typename TypeIdResolverTag = CanonicalTypeIdTag>
            static AZ::TypeId Uuid()
            {
                return AZ::TypeId::CreateNull();
            }
        };

        // VS2015+/clang init them as part of static init, or interlocked (as per standard)
#if AZ_TRAIT_COMPILER_USE_STATIC_STORAGE_FOR_NON_POD_STATICS
        using TypeIdHolder = AZStd::static_storage<AZ::TypeId>;
#else
        using TypeIdHolder = AZ::TypeId;
#endif

        // Represents the "*" typeid that can be combined with non-pointer types T to form a unique T* typeid 
        inline static const AZ::TypeId& PointerTypeId()
        {
            static TypeIdHolder s_uuid("{35C8A027-FE00-4769-AE36-6997CFFAF8AE}");
            return s_uuid;
        }

        template<typename T>
        const char* GetTypeName()
        {
            return AZ::AzTypeInfo<T>::Name();
        }

        template<int N, bool Recursion = false>
        struct NameBufferSize
        {
            static constexpr int Size = 1 + NameBufferSize<N / 10, true>::Size;
        };
        template<> struct NameBufferSize<0, false> { static constexpr const int Size = 2; };
        template<> struct NameBufferSize<0, true> { static constexpr const int Size = 1; };

        template<AZStd::size_t N>
        const char* GetTypeName()
        {
            static char buffer[NameBufferSize<N>::Size] = { 0 };
            if (buffer[0] == 0)
            {
                azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%zu", N);
            }
            return buffer;
        }

        template<typename T, typename TypeIdResolverTag>
        const AZ::TypeId& GetTypeId()
        {
            return AZ::AzTypeInfo<T>::template Uuid<TypeIdResolverTag>();
        }

        template<AZStd::size_t N, typename>
        const AZ::TypeId& GetTypeId()
        {
            static AZ::TypeId uuid = AZ::TypeId::CreateName(GetTypeName<N>());
            return uuid;
        }

    } // namespace Internal


    /**
    * Type Trait structure for tracking of C++ type trait data
    * within the SerializeContext ClassData structure;
    */
    enum class TypeTraits : AZ::u64
    {
        //! Stores the result of the std::is_signed check for a registered type
        is_signed = 0b1,
        //! Stores the result of the std::is_unsigned check for a registered type
        //! NOTE: While this might seem to be redundant due to is_signed, it is NOT
        //! Only numeric types have a concept of signedness
        is_unsigned = 0b10,
        //! Stores the result of the std::is_enum check
        is_enum = 0b100
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::TypeTraits);

    /**
    * Since we fully support cross shared libraries (DLL) operation, we can no longer rely on typeid, static templates, etc.
    * to generate the same result in different compilations. We will need to used (codegen too when used) to generate a unique
    * ID for each type. By default we will try to access to static functions inside a class that will provide this identifiers.
    * For classes when intrusive is not an option, you will need to specialize the AzTypeInfo. All of those are automated with
    * code generator.
    * For a reference we used AZ_FUNCTION_SIGNATURE to generate a type ID, but some compilers failed to always expand the template
    * type, thus giving is the same string for many time and we need a robust system.
    */

    // Specialization for types with intrusive (AZ_TYPE_INFO) type info
    template<class T>
    struct AzTypeInfo<T, false /* is_enum */>
    {
        static const char* Name()
        {
            static_assert(AZ::Internal::HasAZTypeInfoIntrusive<T>::value,
                "You should use AZ_TYPE_INFO or AZ_RTTI in your class/struct, or use AZ_TYPE_INFO_SPECIALIZE() externally. "
                "Make sure to include the header containing this information (usually your class header).");
            return T::TYPEINFO_Name();
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const AZ::TypeId& Uuid()
        {
            static_assert(AZ::Internal::HasAZTypeInfoIntrusive<T>::value,
                "You should use AZ_TYPE_INFO or AZ_RTTI in your class/struct, or use AZ_TYPE_INFO_SPECIALIZE() externally. "
                "Make sure to include the header containing this information (usually your class header).");
            return T::TYPEINFO_Uuid();
        }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            // Track the C++ type traits required by the SerializeContext
            typeTraits |= std::is_signed<T>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<T>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }

        static constexpr size_t Size()
        {
            return sizeof(T);
        }
    };

    // Default Specialization for enums without an overload
    template <class T>
    struct AzTypeInfo<T, true /* is_enum */>
    {
        typedef typename AZStd::RemoveEnum<T>::type UnderlyingType;
        static const char* Name() { return nullptr; }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const AZ::TypeId& Uuid() { static AZ::TypeId nullUuid = AZ::TypeId::CreateNull(); return nullUuid; }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            // Track the C++ type traits required by the SerializeContext
            typeTraits |= std::is_signed<UnderlyingType>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<UnderlyingType>::value ? TypeTraits::is_unsigned : typeTraits;
            typeTraits |= TypeTraits::is_enum;
            return typeTraits;
        }

        static constexpr size_t Size()
        {
            return sizeof(T);
        }
    };

    template<class T, class U>
    inline bool operator==(AzTypeInfo<T> const& lhs, AzTypeInfo<U> const& rhs)
    {
        return lhs.Uuid() == rhs.Uuid();
    }

    template<class T, class U>
    inline bool operator!=(AzTypeInfo<T> const& lhs, AzTypeInfo<U> const& rhs)
    {
        return lhs.Uuid() != rhs.Uuid();
    }

    namespace Internal
    {
        // Sizeof helper to deal with incomplete types and the void type
        template<typename T, typename = void>
        struct TypeInfoSizeof
        {
            static constexpr size_t Size()
            {
                return 0;
            }
        };

        // In this case that sizeof(T) is a compilable, the type is complete
        template<typename T>
        struct TypeInfoSizeof<T, AZStd::void_t<decltype(sizeof(T))>>
        {
            static constexpr size_t Size()
            {
                return sizeof(T);
            }
        };
    }
    // AzTypeInfo specialization helper for non intrusive TypeInfo
#define AZ_TYPE_INFO_INTERNAL_SPECIALIZE(_ClassName, _ClassUuid)      \
    template<>                                                        \
    struct AzTypeInfo<_ClassName, AZStd::is_enum<_ClassName>::value>  \
    {                                                                 \
        static const char* Name() { return #_ClassName; }             \
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>     \
        static const AZ::TypeId& Uuid() {                             \
            static AZ::Internal::TypeIdHolder s_uuid(_ClassUuid);     \
            return s_uuid;                                            \
        }                                                             \
        static constexpr TypeTraits GetTypeTraits()                   \
        {                                                             \
            TypeTraits typeTraits{};                                  \
            typeTraits |= std::is_signed<AZStd::RemoveEnumT<_ClassName>>::value ? TypeTraits::is_signed : typeTraits; \
            typeTraits |= std::is_unsigned<AZStd::RemoveEnumT<_ClassName>>::value ? TypeTraits::is_unsigned : typeTraits; \
            typeTraits |= std::is_enum<_ClassName>::value ? TypeTraits::is_enum: typeTraits; \
            return typeTraits;                                        \
        }                                                             \
        static constexpr size_t Size()                                \
        {                                                             \
            return AZ::Internal::TypeInfoSizeof<_ClassName>::Size();      \
        }                                                             \
        static bool Specialized() { return true; }                    \
    };

    // specialize for function pointers
    template<class R, class... Args>
    struct AzTypeInfo<R(Args...), false>
    {
        static const char* Name()
        {
            static char typeName[64] = { 0 };
            if (typeName[0] == 0)
            {
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "{");
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<R>::Name());
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "(");
                AZ::Internal::AggregateTypes<Args...>::TypeName(typeName, AZ_ARRAY_SIZE(typeName));
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ")}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const AZ::TypeId& Uuid()
        {
            static AZ::Internal::TypeIdHolder s_uuid(AZ::Internal::AggregateTypes<R, Args...>::template Uuid<TypeIdResolverTag>());
            return s_uuid;
        }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            typeTraits |= std::is_signed<R(Args...)>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<R(Args...)>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }
        static constexpr size_t Size()
        {
            return sizeof(R(Args...));
        }
    };

    // specialize for member function pointers
    template<class R, class C, class... Args>
    struct AzTypeInfo<R(C::*)(Args...), false>
    {
        static const char* Name()
        {
            static char typeName[128] = { 0 };
            if (typeName[0] == 0)
            {
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "{");
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<R>::Name());
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "(");
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<C>::Name());
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "::*)");
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "(");
                AZ::Internal::AggregateTypes<Args...>::TypeName(typeName, AZ_ARRAY_SIZE(typeName));
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ")}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const AZ::TypeId& Uuid()
        {
            static AZ::Internal::TypeIdHolder s_uuid(AZ::Internal::AggregateTypes<R, C, Args...>::template Uuid<TypeIdResolverTag>());
            return s_uuid;
        }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            typeTraits |= std::is_signed<R(C::*)(Args...)>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<R(C::*)(Args...)>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }
        static constexpr size_t Size()
        {
            return sizeof(R(C::*)(Args...));
        }
    };

    // specialize for const member function pointers
    template<class R, class C, class... Args>
    struct AzTypeInfo<R(C::*)(Args...) const, false>
        : AzTypeInfo<R(C::*)(Args...), false> {};

    // specialize for member data pointers
    template<class R, class C>
    struct AzTypeInfo<R C::*, false>
    {
        static const char* Name()
        {
            static char typeName[64] = { 0 };
            if (typeName[0] == 0)
            {
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "{");
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<R>::Name());
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), " ");
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<C>::Name());
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "::*}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const AZ::TypeId& Uuid()
        {
            static AZ::Internal::TypeIdHolder s_uuid(AZ::Internal::AggregateTypes<R, C>::template Uuid<TypeIdResolverTag>());
            return s_uuid;
        }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            typeTraits |= std::is_signed<R C::*>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<R C::*>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }
        static constexpr size_t Size()
        {
            return sizeof(R C::*);
        }
    };

    // Helper macro to generically specialize const, references, pointers, etc
#define AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(_T1, _Specialization, _NamePrefix, _NameSuffix)                      \
    template<class _T1>                                                                                          \
    struct AzTypeInfo<_Specialization, false> {                                                                  \
        static const char* Name() {                                                                              \
            static char typeName[64] = { 0 };                                                                    \
            if (typeName[0] == 0) {                                                                              \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                 \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name()); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                 \
            }                                                                                                    \
            return typeName;                                                                                     \
        }                                                                                                        \
        /*                                                                                                       \
        * By default the specialization for pointer types defaults to PointerRemovedTypeIdTag                    \
        * This allows the current use of AzTypeInfo<T*>::Uuid to still return the typeid of type                 \
        */                                                                                                       \
        template<typename TypeIdResolverTag = PointerRemovedTypeIdTag>                                           \
        static const AZ::TypeId& Uuid() {                                                                        \
            static AZ::Internal::TypeIdHolder s_uuid(                                                            \
                !AZStd::is_same<TypeIdResolverTag, PointerRemovedTypeIdTag>::value                               \
                && AZStd::is_pointer<_Specialization>::value ?                                                   \
                AZ::AzTypeInfo<_T1>::template Uuid<TypeIdResolverTag>() + AZ::Internal::PointerTypeId() : AZ::AzTypeInfo<_T1>::template Uuid<TypeIdResolverTag>()); \
            return s_uuid;                                                                                       \
        }                                                                                                        \
        static constexpr TypeTraits GetTypeTraits()                                                              \
        {                                                                                                        \
            TypeTraits typeTraits{};                                                                             \
            typeTraits |= std::is_signed<_Specialization>::value ? TypeTraits::is_signed : typeTraits;           \
            typeTraits |= std::is_unsigned<_Specialization>::value ? TypeTraits::is_unsigned : typeTraits;       \
            return typeTraits;                                                                                   \
        }                                                                                                        \
        static constexpr size_t Size()                                                                           \
        {                                                                                                        \
            return sizeof(_Specialization);                                                                      \
        }                                                                                                        \
        static bool Specialized() { return true; }                                                               \
    }

    // Helper macros to generically specialize template types
#define  AZ_TYPE_INFO_INTERNAL_VARIATION_GENERIC(_Generic, _Uuid)                                                       \
    namespace AzGenericTypeInfo {                                                                                       \
        template<>                                                                                                      \
        inline const AZ::TypeId& Uuid<_Generic>(){ static AZ::Internal::TypeIdHolder s_uuid(_Uuid); return s_uuid; }    \
    }

#define AZ_TYPE_INFO_INTERNAL_TYPENAME__TYPE typename
#define AZ_TYPE_INFO_INTERNAL_TYPENAME__ARG(A) A
#define AZ_TYPE_INFO_INTERNAL_TYPENAME__UUID(Tag, A) AZ::Internal::GetTypeId< A , Tag >()
#define AZ_TYPE_INFO_INTERNAL_TYPENAME__NAME(A) AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::Internal::GetTypeName< A >())

#define AZ_TYPE_INFO_INTERNAL_CLASS__TYPE class
#define AZ_TYPE_INFO_INTERNAL_CLASS__ARG(A) A
#define AZ_TYPE_INFO_INTERNAL_CLASS__UUID(Tag, A) AZ::Internal::GetTypeId< A , Tag >()
#define AZ_TYPE_INFO_INTERNAL_CLASS__NAME(A) AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::Internal::GetTypeName< A >())

#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS__TYPE typename...
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS__ARG(A) A...
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS__UUID(Tag, A) AZ::Internal::AggregateTypes< A... >::template Uuid< Tag >()
#define AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS__NAME(A) AZ::Internal::AggregateTypes< A... >::TypeName(typeName, AZ_ARRAY_SIZE(typeName))

#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS__TYPE class...
#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS__ARG(A) A...
#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS__UUID(Tag, A) AZ::Internal::AggregateTypes< A... >::template Uuid< Tag >()
#define AZ_TYPE_INFO_INTERNAL_CLASS_VARARGS__NAME(A) AZ::Internal::AggregateTypes< A... >::TypeName(typeName, AZ_ARRAY_SIZE(typeName));

// Once C++17 has been introduced size_t can be replaced with auto for all integer non-type arguments
#define AZ_TYPE_INFO_INTERNAL_AUTO__TYPE AZStd::size_t
#define AZ_TYPE_INFO_INTERNAL_AUTO__ARG(A) A
#define AZ_TYPE_INFO_INTERNAL_AUTO__UUID(Tag, A) AZ::Internal::GetTypeId< A , Tag >()
#define AZ_TYPE_INFO_INTERNAL_AUTO__NAME(A) AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::Internal::GetTypeName< A >())

#define AZ_TYPE_INFO_INTERNAL_EXPAND_I(NAME, TARGET)     NAME##__##TARGET
#define AZ_TYPE_INFO_INTERNAL_EXPAND(NAME, TARGET)       AZ_TYPE_INFO_INTERNAL_EXPAND_I(NAME, TARGET)


#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_1(_1)                                                                                  AZ_TYPE_INFO_INTERNAL_EXPAND(_1, TYPE) T1
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_2(_1, _2)             AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_1(_1),             AZ_TYPE_INFO_INTERNAL_EXPAND(_2, TYPE) T2
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3)         AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_2(_1, _2),         AZ_TYPE_INFO_INTERNAL_EXPAND(_3, TYPE) T3
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4)     AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3),     AZ_TYPE_INFO_INTERNAL_EXPAND(_4, TYPE) T4
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4), AZ_TYPE_INFO_INTERNAL_EXPAND(_5, TYPE) T5
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(...) AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_1(_1)                                                                                      AZ_TYPE_INFO_INTERNAL_EXPAND(_1, ARG)(T1)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2)             AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_1(_1),             AZ_TYPE_INFO_INTERNAL_EXPAND(_2, ARG)(T2)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3)         AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2),         AZ_TYPE_INFO_INTERNAL_EXPAND(_3, ARG)(T3)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4)     AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3),     AZ_TYPE_INFO_INTERNAL_EXPAND(_4, ARG)(T4)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4), AZ_TYPE_INFO_INTERNAL_EXPAND(_5, ARG)(T5)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(...) AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_1(_1)                                                                                                                                                          AZ_TYPE_INFO_INTERNAL_EXPAND(_1, NAME)(T1);
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_2(_1, _2)             AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_1(_1)             AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", "); AZ_TYPE_INFO_INTERNAL_EXPAND(_2, NAME)(T2);
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_3(_1, _2, _3)         AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_2(_1, _2)         AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", "); AZ_TYPE_INFO_INTERNAL_EXPAND(_3, NAME)(T3);
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_4(_1, _2, _3, _4)     AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_3(_1, _2, _3)     AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", "); AZ_TYPE_INFO_INTERNAL_EXPAND(_4, NAME)(T4);
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_5(_1, _2, _3, _4, _5) AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_4(_1, _2, _3, _4) AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", "); AZ_TYPE_INFO_INTERNAL_EXPAND(_5, NAME)(T5);
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION(...) AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_1(Tag, _1)                                                                                         AZ_TYPE_INFO_INTERNAL_EXPAND(_1, UUID)(Tag, T1)
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_2(Tag, _1, _2)             (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_1(Tag, _1) +             AZ_TYPE_INFO_INTERNAL_EXPAND(_2, UUID)(Tag, T2))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_3(Tag, _1, _2, _3)         (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_2(Tag, _1, _2) +         AZ_TYPE_INFO_INTERNAL_EXPAND(_3, UUID)(Tag, T3))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_4(Tag, _1, _2, _3, _4)     (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_3(Tag, _1, _2, _3) +     AZ_TYPE_INFO_INTERNAL_EXPAND(_4, UUID)(Tag, T4))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_5(Tag, _1, _2, _3, _4, _5) (AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_4(Tag, _1, _2, _3, _4) + AZ_TYPE_INFO_INTERNAL_EXPAND(_5, UUID)(Tag, T5))
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(Tag, ...) AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION_, AZ_VA_NUM_ARGS(__VA_ARGS__), (Tag, __VA_ARGS__))

#define AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(_Specialization, _Name, _ClassUuid, ...)             \
    AZ_TYPE_INFO_INTERNAL_VARIATION_GENERIC(_Specialization, _ClassUuid)                                            \
    template<AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)>                                            \
    struct AzTypeInfo<_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>, false> {     \
        static const char* Name() {                                                                                 \
            static char typeName[128] = { 0 };                                                                      \
            if (typeName[0] == 0) {                                                                                 \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _Name "<");                      \
                AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION(__VA_ARGS__)                                          \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">");                            \
            }                                                                                                       \
            return typeName;                                                                                        \
        }                                                                                                           \
    private:                                                                                                        \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::CanonicalTypeIdTag)                                \
        {                                                                                                           \
            static AZ::Internal::TypeIdHolder s_uuid(                                                               \
                AZ::TypeId(_ClassUuid) +                                                                            \
                AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(AZ::TypeIdResolverTags::CanonicalTypeIdTag, __VA_ARGS__)); \
            return s_uuid;                                                                                          \
        }                                                                                                           \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::PointerRemovedTypeIdTag)                           \
        {                                                                                                           \
            static AZ::Internal::TypeIdHolder s_uuid(                                                               \
                AZ::TypeId(_ClassUuid) +                                                                            \
                AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(AZ::TypeIdResolverTags::PointerRemovedTypeIdTag, __VA_ARGS__)); \
            return s_uuid;                                                                                          \
        }                                                                                                           \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::GenericTypeIdTag)                                  \
        {                                                                                                           \
            return AzGenericTypeInfo::Uuid<_Specialization>();                                                      \
        }                                                                                                           \
    public:                                                                                                         \
        template<typename Tag = AZ::TypeIdResolverTags::CanonicalTypeIdTag>                                         \
        static const AZ::TypeId& Uuid()                                                                             \
        {                                                                                                           \
            return UuidTag(Tag{});                                                                                  \
        }                                                                                                           \
        static constexpr TypeTraits GetTypeTraits()                                                                 \
        {                                                                                                           \
            TypeTraits typeTraits{};                                                                                \
            typeTraits |= std::is_signed<_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_signed : typeTraits; \
            typeTraits |= std::is_unsigned<_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits;                                                                                      \
        }                                                                                                           \
        static constexpr size_t Size()                                                                              \
        {                                                                                                           \
            return sizeof(_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>);         \
        }                                                                                                           \
        static bool Specialized() { return true; }                                                                  \
    }

#define AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(_Specialization, _ClassUuid, ...)                   \
    AZ_TYPE_INFO_INTERNAL_VARIATION_GENERIC(_Specialization, _ClassUuid)                                            \
    template<AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)>                                            \
    struct AzTypeInfo<_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>, false> {     \
        static const char* Name() {                                                                                 \
            static char typeName[128] = { 0 };                                                                      \
            if (typeName[0] == 0) {                                                                                 \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_Specialization "<");           \
                AZ_TYPE_INFO_INTERNAL_TEMPLATE_NAME_EXPANSION(__VA_ARGS__)                                          \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">");                            \
            }                                                                                                       \
            return typeName;                                                                                        \
        }                                                                                                           \
    private:                                                                                                        \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::CanonicalTypeIdTag)                                \
        {                                                                                                           \
            static AZ::Internal::TypeIdHolder s_uuid(                                                               \
                AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(AZ::TypeIdResolverTags::CanonicalTypeIdTag, __VA_ARGS__) \
                + AZ::TypeId(_ClassUuid));                                                                          \
            return s_uuid;                                                                                          \
        }                                                                                                           \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::PointerRemovedTypeIdTag)                           \
        {                                                                                                           \
            static AZ::Internal::TypeIdHolder s_uuid(                                                               \
                AZ_TYPE_INFO_INTERNAL_TEMPLATE_UUID_EXPANSION(AZ::TypeIdResolverTags::PointerRemovedTypeIdTag, __VA_ARGS__) \
                + AZ::TypeId(_ClassUuid));                                                                          \
            return s_uuid;                                                                                          \
        }                                                                                                           \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::GenericTypeIdTag)                                  \
        {                                                                                                           \
            return AzGenericTypeInfo::Uuid<_Specialization>();                                                      \
        }                                                                                                           \
    public:                                                                                                         \
        template<typename Tag = AZ::TypeIdResolverTags::CanonicalTypeIdTag>                                         \
        static const AZ::TypeId& Uuid()                                                                             \
        {                                                                                                           \
            return UuidTag(Tag{});                                                                                  \
        }                                                                                                           \
        static constexpr TypeTraits GetTypeTraits()                                                                 \
        {                                                                                                           \
            TypeTraits typeTraits{};                                                                                \
            typeTraits |= std::is_signed<_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_signed : typeTraits; \
            typeTraits |= std::is_unsigned<_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits;                                                                                      \
        }                                                                                                           \
        static constexpr size_t Size()                                                                              \
        {                                                                                                           \
            return sizeof(_Specialization<AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>);         \
        }                                                                                                           \
        static bool Specialized() { return true; }                                                                  \
    }

#define AZ_TYPE_INFO_INTERNAL_FUNCTION_VARIATION_SPECIALIZATION(_Specialization, _ClassUuid)\
    AZ_TYPE_INFO_INTERNAL_VARIATION_GENERIC(_Specialization, _ClassUuid) \
    template<typename R, typename... Args> \
    struct AzTypeInfo<_Specialization<R(Args...)>, false> {\
        static const char* Name() { \
            static char typeName[128] = { 0 }; \
            if (typeName[0] == 0) { \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_Specialization "<"); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<R>::Name()); \
                AZ::Internal::AggregateTypes<Args...>::TypeName(typeName, AZ_ARRAY_SIZE(typeName)); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">"); \
            }\
            return typeName; \
        } \
    private: \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::CanonicalTypeIdTag) \
        { \
            static AZ::Internal::TypeIdHolder s_uuid( \
                AZ::Uuid(_ClassUuid) + AZ::AzTypeInfo<R>::template Uuid<AZ::TypeIdResolverTags::CanonicalTypeIdTag>() + \
                AZ::Internal::AggregateTypes<Args...>::template Uuid<AZ::TypeIdResolverTags::CanonicalTypeIdTag>()); \
            return s_uuid; \
        } \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::PointerRemovedTypeIdTag) \
        { \
            static AZ::Internal::TypeIdHolder s_uuid( \
                AZ::Uuid(_ClassUuid) + AZ::AzTypeInfo<R>::template Uuid<AZ::TypeIdResolverTags::PointerRemovedTypeIdTag>() + \
                AZ::Internal::AggregateTypes<Args...>::template Uuid<AZ::TypeIdResolverTags::PointerRemovedTypeIdTag>()); \
            return s_uuid; \
        } \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::GenericTypeIdTag) \
        { \
            return AzGenericTypeInfo::Uuid<_Specialization>(); \
        } \
    public: \
        template<typename Tag = AZ::TypeIdResolverTags::CanonicalTypeIdTag> \
        static const AZ::TypeId& Uuid() \
        { \
            return UuidTag(Tag{}); \
        } \
        static constexpr TypeTraits GetTypeTraits() \
        { \
            TypeTraits typeTraits{}; \
            typeTraits |= std::is_signed<_Specialization<R(Args...)>>::value ? TypeTraits::is_signed : typeTraits; \
            typeTraits |= std::is_unsigned<_Specialization<R(Args...)>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits; \
        } \
        static constexpr size_t Size() \
        { \
            return sizeof(_Specialization<R(Args...)>); \
        } \
        static bool Specialized() { return true; } \
    }

    /* This version of AZ_TYPE_INFO_INTERNAL_VARIATION_SPECIALIZATION_2 only uses the first argument for UUID generation purposes */
#define AZ_TYPE_INFO_INTERNAL_VARIATION_SPECIALIZATION_2_CONCAT_1(_Specialization, _AddUuid, _T1, _T2)               \
    AZ_TYPE_INFO_INTERNAL_VARIATION_GENERIC(_Specialization, _AddUuid)                                               \
    template<class _T1, class _T2>                                                                                   \
    struct AzTypeInfo<_Specialization<_T1, _T2>, false> {                                                            \
        static const char* Name() {                                                                                  \
            static char typeName[128] = { 0 };                                                                       \
            if (typeName[0] == 0) {                                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_Specialization "<");            \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name());     \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">");                             \
            }                                                                                                        \
            return typeName;                                                                                         \
        }                                                                                                            \
    private:                                                                                                         \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::CanonicalTypeIdTag)                                 \
        {                                                                                                            \
            static AZ::Internal::TypeIdHolder s_uuid(                                                                \
                AZ::AzTypeInfo<_T1>::template Uuid<AZ::TypeIdResolverTags::CanonicalTypeIdTag>() +                   \
                AZ::TypeId(_AddUuid));                                                                               \
            return s_uuid;                                                                                           \
        }                                                                                                            \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::PointerRemovedTypeIdTag)                            \
        {                                                                                                            \
            static AZ::Internal::TypeIdHolder s_uuid(                                                                \
                AZ::AzTypeInfo<_T1>::template Uuid<AZ::TypeIdResolverTags::PointerRemovedTypeIdTag>() +              \
                AZ::TypeId(_AddUuid));                                                                               \
            return s_uuid;                                                                                           \
        }                                                                                                            \
        static const AZ::TypeId& UuidTag(AZ::TypeIdResolverTags::GenericTypeIdTag)                                   \
        {                                                                                                            \
            return AzGenericTypeInfo::Uuid<_Specialization>();                                                       \
        }                                                                                                            \
    public:                                                                                                          \
        template<typename Tag = AZ::TypeIdResolverTags::CanonicalTypeIdTag>                                          \
        static const AZ::TypeId& Uuid()                                                                              \
        {                                                                                                            \
            return UuidTag(Tag{});                                                                                   \
        }                                                                                                            \
        static constexpr TypeTraits GetTypeTraits()                                                                  \
        {                                                                                                            \
            TypeTraits typeTraits{};                                                                                 \
            typeTraits |= std::is_signed<_Specialization<_T1, _T2>>::value ? TypeTraits::is_signed : typeTraits;     \
            typeTraits |= std::is_unsigned<_Specialization<_T1, _T2>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits;                                                                                       \
        }                                                                                                            \
        static constexpr size_t Size()                                                                               \
        {                                                                                                            \
            return sizeof(_Specialization<_T1, _T2>);                                                                \
        }                                                                                                            \
        static bool Specialized() { return true; }                                                                   \
    }

    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(char, "{3AB0037F-AF8D-48ce-BCA0-A170D18B2C03}");
    //    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(signed char, "{CFD606FE-41B8-4744-B79F-8A6BD97713D8}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(AZ::s8, "{58422C0E-1E47-4854-98E6-34098F6FE12D}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(short, "{B8A56D56-A10D-4dce-9F63-405EE243DD3C}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(int, "{72039442-EB38-4d42-A1AD-CB68F7E0EEF6}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(long, "{8F24B9AD-7C51-46cf-B2F8-277356957325}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(AZ::s64, "{70D8A282-A1EA-462d-9D04-51EDE81FAC2F}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(unsigned char, "{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(unsigned short, "{ECA0B403-C4F8-4b86-95FC-81688D046E40}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(unsigned int, "{43DA906B-7DEF-4ca8-9790-854106D3F983}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(unsigned long, "{5EC2D6F7-6859-400f-9215-C106F5B10E53}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(AZ::u64, "{D6597933-47CD-4fc8-B911-63F3E2B0993A}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(float, "{EA2C3E90-AFBE-44d4-A90D-FAAF79BAF93D}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(double, "{110C4B14-11A8-4e9d-8638-5051013A56AC}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(bool, "{A0CA880C-AFE4-43cb-926C-59AC48496112}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(AZ::Uuid, "{E152C105-A133-4d03-BBF8-3D4B2FBA3E2A}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(void, "{C0F1AFAD-5CB3-450E-B0F5-ADB5D46B0E22}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(Crc32, "{9F4E062E-06A0-46D4-85DF-E0DA96467D3A}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(PlatformID, "{0635D08E-DDD2-48DE-A7AE-73CC563C57C3}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE(AZStd::monostate, "{B1E9136B-D77A-4643-BE8E-2ABDA246AE0E}");

    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(T, T*, "", "*");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(T, T &, "", "&");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(T, T &&, "", "&&");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(T, const T*, "const ", "*");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(T, const T&, "const ", "&");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(T, const T&&, "const ", "&&");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV(T, const T, "const ", "");
    
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::less,                "{41B40AFC-68FD-4ED9-9EC7-BA9992802E1B}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::less_equal,          "{91CC0BDC-FC46-4617-A405-D914EF1C1902}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::greater,             "{907F012A-7A4F-4B57-AC23-48DC08D0782E}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::greater_equal,       "{EB00488F-E20F-471A-B862-F1E3C39DDA1D}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::equal_to,            "{4377BCED-F78C-4016-80BB-6AFACE6E5137}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::hash,                "{EFA74E54-BDFA-47BE-91A7-5A05DA0306D7}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::pair,                "{919645C1-E464-482B-A69B-04AA688B6847}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::vector,              "{A60E3E61-1FF6-4982-B6B8-9E4350C4C679}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::list,                "{E1E05843-BB02-4F43-B7DC-3ADB28DF42AC}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::forward_list,        "{D7E91EA3-326F-4019-87F0-6F45924B909A}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::set,                 "{6C51837F-B0C9-40A3-8D52-2143341EDB07}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_set,       "{8D60408E-DA65-4670-99A2-8ABB574625AE}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_multiset,  "{B5950921-7F70-4806-9C13-8C7DF841BB90}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::map,                 "{F8ECF58D-D33E-49DC-BF34-8FA499AC3AE1}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_map,       "{41171F6F-9E5E-4227-8420-289F1DD5D005}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_multimap,  "{9ED846FA-31C1-4133-B4F4-91DF9750BA96}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::shared_ptr,          "{FE61C84E-149D-43FD-88BA-1C3DB7E548B4}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::intrusive_ptr,       "{530F8502-309E-4EE1-9AEF-5C0456B1F502}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::optional,            "{AB8C50C0-23A7-4333-81CD-46F648938B1C}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::basic_string,        "{C26397ED-8F60-4DF6-8320-0D0C592DA3CD}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::char_traits,         "{9B018C0C-022E-4BA4-AE91-2C1E8592DBB2}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::basic_string_view,   "{D348D661-6BDE-4C0A-9540-FCEA4522D497}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::fixed_vector,        "{74044B6F-E922-4FD7-915D-EFC5D1DC59AE}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::fixed_list,          "{508B9687-8410-4A73-AE0C-0BA15CF3F773}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::fixed_forward_list,  "{0D9D2AB2-F0CC-4E30-A209-A33D78717649}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::array,               "{911B2EA8-CCB1-4F0C-A535-540AD00173AE}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::bitset,              "{6BAE9836-EC49-466A-85F2-F4B1B70839FB}", AZ_TYPE_INFO_INTERNAL_AUTO);

    // We do things a bit differently for fixed_string due to what appears to be a compiler bug in msbuild
    // Clang has no issue using the forward declaration directly
    template <typename Element, size_t MaxElementCount, typename Traits>
    using FixedStringTypeInfo = AZStd::basic_fixed_string<Element, MaxElementCount, Traits>;
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(FixedStringTypeInfo, "{FA339E31-C383-49C7-80AC-5E1A3D8FA296}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO, AZ_TYPE_INFO_INTERNAL_TYPENAME);

    static constexpr const char* s_variantTypeId{ "{1E8BB1E5-410A-4367-8FAA-D43A4DE14D4B}" };
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(AZStd::variant, "variant", s_variantTypeId, AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS);
    
    AZ_TYPE_INFO_INTERNAL_FUNCTION_VARIATION_SPECIALIZATION(AZStd::function, "{C9F9C644-CCC3-4F77-A792-F5B5DBCA746E}");
} // namespace AZ

// Template class type info
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE(_ClassName, _ClassUuid, ...)\
    void TYPEINFO_Enable() {}\
    static const char* TYPEINFO_Name() {\
         static char typeName[128] = { 0 };\
         if (typeName[0]==0) {\
            AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_ClassName "<");\
            AZ::Internal::AggregateTypes<__VA_ARGS__>::TypeName(typeName, AZ_ARRAY_SIZE(typeName));\
            AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">");\
                           }\
         return typeName; } \
    static const AZ::TypeId& TYPEINFO_Uuid() {\
        static AZ::Internal::TypeIdHolder s_uuid(AZ::TypeId(_ClassUuid) + AZ::Internal::AggregateTypes<__VA_ARGS__>::Uuid());\
        return s_uuid;\
    }

// Template class type info
#define AZ_TYPE_INFO_INTERNAL_TEMPLATE_DEPRECATED(_ClassName, _ClassUuid, ...) \
    static_assert(false, "Embedded type info declaration inside templates are no longer supported. Please use 'AZ_TYPE_INFO_TEMPLATE' outside the template class instead.");

// all template versions are handled by a variadic template handler
#define AZ_TYPE_INFO_INTERNAL_3 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_4 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_5 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_6 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_7 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_8 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_9 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_10 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_11 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_12 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_13 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_14 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_15 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_16 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL_17 AZ_TYPE_INFO_INTERNAL_TEMPLATE
#define AZ_TYPE_INFO_INTERNAL(...) AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

// Fall-back for the original version of AZ_TYPE_INFO that accepted template arguments. This should not be used, unless
// to fix issues where AZ_TYPE_INFO was incorrectly used and the old UUID has to be maintained.
#define AZ_TYPE_INFO_LEGACY AZ_TYPE_INFO_INTERNAL

#include <AzCore/RTTI/TypeInfoSimple.h>

/**
* Use this macro outside a class to allow it to be identified across modules and serialized (in different contexts).
* The expected input is the class and the assigned uuid as a string or an instance of a uuid.
* Note that the AZ_TYPE_INFO_SPECIALIZE always has be declared in "namespace AZ".
* Example:
*   class MyClass
*   {
*   public:
*       ...
*   };
*
*   namespace AZ
*   {
*       AZ_TYPE_INFO_SPECIALIZE(MyClass, "{BD5B1568-D232-4EBF-93BD-69DB66E3773F}");
*   }
*/
#define AZ_TYPE_INFO_SPECIALIZE(_ClassName, _ClassUuid) AZ_TYPE_INFO_INTERNAL_SPECIALIZE(_ClassName, _ClassUuid)

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
* Prefer using AZ_TYPE_INFO_TEMPLATE and include the full namespace of the template class.
*/
#define AZ_TYPE_INFO_TEMPLATE_WITH_NAME(_Template, _Name, _Uuid, ...) AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(_Template, _Name, _Uuid, __VA_ARGS__)
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
*   namespace AZ
*   {
*       AZ_TYPE_INFO_TEMPLATE(MyTemplateClass, "{6BE82E8C-BC3D-4DA3-835E-644864A56405}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_AUTO, AZ_TYPE_INFO_CLASS);
*   }
*/
#define AZ_TYPE_INFO_TEMPLATE(_Template, _Uuid, ...) AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(_Template, #_Template, _Uuid, __VA_ARGS__)


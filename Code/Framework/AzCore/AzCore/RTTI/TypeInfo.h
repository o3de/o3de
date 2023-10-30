/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/TemplateInfo.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/span_fwd.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/optional.h>

#include <AzCore/Math/Uuid.h>

namespace AZ
{
    enum class PlatformID;
    class Crc32;
}

namespace AZStd
{
    class allocator;
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

    struct monostate;

    template<class... Types>
    class variant;
}

namespace AZ
{
    //! Allows a C++ Type to expose deprecated type names through a visitor pattern
    //! provide a static member function which accepts a user provided callback
    //! and invokes for each deprecated type name
    inline namespace DeprecatedTypeNames
    {
        using DeprecatedTypeNameCallback = AZStd::function<void(AZStd::string_view)>;

        template <class T, class = void>
        inline constexpr bool HasDeprecatedTypeNameVisitor_v = false;
        template <class T>
        inline constexpr bool HasDeprecatedTypeNameVisitor_v<T, AZStd::enable_if_t<AZStd::is_invocable_v<
            decltype(&AZStd::remove_cvref_t<T>::DeprecatedTypeNameVisitor), DeprecatedTypeNameCallback>>> = true;

        template <class T>
        struct AzDeprecatedTypeNameVisitor
        {
            constexpr void operator()(
                [[maybe_unused]] const DeprecatedTypeNameCallback& visitCallback) const
            {
                // Base Template delegates to static member function
                if constexpr (HasDeprecatedTypeNameVisitor_v<T>)
                {
                    AZStd::remove_cvref_t<T>::DeprecatedTypeNameVisitor(visitCallback);
                }
            }
        };
        // Specializations to remove reference qualifiers types
        template <class T>
        struct AzDeprecatedTypeNameVisitor<T*>
            : AzDeprecatedTypeNameVisitor<T>
        {};
        template <class T>
        struct AzDeprecatedTypeNameVisitor<T const>
            : AzDeprecatedTypeNameVisitor<T>
        {};
        template <class T>
        struct AzDeprecatedTypeNameVisitor<T&>
            : AzDeprecatedTypeNameVisitor<T>
        {};
        template <class T>
        struct AzDeprecatedTypeNameVisitor<T const&>
            : AzDeprecatedTypeNameVisitor<T>
        {};
        template <class T>
        struct AzDeprecatedTypeNameVisitor<T&&>
            : AzDeprecatedTypeNameVisitor<T>
        {};
        template <class T>
        struct AzDeprecatedTypeNameVisitor<T const&&>
            : AzDeprecatedTypeNameVisitor<T>
        {};
        template <class T>
        inline constexpr AzDeprecatedTypeNameVisitor<T> DeprecatedTypeNameVisitor{};
    }

    namespace Internal
    {
        template <class T>
        using HasAZTypeInfo = AZStd::bool_constant<
            HasGetO3deTypeName_v<AZStd::remove_cvref_t<AZStd::remove_pointer_t<T>>>
            && HasGetO3deTypeId_v<AZStd::remove_cvref_t<AZStd::remove_pointer_t<T>>>>;

        template <class T>
        inline constexpr bool HasAzTypeInfo_v = HasAZTypeInfo<T>::value;

        inline constexpr void AzTypeInfoSafeCat(char* destination, size_t maxDestination, const char* source)
        {
            if (!source || !destination || maxDestination == 0)
            {
                return;
            }
            size_t destinationLen = AZStd::char_traits<char>::length(destination);
            size_t sourceLen = AZStd::char_traits<char>::length(source);
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
            AZStd::char_traits<char>::copy(destination, source, sourceLen);
            destination += sourceLen;
            *destination = 0;
        }

        constexpr AZStd::string_view TypeNameSeparator = ", ";

        template<class... Tn>
        struct AggregateTypes
        {
            static constexpr auto TypeName()
            {
                if constexpr (sizeof...(Tn) == 0)
                {
                    return AZStd::string_view{};
                }
                else
                {
                    AZ::TypeNameString typeNameAggregate = []()
                    {
                        bool prependSeparator = false;
                        AZ::TypeNameString aggregateTypeName;
                        for (AZStd::string_view templateParamName : { AZStd::string_view(AzTypeInfo<Tn>::Name())... })
                        {
                            aggregateTypeName += prependSeparator ? TypeNameSeparator : "";
                            aggregateTypeName += templateParamName;
                            prependSeparator = true;
                        }
                        return aggregateTypeName;
                    }();

                    return typeNameAggregate;
                }
            }

            static constexpr AZ::TypeId GetCanonicalTypeId()
            {
                if constexpr (sizeof...(Tn) == 0)
                {
                    return {};
                }
                else
                {
                    const auto typeIds = AZStd::to_array<AZ::TypeId>({ AzTypeInfo<Tn>::GetCanonicalTypeId()... });
                    if (typeIds.empty())
                    {
                        return {};
                    }

                    AZ::TypeId aggregateTypeId = typeIds.back();

                    // perform binary right fold over all the type ids, by combining them
                    // two elements at a time from the end of the array to the beginning
                    // Skip over the last element and reverse the span
                    auto typeIdSpan = AZStd::span(typeIds.data(), typeIds.size() - 1);
                    for (size_t typeIdIndex = typeIdSpan.size(); typeIdIndex > 0; --typeIdIndex)
                    {
                        const AZ::TypeId& prevTypeId = typeIdSpan[typeIdIndex - 1];
                        if (!aggregateTypeId.IsNull())
                        {
                            // Avoid accumulating a null uuid, since it affects the result.
                            aggregateTypeId = prevTypeId + aggregateTypeId;
                        }
                        else
                        {
                            // Otherwise replace the aggregate type id with the previous typeid
                            aggregateTypeId = prevTypeId;
                        }
                    }

                    return aggregateTypeId;
                }
            }
        };

        extern template struct AggregateTypes<Crc32>;

        template<typename T>
        constexpr AZStd::string_view GetTypeName()
        {
            return AZ::AzTypeInfo<T>::Name();
        }

        template<int N, bool MoreDigits = (N >= 10)>
        struct NameBufferSize
        {
            static constexpr int Size = 1 + NameBufferSize<N / 10>::Size;
        };
        template<int N> struct NameBufferSize<N, false> { static constexpr int Size = 1; };

        // IntTypeName provides Compile time Variable template for c-string
        // of a converted unsigned integer value
        template<AZStd::size_t N>
        inline constexpr AZStd::fixed_string<NameBufferSize<N>::Size> IntTypeName = []() constexpr
        {
            using BufferType = AZStd::fixed_string<NameBufferSize<N>::Size>;
            BufferType numberString;
            auto FillDigitsFromEnd = [](char* buffer, size_t bufferSize)
            {
                // Fill the buffer from the end with stringified
                // conversions of each digit
                // The buffer is exactly the size to store the integer converted to a string
                size_t index = bufferSize;
                for (size_t value = N; index > 0; value /= 10)
                {
                    buffer[--index] = '0' + (value % 10);
                }
                return bufferSize;
            };
            numberString.resize_and_overwrite(numberString.max_size(), FillDigitsFromEnd);
            return numberString;
        }();

        // Supports any non-type template argument which supports conversion to a size_t
        template<auto N>
        constexpr AZStd::string_view GetTypeName()
        {
            return IntTypeName<static_cast<AZStd::size_t>(N)>;
        }

        template<typename T>
        constexpr AZ::TypeId GetCanonicalTypeId()
        {
            return AZ::AzTypeInfo<T>::GetCanonicalTypeId();
        }

        template<AZStd::size_t N>
        inline constexpr AZ::TypeId NumericTypeId_v = AZ::TypeId::CreateName(GetTypeName<N>());

        // Supports any non-type template argument which supports conversion to a size_t
        template<auto N>
        constexpr AZ::TypeId GetCanonicalTypeId()
        {
            return NumericTypeId_v<static_cast<AZStd::size_t>(N)>;
        }

    } // namespace Internal


    // Default function for enums without an overload
    template <class T>
    constexpr auto GetO3deTypeName(AZ::Adl, AZStd::type_identity<T>)
        -> AZStd::enable_if_t<AZStd::is_enum_v<T>, TypeNameString>
    {
        if constexpr (AZ::HasAzEnumTraits_v<T>)
        {
            constexpr AZ::TypeNameString typeName{ AZStd::string_view(AzEnumTraits<T>::EnumName) };
            return typeName;
        }
        else
        {
            return "[enum]";
        }
    }

    template <class T>
    constexpr auto GetO3deTypeId(AZ::Adl, AZStd::type_identity<T>)
        -> AZStd::enable_if_t<AZStd::is_enum_v<T>, AZ::TypeId>
    {
        return AZ::TypeId{};
    }


    // override for function pointers
    template<class R, class... Args>
    inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<R(Args...)>)
    {
        static AZ::TypeNameString s_canonicalTypeName;
        if (s_canonicalTypeName.empty())
        {
            AZStd::fixed_string<512> typeName{ '{' };
            typeName += AZ::AzTypeInfo<R>::Name();
            typeName += '(';
            typeName += AZ::Internal::AggregateTypes<Args...>::TypeName();
            typeName += ")}";
            s_canonicalTypeName = typeName;
        }

        return s_canonicalTypeName;
    }

    template<class R, class... Args>
    inline AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<R(Args...)>)
    {
        static const AZ::TypeId s_canonicalTypeId = AZ::Internal::AggregateTypes<R, Args...>::GetCanonicalTypeId();
        return s_canonicalTypeId;
    }

    // override for member function pointers
    template<class R, class C, class... Args>
    inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<R(C::*)(Args...)>)
    {
        static AZ::TypeNameString s_canonicalTypeName;
        if (s_canonicalTypeName.empty())
        {
            AZStd::fixed_string<512> typeName{ '{' };
            typeName += AZ::AzTypeInfo<R>::Name();
            typeName += '(';
            typeName += AZ::AzTypeInfo<C>::Name();
            typeName += "::*)(";
            typeName += AZ::Internal::AggregateTypes<Args...>::TypeName();
            typeName += ")}";
            s_canonicalTypeName = typeName;
        }

        return s_canonicalTypeName;
    }

    template<class R, class C, class... Args>
    inline AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<R(C::*)(Args...)>)
    {
        static const AZ::TypeId s_canonicalTypeId = AZ::Internal::AggregateTypes<R, C, Args...>::GetCanonicalTypeId();
        return s_canonicalTypeId;
    }

    // override for const member function pointers
    template<class R, class C, class... Args>
    inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<R(C::*)(Args...) const>)
    {
        // Append " const" to the a member function pointer name
        static const AZ::TypeNameString s_canonicalTypeName = GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<R(C::*)(Args...)>{})
            + " const";

        return s_canonicalTypeName;
    }

    template<class R, class C, class... Args>
    inline AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<R(C::*)(Args...) const>)
    {
        // TypeIds are the same between a const and non-const member functions
        return GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<R(C::*)(Args...)>{});
    }

    // override for member data pointers
    template<class R, class C>
    inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<R C::*>)
    {
        static AZ::TypeNameString s_canonicalTypeName;
        if (s_canonicalTypeName.empty())
        {
            AZStd::fixed_string<512> typeName{ '{' };
            typeName += AZ::AzTypeInfo<R>::Name();
            typeName += ' ';
            typeName + AZ::AzTypeInfo<C>::Name();
            typeName += "::*}";
            s_canonicalTypeName = typeName;
        }

        return s_canonicalTypeName;
    }

    template<class R, class C>
    inline AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<R C::*>)
    {
        static const AZ::TypeId s_canonicalTypeId = AZ::Internal::AggregateTypes<R, C>::GetCanonicalTypeId();
        return s_canonicalTypeId;
    }

    // override for std::reference_wrapper
    // Treat it as the raw type
    template <class T>
    inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<std::reference_wrapper<T>>)
    {
        // Append ampersand to the raw type name
        static const AZ::TypeNameString s_canonicalTypeName = GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<T>{})
            + '&';

        return s_canonicalTypeName;
    }

    template <class T>
    inline AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<std::reference_wrapper<T>>)
    {
        // Return the type id of the non-reference type for the reference wrapper
        constexpr TemplateId referenceWrapperId{ "{49A51B21-8302-4E63-8EE8-A4BF51B72FFC}" };
        return referenceWrapperId + AZ::AzTypeInfo<T>::Uuid();
    }
} // namespace AZ

namespace AZ
{
    //! Add GetO3deTypeName and GetO3deTypeId declarations for commonly used O3DE types
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(AZ::Uuid);
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(PlatformID);
}

namespace AZStd
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(AZStd::monostate);
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(AZStd::allocator);

    // Adding specialization of AZStd container types in the AZStd namespace
    // to allow ADL for these types when invoking GetO3deTypeName/GetO3deTypeId from the AzTypeInfo template
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::less, "AZStd::less", "{41B40AFC-68FD-4ED9-9EC7-BA9992802E1B}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::less_equal, "AZStd::less_equal", "{91CC0BDC-FC46-4617-A405-D914EF1C1902}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::greater, "AZStd::greater", "{907F012A-7A4F-4B57-AC23-48DC08D0782E}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::greater_equal, "AZStd::greater_equal", "{EB00488F-E20F-471A-B862-F1E3C39DDA1D}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::equal_to, "AZStd::equal_to", "{4377BCED-F78C-4016-80BB-6AFACE6E5137}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::hash, "AZStd::hash", "{EFA74E54-BDFA-47BE-91A7-5A05DA0306D7}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::pair, "AZStd::pair", "{919645C1-E464-482B-A69B-04AA688B6847}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::vector, "AZStd::vector", "{A60E3E61-1FF6-4982-B6B8-9E4350C4C679}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::list, "AZStd::list", "{E1E05843-BB02-4F43-B7DC-3ADB28DF42AC}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::forward_list, "AZStd::forward_list", "{D7E91EA3-326F-4019-87F0-6F45924B909A}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::set, "AZStd::set", "{6C51837F-B0C9-40A3-8D52-2143341EDB07}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_set, "AZStd::unordered_set", "{8D60408E-DA65-4670-99A2-8ABB574625AE}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_multiset, "AZStd::unordered_multiset", "{B5950921-7F70-4806-9C13-8C7DF841BB90}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::map, "AZStd::map", "{F8ECF58D-D33E-49DC-BF34-8FA499AC3AE1}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_map, "AZStd::unordered_map", "{41171F6F-9E5E-4227-8420-289F1DD5D005}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unordered_multimap, "AZStd::unordered_multimap", "{9ED846FA-31C1-4133-B4F4-91DF9750BA96}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::shared_ptr, "AZStd::shared_ptr", "{FE61C84E-149D-43FD-88BA-1C3DB7E548B4}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::intrusive_ptr, "AZStd::intrusive_ptr", "{530F8502-309E-4EE1-9AEF-5C0456B1F502}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::fixed_vector, "AZStd::fixed_vector", "{74044B6F-E922-4FD7-915D-EFC5D1DC59AE}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::fixed_list, "AZStd::fixed_list", "{508B9687-8410-4A73-AE0C-0BA15CF3F773}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::fixed_forward_list, "AZStd::fixed_forward_list", "{0D9D2AB2-F0CC-4E30-A209-A33D78717649}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::array, "AZStd::array", "{911B2EA8-CCB1-4F0C-A535-540AD00173AE}", AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::bitset, "AZStd::bitset", "{6BAE9836-EC49-466A-85F2-F4B1B70839FB}", AZ_TYPE_INFO_INTERNAL_AUTO);

    static constexpr const char* s_variantTypeId{ "{1E8BB1E5-410A-4367-8FAA-D43A4DE14D4B}" };
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(AZStd::variant, "AZStd::variant", s_variantTypeId, AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS);

    // GetO3deTypeName/GetO3deTypeId overload for AZStd::function
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(AZStd::function, "AZStd::function", "{C9F9C644-CCC3-4F77-A792-F5B5DBCA746E}", AZ_TYPE_INFO_INTERNAL_TYPENAME);

    // Add declarations of GetO3deTypeName and GetO3deTypeId for the basic string templates
    // In TypeInfo.cpp the implementation for common string specializations are added
    // AZStd::string, AZStd::string_view, AZStd::fixed_string<1024>, AZ::OSString
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID_DECL(AZStd::char_traits, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID_DECL(AZStd::basic_string_view, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID_DECL(AZStd::basic_string, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_BOTHFIX_UUID_DECL(AZStd::basic_fixed_string, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO, AZ_TYPE_INFO_INTERNAL_TYPENAME);
}

namespace AZStd
{
    // GetO3deTypeName/GetO3deTypeId overload for AZStd::span<T, Extent>
    // Note the type ID only takes the type T template parameter into account, not the Extent template parameter.
    // An `AZStd::span<AZ::Component*, 50>` and `AZStd::span<AZ::Component*, 100>` will have the same type ID, as the second template argument is not aggregated to the AZStd::span template ID.
    inline constexpr AZ::TemplateId GetO3deTemplateId(AZ::Adl, AZ::AzGenericTypeInfo::Internal::TemplateIdentityTypeAuto<AZStd::span>)
    {
        constexpr AZ::TypeId prefixUuid{ "{2FCDBAB3-45E0-4159-A91D-FD1D37056C0F}" };
        constexpr AZ::TypeId postfixUuid{};
        if constexpr (!prefixUuid.IsNull())
        {
            return prefixUuid;
        }
        else if constexpr (!postfixUuid.IsNull())
        {
            return postfixUuid;
        }
        else
        {
            return AZ::TemplateId{};
        }
    }
    template<typename T1>
    AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<AZStd::span<T1>>)
    {
        AZ::TypeNameString s_canonicalTypeName;
        if (s_canonicalTypeName.empty())
        {
            AZStd::fixed_string<512> typeName{ "AZStd::span"
                                               "<" };
            bool prependSeparator = false;
            for (AZStd::string_view templateParamName : { AZ::Internal::GetTypeName<T1>() })
            {
                typeName += prependSeparator ? AZ::Internal::TypeNameSeparator : "";
                typeName += templateParamName;
                prependSeparator = true;
            }
            typeName += '>';
            s_canonicalTypeName = typeName;
        }
        return s_canonicalTypeName;
    }
    template<typename T1>
    AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<AZStd::span<T1>>)
    {
        AZ::TypeId s_canonicalTypeId;
        if (s_canonicalTypeId.IsNull())
        {
            constexpr AZ::TypeId prefixUuid{ "{2FCDBAB3-45E0-4159-A91D-FD1D37056C0F}" };
            constexpr AZ::TypeId postfixUuid{};
            if constexpr (!prefixUuid.IsNull())
            {
                s_canonicalTypeId = prefixUuid + AZ::Internal::GetCanonicalTypeId<T1>();
            }
            else if constexpr (!postfixUuid.IsNull())
            {
                s_canonicalTypeId = AZ::Internal::GetCanonicalTypeId<T1>() + postfixUuid;
            }
            else
            {
                AZ_Assert(false, "Call to macro with template name %s, requires either a valid template uuid", "AZStd::span");
            }
        }
        return s_canonicalTypeId;
    }
    template<typename T1>
    AZ::TemplateId GetO3deClassTemplateId(AZ::Adl, AZStd::type_identity<AZStd::span<T1>>)
    {
        constexpr AZ::TypeId prefixUuid{ "{2FCDBAB3-45E0-4159-A91D-FD1D37056C0F}" };
        constexpr AZ::TypeId postfixUuid{};
        if constexpr (!prefixUuid.IsNull())
        {
            return prefixUuid;
        }
        else if constexpr (!postfixUuid.IsNull())
        {
            return postfixUuid;
        }
        else
        {
            return AZ::TemplateId{};
        }
    };
}

namespace std
{
    // AZStd::optional is std::optional brought into the AZStd:: namespace
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::optional, "AZStd::optional", "{AB8C50C0-23A7-4333-81CD-46F648938B1C}", AZ_TYPE_INFO_INTERNAL_TYPENAME);

    // Adding overloads for GetO3deTypeName/GetO3deTypeId/GetO3deClassTemplateId and GetO3deTemplateId in the std:: namespace since AZStd::tuple is just std::tuple brought into the AZStd namespace
    // This allows ADL to add those function to the list of function overloads when evaulating the overload set
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(AZStd::tuple, "AZStd::tuple", "{F99F9308-DC3E-4384-9341-89CBF1ABD51E}", AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS);
}

namespace AZ
{
    inline namespace DeprecatedTypeNames
    {
        // Visits the types deprecated name if it exist, otherwise visit the current type name
        template<class T, class Functor>
        static constexpr void VisitDeprecatedNameOrCurrentName(Functor&& visitCallback)
        {
            // Attempt to visit the deprecated name if a DeprecatedTypeNameVisitor exists
            bool deprecatedNameVisited{};
            auto VisitDeprecatedName = [&visitCallback, &deprecatedNameVisited](AZStd::string_view oldName)
            {
                visitCallback(oldName);
                deprecatedNameVisited = true;
            };
            DeprecatedTypeNameVisitor<T>(VisitDeprecatedName);

            // The type doesn't have a deprecated name so visit the current name
            if (!deprecatedNameVisited)
            {
                // const modifiers have been changed to be added after the type
                AZStd::fixed_string<512> oldName;

                using TypeNoQualifiers = AZStd::remove_cvref_t<T>;
                // Needed for the case when checking is_const on a `const int&`
                // the reference must be removed before checking the constantness
                using TypeNoReference = AZStd::remove_reference_t<T>;

                using ValueType = AZStd::conditional_t<AZStd::is_pointer_v<TypeNoQualifiers>,
                    AZStd::remove_pointer_t<TypeNoQualifiers>, T>;
                using ValueTypeNoQualifiers = AZStd::remove_cvref_t<ValueType>;

                // Needed for the case when checking is_const on a `const int* const&`
                using ValueTypeNoReference = AZStd::remove_reference_t<ValueType>;

                // Giving a `const char* const&` and a GetName() function which recursively retrieves
                // the type name until it reaches an unqualified value,
                //  the deprecated name would be calculated using the following steps
                // Call `GetName("const char* const&")`
                // Remove '&' from "const char* const&" and insert after GetName as`GetName("const char* const" ) + "&"`
                // Remove 'const' from "const char* const" and insert before GetName as `"const" + GetName("const char*" ) + "&"`
                // Remove '*' from "const char*" and insert after GetName as `"const" + GetName("const char") + "*" + "&"`
                // Remove 'const' from "const char" and insert before GetName as `"const" + "const" + GetName("char") + "*" + "&"`
                // Resolve 'GetName("char")' as "char" -> `"const" + "const" + "char" + "*" + "&"
                // The final result is "const const char*&".
                // NOTE: "const const char*&" isn't a valid identifier due a grammar error with having two const in a row

                // This is the const if the pointer type is constant
                if constexpr (AZStd::is_pointer_v<TypeNoQualifiers> && AZStd::is_const_v<TypeNoReference>)
                {
                    oldName += "const ";
                }
                // This is the const if the value type is constant
                if constexpr (AZStd::is_const_v<ValueTypeNoReference>)
                {
                    oldName += "const ";
                }

                // The raw type name provided through the GetO3deTypeName overload
                oldName += AZ::AzTypeInfo<ValueTypeNoQualifiers>::Name();

                // If the type is a pointer it is append afterwards
                if constexpr (AZStd::is_pointer_v<TypeNoQualifiers>)
                {
                    oldName += "*";
                }

                // As C++ doesn't support having pointer to a reference, a reference qualifier is always
                // at the end
                if constexpr (AZStd::is_lvalue_reference_v<T> || AZStd::is_lvalue_reference_v<ValueType> )
                {
                    oldName += "&";
                }
                else if constexpr (AZStd::is_rvalue_reference_v<T> || AZStd::is_rvalue_reference_v<ValueType>)
                {
                    oldName += "&&";
                }

                visitCallback(oldName);
            }
        }
        template<class T>
        static constexpr void AggregateTypeNameOld(char* buffer, size_t bufferSize)
        {
            auto AppendDeprecatedName = [buffer, bufferSize](AZStd::string_view typeName)
            {
                AZ::Internal::AzTypeInfoSafeCat(buffer, bufferSize, typeName.data());
                // Old Aggregate TypeName logic always placed a space after each argument
                AZ::Internal::AzTypeInfoSafeCat(buffer, bufferSize, " ");
            };
            VisitDeprecatedNameOrCurrentName<T>(AppendDeprecatedName);
        }

        template<class R, class... Args>
        struct AzDeprecatedTypeNameVisitor<R(Args...)>
        {
            template<class Functor>
            constexpr void operator()(Functor&& visitCallback) const
            {
                // Raw functions pointer used to be stored in a 64 byte buffer
                AZStd::array<char, 64> deprecatedName{};

                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), "{");

                // Tracks if the return type deprecated name was found. If so the current type name isn't used
                auto AppendDeprecatedName = [&deprecatedName](AZStd::string_view retTypeDeprecatedName)
                {
                    AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(),
                        retTypeDeprecatedName.data());
                };
                VisitDeprecatedNameOrCurrentName<R>(AZStd::move(AppendDeprecatedName));

                // Use the old aggregate logic to append the function parameters
                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), "(");
                (AggregateTypeNameOld<Args>(deprecatedName.data(), deprecatedName.size()), ...);
                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), ")}");

                visitCallback(deprecatedName.data());
            }
        };
    }
}

// Adds typeinfo specialization for tuple type
// Done outside of tuple.h to avoid it including TypeInfo.h which cause a circular include in fixed_string.inl
// since it indirectly includes tuple.h via common_view.h -> all_view.h -> owning_view.h -> ranges_adaptor.h -> tuple.h
namespace AZ
{
    // Specialize the AzDeprcatedTypeNameVisitor for tuple to make sure their
    // is a mapping of the old type name to current type id
    inline namespace DeprecatedTypeNames
    {
        template<typename... Types>
        struct AzDeprecatedTypeNameVisitor<AZStd::tuple<Types...>>
        {
            template<class Functor>
            constexpr void operator()(Functor&& visitCallback) const
            {
                // AZStd::tuple previous name was place into a buffer of size 128
                AZStd::array<char, 128> deprecatedName{};

                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), "tuple<");
                (AggregateTypeNameOld<Types>(deprecatedName.data(), deprecatedName.size()), ...);
                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), ">");

                visitCallback(deprecatedName.data());
            }
        };
    }

}

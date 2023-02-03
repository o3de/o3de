/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/sp_convertible.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/extent.h>
#include <AzCore/std/typetraits/remove_extent.h>
#include <AzCore/std/utils.h>
#include <AzCore/Memory/Memory.h>
#include <memory>

namespace AZStd
{
    /// 20.7.12.2 unique_ptr for single objects.
    template <typename T, typename Deleter = std::default_delete<T> >
    using unique_ptr = std::unique_ptr<T, Deleter>;

    template<class T>
    struct hash;

    template <typename T, typename Deleter>
    struct hash<unique_ptr<T, Deleter>>
    {
        typedef unique_ptr<T, Deleter> argument_type;
        typedef AZStd::size_t result_type;
        inline result_type operator()(const argument_type& value) const
        {
            return std::hash<argument_type>()(value);
        }
    };

    template<typename T, typename... Args>
    AZStd::enable_if_t<!AZStd::is_array<T>::value && AZ::HasAZClassAllocator<T>::value, unique_ptr<T>> make_unique(Args&&... args)
    {
        return AZStd::unique_ptr<T>(aznew T(AZStd::forward<Args>(args)...));
    }

    template<typename T, typename... Args>
    AZStd::enable_if_t<!AZStd::is_array<T>::value && !AZ::HasAZClassAllocator<T>::value, unique_ptr<T>> make_unique(Args&&... args)
    {
        return AZStd::unique_ptr<T>(new T(AZStd::forward<Args>(args)...));
    }

    // The reason that there is not an array aznew version version of make_unique is because AZClassAllocator does not support array new
    template<typename T>
    AZStd::enable_if_t<AZStd::is_array<T>::value && AZStd::extent<T>::value == 0, unique_ptr<T>> make_unique(std::size_t size)
    {
        return AZStd::unique_ptr<T>(new typename AZStd::remove_extent<T>::type[size]());
    }

    template<typename T, typename... Args>
    AZStd::enable_if_t<AZStd::is_array<T>::value && AZStd::extent<T>::value != 0, unique_ptr<T>> make_unique(Args&&... args) = delete;

    // GetAzTypeInfo overload for AZStd::unique_ptr
    // The Deleter is not part of the TypeInfo
    template<class T, class Deleter>
    constexpr AZ::TypeInfoObject GetAzTypeInfo(AZStd::type_identity<unique_ptr<T, Deleter>>)
    {
        using Type = AZStd::unique_ptr<T, Deleter>;
        AZStd::fixed_string<512> typeName{ "AZStd::unique_ptr<" };
        typeName += AZ::AzTypeInfo<T>::Name();
        typeName += '>';

        AZ::TypeTraits typeTraits{};
        typeTraits |= AZStd::is_signed_v<Type> ? AZ::TypeTraits::is_signed : AZ::TypeTraits{};
        typeTraits |= AZStd::is_unsigned_v<Type> ? AZ::TypeTraits::is_unsigned : AZ::TypeTraits{};

        AZ::TypeInfoObject typeInfoObject;
        typeInfoObject.m_name = typeName;
        typeInfoObject.m_templateId = AZ::TypeId("{B55F90DA-C21E-4EB4-9857-87BE6529BA6D}");
        typeInfoObject.m_canonicalTypeId = typeInfoObject.m_templateId + AZ::AzTypeInfo<T>::GetCanonicalTypeId();
        typeInfoObject.m_pointerTypeId = typeInfoObject.m_canonicalTypeId + AZ::Internal::PointerId_v;
        typeInfoObject.m_typeTraits = typeTraits;
        typeInfoObject.m_typeSize = sizeof(Type);
    }
} // namespace AZStd

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
    // An alias template is a name for a family of types (https://eel.is/c++draft/temp.alias)
    // It is not related to any template that might be used on the right hand side of the declaration
    // Furthermore alias template are not deduced as part of template argument deduction
    // https://eel.is/c++draft/temp.alias#note-1
    // So instead of using an alias template
    // a using declaration is used to bring the std::unique_ptr into the AZStd:: namespace
    // https://eel.is/c++draft/namespace.udecl
    // This allows the exactly the name"AZStd::unique_ptr" to be an alias of the name "std::unique_ptr"

    /// 20.7.12.2 unique_ptr for single objects.
    using std::unique_ptr;

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

} // namespace AZStd

namespace std
{
    // Add TypeInfo overloads for AZStd::unique_ptr
    // The Deleter is not part of the TypeInfo
    // The POSTFIX version of aggregating the Uuid together with the class template argument T is used to maintain backwards
    // compatibility with existing `unique_ptr<T>` serialized data
    // The Uuid was previously calcaluated using postfixing the unique_ptr template identifier to end
    // `AzTypeInfo<T>::Uuid() + AZ::Uuid("{B55F90DA-C21E-4EB4-9857-87BE6529BA6D}");`
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZStd::unique_ptr, "AZStd::unique_ptr", "{B55F90DA-C21E-4EB4-9857-87BE6529BA6D}", AZ_TYPE_INFO_INTERNAL_TYPENAME);
}

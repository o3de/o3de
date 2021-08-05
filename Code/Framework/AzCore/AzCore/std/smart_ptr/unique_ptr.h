/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_SMART_PTR_UNIQUE_PTR_H
#define AZSTD_SMART_PTR_UNIQUE_PTR_H

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

} // namespace AZStd

namespace AZ
{
    AZ_TYPE_INFO_INTERNAL_VARIATION_SPECIALIZATION_2_CONCAT_1(AZStd::unique_ptr, "{B55F90DA-C21E-4EB4-9857-87BE6529BA6D}", T, Deleter);
}

#endif // AZSTD_SMART_PTR_UNIQUE_PTR_H
#pragma once

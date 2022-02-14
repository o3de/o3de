/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/ranges.h>

namespace AZStd::ranges
{
    template<class T, class = void>
    class empty_view;

    template<class T>
    class empty_view<T, enable_if_t<is_object_v<T>>>
        : public view_interface<empty_view<T>>
    {
    public:
        static constexpr T* begin() noexcept { return nullptr; }
        static constexpr T* end() noexcept { return nullptr; }
        static constexpr T* data() noexcept { return nullptr; }
        static constexpr size_t size() noexcept { return 0; }
        static constexpr bool empty() noexcept { return true; }
    };

    namespace views
    {
        template<class T>
        constexpr empty_view<T> empty{};
    }

    template<class T>
    inline constexpr bool enable_borrowed_range<empty_view<T>> = true;
}

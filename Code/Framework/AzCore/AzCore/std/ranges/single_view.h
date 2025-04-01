/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges
{
    template<class T>
    class single_view;

    namespace views
    {
        namespace Internal
        {
            struct single_view_fn
            {
                template <class View>
                constexpr decltype(auto) operator()(View&& view) const
                {
                    return single_view<decay_t<View>>(AZStd::forward<View>(view));
                }
            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::single_view_fn single{};
        }
    }

    template<class T>
    class single_view
        : public enable_if_t<move_constructible<T> && is_object_v<T>, view_interface<single_view<T>>>
    {
    public:

        template<class T2 = T, class = enable_if_t<default_initializable<T2>>>
        single_view() {}
        constexpr explicit single_view(const T& t)
            : m_value(in_place, t)
        {}
        constexpr explicit single_view(T&& t)
            : m_value(in_place, AZStd::move(t))
        {}
        template<class... Args, enable_if_t<constructible_from<T, Args...>>* = nullptr>
        constexpr explicit single_view(in_place_t, Args&&... args)
            : m_value{ in_place, AZStd::forward<Args>(args)... }
        {}

        constexpr T* begin() noexcept
        {
            return data();
        }
        constexpr const T* begin() const noexcept
        {
            return data();
        }
        constexpr T* end() noexcept
        {
            return data() + 1;
        }
        constexpr const T* end() const noexcept
        {
            return data() + 1;
        }
        static constexpr size_t size() noexcept
        {
            return 1;
        }
        constexpr T* data() noexcept
        {
            return m_value.operator->();
        }
        constexpr const T* data() const noexcept
        {
            return m_value.operator->();
        }
    private:
        Internal::movable_box<T> m_value;
    };

    template<class T>
    single_view(T) -> single_view<T>;
}

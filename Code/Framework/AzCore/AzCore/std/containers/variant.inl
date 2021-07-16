/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZStd
{
    // Variant constructor #1
    template <class... Types>
    template <typename, enable_if_t<is_default_constructible<variant_alternative_t<0, variant<Types...>>>::value, size_t>>
    inline constexpr variant<Types...>::variant()
        : m_impl(in_place_index_t<0>{})
    {
    }

    // Variant constructor #4
    template <class... Types>
    template <class T,
        enable_if_t<!is_same<remove_cvref_t<T>, variant<Types...>>::value, int>,
        enable_if_t<!is_same<remove_cvref_t<T>, in_place_type_t<remove_cvref_t<T>>>::value, int>,
        enable_if_t<!is_same<remove_cvref_t<T>, Internal::is_in_place_index_t<remove_cvref_t<T>>>::value, int>,
        enable_if_t<variant_size<variant<Types...>>::value != 0, int>,
        class Alternative,
        size_t Index,
        enable_if_t<is_constructible<Alternative, T>::value, int>>
    inline constexpr variant<Types...>::variant(T&& arg)
        : m_impl(in_place_index_t<Index>{}, AZStd::forward<T>(arg))
    {
    }

    // Variant constructor #5
    template <class... Types>
    template <class T, class... Args,
        size_t Index,
        enable_if_t<is_constructible<T, Args...>::value, int>>
    inline constexpr variant<Types...>::variant(in_place_type_t<T>, Args&&... args)
        : m_impl(in_place_index_t<Index>{}, AZStd::forward<Args>(args)...)

    {
    }

    // Variant constructor #6
    template <class... Types>
    template <class T, class U, class... Args,
        size_t Index,
        enable_if_t<is_constructible<T, std::initializer_list<U>&, Args...>::value, int>>
    inline constexpr variant<Types...>::variant(in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
        : m_impl(in_place_index_t<Index>{}, il, AZStd::forward<Args>(args)...)
    {
    }

    // Variant constructor #7
    template <class... Types>
    template <size_t Index, class... Args,
        class,
        class Alternative,
        enable_if_t<is_constructible<Alternative, Args...>::value, int>>
    inline constexpr variant<Types...>::variant(in_place_index_t<Index>, Args&&... args)
        : m_impl(in_place_index_t<Index>{}, AZStd::forward<Args>(args)...)
    {
    }

    // Variant constructor #8
    template <class... Types>
    template <size_t Index, class U, class... Args,
        enable_if_t<(Index < variant_size<variant<Types...>>::value), int>,
        class Alternative,
        enable_if_t<is_constructible<Alternative, std::initializer_list<U>&, Args...>::value, int>>
    inline constexpr variant<Types...>::variant(in_place_index_t<Index>, std::initializer_list<U> il, Args&&... args)
        : m_impl(in_place_index_t<Index>{}, il, AZStd::forward<Args>(args)...)
    {
    }

    // Variant assignment operator #3
    template <class... Types>
    template <class T, enable_if_t<!is_same<remove_cvref_t<T>, variant<Types...>>::value, int>, class Alternative, size_t Index, enable_if_t<is_assignable<Alternative&, T>::value && is_constructible<Alternative, T>::value, int>>
    inline auto variant<Types...>::operator=(T&& arg) -> variant&
    {
        m_impl.template assign<Index>(AZStd::forward<T>(arg));
        return *this;
    }

    // std::variant member functions
    // Variant emplace #1
    template <class... Types>
    template <class T, class... Args, size_t Index, enable_if_t<is_constructible<T, Args...>::value, int> >
    inline T& variant<Types...>::emplace(Args&&... args)
    {
        return m_impl.template emplace<Index>(AZStd::forward<Args>(args)...);
    }

    // Variant emplace #2
    template <class... Types>
    template <class T, class U, class... Args, size_t Index, enable_if_t<is_constructible<T, std::initializer_list<U>&, Args...>::value, int>>
    inline T& variant<Types...>::emplace(std::initializer_list<U> il, Args&&... args)
    {
        return m_impl.template emplace<Index>(il, AZStd::forward<Args>(args)...);
    }

    // Variant emplace #3
    template <class... Types>
    template <size_t Index, class... Args, enable_if_t<(Index < variant_size<variant<Types...>>::value), int>, class Alternative, enable_if_t<is_constructible<Alternative, Args...>::value, int>>
    inline Alternative& variant<Types...>::emplace(Args&&... args)
    {
        return m_impl.template emplace<Index>(AZStd::forward<Args>(args)...);
    }

    // Variant emplace #4
    template <class... Types>
    template <size_t Index, class U, class... Args, enable_if_t<(Index < variant_size<variant<Types...>>::value), int>, class Alternative, enable_if_t<is_constructible<Alternative, std::initializer_list<U>&, Args...>::value, int>>
    inline Alternative& variant<Types...>::emplace(std::initializer_list<U> il, Args&&... args)
    {
        return m_impl.template emplace<Index>(il, AZStd::forward<Args>(args)...);
    }

    template <class... Types>
    inline constexpr bool variant<Types...>::valueless_by_exception() const
    {
        return m_impl.valueless_by_exception();
    }

    template <class... Types>
    inline constexpr size_t variant<Types...>::index() const
    {
        return m_impl.index();
    }

    template <class... Types>
    template <bool Placeholder, enable_if_t<conjunction<bool_constant<Placeholder && is_swappable<Types>::value && is_move_constructible<Types>::value>...>::value, bool>>
    inline void variant<Types...>::swap(variant& other)
    {
        m_impl.swap(other.m_impl);
    }

    // Variant holds_alternative function
    template <class T, class... Types>
    inline constexpr bool holds_alternative(const variant<Types...>& variantInst)
    {
        return variant_detail::holds_alternative_at_index<find_type::find_exactly_one_alternative_v<T, Types...>>(variantInst);
    }

    // Variant AZStd::get<variant> specializations
    template <size_t Index, class... Types>
    inline constexpr variant_alternative_t<Index, variant<Types...>>& get(variant<Types...>& variantInst)
    {
        static_assert(Index < sizeof...(Types), "index is out of bounds of variant alternatives");
        static_assert(!is_void_v<variant_alternative_t<Index, variant<Types...>>>, "Cannot retrieve a variant with alternative void type");
        return variant_detail::generic_get<Index>(variantInst);
    }

    template <size_t Index, class... Types>
    inline constexpr variant_alternative_t<Index, variant<Types...>>&& get(variant<Types...>&& variantInst)
    {
        static_assert(Index < sizeof...(Types), "index is out of bounds of variant alternatives");
        static_assert(!is_void_v<variant_alternative_t<Index, variant<Types...>>>, "Cannot retrieve a variant with alternative void type");
        return variant_detail::generic_get<Index>(AZStd::move(variantInst));
    }

    template <size_t Index, class... Types>
    inline constexpr const variant_alternative_t<Index, variant<Types...>>& get(const variant<Types...>& variantInst)
    {
        static_assert(Index < sizeof...(Types), "index is out of bounds of variant alternatives");
        static_assert(!is_void_v<variant_alternative_t<Index, variant<Types...>>>, "Cannot retrieve a variant with alternative void type");
        return variant_detail::generic_get<Index>(variantInst);
    }

    template <size_t Index, class... Types>
    inline constexpr const variant_alternative_t<Index, variant<Types...>>&& get(const variant<Types...>&& variantInst)
    {
        static_assert(Index < sizeof...(Types), "index is out of bounds of variant alternatives");
        static_assert(!is_void_v<variant_alternative_t<Index, variant<Types...>>>, "Cannot retrieve a variant with alternative void type");
        return variant_detail::generic_get<Index>(AZStd::move(variantInst));
    }

    template <class T, class... Types>
    inline constexpr T& get(variant<Types...>& variantInst)
    {
        static_assert(!is_void_v<T>, "Cannot retrieve a variant with alternative void type");
        return get<find_type::find_exactly_one_alternative_v<T, Types...>>(variantInst);
    }

    template <class T, class... Types>
    inline constexpr T&& get(variant<Types...>&& variantInst)
    {
        static_assert(!is_void_v<T>, "Cannot retrieve a variant with alternative void type");
        return get<find_type::find_exactly_one_alternative_v<T, Types...>>(AZStd::move(variantInst));
    }

    template <class T, class... Types>
    inline constexpr const T& get(const variant<Types...>& variantInst)
    {
        static_assert(!is_void_v<T>, "Cannot retrieve a variant with alternative void type");
        return get<find_type::find_exactly_one_alternative_v<T, Types...>>(variantInst);
    }

    template <class T, class... Types>
    inline constexpr const T&& get(const variant<Types...>&& variantInst)
    {
        static_assert(!is_void_v<T>, "Cannot retrieve a variant with alternative void type");
        return get<find_type::find_exactly_one_alternative_v<T, Types...>>(AZStd::move(variantInst));
    }

    template <size_t Index, class... Types>
    inline constexpr add_pointer_t<variant_alternative_t<Index, variant<Types...>>> get_if(variant<Types...>* variantInst)
    {
        static_assert(Index < sizeof...(Types), "index is out of bounds of variant");
        static_assert(!is_void_v<variant_alternative_t<Index, variant<Types...>>>, "Cannot retrieve a variant with alternative void type");
        return variant_detail::generic_get_if<Index>(variantInst);
    }

    template <size_t Index, class... Types>
    inline constexpr add_pointer_t<const variant_alternative_t<Index, variant<Types...>>> get_if(const variant<Types...>* variantInst)
    {
        static_assert(Index < sizeof...(Types), "index is out of bounds of variant");
        static_assert(!is_void_v<variant_alternative_t<Index, variant<Types...>>>, "Cannot retrieve a variant with alternative void type");
        return variant_detail::generic_get_if<Index>(variantInst);
    }

    template <class T, class... Types>
    inline constexpr add_pointer_t<T> get_if(variant<Types...>* variantInst)
    {
        static_assert(!is_void_v<T>, "Cannot retrieve a variant with alternative void type");
        return get_if<find_type::find_exactly_one_alternative_v<T, Types...>>(variantInst);
    }

    template <class T, class... Types>
    inline constexpr add_pointer_t<const T> get_if(const variant<Types...>* variantInst)
    {
        static_assert(!is_void_v<T>, "Cannot retrieve a variant with alternative void type");
        return get_if<find_type::find_exactly_one_alternative_v<T, Types...>>(variantInst);
    }

    // Variant comparison operators and swap
    template <class... Types>
    inline constexpr bool operator==(const variant<Types...>& lhs, const variant<Types...>& rhs)
    {
        return lhs.index() == rhs.index() && (lhs.valueless_by_exception()
            || variant_detail::visitor::variant::visit_value_at(lhs.index(),
                [](auto&& altLeft, auto&& altRight) -> bool
                {
                    return altLeft == altRight;
                },
                lhs, rhs));
    }

    template <class... Types>
    inline constexpr bool operator!=(const variant<Types...>& lhs, const variant<Types...>& rhs)
    {
        return !operator==(lhs, rhs);
    }

    template <class... Types>
    inline constexpr bool operator<(const variant<Types...>& lhs, const variant<Types...>& rhs)
    {
        return !rhs.valueless_by_exception() && (lhs.valueless_by_exception() || lhs.index() < rhs.index()
            || (lhs.index() == rhs.index() && variant_detail::visitor::variant::visit_value_at(lhs.index(),
                [](auto&& altLeft, auto&& altRight) -> bool
                {
                    return altLeft < altRight;
                },
                lhs, rhs)));
    }

    template <class... Types>
    inline constexpr bool operator>(const variant<Types...>& lhs, const variant<Types...>& rhs)
    {
        return operator<(rhs, lhs);
    }

    template <class... Types>
    inline constexpr bool operator<=(const variant<Types...>& lhs, const variant<Types...>& rhs)
    {
        return !operator>(lhs, rhs);
    }

    template <class... Types>
    inline constexpr bool operator>=(const variant<Types...>& lhs, const variant<Types...>& rhs)
    {
        return !operator<(lhs, rhs);
    }

    template <typename... Types>
    inline void swap(variant<Types...>& lhs, variant<Types...>& rhs)
    {
        lhs.swap(rhs);
    }

    template <class Visitor, class... VariantTypes>
    inline constexpr decltype(auto) visit(Visitor&& visitor, VariantTypes&&... variants)
    {
        // The following code validates that a variant that is valueless due to an exception
        // being thrown in one of the alternative constructor is not being supplied to Visit
        return variant_detail::visitor::variant::visit_value(AZStd::forward<Visitor>(visitor), AZStd::forward<VariantTypes>(variants)...);
    }

    template <class R, class Visitor, class... VariantTypes>
    inline constexpr R visit(Visitor&& visitor, VariantTypes&&... variants)
    {
        // The following code validates that a variant that is valueless due to an exception
        // being thrown in one of the alternative constructor is not being supplied to Visit
        return variant_detail::visitor::variant::visit_value_r<R>(AZStd::forward<Visitor>(visitor), AZStd::forward<VariantTypes>(variants)...);
    }

    // monostate comparison functions
    inline constexpr bool operator<(monostate, monostate)
    {
        return false;
    }
    inline constexpr bool operator>(monostate, monostate)
    {
        return false;
    }
    inline constexpr bool operator<=(monostate, monostate)
    {
        return true;
    }
    inline constexpr bool operator>=(monostate, monostate)
    {
        return true;
    }
    inline constexpr bool operator==(monostate, monostate)
    {
        return true;
    }
    constexpr bool operator!=(monostate, monostate)
    {
        return false;
    }
} // namespace AZStd

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/remove_cvref.h>

/* Microsoft C++ ABI puts 1 byte of padding between each empty base class when multiple inheritance is being used
 * As compressed_pair can inherits from two potentially empty base classes it would put the second empty base class at offset 1.
 *
 * Luckily in Visual Studio 2015 Update 2 an attribute was added to allow VS2015 to take full advantage
 * of the Empty Base Class Optimization and not add padding between multiple empty sub objects
 * This is detail on the Visual Studio blog https://devblogs.microsoft.com/cppblog/optimizing-the-layout-of-empty-base-classes-in-vs2015-update-2-3/
 */
#if defined(AZ_COMPILER_MSVC)
#define AZSTD_COMPRESSED_PAIR_EMPTY_BASE_OPTIMIZATION __declspec(empty_bases)
#else
#define AZSTD_COMPRESSED_PAIR_EMPTY_BASE_OPTIMIZATION 
#endif

namespace AZStd
{
    /*
     * compressed_pair class is used to take advantage of the Empty Base Optimization(ebo)
     * to prevent empty classes from counting against the size of a non-empty class
     * The C++ standard mandates that all complete objects have a non-zero size
     * unless the class is a base class sub object or the class has been decorated with
     * the C++17 no_unique_address attribute is used
     * therefore an empty class in C++ has size of 1 byte.
     * From the C++20 draft chapter 10, note 5
     * "[Note: Complete objects of class type have nonzero size. Base class subobjects and "
     * "members declared with the no_unique_address attribute ([dcl.attr.nouniqueaddr]) are not so constrained. -end note]"
     * The Index template parameter is used to disambiguate a compressed pair containing multiple elements of the same types
     * This is used to allow multiple inheritance from 2 of the same types underlying elements
     */
    template <typename T, size_t Index, bool CanBeEmptyBase = std::is_empty<T>::value && !std::is_final<T>::value>
    struct compressed_pair_element
    {
        using value_type = T;

        constexpr compressed_pair_element() = default;

        template <typename U, typename = AZStd::enable_if_t<!AZStd::is_same<AZStd::remove_cvref_t<U>, compressed_pair_element>::value>>
        constexpr explicit compressed_pair_element(U&& value);

        template <template <typename...> class TupleType, class... Args, size_t... Indices>
        constexpr compressed_pair_element(AZStd::piecewise_construct_t, TupleType<Args...>&& args,
            AZStd::index_sequence<Indices...>);

        constexpr T& get();
        constexpr const T& get() const;

    private:
        T m_element{};
    };

    /*
     * Empty class is not final so it can be used as a base class
     */
    template <typename T, size_t Index>
    struct compressed_pair_element<T, Index, true>
        : private T
    {
        using value_type = T;

        constexpr compressed_pair_element() = default;

        template <typename U, typename = AZStd::enable_if_t<!AZStd::is_same<AZStd::remove_cvref_t<U>, compressed_pair_element>::value>>
        constexpr explicit compressed_pair_element(U&& value);

        template <template <typename...> class TupleType, class... Args, size_t... Indices>
        constexpr compressed_pair_element(AZStd::piecewise_construct_t, TupleType<Args...>&& args, AZStd::index_sequence<Indices...>);

        constexpr T& get();
        constexpr const T& get() const;
    };

    // Structure to pass as the first element when only the second element of the pair
    // needs to be constructed
    struct skip_element_tag
    {
    };

    template <class T1, class T2>
    class AZSTD_COMPRESSED_PAIR_EMPTY_BASE_OPTIMIZATION compressed_pair
        : private compressed_pair_element<T1, 0>
        , private compressed_pair_element<T2, 1>
    {
        using first_base_type = compressed_pair_element<T1, 0>;
        using second_base_type = compressed_pair_element<T2, 1>;
        using first_base_value_type = typename first_base_type::value_type;
        using second_base_value_type = typename second_base_type::value_type;

    public:
        // First template argument is a placeholder argument of void as MSVC examines the types
        // of a templated function to determine if they are the same template
        // Due to the "template <class T> compressed_pair(skip_element_tag, T&&)"
        // constructor below, the default constructor template types needs to be distinguished from it
        template <typename = void, typename = AZStd::enable_if_t<
            AZStd::is_default_constructible<first_base_value_type>::value
            && AZStd::is_default_constructible<second_base_value_type>::value>>
        constexpr compressed_pair();

        template <typename T, AZStd::enable_if_t<!is_same<remove_cvref_t<T>, compressed_pair>::value, bool> = true>
        constexpr explicit compressed_pair(T&& firstElement);

        template <typename T>
        constexpr compressed_pair(skip_element_tag, T&& secondElement);

        template <typename U1, typename U2>
        constexpr compressed_pair(U1&& firstElement, U2&& secondElement);

        template <template <typename...> class TupleType, typename... Args1, typename... Args2>
        constexpr compressed_pair(piecewise_construct_t piecewiseTag, TupleType<Args1...>&& firstArgs, TupleType<Args2...>&& secondArgs);

        constexpr auto first() -> first_base_value_type&;
        constexpr auto first() const -> const first_base_value_type&;
        constexpr auto second() -> second_base_value_type&;
        constexpr auto second() const -> const second_base_value_type&;

        void swap(compressed_pair& other);
    };

    template <class T1, class T2>
    void swap(compressed_pair<T1, T2>& lhs, compressed_pair<T1, T2>& rhs);
} // namespace AZStd

#include <AzCore/std/containers/compressed_pair.inl>

// undefine Visual Studio Empty Base Class Optimization Macro
#undef AZSTD_COMPRESSED_PAIR_EMPTY_BASE_OPTIMIZATION
// undefine Visual Studio 2015 constexpr workaround macro

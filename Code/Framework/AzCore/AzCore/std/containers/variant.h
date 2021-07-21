/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/variant_impl.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/is_swappable.h>

/* Microsoft C++ ABI puts 1 byte of padding between each empty base class when multiple inheritance is being used
 * AZStd::variant implementation inherits from both make_constructor_overloads and make_assignment_overloads
 * which are both empty base classes. The second class make_assignment_overloads was put at offset 1.
 * Therefore the AZStd::variant_detail::impl class is placed at offset 8 in order to be aligned and causes
 * variants to be 8 bytes larger.
 * In Visual Studio 2015 Update 2 an attribute was added to allow VS2015 to take full advantage
 * of the Empty Base Class Optimization and not add padding between multiple empty subojects
 * This is detail on the Visual Studio blog https://devblogs.microsoft.com/cppblog/optimizing-the-layout-of-empty-base-classes-in-vs2015-update-2-3/
 * Windows Clang also requires this attribute as by default it generates classes structs which are compatible with Microsoft C++ ABI
 */
#if defined(AZ_COMPILER_MSVC) || (defined(AZ_COMPILER_CLANG) && defined(AZ_PLATFORM_WINDOWS))
#define AZSTD_VARIANT_EMPTY_BASE_OPTIMIZATION __declspec(empty_bases)
#else
#define AZSTD_VARIANT_EMPTY_BASE_OPTIMIZATION
#endif

namespace AZStd
{
    /*
     * std::variant is implemented according to the c++20 draft avialable at http://eel.is/c++draft/variant
     * variant acts as a type safe structure that wraps several types in a union
     * A variant object holds and manages the lifetime of one of its template types
     * A variant holds either the value of one of its alternatives(i.e Types...) or
     * no value at all. The template arguments are called alternatives.
     * 
     * The implementation is made to be as lightweight as possible by only requiring
     * space for the union to contain the type elements and an index which tracks
     * active alternative. The sizeof the index is dependent on the number of alternatives.
     * When less than 255 alternatives are stored in a variant only a single byte is used 
     * to store the index
     *
     * A use case for an variant is replacing a c style union of types and an enum
     * which keeps track of the active member.
     * The drawbacks of using a variant to using c style union is 
     * 1. The inability to name the data members
     * 2. The inability to store raw c style arrays within the variant(std::array can be used instead to mitigate this issue)
     * ```
     * /// SerializationType as a c-style union
     * union SerializationType
     * {
     *     char* m_string;
     *     float m_color[4];
     * }
     * /// SerializationType as variant
     * using SerializationType = AZStd::variant<char*, AZStd::array<float, 4>>;
     * ```
     * 
     * For example a variant can be used to implement a restricted set of types for serialization and marshalling.
     * So if the requirements for the marshaling system is that it would be able to serialize booleans, numbers, strings
     * and binary data a variant with the types of `AZStd::variant<double, int64_t, AZStd::string, AZStd::vector<uint8_t>>`
     * can be used to enforce that only those subset of types can be marshaled.
     *
     * variant and any share much of the same uses cases and there are pros and cons to using both
     * any pros:
     * 1. It can be used to store any C++ type that has AzTypeInfo associated with it.
     * 2. It is not a templated class so there is no additional compile overhead for using an any
     * 3. It can be used in algorithms that require type-erasure. An example is storing a set of factory functions
     *    that all return an any, but the functions themselves are templated.
     *  Ex.
     *  using AnyFactoryCallback = AZStd::any(*)();
     *  AnyFactoryCallback cb = &CreateInstance<Foo>;
     *  cb = &CreateInstance<Bar>;
     * any cons:
     * 1. As it can store any type it is not possible to create restrictions on the acceptable types at compile time
     * 2. Storing an type instance cannot be done at compile time. All instance data is stored at run time
     * 3. Due to not being a templated class it is not possible to perform compile time validation of the type stored
     *    within the any. Therefore any checks that the proper type is being accessed happens at runtime
     * 4. Also due to not being templated, it maintains the storage for the object using aligned_storage.
     *    This increases the size of any as it has to maintain the storage buffer as well as the typeid UUID within it
     *    Furthermore This makes it hard to add debug visualizers for visualizing the current type of element stored in the any/
     * 5. Requires the use of the heap if an object is type is stored within it that is larger than it's internal small object buffer
     * 
     * variant pros:
     * 1. It restricts the set of types that can be stored within it at compile time.
     *    This allows it to optimize the space required to store the types to that of the largest type
     * 2. It supports storing an alternative at compile time and is therefore usable in constexpr.
     * 3. It performs type validation of the element at compile, enforcing that the alternative being returned  
     *    is of the correct type(though whether that alternative is active can depend on run time code)
     * 4. Due to being a template class, the types stored within the variant can be used in algorithm at compile time
     *    For example serialization of a :variant can be coded to cast to the type of active alternative
     *    and serialize that directly.
     *    For an any it requires the typeid UUID and a pointer to the stored instance to serialize
     * 5. Does not use heap memory at all.
     * variant cons:
     * 1. There is a cost actual time it takes to compile as the number of alternatives increase due to the need to recursively
     *    instantiate the underlying union class
     * 2. When the index of the active alternative is not known at compile time a visit must be used to access the active alternative
     *    this requires a visitor function to be supplied
     *
     * A use case of using an any vs a variant is when implementing a visual scripting language(on some kind of canvas)
     * An any can be used for marshaling the data between C++ and the visual scripting language, while a variant can be
     * used for serializing a restricted set of types supported by the language to disk
     */
    template <class... Types>
    class AZSTD_VARIANT_EMPTY_BASE_OPTIMIZATION variant
        : private variant_detail::make_constructor_overloads<conjunction_v<is_copy_constructible<Types>...>, conjunction_v<is_move_constructible<Types>...>>
        , private variant_detail::make_assignment_overloads<conjunction_v<is_copy_assignable<Types>...>, conjunction_v<is_move_assignable<Types>...>>
    {
        static constexpr size_t num_alternatives = sizeof...(Types);
        static_assert(num_alternatives != 0, "variant must contain at least one alternative.");
        static_assert(!disjunction_v<is_reference<Types>...>, "variant can not have a reference type as an alternative. Per C++ draft standard 19.7.3 Note 1");
        static_assert(!disjunction_v<is_array<Types>...>, "variant can not have an array type as an alternative. Per C++ draft standard 19.7.3 Note 2");
        static_assert(!disjunction_v<is_void<Types>...>, "variant can not have a void type as an alternative. Per C++ draft standard 19.7.3 Note 2");

    public:
        // Variant constructor #1
        template <typename = void, enable_if_t<is_default_constructible<variant_alternative_t<0, variant>>::value, size_t> = 0>
        constexpr variant();
        // Variant constructor #2
        variant(const variant&) = default;
        // Variant constructor #3
        variant(variant&&) = default;
        // Variant constructor #4
        template <class T,
            enable_if_t<!is_same<remove_cvref_t<T>, variant>::value, int> = 0,
            enable_if_t<!is_same<remove_cvref_t<T>, in_place_type_t<remove_cvref_t<T>>>::value, int> = 0,
            enable_if_t<!is_same<remove_cvref_t<T>, Internal::is_in_place_index_t<remove_cvref_t<T>>>::value, int> = 0,
            enable_if_t<variant_size<variant>::value != 0, int> = 0,
            class Alternative = variant_detail::best_alternative_t<T, Types...>,
            size_t Index = find_type::find_exactly_one_alternative_v<Alternative, Types...>,
            enable_if_t<is_constructible<Alternative, T>::value, int> = 0>
        constexpr variant(T&& arg);

        // Variant constructor #5
        template <class T, class... Args,
            size_t Index = find_type::find_exactly_one_alternative_v<T, Types...>,
            enable_if_t<is_constructible<T, Args...>::value, int> = 0>
        explicit constexpr variant(in_place_type_t<T>, Args&&... args);

        // Variant constructor #6
        template <class T, class U, class... Args,
            size_t Index = find_type::find_exactly_one_alternative_v<T, Types...>,
            enable_if_t<is_constructible<T, std::initializer_list<U>&, Args...>::value, int> = 0>
        explicit constexpr variant(in_place_type_t<T>, std::initializer_list<U> il, Args&&... args);

        // Variant constructor #7
        template <size_t Index, class... Args,
            class = enable_if_t<(Index < variant_size_v<variant>), int>,
            class Alternative = variant_alternative_t<Index, variant>,
            enable_if_t<is_constructible<Alternative, Args...>::value, int> = 0>
        explicit constexpr variant(in_place_index_t<Index>, Args&&... args);

        // Variant constructor #8
        template <size_t Index, class U, class... Args,
            enable_if_t<(Index < variant_size<variant>::value), int> = 0,
            class Alternative = variant_alternative_t<Index, variant>,
            enable_if_t<is_constructible<Alternative, std::initializer_list<U>&, Args...>::value, int> = 0>
        explicit constexpr variant(in_place_index_t<Index>, std::initializer_list<U> il, Args&&... args);

        ~variant() = default;

        variant& operator=(const variant&) = default;
        variant& operator=(variant&&) = default;

        // Variant assignment operator #3
        template <class T,
            enable_if_t<!is_same<remove_cvref_t<T>, variant>::value, int> = 0,
            class Alternative = variant_detail::best_alternative_t<T, Types...>,
            size_t Index = find_type::find_exactly_one_alternative_v<Alternative, Types...>,
            enable_if_t<is_assignable<Alternative&, T>::value && is_constructible<Alternative, T>::value, int> = 0>
        auto operator=(T&& arg)->variant&;

        // Variant emplace #1
        template <class T, class... Args,
            size_t Index = find_type::find_exactly_one_alternative_v<T, Types...>,
            enable_if_t<is_constructible<T, Args...>::value, int> = 0>
        T& emplace(Args&&... args);

        // Variant emplace #2
        template <class T, class U, class... Args,
            size_t Index = find_type::find_exactly_one_alternative_v<T, Types...>,
            enable_if_t<is_constructible<T, std::initializer_list<U>&, Args...>::value, int> = 0>
        T& emplace(std::initializer_list<U> il, Args&&... args);

        // Variant emplace #3
        template <size_t Index, class... Args,
            enable_if_t<(Index < variant_size<variant>::value), int> = 0,
            class Alternative = variant_alternative_t<Index, variant>,
            enable_if_t<is_constructible<Alternative, Args...>::value, int> = 0>
        Alternative& emplace(Args&&... args);

        // Variant emplace #4
        template <size_t Index, class U, class... Args,
            enable_if_t<(Index < variant_size<variant>::value), int> = 0,
            class Alternative = variant_alternative_t<Index, variant>,
            enable_if_t<is_constructible<Alternative, std::initializer_list<U>&, Args...>::value, int> = 0>
        Alternative& emplace(std::initializer_list<U> il, Args&&... args);

        /// Returns false if and only if the variant holds a value.
        constexpr bool valueless_by_exception() const;
        /// Returns the zero-based index of the alternative that is currently held by the variant.
        /// If the variant is valueless_by_exception, returns variant_npos.
        constexpr size_t index() const;

        /// Overloads the std::swap algorithm for std::variant. Effectively calls lhs.swap(rhs).
        template <bool Placeholder = true, enable_if_t<conjunction<bool_constant<Placeholder && is_swappable<Types>::value && is_move_constructible<Types>::value>...>::value, bool> = false>
        void swap(variant& other);

    private:
        variant_detail::impl<Types...> m_impl;
        friend struct variant_detail::visitor::variant;
        friend struct variant_detail::get_alternative::variant;
    };

    template <class T, class... Types>
    constexpr bool holds_alternative(const variant<Types...>& variantInst);

    template <size_t Index, class... Types>
    constexpr variant_alternative_t<Index, variant<Types...>>& get(variant<Types...>& variantInst);

    template <size_t Index, class... Types>
    constexpr variant_alternative_t<Index, variant<Types...>>&& get(variant<Types...>&& variantInst);

    template <size_t Index, class... Types>
    constexpr const variant_alternative_t<Index, variant<Types...>>& get(const variant<Types...>& variantInst);

    template <size_t Index, class... Types>
    constexpr const variant_alternative_t<Index, variant<Types...>>&& get(const variant<Types...>&& variantInst);

    template <class T, class... Types>
    constexpr T& get(variant<Types...>& variantInst);

    template <class T, class... Types>
    constexpr T&& get(variant<Types...>&& variantInst);

    template <class T, class... Types>
    constexpr const T& get(const variant<Types...>& variantInst);

    template <class T, class... Types>
    constexpr const T&& get(const variant<Types...>&& variantInst);

    template <size_t Index, class... Types>
    constexpr add_pointer_t<variant_alternative_t<Index, variant<Types...>>> get_if(variant<Types...>* variantInst);

    template <size_t Index, class... Types>
    constexpr add_pointer_t<const variant_alternative_t<Index, variant<Types...>>> get_if(const variant<Types...>* variantInst);

    template <class T, class... Types>
    constexpr add_pointer_t<T> get_if(variant<Types...>* variantInst);

    template <class T, class... Types>
    constexpr add_pointer_t<const T> get_if(const variant<Types...>* variantInst);

    template <class... Types>
    constexpr bool operator==(const variant<Types...>& lhs, const variant<Types...>& rhs);

    template <class... Types>
    constexpr bool operator!=(const variant<Types...>& lhs, const variant<Types...>& rhs);

    template <class... Types>
    constexpr bool operator<(const variant<Types...>& lhs, const variant<Types...>& rhs);

    template <class... Types>
    constexpr bool operator>(const variant<Types...>& lhs, const variant<Types...>& rhs);

    template <class... Types>
    constexpr bool operator<=(const variant<Types...>& lhs, const variant<Types...>& rhs);

    template <class... Types>
    constexpr bool operator>=(const variant<Types...>& lhs, const variant<Types...>& rhs);


    template <typename... Types>
    void swap(variant<Types...>& lhs, variant<Types...>& rhs);

    template <class Visitor, class... Variants>
    constexpr decltype(auto) visit(Visitor&& visitor, Variants&&... variants);

    template <class R, class Visitor, class... Variants>
    constexpr R visit(Visitor&& visitor, Variants&&... variants);

    /* monostate is a unit type intended to act as an empty alternative for an AZStd::variant.
     * It can be used to make a variant default constructible when every alternative
     * is a non-default constructible type
     */
    struct monostate {};

    constexpr bool operator<(monostate, monostate);
    constexpr bool operator>(monostate, monostate);
    constexpr bool operator<=(monostate, monostate);
    constexpr bool operator>=(monostate, monostate);
    constexpr bool operator==(monostate, monostate);
    constexpr bool operator!=(monostate, monostate);

    template <>
    struct hash<monostate>
    {
        constexpr size_t operator()(const monostate&) const
        {
            return 'M' | ('O' << 8) | ('N' << 16) | ('O' << 24);
        }
    };

    template <typename... Types>
    struct hash<variant<Types...>>
    {
        template<typename... VariantTypes, typename = AZStd::enable_if_t<hash_enabled_concept_v<VariantTypes...>>>
        constexpr size_t operator()(const variant<VariantTypes...>& variantKey) const
        {
            constexpr size_t valuelessHashValue = 'V' | ('A' << 8) | ('R' << 16) | ('I' << 24);

            auto hashVisitor = [](const auto& variantAlt)
            {
                using value_type = AZStd::remove_cvref_t<decltype(variantAlt)>;
                return hash<value_type>{}(variantAlt);
            };
            size_t result = variantKey.valueless_by_exception() ? valuelessHashValue : visit(hashVisitor, variantKey);
            hash_combine(result, hash<size_t>{}(variantKey.index()));
            return result;
        }
    };

} // namespace AZStd

#include <AzCore/std/containers/variant.inl>

// undefine Visual Studio Empty Base Class Optimization Macro
#undef AZSTD_VARIANT_EMPTY_BASE_OPTIMIZATION

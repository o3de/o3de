/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/typetraits/add_const.h>
#include <AzCore/std/typetraits/add_cv.h>
#include <AzCore/std/typetraits/add_volatile.h>
#include <AzCore/std/typetraits/common_type.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_destructible.h>
#include <AzCore/std/typetraits/internal/type_sequence_traits.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/type_identity.h>


namespace AZStd
{
    template <class... Types>
    class variant;

    template <class T>
    struct variant_size;

    template <class T>
    constexpr size_t variant_size_v = variant_size<T>::value;

    template <class... Types>
    struct variant_size<variant<Types...>> : integral_constant<size_t, sizeof...(Types)> {};

    template <class T>
    struct variant_size<const T> : variant_size<T> {};

    template <class T>
    struct variant_size<volatile T> : variant_size<T> {};

    template <class T>
    struct variant_size<const volatile T> : variant_size<T> {};

    template <size_t Index, class T>
    struct variant_alternative;

    template <size_t Index, class T>
    using variant_alternative_t = typename variant_alternative<Index, T>::type;

    template <size_t Index, class... Types>
    struct variant_alternative<Index, variant<Types...>>
    {
        static_assert(Index < sizeof...(Types), "Index is out of bounds of the number of variant alternative types");
        using type = Internal::pack_traits_get_arg_t<Index, Types...>;
    };

    template <size_t Index, class T>
    struct variant_alternative<Index, const T>
        : add_const<variant_alternative_t<Index, T>> {};

    template <size_t Index, class T>
    struct variant_alternative<Index, volatile T>
        : add_volatile<variant_alternative_t<Index, T>> {};

    template <size_t Index, class T>
    struct variant_alternative<Index, const volatile T>
        : add_cv<variant_alternative_t<Index, T>> {};

    constexpr size_t variant_npos = static_cast<size_t>(-1);

#pragma push_macro("max")
#undef max
    // Optimize variant_index_t size based on the number of alternatives
    // 254 alternatives can fit in a uint8_t for the index type
    template <size_t NumAlternatives>
    using variant_index_t = AZStd::conditional_t<(NumAlternatives < std::numeric_limits<uint8_t>::max()), uint8_t,
        AZStd::conditional_t<(NumAlternatives < std::numeric_limits<uint16_t>::max()), uint16_t,
        AZStd::conditional_t<(NumAlternatives < std::numeric_limits<uint32_t>::max()), uint32_t, uint64_t>>>;

    // find_exactly_one_v helper for finding a type T in a variadic list of Types
    // if that variadic list of Types only contains type T once
    inline namespace find_type
    {
        static constexpr size_t not_found = static_cast<size_t>(-1);
        static constexpr size_t ambiguous = not_found - 1;

        inline constexpr size_t find_index_return(size_t currentIndex, size_t resultIndex, bool doesIndexMatch)
        {
            // Returns the previous result index if the currentIndex does not match
            // otherwise the resultIndex is update to the currentIndex unless it was matched at a later index
            return doesIndexMatch ? (resultIndex == not_found ? currentIndex : ambiguous) : resultIndex;
        }

        template <size_t IndexMax>
        inline constexpr size_t find_index(size_t currentIndex, const bool(&matches)[IndexMax])
        {
            return currentIndex == IndexMax ? not_found : find_index_return(currentIndex, find_index(currentIndex + 1, matches), matches[currentIndex]);
        }

        template <typename T, typename... Types>
        struct find_exactly_one_variadic
        {
            static constexpr bool matches[sizeof...(Types)] = { is_same<T, Types>::value... };
            static constexpr size_t value = find_index(0, matches);
            static_assert(value != not_found, "type not found in type list");
            static_assert(value != ambiguous, "type occurs more than once in type list");
        };

        template <class T>
        struct find_exactly_one_variadic<T>
        {
            static_assert(!AZStd::is_same_v<T, T>, "T cannot be found in an empty variadic Types list");
        };

        template <typename T, typename... Types>
        struct find_exactly_one_alternative : find_exactly_one_variadic<T, Types...>
        {
        };

        template <typename T, typename... Types>
        constexpr size_t find_exactly_one_alternative_v = find_exactly_one_alternative<T, Types...>::value;
    }

    namespace variant_detail
    {
        struct valueless_t {};

        enum class SpecialFunctionTraits
        {
            TriviallyAvailable,
            Available,
            Unavailable
        };

        template <typename... Types>
        constexpr SpecialFunctionTraits destructor_traits = conjunction_v<is_trivially_destructible<Types>...>
            ? SpecialFunctionTraits::TriviallyAvailable : (conjunction_v<is_destructible<Types>...> ? SpecialFunctionTraits::Available : SpecialFunctionTraits::Unavailable);

        template <typename... Types>
        constexpr SpecialFunctionTraits move_constructor_traits = conjunction_v<is_trivially_move_constructible<Types>...>
            ? SpecialFunctionTraits::TriviallyAvailable : (conjunction_v<is_move_constructible<Types>...> ? SpecialFunctionTraits::Available : SpecialFunctionTraits::Unavailable);

        template <typename... Types>
        constexpr SpecialFunctionTraits copy_constructor_traits = conjunction_v<is_trivially_copy_constructible<Types>...>
            ? SpecialFunctionTraits::TriviallyAvailable : (conjunction_v<is_copy_constructible<Types>...> ? SpecialFunctionTraits::Available : SpecialFunctionTraits::Unavailable);

        template <typename... Types>
        constexpr SpecialFunctionTraits move_assignable_traits = conjunction_v<is_trivially_move_assignable<Types>...>
            ? SpecialFunctionTraits::TriviallyAvailable : (conjunction_v<is_move_assignable<Types>...> ? SpecialFunctionTraits::Available : SpecialFunctionTraits::Unavailable);

        template <typename... Types>
        constexpr SpecialFunctionTraits copy_assignable_traits = conjunction_v<is_trivially_copy_assignable<Types>...>
            ? SpecialFunctionTraits::TriviallyAvailable : (conjunction_v<is_copy_assignable<Types>...> ? SpecialFunctionTraits::Available : SpecialFunctionTraits::Unavailable);

        namespace get_alternative
        {
            struct union_
            {
                template<class Union>
                static constexpr auto&& get_alt(Union&& unionData, in_place_index_t<0>)
                {
                    return AZStd::forward<Union>(unionData).m_head;
                }

                template <size_t AltIndex, class Union>
                static constexpr auto&& get_alt(Union&& unionData, in_place_index_t<AltIndex>)
                {
                    return get_alt(AZStd::forward<Union>(unionData).m_tail, in_place_index_t<AltIndex - 1>{});
                }
            };

            struct impl
            {
                template<size_t Index, class VariantImpl>
                static constexpr auto&& get_alt(VariantImpl&& var)
                {
                    return union_::get_alt(AZStd::forward<VariantImpl>(var).m_union_data, in_place_index_t<Index>{});
                }
            };

            struct variant
            {
                template<size_t Index, class Variant>
                static constexpr auto&& get_alt(Variant&& var)
                {
                    return impl::get_alt<Index>(AZStd::forward<Variant>(var).m_impl);
                }
            };
        }

        namespace visitor
        {
            struct impl
            {
                template <class Visitor, class... Variants>
                static constexpr decltype(auto) visit_alt_at(size_t index, Visitor&& visitor, Variants&&... variants)
                {
                    return make_dispatch_for_index<Visitor&&, decltype(AZStd::forward<Variants>(variants).as_base())...>()[index](AZStd::forward<Visitor>(visitor), AZStd::forward<Variants>(variants).as_base()...);
                }

                template <class Visitor, class... Variants>
                static constexpr decltype(auto) visit_alt(Visitor&& visitor, Variants&&... variants)
                {
                    return at(make_dispatch_for_all<Visitor&&, decltype(AZStd::forward<Variants>(variants).as_base())...>(), variants.index()...)(AZStd::forward<Visitor>(visitor), AZStd::forward<Variants>(variants).as_base()...);
                }

            private:
                template <class T>
                static constexpr const T& at(const T& elem)
                {
                    return elem;
                }

                template <class T, size_t N, class... Indices>
                static constexpr auto&& at(const array<T, N>& elems, size_t index, Indices... indices)
                {
                    return at(elems[index], indices...);
                }

                template <class DispatchFunc1, class... DispatchFuncs>
                static constexpr void visitor_return_type_check()
                {
                    static_assert(conjunction<is_same<DispatchFunc1, DispatchFuncs>...>::value, "AZStd::visit requires the visitor to have a single return type.");
                }

                template <class... DispatchFuncs>
                static constexpr auto make_dispatcher_array(DispatchFuncs&&... dispatchFuncs)
                {
                    visitor_return_type_check<remove_cvref_t<DispatchFuncs>...>();
                    using VisitorReturnArray = array<common_type_t<remove_cvref_t<DispatchFuncs>...>, sizeof...(dispatchFuncs)>;
                    return VisitorReturnArray{ {AZStd::forward<DispatchFuncs>(dispatchFuncs)...} };
                }


                template <size_t... Indices>
                struct dispatcher
                {
                    template <class Visitor, class... Variants>
                    static constexpr decltype(auto) dispatch(Visitor&& visitor, Variants&&... variants)
                    {
                        return AZStd::invoke(AZStd::forward<Visitor>(visitor), get_alternative::impl::get_alt<Indices>(AZStd::forward<Variants>(variants))...);
                    }
                };

                // make_dispatch returns function pointer to dispatch function
                template <class Visitor, class... Variants, size_t... Indices>
                static constexpr auto make_dispatch(index_sequence<Indices...>)
                {
                    return dispatcher<Indices...>::template dispatch<Visitor, Variants...>;
                }

                template<size_t Index, typename Variant>
                static constexpr size_t expand_pack_to_index()
                {
                    return Index;
                }

                template<size_t Size, typename Variant>
                struct is_variant_size_equal
                {
                    static constexpr bool value = Size == Variant::size();
                };

                template <size_t Index, class Visitor, class... Variants>
                static constexpr auto make_dispatch_for_index_impl()
                {
                    /* type_identity_t<Variants> is used to form a pack expansion equal to sizeof...(Variants)
                     The comma operator is used on type_identity_t<Variants> to discard the result of the construction and replace it with the Index value
                     This is used to form an index_sequence with the same Index value, for each of the supplied Variants arguments
                     i.e make_fdiagonal_impl<1, Visitor, Variant1, Variant2, Variant3> creates an index_sequence<1, 1, 1>
                     */
                    return make_dispatch<Visitor, Variants...>(index_sequence<expand_pack_to_index<Index, Variants>()...> {});
                }

                template <class Visitor, class... Variants, size_t... Indices>
                static constexpr auto make_dispatch_for_index_impl(index_sequence<Indices...>)
                {
                    return make_dispatcher_array(make_dispatch_for_index_impl<Indices, Visitor, Variants...>()...);
                }

                template <class Visitor, class Variant1, class... Variants>
                static constexpr auto make_dispatch_for_index()
                {
                    static_assert(conjunction<is_variant_size_equal<remove_cvref_t<Variant1>::size(), remove_cvref_t<Variants>>...>::value, "All Variants must contain the same number of union alternatives");
                    return make_dispatch_for_index_impl<Visitor, Variant1, Variants...>(make_index_sequence<remove_cvref_t<Variant1>::size()>{});
                }

                template <class Visitor, class... Variants, size_t... Indices>
                static constexpr auto make_dispatch_for_all_impl(index_sequence<Indices...> sequence)
                {
                    return make_dispatch<Visitor, Variants...>(sequence);

                }

                template <size_t... Indices1>
                struct vs2017_index_sequence_error_3528_workaround
                {
                    template<size_t NewIndex>
                    constexpr static index_sequence<Indices1..., NewIndex> make_index_sequence()
                    {
                        return {};
                    }
                };
                template <class Visitor, class... Variants, size_t... Indices1, size_t... Indices2, class... IndiceSequences>
                static constexpr auto make_dispatch_for_all_impl(index_sequence<Indices1...>, index_sequence<Indices2...>, IndiceSequences... sequences)
                {   
                    using index_sequence_workaround = vs2017_index_sequence_error_3528_workaround<Indices1...>;
                    return make_dispatcher_array(make_dispatch_for_all_impl<Visitor, Variants...>(index_sequence_workaround::template make_index_sequence<Indices2>(), sequences...)...);
                }

                /// This function makes (m^n) visitor functions where m is the number of variants and n is the number of alternatives per variant
                /// This tree of visitors can invoke the visitor for any combination of alternatives. 
                /// For three variants with three alternatives 27 functions are generated
                /// For two variants with three alternatives 9 functions are generated
                /// For three variants with two alternatives each 8 functions are generated
                template <class Visitor, class... Variants>
                static constexpr auto make_dispatch_for_all()
                {
                    return make_dispatch_for_all_impl<Visitor, Variants...>(index_sequence<>{}, make_index_sequence<remove_cvref_t<Variants>::size()>{}...);
                }
            };

            /// Variant Visitor functions
            struct variant
            {
                template <class Visitor, class... VariantTypes>
                static constexpr decltype(auto) visit_alt_at(size_t index, Visitor&& visitor, VariantTypes&&... vs)
                {
                    return impl::visit_alt_at(index, AZStd::forward<Visitor>(visitor), AZStd::forward<VariantTypes>(vs).m_impl...);
                }

                template <class Visitor, class... VariantTypes>
                static constexpr decltype(auto) visit_alt(Visitor&& visitor, VariantTypes&&... vs)
                {
                    return impl::visit_alt(AZStd::forward<Visitor>(visitor), AZStd::forward<VariantTypes>(vs).m_impl...);
                }
                template <class R, class Visitor, class... VariantTypes>
                static constexpr R visit_alt_r(Visitor&& visitor, VariantTypes&&... vs)
                {
                    return impl::visit_alt(AZStd::forward<Visitor>(visitor), AZStd::forward<VariantTypes>(vs).m_impl...);
                }

                template <class Visitor, class... VariantTypes>
                static constexpr decltype(auto) visit_value_at(size_t index, Visitor&& visitor, VariantTypes&&... vs)
                {
                    return visit_alt_at(index, make_value_visitor(AZStd::forward<Visitor>(visitor)), AZStd::forward<VariantTypes>(vs)...);
                }

                template <class Visitor, class... VariantTypes>
                static constexpr decltype(auto) visit_value(Visitor&& visitor, VariantTypes&&... vs)
                {
                    return visit_alt(make_value_visitor(AZStd::forward<Visitor>(visitor)), AZStd::forward<VariantTypes>(vs)...);
                }

                template <class R, class Visitor, class... VariantTypes>
                static constexpr R visit_value_r(Visitor&& visitor, VariantTypes&&... vs)
                {
                    return visit_alt_r<R>(make_value_visitor(AZStd::forward<Visitor>(visitor)), AZStd::forward<VariantTypes>(vs)...);
                }

            private:
                template <class Visitor>
                struct value_visitor
                {
                    template <class... Alternatives>
                    constexpr decltype(auto) operator()(Alternatives&&... alts) const
                    {
                        static_assert(is_invocable_v<Visitor, decltype((AZStd::forward<Alternatives>(alts).m_value))...>,
                            "visitor must be invocable with all supplied values");
                        return AZStd::invoke(AZStd::forward<Visitor>(m_visitor), AZStd::forward<Alternatives>(alts).m_value...);
                    }

                    Visitor&& m_visitor;
                };

                template <class Visitor>
                static constexpr auto make_value_visitor(Visitor&& visitor)
                {
                    return value_visitor<Visitor>{AZStd::forward<Visitor>(visitor)};
                }
            };

        } // namespace variant_visitor

        template <size_t Index, class T>
        struct alternative_impl
        {
            using value_type = T;

            template <class... Args>
            explicit constexpr alternative_impl(in_place_t, Args&&... args)
                : m_value(AZStd::forward<Args>(args)...) {}

            T m_value;
        };

        template <SpecialFunctionTraits, size_t Index, class... Types>
        union union_impl;

        template <SpecialFunctionTraits DestructorTrait, size_t Index>
        union union_impl<DestructorTrait, Index>
        {};

        template <SpecialFunctionTraits DestructorTrait, size_t Index, class T, class... Types>
        union union_impl<DestructorTrait, Index, T, Types...>
        {
        public:
            explicit constexpr union_impl(valueless_t tag)
                : m_valueless{ tag }
            {
            }

            template <class... Args>
            explicit constexpr union_impl(in_place_index_t<0>, Args&&... args)
                : m_head{ in_place, AZStd::forward<Args>(args)... }   
            {
            }

            template <size_t AltIndex, class... Args>
            explicit constexpr union_impl(in_place_index_t<AltIndex>, Args&&... args)
                : m_tail{ in_place_index_t<AltIndex - 1>{}, AZStd::forward<Args>(args)... }
            {
            }

            ~union_impl() = default;
            union_impl(const union_impl&) = default;
            union_impl(union_impl&&) = default;

            union_impl& operator=(const union_impl&) = default;
            union_impl& operator=(union_impl&&) = default;

        private:
            valueless_t m_valueless;
            alternative_impl<Index, T> m_head;
            union_impl<DestructorTrait, Index + 1, Types...> m_tail;
            friend struct get_alternative::union_;
        };

        template <size_t Index, class T, class... Types>
        union union_impl<SpecialFunctionTraits::Available, Index, T, Types...>
        {
        public:
            explicit constexpr union_impl(valueless_t tag)
                : m_valueless{ tag }
            {
            }

            template <class... Args>
            explicit constexpr union_impl(in_place_index_t<0>, Args&&... args)
                : m_head{ in_place, AZStd::forward<Args>(args)... }
            {
            }

            template <size_t AltIndex, class... Args>
            explicit constexpr union_impl(in_place_index_t<AltIndex>, Args&&... args)
                : m_tail{ in_place_index_t<AltIndex - 1>{}, AZStd::forward<Args>(args)... }
            {
            }

            ~union_impl()
            {
            }

            union_impl(const union_impl&) = default;
            union_impl(union_impl&&) = default;

            union_impl& operator=(const union_impl&) = default;
            union_impl& operator=(union_impl&&) = default;

        private:
            valueless_t m_valueless;
            alternative_impl<Index, T> m_head;
            union_impl<SpecialFunctionTraits::Available, Index + 1, Types...> m_tail;
            friend struct get_alternative::union_;
        };

        template <size_t Index, class T, class... Types>
        union union_impl<SpecialFunctionTraits::Unavailable, Index, T, Types...>
        {
        public:
            explicit constexpr union_impl(valueless_t tag)
                : m_valueless{ tag }
            {
            }

            template <class... Args>
            explicit constexpr union_impl(in_place_index_t<0>, Args&&... args)
                : m_head{ in_place, AZStd::forward<Args>(args)... }
            {
            }

            template <size_t AltIndex, class... Args>
            explicit constexpr union_impl(in_place_index_t<AltIndex>, Args&&... args)
                : m_tail{ in_place_index_t<AltIndex - 1>{}, AZStd::forward<Args>(args)... }
            {
            }

            ~union_impl() = delete;

            union_impl(const union_impl&) = default;
            union_impl(union_impl&&) = default;

            union_impl& operator=(const union_impl&) = default;
            union_impl& operator=(union_impl&&) = default;

        private:
            valueless_t m_valueless;
            alternative_impl<Index, T> m_head;
            union_impl<SpecialFunctionTraits::Unavailable, Index + 1, Types...> m_tail;
            friend struct get_alternative::union_;
        };


        template <typename IndexType>
        constexpr size_t variant_index_t_npos = static_cast<IndexType>(-1);

        template <class... Types>
        class variant_impl_base
        {
        public:
            static constexpr size_t num_alternatives = sizeof...(Types);
            using index_t = variant_index_t<num_alternatives>;

            explicit constexpr variant_impl_base(valueless_t valueless_tag)
                : m_union_data(valueless_tag)
                , m_index(variant_index_t_npos<index_t>)
            {}

            template <size_t Index, class... Args>
            explicit constexpr variant_impl_base(in_place_index_t<Index>, Args&&... args)
                : m_union_data(in_place_index_t<Index>{}, AZStd::forward<Args>(args)...)
                , m_index(Index)
            {}

            constexpr bool valueless_by_exception() const
            {
                return index() == variant_npos;
            }

            constexpr size_t index() const
            {
                return m_index == variant_index_t_npos<index_t> ? variant_npos : m_index;
            }

        protected:
            constexpr auto& as_base() &
            {
                return *this;
            }

            constexpr auto&& as_base() &&
            {
                return AZStd::move(*this);
            }

            constexpr auto&& as_base() const &
            {
                return *this;
            }

            constexpr auto&& as_base() const &&
            {
                return AZStd::move(*this);
            }

            static constexpr size_t size()
            {
                return num_alternatives;
            }

            union_impl<destructor_traits<Types...>, 0, Types...> m_union_data;
            index_t m_index;

            friend struct get_alternative::impl;
            friend struct visitor::impl;
        };

        // trivial destructor case
        template <SpecialFunctionTraits, class... Types>
        class variant_impl_destructor
            : public variant_impl_base<Types...>
        {
            using base_type = variant_impl_base<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_destructor() = default;

            variant_impl_destructor(variant_impl_destructor&&) = default;
            variant_impl_destructor(const variant_impl_destructor&) = default;
            variant_impl_destructor& operator=(variant_impl_destructor&&) = default;
            variant_impl_destructor& operator=(const variant_impl_destructor&) = default;

            void destroy()
            {
                this->m_index = variant_index_t_npos<typename base_type::index_t>;
            };
        };

        // non-trivial destructor case
        template <class... Types>
        class variant_impl_destructor<SpecialFunctionTraits::Available, Types...>
            : public variant_impl_base<Types...>
        {
            using base_type = variant_impl_base<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_destructor()
            {
                destroy();
            }

            variant_impl_destructor(variant_impl_destructor&&) = default;
            variant_impl_destructor(const variant_impl_destructor&) = default;
            variant_impl_destructor& operator=(variant_impl_destructor&&) = default;
            variant_impl_destructor& operator=(const variant_impl_destructor&) = default;

            void destroy()
            {
                if (!this->valueless_by_exception())
                {
                    auto destructVisitFunc = [](auto& variantAlt)
                    {
                        (void)variantAlt;
                        using alternative_type = AZStd::remove_cvref_t<decltype(variantAlt)>;
                        variantAlt.~alternative_type();
                    };
                    visitor::impl::visit_alt(AZStd::move(destructVisitFunc), *this);
                }
                this->m_index = variant_index_t_npos<typename base_type::index_t>;
            }
        };

        // deleted destructor case
        template <class... Types>
        class variant_impl_destructor<SpecialFunctionTraits::Unavailable, Types...>
            : public variant_impl_base<Types...>
        {
            using base_type = variant_impl_base<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_destructor() = delete;
            variant_impl_destructor(variant_impl_destructor&&) = default;
            variant_impl_destructor(const variant_impl_destructor&) = default;
            variant_impl_destructor& operator=(variant_impl_destructor&&) = default;
            variant_impl_destructor& operator=(const variant_impl_destructor&) = default;
            void destroy() = delete;
        };

        // Constructor
        template <class... Types>
        class variant_impl_constructor
            : public variant_impl_destructor<destructor_traits<Types...>, Types...>
        {
            using base_type = variant_impl_destructor<destructor_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

        protected:
            template <size_t Index, class T, class... Args>
            static T& construct_alt(alternative_impl<Index, T>& alt, Args&&... args)
            {
                new (&alt) alternative_impl<Index, T>{in_place, AZStd::forward<Args>(args)...};
                return alt.m_value;
            }

            template <class Rhs>
            static void generic_construct(variant_impl_constructor& lhs, Rhs&& rhs)
            {
                lhs.destroy();
                auto constructVisitFunc = [](auto& lhs_alt, auto&& rhs_alt)
                {
                    construct_alt(lhs_alt, AZStd::forward<decltype(rhs_alt)>(rhs_alt).m_value);
                };
                if (!rhs.valueless_by_exception())
                {
                    visitor::impl::visit_alt_at(rhs.index(), AZStd::move(constructVisitFunc), lhs, AZStd::forward<Rhs>(rhs));
                    lhs.m_index = static_cast<typename base_type::index_t>(rhs.index());
                }
            }
        };

        // trivial move constructor case
        template <SpecialFunctionTraits, class... Types>
        class variant_impl_move_constructor
            : public variant_impl_constructor<Types...>
        {
            using base_type = variant_impl_constructor<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            variant_impl_move_constructor(variant_impl_move_constructor&&) = default;

            variant_impl_move_constructor(const variant_impl_move_constructor&) = default;
            ~variant_impl_move_constructor() = default;
            variant_impl_move_constructor& operator=(const variant_impl_move_constructor&) = default;
            variant_impl_move_constructor& operator=(variant_impl_move_constructor&&) = default;
        };

        // non-trivial move constructor case
        template <class... Types>
        class variant_impl_move_constructor<SpecialFunctionTraits::Available, Types...>
            : public variant_impl_constructor<Types...>
        {
            using base_type = variant_impl_constructor<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            variant_impl_move_constructor(variant_impl_move_constructor&& other)
                : variant_impl_move_constructor(valueless_t{})
            {
                this->generic_construct(*this, AZStd::move(other));
            }

            variant_impl_move_constructor(const variant_impl_move_constructor&) = default;
            ~variant_impl_move_constructor() = default;
            variant_impl_move_constructor& operator=(const variant_impl_move_constructor&) = default;
            variant_impl_move_constructor& operator=(variant_impl_move_constructor&&) = default;
        };

        // deleted move constructor case
        template <class... Types>
        class variant_impl_move_constructor<SpecialFunctionTraits::Unavailable, Types...>
            : public variant_impl_constructor<Types...>
        {
            using base_type = variant_impl_constructor<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            variant_impl_move_constructor(variant_impl_move_constructor&&) = delete;

            variant_impl_move_constructor(const variant_impl_move_constructor&) = default;
            ~variant_impl_move_constructor() = default;
            variant_impl_move_constructor& operator=(const variant_impl_move_constructor&) = default;
            variant_impl_move_constructor& operator=(variant_impl_move_constructor&&) = default;
        };

        // trivial copy constructor case
        template <SpecialFunctionTraits, class... Types>
        class variant_impl_copy_constructor
            : public variant_impl_move_constructor<move_constructor_traits<Types...>, Types...>
        {
            using base_type = variant_impl_move_constructor<move_constructor_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            variant_impl_copy_constructor(const variant_impl_copy_constructor&) = default;

            ~variant_impl_copy_constructor() = default;
            variant_impl_copy_constructor(variant_impl_copy_constructor&&) = default;
            variant_impl_copy_constructor& operator=(variant_impl_copy_constructor&&) = default;
            variant_impl_copy_constructor& operator=(const variant_impl_copy_constructor&) = default;
        };

        // non-trivial copy constructor case
        template <class... Types>
        class variant_impl_copy_constructor<SpecialFunctionTraits::Available, Types...>
            : public variant_impl_move_constructor<move_constructor_traits<Types...>, Types...>
        {
            using base_type = variant_impl_move_constructor<move_constructor_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            variant_impl_copy_constructor(const variant_impl_copy_constructor& other)
                : variant_impl_copy_constructor(valueless_t{})
            {
                this->generic_construct(*this, other);
            }

            ~variant_impl_copy_constructor() = default;
            variant_impl_copy_constructor(variant_impl_copy_constructor&&) = default;
            variant_impl_copy_constructor& operator=(variant_impl_copy_constructor&&) = default;
            variant_impl_copy_constructor& operator=(const variant_impl_copy_constructor&) = default;
        };

        // deleted copy constructor case
        template <class... Types>
        class variant_impl_copy_constructor<SpecialFunctionTraits::Unavailable, Types...>
            : public variant_impl_move_constructor<move_constructor_traits<Types...>, Types...>
        {
            using base_type = variant_impl_move_constructor<move_constructor_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            variant_impl_copy_constructor(const variant_impl_copy_constructor&) = delete;

            ~variant_impl_copy_constructor() = default;
            variant_impl_copy_constructor(variant_impl_copy_constructor&&) = default;
            variant_impl_copy_constructor& operator=(variant_impl_copy_constructor&&) = default;
            variant_impl_copy_constructor& operator=(const variant_impl_copy_constructor&) = default;
        };

        // Assignment
        template <class... Types>
        class variant_impl_assignment
            : public variant_impl_copy_constructor<copy_constructor_traits<Types...>, Types...>
        {
            using base_type = variant_impl_copy_constructor<copy_constructor_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;


            template <size_t Index, class... Args>
            auto& emplace(Args&&... args)
            {
                this->destroy();
                auto& result_alternative = this->construct_alt(get_alternative::impl::get_alt<Index>(*this), AZStd::forward<Args>(args)...);
                this->m_index = Index;
                return result_alternative;
            }

        protected:
            template <size_t Index, class T, class Arg>
            void assign_alt(alternative_impl<Index, T>& alt, Arg&& arg)
            {
                // If the alternative being assigned is the same index,
                // the assignment operator of the union element
                if (this->index() == Index)
                {
                    alt.m_value = AZStd::forward<Arg>(arg);
                    return;
                }

                constexpr bool is_emplacable = is_nothrow_constructible_v<T, Arg> || !is_nothrow_move_constructible_v<T>;
                emplace_alt<Index, T>(AZStd::forward<Arg>(arg), bool_constant<is_emplacable>{});
            }
            template <size_t Index, class T, class Arg>
            void emplace_alt(Arg&& arg, AZStd::true_type)
            {
                this->emplace<Index>(AZStd::forward<Arg>(arg));
            }

            template <size_t Index, class T, class Arg>
            void emplace_alt(Arg&& arg, AZStd::false_type)
            {
                this->emplace<Index>(T(AZStd::forward<Arg>(arg)));
            }

            template <class OtherVariant>
            void generic_assign(OtherVariant&& other)
            {
                // Both variants are valueless so return
                if (this->valueless_by_exception() && other.valueless_by_exception())
                {
                    return;
                }

                // The other variant being assigned is value, so set this variant to be valueless as well
                if (other.valueless_by_exception())
                {
                    this->destroy();
                    return;
                }

                auto assignmentVisitor = [this](auto& thisAlt, auto&& otherAlt)
                {
                    this->assign_alt(thisAlt, AZStd::forward<decltype(otherAlt)>(otherAlt).m_value);
                };
                visitor::impl::visit_alt_at(other.index(), AZStd::move(assignmentVisitor), *this, AZStd::forward<OtherVariant>(other));
            }
        };

        // trivial move assignment case
        template <SpecialFunctionTraits, class... Types>
        class variant_impl_move_assignment
            : public variant_impl_assignment<Types...>
        {
            using base_type = variant_impl_assignment<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_move_assignment() = default;
            variant_impl_move_assignment(variant_impl_move_assignment&&) = default;
            variant_impl_move_assignment(const variant_impl_move_assignment&) = default;
            variant_impl_move_assignment& operator=(variant_impl_move_assignment&&) = default;
            variant_impl_move_assignment& operator=(const variant_impl_move_assignment&) = default;
        };

        // non-trivial move assignment case
        template <class... Types>
        class variant_impl_move_assignment<SpecialFunctionTraits::Available, Types...>
            : public variant_impl_assignment<Types...>
        {
            using base_type = variant_impl_assignment<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_move_assignment() = default;
            variant_impl_move_assignment(variant_impl_move_assignment&&) = default;
            variant_impl_move_assignment(const variant_impl_move_assignment&) = default;
            variant_impl_move_assignment& operator=(variant_impl_move_assignment&& rhs)
            {
                this->generic_assign(AZStd::move(rhs));
                return *this;
            }
            variant_impl_move_assignment& operator=(const variant_impl_move_assignment&) = default;
        };

        // deleted move assignment case
        template <class... Types>
        class variant_impl_move_assignment<SpecialFunctionTraits::Unavailable, Types...>
            : public variant_impl_assignment<Types...>
        {
            using base_type = variant_impl_assignment<Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_move_assignment() = default;
            variant_impl_move_assignment(variant_impl_move_assignment&&) = default;
            variant_impl_move_assignment(const variant_impl_move_assignment&) = default;
            variant_impl_move_assignment& operator=(variant_impl_move_assignment&&) = delete;
            variant_impl_move_assignment& operator=(const variant_impl_move_assignment&) = default;
        };

        // trivial copy assignment case
        template <SpecialFunctionTraits, class... Types>
        class variant_impl_copy_assignment
            : public variant_impl_move_assignment<move_assignable_traits<Types...>, Types...>
        {
            using base_type = variant_impl_move_assignment<move_assignable_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_copy_assignment() = default;
            variant_impl_copy_assignment(variant_impl_copy_assignment&&) = default;
            variant_impl_copy_assignment(const variant_impl_copy_assignment&) = default;
            variant_impl_copy_assignment& operator=(variant_impl_copy_assignment&&) = default;

            variant_impl_copy_assignment& operator=(const variant_impl_copy_assignment&) = default;
        };

        // non-trivial copy assignment case
        template <class... Types>
        class variant_impl_copy_assignment<SpecialFunctionTraits::Available, Types...>
            : public variant_impl_move_assignment<move_assignable_traits<Types...>, Types...>
        {
            using base_type = variant_impl_move_assignment<move_assignable_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_copy_assignment() = default;
            variant_impl_copy_assignment(variant_impl_copy_assignment&&) = default;
            variant_impl_copy_assignment(const variant_impl_copy_assignment&) = default;
            variant_impl_copy_assignment& operator=(variant_impl_copy_assignment&&) = default;

            variant_impl_copy_assignment& operator=(const variant_impl_copy_assignment& rhs)
            {
                this->generic_assign(rhs);
                return *this;
            }
        };

        // deleted copy assignment case
        template <class... Types>
        class variant_impl_copy_assignment<SpecialFunctionTraits::Unavailable, Types...>
            : public variant_impl_move_assignment<move_assignable_traits<Types...>, Types...>
        {
            using base_type = variant_impl_move_assignment<move_assignable_traits<Types...>, Types...>;
        public:
            using base_type::base_type;
            using base_type::operator=;

            ~variant_impl_copy_assignment() = default;
            variant_impl_copy_assignment(variant_impl_copy_assignment&&) = default;
            variant_impl_copy_assignment(const variant_impl_copy_assignment&) = default;
            variant_impl_copy_assignment& operator=(variant_impl_copy_assignment&&) = default;

            variant_impl_copy_assignment& operator=(const variant_impl_copy_assignment&) = delete;
        };

        template <class... Types>
        class impl
            : public variant_impl_copy_assignment<copy_assignable_traits<Types...>, Types...>
        {
            using base_type = variant_impl_copy_assignment<copy_assignable_traits<Types...>, Types...>;
            static constexpr bool alternative_move_constructible_array[] = { is_nothrow_move_constructible_v<Types>... };
        public:
            using base_type::base_type;
            using base_type::operator=;

            template <size_t Index, class Arg>
            void assign(Arg&& arg)
            {
                this->assign_alt(get_alternative::impl::get_alt<Index>(*this), AZStd::forward<Arg>(arg));
            }

            void swap(impl& other)
            {
                if (this->valueless_by_exception() && other.valueless_by_exception())
                {
                    return;
                }

                if (this->index() == other.index())
                {
                    auto swapAlternativeFunc = [](auto& this_alt, auto& other_alt)
                    {
                        using AZStd::swap;
                        swap(this_alt.m_value, other_alt.m_value);
                    };
                    visitor::impl::visit_alt_at(this->index(), swapAlternativeFunc, *this, other);
                }
                else
                {
                    impl* lhs = this;
                    impl* rhs = AZStd::addressof(other);
                    bool lhs_move_constructible = lhs->valueless_by_exception() || alternative_move_constructible_array[lhs->index()];
                    bool rhs_move_constructible = rhs->valueless_by_exception() || alternative_move_constructible_array[rhs->index()];
                    if (lhs_move_constructible && !rhs_move_constructible)
                    {
                        AZStd::swap(lhs, rhs);
                    }
                    impl tmp(AZStd::move(*rhs));

                    this->generic_construct(*rhs, AZStd::move(*lhs));
                    this->generic_construct(*lhs, AZStd::move(tmp));
                }
            }
        };

        template<typename... Types>
        constexpr bool impl<Types...>::alternative_move_constructible_array[];

        template <bool CanCopy, bool CanMove>
        struct make_constructor_overloads
        {};

        template <>
        struct make_constructor_overloads<false, false>
        {
            constexpr make_constructor_overloads() = default;
            constexpr make_constructor_overloads(const make_constructor_overloads&) = delete;
            constexpr make_constructor_overloads(make_constructor_overloads&&) = delete;

            make_constructor_overloads& operator=(const make_constructor_overloads&) = default;
            make_constructor_overloads& operator=(make_constructor_overloads&&) = default;
        };

        template <>
        struct make_constructor_overloads<false, true>
        {
            constexpr make_constructor_overloads() = default;
            constexpr make_constructor_overloads(const make_constructor_overloads&) = delete;
            constexpr make_constructor_overloads(make_constructor_overloads&&) = default;

            make_constructor_overloads& operator=(const make_constructor_overloads&) = default;
            make_constructor_overloads& operator=(make_constructor_overloads&&) = default;
        };

        template <>
        struct make_constructor_overloads<true, false>
        {
            constexpr make_constructor_overloads() = default;
            constexpr make_constructor_overloads(const make_constructor_overloads&) = default;
            constexpr make_constructor_overloads(make_constructor_overloads&&) = delete;

            make_constructor_overloads& operator=(const make_constructor_overloads&) = default;
            make_constructor_overloads& operator=(make_constructor_overloads&&) = default;
        };

        template <bool CanCopy, bool CanMove>
        struct make_assignment_overloads
        {};

        template <>
        struct make_assignment_overloads<false, false>
        {
            constexpr make_assignment_overloads() = default;
            constexpr make_assignment_overloads(const make_assignment_overloads&) = default;
            constexpr make_assignment_overloads(make_assignment_overloads&&) = default;

            make_assignment_overloads& operator=(const make_assignment_overloads&) = delete;
            make_assignment_overloads& operator=(make_assignment_overloads&&) = delete;
        };

        template <>
        struct make_assignment_overloads<false, true>
        {
            constexpr make_assignment_overloads() = default;
            constexpr make_assignment_overloads(const make_assignment_overloads&) = default;
            constexpr make_assignment_overloads(make_assignment_overloads&&) = default;

            make_assignment_overloads& operator=(const make_assignment_overloads&) = delete;
            make_assignment_overloads& operator=(make_assignment_overloads&&) = default;
        };

        template <>
        struct make_assignment_overloads<true, false>
        {
            constexpr make_assignment_overloads() = default;
            constexpr make_assignment_overloads(const make_assignment_overloads&) = default;
            constexpr make_assignment_overloads(make_assignment_overloads&&) = default;

            make_assignment_overloads& operator=(const make_assignment_overloads&) = default;
            make_assignment_overloads& operator=(make_assignment_overloads&&) = delete;
        };

        /*
         imaginary_function represents an invented function which is only used for it's function signature
         to determine to provide an operator() that accepts type T.
         This allows it to be used in a non-odr use such as decltype to provide a set of overloads for the best_alternative_t
         type alias. This facilitates allowing the compiler to choose the best type for constructing a variant
         An example use case of uses of this struct is when dealing with a `variant<const char*, AZStd::string>`
         If the variant is initializes using `variant<const char*, AZStd::string> var1("Hello")`, the best union value to use
         is a const char*.
         If the variant that being used is of type `variant<int, AZStd::string>`, then the best union value to use is AZStd::string
         */

        template <typename... Types>
        struct implicit_convertible_operators;

        template <>
        struct implicit_convertible_operators<>
        {
            void operator()() const;
        };
        template <typename T, typename... Types>
        struct implicit_convertible_operators<T, Types...>
            : implicit_convertible_operators<Types...>
        {
            using implicit_convertible_operators<Types...>::operator();
            type_identity<T> operator()(T) const;
        };

        template <class T, class... Types>
        using best_alternative_t = typename AZStd::invoke_result_t<implicit_convertible_operators<Types...>, T>::type;

        /// Variant std::holds_alternative and std::get helper functions
        template <size_t Index, class... Types>
        inline constexpr bool holds_alternative_at_index(const variant<Types...>& variantInst)
        {
            return variantInst.index() == Index;
        }

        template <size_t Index, class VariantType>
        inline constexpr auto&& generic_get(VariantType&& variantInst)
        {
            if (!holds_alternative_at_index<Index>(variantInst))
            {
                AZSTD_CONTAINER_ASSERT(false, "bad_variant_access: Variant does not contain alternative at index %zu", Index);
            }
            return get_alternative::variant::get_alt<Index>(AZStd::forward<VariantType>(variantInst)).m_value;
        }

        template <size_t Index, class VariantType>
        inline constexpr auto* generic_get_if(VariantType* variantInst)
        {
            return variantInst != nullptr && holds_alternative_at_index<Index>(*variantInst)
                ? AZStd::addressof(get_alternative::variant::get_alt<Index>(*variantInst).m_value)
                : nullptr;
        }
    }
} // namespace AZStd
#pragma pop_macro("max")

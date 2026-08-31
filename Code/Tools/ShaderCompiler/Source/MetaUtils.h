/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "StdUtils.h"

#include <type_traits>
#include <utility>

namespace AZ
{
    // meta-variable for syntax simplification to test multiple types for equivalence with one:
    template <typename IsT, typename ...AnyOfThese>
    constexpr bool isAnyOf_v = std::disjunction< std::is_same<IsT, AnyOfThese>... >::value;

    //examples:
    static_assert(isAnyOf_v< int, float, double, void, int, int* >);
    static_assert(!isAnyOf_v< size_t&, float, double, void, int, int* >);

    template <typename... Types>
    struct TypeList;

    // auto: http://open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0127r1.html
    template <auto... Values>
    struct ValueTplList;

    // typelist to tuple type converter
    template<typename T>
    struct TypeListAsTuple;
    template<template <typename...> class TL, typename... Ts>
    struct TypeListAsTuple<TL<Ts...>>
    {
        using type = tuple<Ts...>;
    };
    template<typename TL>
    using TypeListAsTuple_t = typename TypeListAsTuple<TL>::type;

    // value list to tuple constexpr variable converter
    template<typename T>
    struct ValueListAsTuple;
    template<template <auto...> class L, auto... Vs>
    struct ValueListAsTuple<L<Vs...>>
    {
        static constexpr auto value = std::make_tuple(Vs...);
        using type = std::remove_const_t<decltype(value)>; // need to remove const because constexpr implies const, which is not useful in the typedef
    };
    template<typename T>
    constexpr auto valueListAsTuple_v = ValueListAsTuple<T>::value;
    template<typename T>
    using ValueListAsTuple_t = typename ValueListAsTuple<T>::type;

    // meta-construction capable to extract the arity of a template parameter pack, or list. (of any templated type, be it TypeList or tuple or variant.)
    template <class TypeListT>
    struct CountTemplateParameters;
    // version for type lists
    template <template<class...> class Tmpl, class... Ts>
    struct CountTemplateParameters<Tmpl<Ts...> >
    {
        static constexpr int value = sizeof...(Ts);
    };
    // version for value lists
    template <template<auto...> class Tmpl, auto... Ts>
    struct CountTemplateParameters<Tmpl<Ts...> >
    {
        static constexpr int value = sizeof...(Ts);
    };
    // alias template variable
    template <class TypeListT>
    constexpr int countTemplateParameters_v = CountTemplateParameters<TypeListT>::value;

    // example usage:
    static_assert(countTemplateParameters_v< TypeList<int, bool, float> > == 3);
    static_assert(countTemplateParameters_v< ValueTplList<1, 2, 4, 8> > == 4);

    struct NotFoundT { using type = NotFoundT; };

    // lazy meta evaluation enabler
    template<class T> struct IdentityT { using type = T; };

    // indexed direct accessor at<n>
    template<size_t at, typename TL>
    struct AtT
    {
        static constexpr bool out_of_bounds = at >= countTemplateParameters_v<TL>;
        using type = typename std::conditional< out_of_bounds,
            IdentityT<NotFoundT>,  // we need Identity to enable SFINAE
            IdentityT<
            std::tuple_element<at, TypeListAsTuple_t<TL> >
            > >::type::type::type;
    };
    // shortcut
    template <size_t at, typename TL>
    using At_t = typename AtT<at, TL>::type;

    // tests
    static_assert(std::is_same_v< At_t<1, TypeList<int, bool, float>>, bool >);
    // verify that we can access out of bound without a complete build stop
    static_assert(std::is_same_v< At_t<10, TypeList<int, bool, float>>, NotFoundT >);

    template <auto V>
    struct ConstVal : std::integral_constant<decltype(V), V>
    {};

    // is-integral-type-or-enum-type trait
    template <typename T>
    struct IsIntegralConstant
    {
        static constexpr bool value = std::is_integral_v<T> || std::is_enum<T>::value;
    };
    template <typename T, auto N>
    struct IsIntegralConstant<std::integral_constant<T, N>>
    {
        static constexpr bool value = IsIntegralConstant<T>::value;
    };
    template <auto N>
    struct IsIntegralConstant<ConstVal<N>>
    {
        static constexpr bool value = IsIntegralConstant<decltype(N)>::value;
    };
    template <typename T> constexpr auto isIntegralConstant_v = IsIntegralConstant<T>::value;

    // tests:
    static_assert(!isIntegralConstant_v<int*>);
    static_assert(isIntegralConstant_v<std::integral_constant<int, 3>>);
    static_assert(isIntegralConstant_v<ConstVal<2>>);
    static_assert(std::is_same_v< At_t<1, TypeList< int, ConstVal<2>>>, ConstVal<2> >);
    static_assert(isIntegralConstant_v< At_t<1, TypeList< int, ConstVal<2>>>>);

    // type list head extractor
    template <class TypeListT>
    struct HeadType;
    template <template<class...> class Tmpl, class T, class... Ts>
    struct HeadType<Tmpl<T, Ts...> >
    {
        using type = T;
    };
    // same thing for values
    template <class TypeListT>
    struct HeadValue;
    template <template<auto...> class Tmpl, auto T, auto... Ts>
    struct HeadValue<Tmpl<T, Ts...> >
    {
        static constexpr auto value = T;
    };
    // alias template
    template <class TypeListT>
    using HeadType_t = typename HeadType<TypeListT>::type;
    // same for value
    template <class ValueListT>
    constexpr auto HeadValue_v = HeadValue<ValueListT>::value;

    // test
    static_assert(std::is_same_v< HeadType_t< TypeList<int, double> >, int >);
    static_assert(HeadValue_v< ValueTplList<1, 2, 3> > == 1);

    // type list tail extractor
    template <class TypeListT>
    struct TailTypeList;
    template <template<class...> class Tmpl, class T, class... Ts>
    struct TailTypeList<Tmpl<T, Ts...> >
    {
        using type = TypeList<Ts...>;
    };
    // same thing for values
    template <class TypeListT>
    struct TailValueList;
    template <template<auto...> class Tmpl, auto T, auto... Ts>
    struct TailValueList<Tmpl<T, Ts...> >
    {
        using type = ValueTplList<Ts...>;
    };
    // alias template
    template <typename TypeListT>
    using TailTypeList_t = typename TailTypeList<TypeListT>::type;
    // ends up being the same for values, because the tail of a valuelist is still a type.
    template <typename TypeListT>
    using TailValueList_t = typename TailValueList<TypeListT>::type;

    // since it's a runtime (unrolled) loop, you will get default-constructed instances of each type as parameter to your functor
    template <typename TypeListT, typename GenericLambda, int index = 0>
    void ForEachType(GenericLambda functor)
    {
        auto head = HeadType_t<TypeListT>{};
        functor(head, ConstVal<index>{});
        if constexpr (1 < countTemplateParameters_v<TypeListT>)
        {
            using Rest = TailTypeList_t<TypeListT>;
            ForEachType<Rest, GenericLambda, index + 1>(functor);
        }
    };

    template <typename ValueTplListT, typename GenericLambda, int index = 0>
    void ForEachValue(GenericLambda functor)
    {
        auto head = HeadValue_v<ValueTplListT>;
        functor(head, ConstVal<index>{});
        if constexpr (1 < countTemplateParameters_v<ValueTplListT>)
        {
            using Rest = TailValueList_t<ValueTplListT>;
            ForEachValue<Rest, GenericLambda, index + 1>(functor);
        }
    };

    // Constructs a default-initialized value with the type_atN in the variant<type_at0, type_at1...> at index `index`.
    // with `index` being a runtime parameter. And re-assign the variant to that new object
    // Requires that all types in the variant are default constructible.
    template <typename VariantT>
    VariantT&& IndexedFactory(VariantT&& variant, int index)
    {
        ForEachType< std::remove_reference_t<VariantT> >([&variant, index](auto var, auto ii_c)
            {
                if (ii_c == index)
                {
                    static_assert(std::is_default_constructible_v< decltype(var) >);
                    variant = decltype(var){};
                }
            });
        return variant;
    }

    // meta technique to find index of a type in a typelist
    template <typename>
    constexpr int MetaFind(int)
    {   // not found case
        return -1;
    }
    template <typename NeedleT, typename T, typename... Ts>
    constexpr int MetaFind(int ind = 0)
    {
        if (std::is_same_v<NeedleT, T>)
        {
            return ind;
        }
        else
        {
            return MetaFind<NeedleT, Ts...>(ind + 1);
        }
    }
    // flat 2 template parameters versions
    template <typename T, typename T2>
    struct MetaIndexOf : std::integral_constant<int, -1>
    {};
    template <typename T>
    struct MetaIndexOf<T, T> : std::integral_constant<int, 0>
    {};
    // destructurer version (access the contents of a typelist)
    template <typename T, template<typename...> class Tmpl, typename... Ts>
    struct MetaIndexOf<T, Tmpl<Ts...> >
        : std::integral_constant< int, MetaFind<T, Ts...>() >
    {};
    // template variable version of it:
    template <typename Needle, typename HaystackList>
    constexpr int metaFind_v = MetaIndexOf<Needle, HaystackList>::value;

    // test
    static_assert(metaFind_v< bool, bool > == 0);
    static_assert(metaFind_v< bool, int > == -1);
    static_assert(metaFind_v< int, TypeList<bool, float, int, double> > == 2);
    static_assert(metaFind_v< double, TypeList<bool, float, int, double> > == 3);
    static_assert(metaFind_v< bool, TypeList<bool, float, int, double> > == 0);
    static_assert(metaFind_v< long, TypeList<bool, float, int, double> > == -1);

    // make a metaFind for values:

    // let's try with a value wrapped in a type because going direct auto route didn't work in MSVC (compiler not good enough)

    template <typename>
    struct BoxAll;
    // convenience helper to directly make a typelist of boxed values from values
    template< template<auto...> class VL, auto... Vs >
    struct BoxAll< VL<Vs...> >
    {
        using type = TypeList< ConstVal<Vs>... >;
    };
    template<typename T>
    using BoxAll_t = typename BoxAll<T>::type;

    static_assert(std::is_same_v< BoxAll_t<ValueTplList<0>>, TypeList<ConstVal<0>> >);

    // At for values
    template <size_t at, typename VL>
    constexpr int at_v = AtT<at, BoxAll_t<VL>>::type::value;

    static_assert(at_v< 2, ValueTplList<2, 4, 8, 16, 32> > == 8);

    // finder
    template <auto V, typename VL>
    constexpr int metaFindV_v = metaFind_v< ConstVal<V>, BoxAll_t<VL> >;

    // tests
    static_assert(metaFindV_v< 0, ValueTplList<0> > == 0);
    static_assert(metaFindV_v< 5, ValueTplList<0> > == -1);
    static_assert(metaFindV_v< 0, ValueTplList<nullptr, 0> > == 1);
    // this test doesn't pass on visual studio because of the bad quality of the compiler.
    // however funnily intellisense correctly passes it.
    //static_assert(metaFindV_v< 2, ValueTplList<1, (long)2, 2, 3, 4> > == 2);
    static_assert(metaFindV_v< -1, ValueTplList<-2, -1, 42> > == 1);

    // define isContainedIn in terms of find
    template <typename T, typename TemplInst>
    constexpr bool isContainedIn_v = metaFind_v<T, TemplInst> != -1;

    // example use:
    static_assert(isContainedIn_v< bool, pair<bool, int> >);
    static_assert(!isContainedIn_v< float, pair<bool, int> >);
    // it works with variant too... anything with a template type list.
    static_assert(isContainedIn_v< int, tuple<bool, int, float> >);
    static_assert(isContainedIn_v< int, TypeList<bool, int, float> >);
    static_assert(!isContainedIn_v< long, TypeList<bool, int, float> >);


    template<typename L, typename T>
    struct PushFrontImpl;
    template<template<class...> class L, typename... U, typename T>
    struct PushFrontImpl<L<U...>, T>
    {
        using type = L<T, U...>;
    };
    // the API is you can push a bunch of T at once, but usually used with one T. PushFront_t< List, float >
    template<typename L, typename... T>
    using PushFront_t = typename PushFrontImpl<L, T...>::type;

    // find a key, in a typelist that is arranged like a map: TypeList< key, value, key, value ... >
    // and return the associated value
    // KeyT: the key to look for (needle)
    // KVPTypeList: a key-value paired typelist
    // may return NotFound type
    template <typename KeyT, typename KVPTypeList>
    struct MetaFindValueNextToKey
    {
        static constexpr auto index = metaFind_v< KeyT, KVPTypeList >;
        // the sub type of interest is just coming at the next index in the list:
        using type = std::conditional_t< (index < 0), NotFoundT,
            At_t<index + 1, KVPTypeList> >;
    };
    // Simplified API
    template <typename KeyT, typename KVPTypeList>
    using MetaFindValueNextToKey_t = typename MetaFindValueNextToKey<KeyT, KVPTypeList>::type;


    // detected_or from upcoming standards
    namespace detail
    {
        template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
        struct Detector
        {
            using value_t = std::false_type;
            using type = Default;
        };

        template <class Default, template<class...> class Op, class... Args>
        struct Detector<Default, std::void_t<Op<Args...>>, Op, Args...>
        {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };
    }

    template <class Default, template<class...> class Op, class... Args>
    using DetectedOr = detail::Detector<Default, void, Op, Args...>;

    template <class Default, template<class...> class Op, class... Args>
    using DetectedOr_t = typename DetectedOr<Default, Op, Args...>::type;

    //! define a Pair typealias that has two same T, without the need to repeat yourself
    template <typename T>
    using Pair = std::pair<T, T>;
}

#ifndef NDEBUG
#include <cassert>
#include <string>

namespace AZ::Tests
{
    inline void DoAsserts3()
    {
        ForEachValue< ValueTplList<1, 2, (size_t)3> >([](auto t, auto) { static_assert(std::is_same_v<decltype(t), int> || std::is_same_v<decltype(t), size_t>); });

        ForEachValue< ValueTplList<(long)1, (long)2> >([](auto t, auto) { static_assert(std::is_same_v<decltype(t), long>); });

        ForEachValue< ValueTplList<'a', 'b', 'c'> >([](auto t, auto i)
            {
                assert(i == 0 && t == 'a'
                    || i == 1 && t == 'b'
                    || i == 2 && t == 'c');
            });

        using TL = TypeList< ConstVal<10>, int >;
        ForEachType< TL >([](auto inst, auto ii_c)
            {
                using KeyAtii = At_t<decltype(ii_c)::value, TL>;
                static_assert(ii_c == 0 && isIntegralConstant_v< KeyAtii > && KeyAtii{} == 10
                    || ii_c == 1 && std::is_same_v< KeyAtii, int >);
            });

        {  // check that the variant factory works
            using AZStd::variant;
            using AZStd::monostate;
            using AZStd::get;
            using AZStd::holds_alternative;

            struct S { bool b = true; };
            using V = variant<monostate, int, string, S>;
            V v;

            assert(holds_alternative<monostate>(v));

            IndexedFactory(v, 1);
            assert(holds_alternative<int>(v));

            IndexedFactory(v, 2);
            assert(holds_alternative<string>(v));

            IndexedFactory(v, 0);
            assert(holds_alternative<monostate>(v));

            IndexedFactory(v, 3);
            assert(holds_alternative<S>(v));
            assert(get<3>(v).b == true);
        }
    }
}
#endif

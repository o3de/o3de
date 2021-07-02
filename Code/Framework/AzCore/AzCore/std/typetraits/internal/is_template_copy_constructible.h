/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/is_constructible.h>

namespace AZStd
{
    template<class T, class Allocator>
    class vector;
    
    template<class T, class Allocator>
    class list;
    template<class T, class Allocator>
    class forward_list;
    
    template<class Key, class MappedType, class Compare, class Allocator>
    class map;
    template<class Key, class MappedType, class Compare, class Allocator>
    class multimap;
    
    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    class unordered_map;
    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    class unordered_multimap;
    
    template <class Key, class Compare, class Allocator>
    class set;
    template <class Key, class Compare, class Allocator>
    class multiset;

    template<class Key, class Hasher, class EqualKey, class Allocator>
    class unordered_set;
    template<class Key, class Hasher, class EqualKey, class Allocator>
    class unordered_multiset;

    template<class T1, class T2>
    struct pair;

    namespace Internal
    {
        // C++ Container types always defines a copy constructor even when it's elements aren't copyable.
        // To get around issue, a template_is_copy_constructible type trait is added that is specialized
        // on the AZStd containers types to check the contained type copyability
        template<typename T>
        struct template_is_copy_constructible
            : is_copy_constructible<T>
        {
        };

        template<class T, class Allocator>
        struct template_is_copy_constructible<vector<T, Allocator>>
            : conjunction<is_copy_constructible<T>, is_copy_constructible<Allocator>>
        {
        };

        template<class T, class Allocator>
        struct template_is_copy_constructible<list<T, Allocator>>
            : conjunction<is_copy_constructible<T>, is_copy_constructible<Allocator>>
        {
        };

        template<class T, class Allocator>
        struct template_is_copy_constructible<forward_list<T, Allocator>>
            : conjunction<is_copy_constructible<T>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class MappedType, class Compare, class Allocator>
        struct template_is_copy_constructible<map<Key, MappedType, Compare, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<MappedType>, is_copy_constructible<Compare>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class MappedType, class Compare, class Allocator>
        struct template_is_copy_constructible<multimap<Key, MappedType, Compare, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<MappedType>, is_copy_constructible<Compare>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
        struct template_is_copy_constructible<unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<MappedType>, is_copy_constructible<Hasher>, is_copy_constructible<EqualKey>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
        struct template_is_copy_constructible<unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<MappedType>, is_copy_constructible<Hasher>, is_copy_constructible<EqualKey>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class Compare, class Allocator>
        struct template_is_copy_constructible<set<Key,  Compare, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<Compare>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class Compare, class Allocator>
        struct template_is_copy_constructible<multiset<Key, Compare, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<Compare>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class Hasher, class EqualKey, class Allocator>
        struct template_is_copy_constructible<unordered_set<Key, Hasher, EqualKey, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<Hasher>, is_copy_constructible<EqualKey>, is_copy_constructible<Allocator>>
        {
        };

        template<class Key, class Hasher, class EqualKey, class Allocator>
        struct template_is_copy_constructible<unordered_multiset<Key, Hasher, EqualKey, Allocator>>
            : conjunction<is_copy_constructible<Key>, is_copy_constructible<Hasher>, is_copy_constructible<EqualKey>, is_copy_constructible<Allocator>>
        {
        };

        template<class T1, class T2>
        struct template_is_copy_constructible<pair<T1, T2>>
            : conjunction<is_copy_constructible<T1>, is_copy_constructible<T2>>
        {
        };
    }
}

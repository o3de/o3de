/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/ScriptCanvas/ScriptCanvasOnDemandNames.h>
#include <AzCore/ScriptCanvas/ScriptCanvasAttributes.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/tokenize.h>
#include "AzCore/std/string/string.h"

// forward declare specialized types
namespace AZStd
{
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class vector;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class forward_list;
    template< class T, size_t Capacity >
    class fixed_vector;
    template< class T, size_t N >
    class array;
    template<class Key, class MappedType, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_map;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_set;
    template<AZStd::size_t NumBits>
    class bitset;
    template<class Element, class Traits, class Allocator>
    class basic_string;
    template<class Element>
    struct char_traits;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;
}

namespace AZ
{
    namespace ScriptCanvasOnDemandReflection
    {
         template<class Element, class Traits, class Allocator>
         struct OnDemandPrettyName< AZStd::basic_string<Element, Traits, Allocator> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "String";
             }
         };
 
         template<class Element, class Traits>
         struct OnDemandPrettyName< AZStd::basic_string_view<Element, Traits> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "String";
             }
         };
 
         template<class T>
         struct OnDemandPrettyName< AZStd::intrusive_ptr<T> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Intrusive<%s>", OnDemandPrettyName<T>::Get(context).data());
             }
         };

         template<class T>
         struct OnDemandToolTip< AZStd::intrusive_ptr<T> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A smart pointer which manages the life cycle of an object, and guarantees a single point of ownership for the specified memory.";
             }
         };
 
         template<class T>
         struct OnDemandPrettyName< AZStd::shared_ptr<T> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Shared<%s>", OnDemandPrettyName<T>::Get(context).data());
             }
         };

         template<class T>
         struct OnDemandToolTip< AZStd::shared_ptr<T> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A smart pointer which manages the life cycle of an object, and provides multiple points of ownership for the specified memory(The memory will remain active so long as any shared_ptr has a reference to it).";
             }
         };
 
         template<class T, class A>
         struct OnDemandPrettyName< AZStd::vector<T, A> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Array<%s>", OnDemandPrettyName<T>::Get(context).data());
             }
         };

         template<class T, class A>
         struct OnDemandToolTip< AZStd::vector<T, A> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A dynamically sized container of elements.";
             }
         };
 
         template<class T, AZStd::size_t N>
         struct OnDemandPrettyName< AZStd::array<T, N> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Fixed Size Array<%s, %zu>", OnDemandPrettyName<T>::Get(context).data(), N);
             }
         };

         template<class T, AZStd::size_t N>
         struct OnDemandToolTip< AZStd::array<T, N> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A fixed-sized container of elements.";
             }
         };
 
         template<typename T1, typename T2>
         struct OnDemandPrettyName< AZStd::pair<T1, T2> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Pair<%s, %s>", OnDemandPrettyName<T1>::Get(context).data(), OnDemandPrettyName<T2>::Get(context).data());
             }
         };

         template<typename T1, typename T2>
         struct OnDemandToolTip< AZStd::pair<T1, T2> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A pair is an fixed size collection of two elements.";
             }
         };

         template<typename T>
         void GetTypeNamesFold(AZStd::vector<AZStd::string>& result, AZ::BehaviorContext& context)
         {
             result.push_back(OnDemandPrettyName<T>::Get(context));
         };

         template<typename... T>
         void GetTypeNames(AZStd::vector<AZStd::string>& result, AZ::BehaviorContext& context)
         {
             (GetTypeNamesFold<T>(result, context), ...);
         };

         template<typename T>
         void GetTypeNamesFold(AZStd::string& result, AZ::BehaviorContext& context)
         {
             if (!result.empty())
             {
                 result += ", ";
             }

             result += OnDemandPrettyName<T>::Get(context);
         };

         template<typename... T>
         void GetTypeNames(AZStd::string& result, AZ::BehaviorContext& context)
         {
             (GetTypeNamesFold<T>(result, context), ...);
         };

         template<typename... T>
         struct OnDemandPrettyName<AZStd::tuple<T...>>
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 AZStd::string typeNames;
                 GetTypeNames<T...>(typeNames, context);
                 return AZStd::string::format("Tuple<%s>", typeNames.c_str());
             }
         };

         template<typename... T>
         struct OnDemandToolTip<AZStd::tuple<T...>>
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A tuple is an fixed size collection of any number of any type of element.";
             }
         };

         template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
         struct OnDemandPrettyName< AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Map<%s, %s>", OnDemandPrettyName<Key>::Get(context).data(), OnDemandPrettyName<MappedType>::Get(context).data());
             }
         };

         template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
         struct OnDemandToolTip< AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A collection of Key/Value pairs, where each Key value must be unique.";
             }
         };
 
         template<class Key, class Hasher, class EqualKey, class Allocator>
         struct OnDemandPrettyName< AZStd::unordered_set<Key, Hasher, EqualKey, Allocator> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Set<%s>", OnDemandPrettyName<Key>::Get(context).data());
             }
         };

         template<class Key, class Hasher, class EqualKey, class Allocator>
         struct OnDemandToolTip< AZStd::unordered_set<Key, Hasher, EqualKey, Allocator> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "A dynamically sized container, where each element is unique.";
             }
         };

         template<typename t_Success, typename t_Failure>
         struct OnDemandPrettyName< AZ::Outcome<t_Success, t_Failure> >
         {
             static AZStd::string Get(AZ::BehaviorContext& context)
             {
                 return AZStd::string::format("Outcome<%s, %s>", OnDemandPrettyName<t_Success>::Get(context).data(), OnDemandPrettyName<t_Failure>::Get(context).data());
             }
         };

         template<typename t_Success, typename t_Failure>
         struct OnDemandToolTip< AZ::Outcome<t_Success, t_Failure> >
         {
             static AZStd::string Get(AZ::BehaviorContext&)
             {
                 return "An outcome is a combination of a Success/Failure state along with a resulting result type.";
             }
         };
    }
}


/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Part of CryEngine's extension framework.


#ifndef CRYINCLUDE_CRYEXTENSION_TYPELIST_H
#define CRYINCLUDE_CRYEXTENSION_TYPELIST_H
#pragma once


namespace TL
{
    // typelist terminator
    class NullType
    {
    };


    // structure for typelist generation
    template <class T, class U = NullType>
    struct Typelist
    {
        typedef T Head;
        typedef U Tail;
    };


    // helper structure to automatically build typelists containing n types
    template
    <
        typename T0  = NullType, typename T1  = NullType, typename T2  = NullType, typename T3  = NullType, typename T4  = NullType,
        typename T5  = NullType, typename T6  = NullType, typename T7  = NullType, typename T8  = NullType, typename T9  = NullType,
        typename T10 = NullType, typename T11 = NullType, typename T12 = NullType, typename T13 = NullType, typename T14 = NullType,
        typename T15 = NullType, typename T16 = NullType, typename T17 = NullType, typename T18 = NullType, typename T19 = NullType
    >
    struct BuildTypelist
    {
    private:
        typedef typename BuildTypelist<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19>::Result TailResult;

    public:
        typedef Typelist<T0, TailResult> Result;
    };

    template <>
    struct BuildTypelist<>
    {
        typedef NullType Result;
    };

    // typelist operation : Length
    template <class TList>
    struct Length;

    template <>
    struct Length<NullType>
    {
        enum
        {
            value = 0
        };
    };

    template <class T, class U>
    struct Length<Typelist<T, U> >
    {
        enum
        {
            value = 1 + Length<U>::value
        };
    };


    // typelist operation : TypeAt
    template <class TList, unsigned int index>
    struct TypeAt;

    template <class Head, class Tail>
    struct TypeAt<Typelist<Head, Tail>, 0>
    {
        typedef Head Result;
    };

    template <class Head, class Tail, unsigned int index>
    struct TypeAt<Typelist<Head, Tail>, index>
    {
        typedef typename TypeAt<Tail, index - 1>::Result Result;
    };


    // typelist operation : IndexOf
    template <class TList, class T>
    struct IndexOf;

    template <class T>
    struct IndexOf<NullType, T>
    {
        enum
        {
            value = -1
        };
    };

    template <class T, class Tail>
    struct IndexOf<Typelist<T, Tail>, T>
    {
        enum
        {
            value = 0
        };
    };

    template <class Head, class Tail, class T>
    struct IndexOf<Typelist<Head, Tail>, T>
    {
    private:
        enum
        {
            temp = IndexOf<Tail, T>::value
        };
    public:
        enum
        {
            value = temp == -1 ? -1 : 1 + temp
        };
    };


    // typelist operation : Append
    template <class TList, class T>
    struct Append;

    template <>
    struct Append<NullType, NullType>
    {
        typedef NullType Result;
    };

    template <class T>
    struct Append<NullType, T>
    {
        typedef Typelist<T, NullType> Result;
    };

    template <class Head, class Tail>
    struct Append<NullType, Typelist<Head, Tail> >
    {
        typedef Typelist<Head, Tail> Result;
    };

    template <class Head, class Tail, class T>
    struct Append<Typelist<Head, Tail>, T>
    {
        typedef Typelist<Head, typename Append<Tail, T>::Result> Result;
    };


    // typelist operation : Erase
    template <class TList, class T>
    struct Erase;

    template <class T>
    struct Erase<NullType, T>
    {
        typedef NullType Result;
    };

    template <class T, class Tail>
    struct Erase<Typelist<T, Tail>, T>
    {
        typedef Tail Result;
    };

    template <class Head, class Tail, class T>
    struct Erase<Typelist<Head, Tail>, T>
    {
        typedef Typelist<Head, typename Erase<Tail, T>::Result> Result;
    };


    // typelist operation : Erase All
    template <class TList, class T>
    struct EraseAll;

    template <class T>
    struct EraseAll<NullType, T>
    {
        typedef NullType Result;
    };

    template <class T, class Tail>
    struct EraseAll<Typelist<T, Tail>, T>
    {
        typedef typename EraseAll<Tail, T>::Result Result;
    };

    template <class Head, class Tail, class T>
    struct EraseAll<Typelist<Head, Tail>, T>
    {
        typedef Typelist<Head, typename EraseAll<Tail, T>::Result> Result;
    };


    // typelist operation : NoDuplicates
    template <class TList>
    struct NoDuplicates;

    template <>
    struct NoDuplicates<NullType>
    {
        typedef NullType Result;
    };

    template <class Head, class Tail>
    struct NoDuplicates<Typelist<Head, Tail> >
    {
    private:
        typedef typename NoDuplicates<Tail>::Result L1;
        typedef typename Erase<L1, Head>::Result L2;
    public:
        typedef Typelist<Head, L2> Result;
    };
} // namespace TL

#endif // CRYINCLUDE_CRYEXTENSION_TYPELIST_H

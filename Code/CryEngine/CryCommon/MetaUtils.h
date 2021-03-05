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

#ifndef CRYINCLUDE_CRYCOMMON_METAUTILS_H
#define CRYINCLUDE_CRYCOMMON_METAUTILS_H
#pragma once



namespace metautils
{
    // select

    template<bool Condition, typename Ty1, typename Ty2>
    struct select;

    template<typename Ty1, typename Ty2>
    struct select<true, Ty1, Ty2>
    {
        typedef Ty1 type;
    };

    template<typename Ty1, typename Ty2>
    struct select<false, Ty1, Ty2>
    {
        typedef Ty2 type;
    };

    // is_same
    // Identifies whether types Ty1 and Ty2 are the same including const & volatile.

    template<typename Ty1, typename Ty2>
    struct is_same;

    template<typename Ty1>
    struct is_same<Ty1, Ty1>
    {
        enum
        {
            value = true,
        };
    };

    template<typename Ty1, typename Ty2>
    struct is_same
    {
        enum
        {
            value = false,
        };
    };

    // remove_const
    // Removes top level const qualifier.

    template<class Ty>
    struct remove_const
    {
        typedef Ty type;
    };

    template<class Ty>
    struct remove_const<const Ty>
    {
        typedef Ty type;
    };

    template<class Ty>
    struct remove_const<const Ty[]>
    {
        typedef Ty type[];
    };

    template<class Ty, unsigned int N>
    struct remove_const<const Ty[N]>
    {
        typedef Ty type[N];
    };

    // is_const
    // Determines whether type Ty is const qualified.

    template <typename Ty>
    struct is_const
    {
        enum
        {
            value = false
        };
    };

    template <typename Ty>
    struct is_const<const Ty>
    {
        enum
        {
            value = true
        };
    };

    template<class Ty, unsigned int N>
    struct is_const<Ty[N]>
    {
        enum
        {
            value = false
        };
    };

    template<class Ty, unsigned int N>
    struct is_const<const Ty[N]>
    {
        enum
        {
            value = true
        };
    };

    template<class Ty>
    struct is_const<Ty&>
    {
        enum
        {
            value = false
        };
    };
};

#endif // CRYINCLUDE_CRYCOMMON_METAUTILS_H

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
#ifndef AZSTD_SMART_PTR_SHARED_ARRAY_H
#define AZSTD_SMART_PTR_SHARED_ARRAY_H

//
//  shared_array.hpp
//
//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/shared_array.htm for documentation.
//
#include <AzCore/std/smart_ptr/shared_count.h>

namespace AZStd
{
    //
    //  shared_array
    //
    //  shared_array extends shared_ptr to arrays.
    //  The array pointed to is deleted when the last shared_array pointing to it
    //  is destroyed or reset.
    //
    template<class T>
    class shared_array
    {
    private:

        // Borland 5.5.1 specific workarounds
        typedef checked_array_deleter<T> deleter;
        typedef shared_array<T> this_type;

    public:

        typedef T element_type;

        explicit shared_array(T* p = 0)
            : px(p)
            , pn(p, deleter())
        {}

        //
        // Requirements: D's copy constructor must not throw
        //
        // shared_array will release p by calling d(p)
        //
        template<class D>
        shared_array(T* p, D d)
            : px(p)
            , pn(p, d)
        {}

        template<class D, class A>
        shared_array(T* p, D d, A a)
            : px(p)
            , pn(p, d, a)
        {}

        //  generated copy constructor, assignment, destructor are fine

        void reset(T* p = 0)
        {
            AZ_Assert(p == 0 || p != px, "Self reset");
            this_type(p).swap(*this);
        }

        template <class D>
        void reset(T* p, D d)
        {
            this_type(p, d).swap(*this);
        }

        T& operator[] (AZStd::ptrdiff_t i) const  // never throws
        {
            AZ_Assert(px != 0, "Invalid data pointer");
            AZ_Assert(i >= 0, "Invalid array index");
            return px[i];
        }

        T* get() const  // never throws
        {
            return px;
        }

        typedef T* this_type::* unspecified_bool_type;
        operator unspecified_bool_type() const {
            return px == 0 ? 0 : &this_type::px;
        }                                                                              // never throws
        // operator! is redundant, but some compilers need it
        bool operator! () const { return px == 0; } // never throws

        bool unique() const // never throws
        {
            return pn.unique();
        }

        long use_count() const // never throws
        {
            return pn.use_count();
        }

        void swap(shared_array<T>& other)  // never throws
        {
            AZStd::swap(px, other.px);
            pn.swap(other.pn);
        }

    private:

        T* px;                      // contained pointer
        Internal::shared_count pn;    // reference counter
    };  // shared_array

    template<class T>
    inline bool operator==(shared_array<T> const& a, shared_array<T> const& b)                     // never throws
    {
        return a.get() == b.get();
    }

    template<class T>
    inline bool operator!=(shared_array<T> const& a, shared_array<T> const& b)                     // never throws
    {
        return a.get() != b.get();
    }

    template<class T>
    inline bool operator<(shared_array<T> const& a, shared_array<T> const& b)                     // never throws
    {
        return AZStd::less<T*>()(a.get(), b.get());
    }

    template<class T>
    void swap(shared_array<T>& a, shared_array<T>& b)                     // never throws
    {
        a.swap(b);
    }
} // namespace AZStd

#endif  // #ifndef AZSTD_SMART_PTR_SHARED_ARRAY_H
#pragma once

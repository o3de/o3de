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
#ifndef AZSTD_SMART_PTR_SCOPED_ARRAY_H
#define AZSTD_SMART_PTR_SCOPED_ARRAY_H

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_array.htm
//

#include <AzCore/std/smart_ptr/checked_delete.h>

namespace AZStd
{
    // Debug hooks
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
    void sp_array_constructor_hook(void* p);
    void sp_array_destructor_hook(void* p);
#endif

    //  scoped_array extends scoped_ptr to arrays. Deletion of the array pointed to
    //  is guaranteed, either on destruction of the scoped_array or via an explicit
    //  reset(). Use shared_array or AZStd::vector if your needs are more complex.
    template<class T>
    class scoped_array                   // noncopyable
    {
    private:

        T* px;

        scoped_array(scoped_array const&);
        scoped_array& operator=(scoped_array const&);

        typedef scoped_array<T> this_type;

        void operator==(scoped_array const&) const;
        void operator!=(scoped_array const&) const;

    public:

        typedef T element_type;

        explicit scoped_array(T* p = 0)
            : px(p)                                  // never throws
        {
    #if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            AZStd::sp_array_constructor_hook(px);
    #endif
        }

        ~scoped_array() // never throws
        {
    #if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            AZStd::sp_array_destructor_hook(px);
    #endif
            AZStd::checked_array_delete(px);
        }

        void reset(T* p = 0)  // never throws
        {
            AZ_Assert(p == 0 || p != px, "Self reset");
            this_type(p).swap(*this);
        }

        T& operator[](AZStd::ptrdiff_t i) const  // never throws
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

        void swap(scoped_array& b)  // never throws
        {
            T* tmp = b.px;
            b.px = px;
            px = tmp;
        }
    };

    template<class T>
    inline void swap(scoped_array<T>& a, scoped_array<T>& b)                     // never throws
    {
        a.swap(b);
    }
} // namespace AZStd

#endif  // #ifndef AZSTD_SMART_PTR_SCOPED_ARRAY_H
#pragma once

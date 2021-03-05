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
#ifndef AZSTD_SMART_PTR_SCOPED_PTR_H
#define AZSTD_SMART_PTR_SCOPED_PTR_H

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_ptr.htm
//

#include <AzCore/std/smart_ptr/checked_delete.h>

namespace AZStd
{
    // Debug hooks
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
    void sp_scalar_constructor_hook(void* p);
    void sp_scalar_destructor_hook(void* p);
#endif

    //  scoped_ptr mimics a built-in pointer except that it guarantees deletion
    //  of the object pointed to, either on destruction of the scoped_ptr or via
    //  an explicit reset(). scoped_ptr is a simple solution for simple needs;
    //  use shared_ptr or std::auto_ptr if your needs are more complex.

    template<class T>
    class scoped_ptr                   // noncopyable
    {
    private:
        T* px;

        scoped_ptr(scoped_ptr const&);
        scoped_ptr& operator=(scoped_ptr const&);

        typedef scoped_ptr<T> this_type;

        void operator==(scoped_ptr const&) const;
        void operator!=(scoped_ptr const&) const;

    public:

        typedef T element_type;

        explicit scoped_ptr(T* p = 0)
            : px(p)                               // never throws
        {
    #if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            AZStd::sp_scalar_constructor_hook(px);
    #endif
        }

#ifndef AZ_NO_AUTO_PTR
        explicit scoped_ptr(std::auto_ptr<T> p)
            : px(p.release())                                        // never throws
        {
    #if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            AZStd::sp_scalar_constructor_hook(px);
    #endif
        }
#endif // AZ_NO_AUTO_PTR

        ~scoped_ptr() // never throws
        {
    #if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            AZStd::sp_scalar_destructor_hook(px);
    #endif
            AZStd::checked_delete(px);
        }

        void reset(T* p = 0)  // never throws
        {
            AZ_Assert(p == 0 || p != px, "Self reset");
            this_type(p).swap(*this);
        }

        T& operator*() const  // never throws
        {
            AZ_Assert(px != 0, "You can't access a null pointer");
            return *px;
        }

        T* operator->() const  // never throws
        {
            AZ_Assert(px != 0, "You can't access a null pointer");
            return px;
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

        void swap(scoped_ptr& b)  // never throws
        {
            T* tmp = b.px;
            b.px = px;
            px = tmp;
        }
    };

    template<class T>
    inline void swap(scoped_ptr<T>& a, scoped_ptr<T>& b)                     // never throws
    {
        a.swap(b);
    }

    // get_pointer(p) is a generic way to say p.get()

    template<class T>
    inline T* get_pointer(scoped_ptr<T> const& p)
    {
        return p.get();
    }
} // namespace AZStd

#endif // #ifndef AZSTD_SMART_PTR_SCOPED_PTR_H
#pragma once

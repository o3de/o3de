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
#ifndef AZSTD_SMART_PTR_INTRUSIVE_PTR_H
#define AZSTD_SMART_PTR_INTRUSIVE_PTR_H

//
//  intrusive_ptr.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/intrusive_ptr.html for documentation.
//

#include <AzCore/std/smart_ptr/sp_convertible.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/typetraits/is_abstract.h>

namespace AZStd
{
    /**
     * By default we use object internal reference counting. We will call
     * pObj->add_ref and pObj->release. The pointer will call
     * class functions to do the reference counting. There is
     * no need for ADL and you can implement your functions as you wish.
     *
     * If you want external reference count (boost style), which is useful
     * for structs or retrofitting in existing code (you don't want to change).
     * To do so specialize IntrusivePtrCountPolicy for your object type.
     * Example:
     *   template<>
     *   struct IntrusivePtrCountPolicy<MyObject> {
     *          static AZ_FORCE_INLINE void add_ref(MyObject* p)    { your code to add a reference }
     *          static AZ_FORCE_INLINE void release(MyObject* p)    { your code to remove a reference }
     *   }
     */
    template<class T>
    struct IntrusivePtrCountPolicy
    {
        static AZ_FORCE_INLINE void add_ref(T* p)   { p->add_ref(); }
        static AZ_FORCE_INLINE void release(T* p)   { p->release(); }
    };

    /**
     * intrusive_ptr
     *
     * A smart pointer that uses intrusive reference counting.
     *
     * This pointer is not part of the C++ standard yet. It is considered AZStd
     * extension. It extends the boost implementation.
     *
     * There are many benefits in using intrusive reference counting.
     * They are the recommended pointer in performance critical systems. The
     * reason for that is you can control: no allocation occurs (counter is internal),
     * you control the size of ref count, you know if it needs to be atomic or not,
     * better cache coherency, you can convert back and forth to raw pointer, etc.
     *
     * For all other cases shared_ptr is recommended. In shared pointer if you use
     * make_shared/allocate_shared you will save the second allocation too.
     */
    template<class T>
    class intrusive_ptr
    {
    private:
        // Allow construction between compatible contained types (base / derived classes).
        template <typename U>
        friend class intrusive_ptr;

        typedef intrusive_ptr this_type;
        typedef IntrusivePtrCountPolicy<T> CountPolicy;

    public:
        typedef T   element_type;
        typedef T   value_type;

        intrusive_ptr()
            : px(0)
        {
        }

        intrusive_ptr(T* p)
            : px(p)
        {
            if (px != 0)
            {
                CountPolicy::add_ref(px);
            }
        }

        template<class U>
        intrusive_ptr(intrusive_ptr<U> const& rhs, enable_if_t<is_convertible<U*, T*>::value, int> = 0)
            : px(rhs.get())
        {
            if (px != 0)
            {
                CountPolicy::add_ref(px);
            }
        }
        intrusive_ptr(intrusive_ptr const& rhs)
            : px(rhs.px)
        {
            if (px != 0)
            {
                CountPolicy::add_ref(px);
            }
        }
        
        ~intrusive_ptr()
        {
            if (px != 0)
            {
                CountPolicy::release(px);
            }
        }
        
        template<class U>
        enable_if_t<is_convertible<U*, T*>::value, intrusive_ptr&> operator=(intrusive_ptr<U> const& rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        template<class U>
        intrusive_ptr(intrusive_ptr<U>&& rhs, enable_if_t<is_convertible<U*, T*>::value, int> = 0)
            : px(rhs.get())
        {
            rhs.px = 0;
        }
        intrusive_ptr(intrusive_ptr&& rhs)
            : px(rhs.px)
        {
            rhs.px = 0;
        }

        template<class U>
        enable_if_t<is_convertible<U*, T*>::value, intrusive_ptr&> operator=(intrusive_ptr<U>&& rhs)
        {
            this_type(move(rhs)).swap(*this);
            return *this;
        }
        intrusive_ptr& operator=(intrusive_ptr&& rhs)
        {
            this_type(static_cast< intrusive_ptr && >(rhs)).swap(*this);
            return *this;
        }

        intrusive_ptr& operator=(intrusive_ptr const& rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }
        
        intrusive_ptr& operator=(T* rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }
        
        void reset()
        {
            this_type().swap(*this);
        }
        
        void reset(T* rhs)
        {
            this_type(rhs).swap(*this);
        }
        
        T* get() const
        {
            return px;
        }
        
        T& operator*() const
        {
            AZ_Assert(px != 0, "You can't dereference a null pointer");
            return *px;
        }

        T* operator->() const
        {
            AZ_Assert(px != 0, "You can't dereference a null pointer");
            return px;
        }

        typedef T* this_type::* unspecified_bool_type;
        operator unspecified_bool_type() const {
            return px == 0 ? 0 : &this_type::px;
        }                                                                              // never throws

        // operator! is redundant, but some compilers need it
        bool operator! () const { return px == 0; } // never throws

        void swap(intrusive_ptr& rhs)
        {
            T* tmp = px;
            px = rhs.px;
            rhs.px = tmp;
        }

    private:
        T* px;
    };

    template<class T, class U>
    inline bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() == b.get();
    }

    template<class T, class U>
    inline bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() != b.get();
    }

    template<class T, class U>
    inline bool operator==(intrusive_ptr<T> const& a, U* b)
    {
        return a.get() == b;
    }

    template<class T, class U>
    inline bool operator!=(intrusive_ptr<T> const& a, U* b)
    {
        return a.get() != b;
    }

    template<class T, class U>
    inline bool operator==(T* a, intrusive_ptr<U> const& b)
    {
        return a == b.get();
    }

    template<class T, class U>
    inline bool operator!=(T* a, intrusive_ptr<U> const& b)
    {
        return a != b.get();
    }

    template<class T>
    inline bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<T> const& b)
    {
        return (a.get() < b.get());
    }

    template<class T>
    void swap(intrusive_ptr<T>& lhs, intrusive_ptr<T>& rhs)
    {
        lhs.swap(rhs);
    }

    // mem_fn support
    template<class T>
    T* get_pointer(intrusive_ptr<T> const& p)
    {
        return p.get();
    }

    template<class T, class U>
    intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const& p)
    {
        return static_cast<T*>(p.get());
    }

    template<class T, class U>
    intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const& p)
    {
        return const_cast<T*>(p.get());
    }

    template<class T, class U>
    intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const& p)
    {
        return azdynamic_cast<T*>(p.get());
    }

    template <typename T>
    struct hash;

    // hashing support for STL containers
    template <typename T>
    struct hash<intrusive_ptr<T>>
    {
        inline size_t operator()(const intrusive_ptr<T>& value) const
        {
            return hash<T*>()(value.get());
        }
    };

    // operator<< - not supported
} // namespace AZStd

#endif  // #ifndef AZSTD_SMART_PTR_INTRUSIVE_PTR_H
#pragma once

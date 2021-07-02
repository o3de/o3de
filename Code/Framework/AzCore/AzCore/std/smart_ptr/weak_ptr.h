/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_SMART_PTR_WEAK_PTR_H
#define AZSTD_SMART_PTR_WEAK_PTR_H

//
//  weak_ptr.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/weak_ptr.htm for documentation.
//
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZStd
{
    template<class T>
    class weak_ptr
    {
    private:
        typedef weak_ptr<T> this_type;
    public:
        typedef T element_type;
        weak_ptr()
            : px(0)
            , pn()              // never throws in 1.30+
        {}
        //  generated copy constructor, assignment, destructor are fine


        //
        //  The "obvious" converting constructor implementation:
        //
        //  template<class Y>
        //  weak_ptr(weak_ptr<Y> const & r): px(r.px), pn(r.pn) // never throws
        //  {
        //  }
        //
        //  has a serious problem.
        //
        //  r.px may already have been invalidated. The px(r.px)
        //  conversion may require access to *r.px (virtual inheritance).
        //
        //  It is not possible to avoid spurious access violations since
        //  in multithreaded programs r.px may be invalidated at any point.
        //
        template<class Y>
        weak_ptr(weak_ptr<Y> const& r, typename AZStd::Internal::sp_enable_if_convertible<Y, T>::type = AZStd::Internal::sp_empty())
            : px(r.lock().get())
            , pn(r.pn)                 // never throws
        {}
        template<class Y>
        weak_ptr(weak_ptr<Y>&& r, typename AZStd::Internal::sp_enable_if_convertible<Y, T>::type = AZStd::Internal::sp_empty())
            : px(r.lock().get())
            , pn(static_cast< AZStd::Internal::weak_count && >(r.pn))                       // never throws
        {
            r.px = 0;
        }
        // for better efficiency in the T == Y case
        weak_ptr(weak_ptr&& r)
            : px(r.px)
            , pn(static_cast< AZStd::Internal::weak_count && >(r.pn))                                      // never throws
        {
            r.px = 0;
        }

        // for better efficiency in the T == Y case
        weak_ptr& operator=(weak_ptr&& r)     // never throws
        {
            this_type(static_cast< weak_ptr && >(r)).swap(*this);
            return *this;
        }

        template<class Y>
        weak_ptr(shared_ptr<Y> const& r, typename AZStd::Internal::sp_enable_if_convertible<Y, T>::type = AZStd::Internal::sp_empty())
            : px(r.px)
            , pn(r.pn)           // never throws
        {
        }

        weak_ptr(const this_type& r)
            : px(r.lock().get())
            , pn(r.pn)
        {
        }

        template<class Y>
        weak_ptr& operator=(weak_ptr<Y> const& r)   // never throws
        {
            px = r.lock().get();
            pn = r.pn;
            return *this;
        }

        this_type& operator=(const this_type& r)
        {
            px = r.lock().get();
            pn = r.pn;
            return *this;
        }

        template<class Y>
        weak_ptr& operator=(weak_ptr<Y>&& r)
        {
            this_type(static_cast< weak_ptr<Y> && >(r)).swap(*this);
            return *this;
        }

        template<class Y>
        weak_ptr& operator=(shared_ptr<Y> const& r)   // never throws
        {
            px = r.px;
            pn = r.pn;
            return *this;
        }


        shared_ptr<T> lock() const // never throws
        {
            return shared_ptr<element_type>(*this, AZStd::Internal::sp_nothrow_tag());
        }

        long use_count() const // never throws
        {
            return pn.use_count();
        }

        bool expired() const // never throws
        {
            return pn.use_count() == 0;
        }

        bool _empty() const // extension, not in std::weak_ptr
        {
            return pn.empty();
        }

        void reset() // never throws in 1.30+
        {
            this_type().swap(*this);
        }

        void swap(this_type& other)  // never throws
        {
            AZStd::swap(px, other.px);
            pn.swap(other.pn);
        }

        void _internal_assign(T* px2, AZStd::Internal::shared_count const& pn2)
        {
            px = px2;
            pn = pn2;
        }

        template<class Y>
        bool _internal_less(weak_ptr<Y> const& rhs) const
        {
            return pn < rhs.pn;
        }

        // Tasteless as this may seem, making all members public allows member templates
        // to work in the absence of member template friends. (Matthew Langston)
    private:
        template<class Y>
        friend class weak_ptr;
        template<class Y>
        friend class shared_ptr;

        T* px;                        // contained pointer
        AZStd::Internal::weak_count pn; // reference counter
    };  // weak_ptr

    template<class T, class U>
    inline bool operator<(weak_ptr<T> const& a, weak_ptr<U> const& b)
    {
        return a._internal_less(b);
    }

    template<class T>
    void swap(weak_ptr<T>& a, weak_ptr<T>& b)
    {
        a.swap(b);
    }
} // namespace AZStd

/*#ifdef AZ_COMPILER_MSVC
# pragma warning(pop)
#endif */

#endif  // #ifndef AZSTD_SMART_PTR_WEAK_PTR_H
#pragma once

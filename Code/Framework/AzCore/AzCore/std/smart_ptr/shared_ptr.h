/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_SMART_PTR_SHARED_PTR
#define AZSTD_SMART_PTR_SHARED_PTR

//////////////////////////////////////////////////////////////////////////
// Standard C++0x 20.8.10.2, which was drafted from boost.
// We use as a base boost 1.46 implementation
//
// IMPORTANT:
// Even though we do follow the standard 100%, there are some important internal
// differences (from the boost implementation) and some important notes on
// how we use shared_ptr.
// - We never call new ... all allocation are made using the allocators.
// - By default (when no user allocator is provided) we use AZStd::allocator
//     which keeps the same behavior as all containers.
// - Allocators are passed at construction time; this keeps the shared_ptr<T> as is in the
//     standard and keeps the templated type simple.
// - As with containers in libs, you should always use lib allocators and you should never
//     use the default one. This is because we work in memory managed environments and the lib
//     users have the right to choose where to allocate memory from! \ref AZStd::allocator
// \ref Containers
//

//
//  shared_ptr.hpp
//
//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001-2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/shared_ptr.htm for documentation.
//

#include <AzCore/std/allocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_count.h>
#include <AzCore/std/smart_ptr/sp_convertible.h>
#include <AzCore/std/utils.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZStd
{
    template<class T>
    class shared_ptr;
    template<class T>
    class weak_ptr;
    template<class T>
    class enable_shared_from_this;
    template<class T>
    class enable_shared_from_this2;

    namespace Internal
    {
        struct static_cast_tag {};
        struct const_cast_tag {};
        struct dynamic_cast_tag {};
        struct reinterpret_cast_tag {};
        struct rtti_cast_tag {};
        template<class T>
        struct shared_ptr_traits
        {
            typedef T& reference;
        };
        template<>
        struct shared_ptr_traits<void>
        {
            typedef void reference;
        };
        // CV specializations
        template<>
        struct shared_ptr_traits<void const>
        {
            typedef void reference;
        };
        template<>
        struct shared_ptr_traits<void volatile>
        {
            typedef void reference;
        };
        template<>
        struct shared_ptr_traits<void const volatile>
        {
            typedef void reference;
        };

        // enable_shared_from_this support
        template< class X, class Y, class T >
        inline void sp_enable_shared_from_this(AZStd::shared_ptr<X> const* ppx, Y const* py, AZStd::enable_shared_from_this< T > const* pe)
        {
            if (pe != 0)
            {
                pe->_internal_accept_owner(ppx, const_cast< Y* >(py));
            }
        }
        template< class X, class Y, class T >
        inline void sp_enable_shared_from_this(AZStd::shared_ptr<X>* ppx, Y const* py, AZStd::enable_shared_from_this2< T > const* pe)
        {
            if (pe != 0)
            {
                pe->_internal_accept_owner(ppx, const_cast< Y* >(py));
            }
        }
        inline void sp_enable_shared_from_this(...)   {}

#ifndef AZ_NO_AUTO_PTR
        // rvalue auto_ptr support based on a technique by Dave Abrahams
        template< class T, class R >
        struct sp_enable_if_auto_ptr   {};
        template< class T, class R >
        struct sp_enable_if_auto_ptr< std::auto_ptr< T >, R >
        {
            typedef R type;
        };
#endif
    } // namespace Internal

    //
    //  shared_ptr
    //
    //  An enhanced relative of scoped_ptr with reference counted copy semantics.
    //  The object pointed to is deleted when the last shared_ptr pointing to it
    //  is destroyed or reset.
    //
    template<class T>
    class shared_ptr
    {
    private:
        typedef shared_ptr<T> this_type;
    public:
        typedef T element_type;
        typedef T value_type;
        typedef typename AZStd::Internal::shared_ptr_traits<T>::reference reference;
        shared_ptr()
            : px(0)
            , pn()                // never throws in 1.30+
        {
        }
        shared_ptr(nullptr_t)
            : shared_ptr()
        {
        }
        template<class Y>
        explicit shared_ptr(Y* p)
            : px(p)
            , pn(p, AZStd::allocator())                                    // Y must be complete
        {
            AZStd::Internal::sp_enable_shared_from_this(this, p, p);
        }
        //
        // Requirements: D's copy constructor must not throw
        //
        // shared_ptr will release p by calling d(p)
        //
        template<class Y, class D>
        shared_ptr(Y* p, D d)
            : px(p)
            , pn(p, d, AZStd::allocator())
        {
            AZStd::Internal::sp_enable_shared_from_this(this, p, p);
        }
        // As above, but with allocator. A's copy constructor shall not throw.
        template<class Y, class D, class A>
        shared_ptr(Y* p, D d, const A& a)
            : px(p)
            , pn(p, d, a)
        {
            AZStd::Internal::sp_enable_shared_from_this(this, p, p);
        }
        //  generated copy constructor, destructor are fine
        template<class Y>
        explicit shared_ptr(weak_ptr<Y> const& r)
            : pn(r.pn)                                       // may throw
        {
            // it is now safe to copy r.px, as pn(r.pn) did not throw
            px = r.px;
        }
        template<class Y>
        shared_ptr(weak_ptr<Y> const& r, AZStd::Internal::sp_nothrow_tag)
            : px(0)
            , pn(r.pn, AZStd::Internal::sp_nothrow_tag())                                                                            // never throws
        {
            if (!pn.empty())
            {
                px = r.px;
            }
        }
        template<class Y>
        shared_ptr(shared_ptr<Y> const& r, typename AZStd::Internal::sp_enable_if_convertible<Y, T>::type = AZStd::Internal::sp_empty())
            : px(r.px)
            , pn(r.pn)           // never throws
        {
        }

        // aliasing
        template< class Y >
        shared_ptr(shared_ptr<Y> const& r, T* p)
            : px(p)
            , pn(r.pn)                                                    // never throws
        {}
        template<class Y>
        shared_ptr(shared_ptr<Y> const& r, AZStd::Internal::static_cast_tag)
            : px(static_cast<element_type*>(r.px))
            , pn(r.pn)
        {}
        template<class Y>
        shared_ptr(shared_ptr<Y> const& r, AZStd::Internal::const_cast_tag)
            : px(const_cast<element_type*>(r.px))
            , pn(r.pn)
        {}

        template <class Y>
        shared_ptr(shared_ptr<Y> const& r, AZStd::Internal::reinterpret_cast_tag)
            : px(reinterpret_cast<element_type*>(r.px))
            , pn(r.pn)
        {
        }

#if defined(AZ_USE_RTTI)
        template<class Y>
        shared_ptr(shared_ptr<Y> const& r, AZStd::Internal::dynamic_cast_tag)
            : px(dynamic_cast<element_type*>(r.px))
            , pn(r.pn)
        {
            if (px == 0) // need to allocate new counter -- the cast failed
            {
                pn = AZStd::Internal::shared_count();
            }
        }
#endif
        template <class Y>
        shared_ptr(shared_ptr<Y> const& r, AZStd::Internal::rtti_cast_tag)
            : px(azrtti_cast<element_type*>(r.px))
            , pn(r.pn)
        {
            if (px == nullptr)
            {
                pn = AZStd::Internal::shared_count();
            }
        }

#ifndef AZ_NO_AUTO_PTR
        template<class Y>
        explicit shared_ptr(std::auto_ptr<Y>& r)
            : px(r.get())
            , pn()
        {
            Y* tmp = r.get();
            pn = AZStd::Internal::shared_count(r, AZStd::allocator());
            AZStd::Internal::sp_enable_shared_from_this(this, tmp, tmp);
        }
        template<class Y, class A>
        explicit shared_ptr(std::auto_ptr<Y>& r, const A& a)
            : px(r.get())
            , pn()
        {
            Y* tmp = r.get();
            pn = AZStd::Internal::shared_count(r, a);
            AZStd::Internal::sp_enable_shared_from_this(this, tmp, tmp);
        }
        template<class Ap>
        explicit shared_ptr(Ap r, typename AZStd::Internal::sp_enable_if_auto_ptr<Ap, int>::type = 0)
            : px(r.get())
            , pn()
        {
            typename Ap::element_type * tmp = r.get();
            pn = AZStd::Internal::shared_count(r);
            AZStd::Internal::sp_enable_shared_from_this(this, tmp, tmp);
        }
#endif // AZ_NO_AUTO_PTR

        // assignment
        shared_ptr& operator=(shared_ptr const& r)     // never throws
        {
            this_type(r).swap(*this);
            return *this;
        }

        template<class Y>
        shared_ptr& operator=(shared_ptr<Y> const& r)   // never throws
        {
            this_type(r).swap(*this);
            return *this;
        }
#ifndef AZ_NO_AUTO_PTR
        template<class Y>
        shared_ptr& operator=(std::auto_ptr<Y>& r)
        {
            this_type(r).swap(*this);
            return *this;
        }
        template<class Ap>
        typename AZStd::Internal::sp_enable_if_auto_ptr< Ap, shared_ptr& >::type operator=(Ap r)
        {
            this_type(r).swap(*this);
            return *this;
        }
#endif // AZ_NO_AUTO_PTR
       // Move support
        shared_ptr(shared_ptr&& r)
            : px(r.px)
            , pn()                                      // never throws
        {
            pn.swap(r.pn);
            r.px = 0;
        }
        template<class Y>
        shared_ptr(shared_ptr<Y>&& r, typename AZStd::Internal::sp_enable_if_convertible<Y, T>::type = AZStd::Internal::sp_empty())
            : px(r.px)
            , pn()         // never throws
        {
            pn.swap(r.pn);
            r.px = 0;
        }
        shared_ptr& operator=(shared_ptr&& r)     // never throws
        {
            this_type(static_cast< shared_ptr && >(r)).swap(*this);
            return *this;
        }
        template<class Y>
        shared_ptr& operator=(shared_ptr<Y>&& r)     // never throws
        {
            this_type(static_cast< shared_ptr<Y> && >(r)).swap(*this);
            return *this;
        }
        template<class Y, class Deleter>
        shared_ptr(unique_ptr<Y, Deleter>&& r)
            : shared_ptr(r.release(), r.get_deleter())
        {}
        template<class Y, class Deleter>
        shared_ptr& operator=(unique_ptr<Y, Deleter>&& r)
        {
            this_type(AZStd::move(r)).swap(*this);
            return *this;
        }
        shared_ptr(const this_type& r)
            : px(r.px)
            , pn(r.pn)
        {}
        void reset() // never throws in 1.30+
        {
            this_type().swap(*this);
        }
        template<class Y>
        void reset(Y* p)                    // Y must be complete
        {
            AZ_Assert(p == 0 || p != px, "Self reset error"); // catch self-reset errors
            this_type(p).swap(*this);
        }
        template<class Y, class D>
        void reset(Y* p, D d)                     { this_type(p, d).swap(*this); }
        template<class Y, class D, class A>
        void reset(Y* p, D d, A a)       { this_type(p, d, a).swap(*this); }
        template<class Y>
        void reset(shared_ptr<Y> const& r, T* p)          { this_type(r, p).swap(*this); }
        reference operator* () const // never throws
        {
            AZ_Assert(px != 0, "Pointer is NULL, you can't dereference!");
            return *px;
        }
        T* operator-> () const  // never throws
        {
            AZ_Assert(px != 0, "Pointer is NULL, you can't dereference!");
            return px;
        }

        T* get() const    { return px; }                // never throws

        typedef T* this_type::* unspecified_bool_type;
        operator unspecified_bool_type() const {
            return px == 0 ? 0 : &this_type::px;
        }                                                                              // never throws
        // operator! is redundant, but some compilers need it
        bool operator! () const { return px == 0; } // never throws
        bool unique() const     { return pn.unique(); }     // never throws
        long use_count() const  { return pn.use_count(); }  // never throws
        void swap(shared_ptr<T>& other)  // never throws
        {
            AZStd::swap(px, other.px);
            pn.swap(other.pn);
        }
        template<class Y>
        bool _internal_less(shared_ptr<Y> const& rhs) const          { return pn < rhs.pn; }
        void* _internal_get_deleter(AZStd::Internal::sp_typeinfo const& ti) const   { return pn.get_deleter(ti); }
        bool _internal_equiv(shared_ptr const& r) const                              { return px == r.px && pn == r.pn; }

        // Tasteless as this may seem, making all members public allows member templates
        // to work in the absence of member template friends. (Matthew Langston)
    private:
        template<class Y>
        friend class shared_ptr;
        template<class Y>
        friend class weak_ptr;

        T* px;                              // contained pointer
        AZStd::Internal::shared_count pn;    // reference counter
    };  // shared_ptr

    template<class T, class U>
    inline bool operator==(shared_ptr<T> const& a, shared_ptr<U> const& b) { return a.get() == b.get(); }
    template<class T, class U>
    inline bool operator!=(shared_ptr<T> const& a, shared_ptr<U> const& b) { return a.get() != b.get(); }

    template<class T, class U>
    inline bool operator<(shared_ptr<T> const& a, shared_ptr<U> const& b)  { return a._internal_less(b); }
    template<class T>
    inline void swap(shared_ptr<T>& a, shared_ptr<T>& b)                            { a.swap(b); }
    template<class T, class U>
    shared_ptr<T> static_pointer_cast(shared_ptr<U> const& r)               { return shared_ptr<T>(r, AZStd::Internal::static_cast_tag()); }
    template<class T, class U>
    shared_ptr<T> const_pointer_cast(shared_ptr<U> const& r)                { return shared_ptr<T>(r, AZStd::Internal::const_cast_tag()); }
    // NOTE: This is disabled when rtti is disabled in the compiler flags for your build target
#if defined(AZ_USE_RTTI)
    template<class T, class U>
    shared_ptr<T> dynamic_pointer_cast(shared_ptr<U> const& r)              { return shared_ptr<T>(r, AZStd::Internal::dynamic_cast_tag()); }
#endif
    template <class T, class U>
    shared_ptr<T> reinterpret_pointer_cast(shared_ptr<U> const& r)          { return shared_ptr<T>(r, AZStd::Internal::reinterpret_cast_tag()); }
    // extension: uses azrtti_cast and AzRtti for dynamic casting without compiler rtti
    template <class T, class U>
    shared_ptr<T> rtti_pointer_cast(shared_ptr<U> const& r)                 { return shared_ptr<T>(r, AZStd::Internal::rtti_cast_tag()); }

    // get_pointer() enables AZStd::mem_fn to recognize shared_ptr
    template<class T>
    inline T* get_pointer(shared_ptr<T> const& p)           { return p.get(); }
    // operator << we don't use or support the streams
    // get_deleter
    template<class D, class T>
    D* get_deleter(shared_ptr<T> const& p)
    {
        return static_cast<D*>(p._internal_get_deleter(aztypeid(D)));
    }
} // namespace AZStd

#endif  // #ifndef AZSTD_SMART_PTR_SHARED_PTR
#pragma once

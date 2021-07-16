/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_SMART_PTR_MAKE_SHARED_H
#define AZSTD_SMART_PTR_MAKE_SHARED_H

//  make_shared.hpp
//
//  Copyright (c) 2007, 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/make_shared.html
//  for documentation.

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/typetraits/aligned_storage.h>

namespace AZStd
{
    namespace Internal
    {
        template< class T >
        class sp_ms_deleter
        {
        private:

            typedef typename AZStd::aligned_storage< sizeof(T), ::AZStd::alignment_of< T >::value >::type storage_type;

            bool initialized_;
            storage_type storage_;

        private:

            void destroy()
            {
                if (initialized_)
                {
                    reinterpret_cast< T* >(&storage_)->~T();
                    initialized_ = false;
                }
            }

        public:
            sp_ms_deleter()
                : initialized_(false)
            {
            }

            // optimization: do not copy storage_
            sp_ms_deleter(sp_ms_deleter const&)
                : initialized_(false)
            {
            }

            ~sp_ms_deleter()
            {
                destroy();
            }

            void operator()(T*)
            {
                destroy();
            }

            void* address()
            {
                return &storage_;
            }

            void set_initialized()
            {
                initialized_ = true;
            }
        };

        template< class T >
        T && sp_forward(T & t)
        {
            return static_cast< T && >(t);
        }
    } // namespace Internal

    // Zero-argument versions
    //
    // Used even when variadic templates are available because of the new T() vs new T issue

    template< class T >
    AZStd::shared_ptr< T > make_shared()
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_inplace_tag< AZStd::Internal::sp_ms_deleter< T > >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T();
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A >
    AZStd::shared_ptr< T > allocate_shared(A const& a)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_inplace_tag< AZStd::Internal::sp_ms_deleter< T > >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T();
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    // Variadic templates, rvalue reference
    template< class T, class ... Args >
    AZStd::shared_ptr< T > make_shared(Args&& ... args)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_inplace_tag<AZStd::Internal::sp_ms_deleter< T > >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(AZStd::Internal::sp_forward<Args>(args) ...);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class Arg1, class ... Args >
    AZStd::shared_ptr< T > allocate_shared(A const& a, Arg1&& arg1, Args&& ... args)
    {
        AZStd::shared_ptr< T > pt(static_cast<T*>(0), AZStd::Internal::sp_inplace_tag<AZStd::Internal::sp_ms_deleter< T > >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new(pv)T(AZStd::Internal::sp_forward<Arg1>(arg1), AZStd::Internal::sp_forward<Args>(args) ...);
        pd->set_initialized();

        T* pt2 = static_cast<T*>(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

} // namespace AZStd

#endif // #ifndef AZSTD_SMART_PTR_MAKE_SHARED_H
#pragma once

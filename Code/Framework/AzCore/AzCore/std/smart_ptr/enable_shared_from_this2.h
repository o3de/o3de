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
#ifndef AZSTD_ENABLE_SHARED_FROM_THIS2_H
#define AZSTD_ENABLE_SHARED_FROM_THIS2_H

//
//  enable_shared_from_this2.hpp
//
//  Copyright 2002, 2009 Peter Dimov
//  Copyright 2008 Frank Mori Hess
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZStd
{
    namespace Internal
    {
        class esft2_deleter_wrapper
        {
        private:
            shared_ptr<void> deleter_;
        public:
            esft2_deleter_wrapper()
            {
            }
            template< class T >
            void set_deleter(shared_ptr<T> const& deleter)
            {
                deleter_ = deleter;
            }
            template< class T>
            void operator()(T*)
            {
                AZ_Assert(deleter_.use_count() <= 1, "Deleter is in use!");
                deleter_.reset();
            }
        };
    } // namespace Internal

    template< class T >
    class enable_shared_from_this2
    {
    protected:

        enable_shared_from_this2()
        {
        }

        enable_shared_from_this2(enable_shared_from_this2 const&)
        {
        }

        enable_shared_from_this2& operator=(enable_shared_from_this2 const&)
        {
            return *this;
        }

        ~enable_shared_from_this2()
        {
            AZ_Assert(shared_this_.use_count() <= 1, "Shared pointer is still in use");  // make sure no dangling shared_ptr objects exist
        }

    private:

        mutable weak_ptr<T> weak_this_;
        mutable shared_ptr<T> shared_this_;

    public:

        shared_ptr<T> shared_from_this()
        {
            init_weak_once();
            return shared_ptr<T>(weak_this_);
        }

        shared_ptr<T const> shared_from_this() const
        {
            init_weak_once();
            return shared_ptr<T>(weak_this_);
        }

    private:

        void init_weak_once() const
        {
            if (weak_this_._empty())
            {
                shared_this_.reset(static_cast< T* >(0), Internal::esft2_deleter_wrapper());
                weak_this_ = shared_this_;
            }
        }

    public: // actually private, but avoids compiler template friendship issues

        // Note: invoked automatically by shared_ptr; do not call
        template<class X, class Y>
        void _internal_accept_owner(shared_ptr<X>* ppx, Y* py) const
        {
            AZ_Assert(ppx != 0, "Pointer is NULL");

            if (weak_this_.use_count() == 0)
            {
                weak_this_ = shared_ptr<T>(*ppx, py);
            }
            else if (shared_this_.use_count() != 0)
            {
                AZ_Assert(ppx->unique(), "Weak pointer should not exist!");   // no weak_ptrs should exist either, but there's no way to check that

                Internal::esft2_deleter_wrapper* pd = AZStd::get_deleter<Internal::esft2_deleter_wrapper>(shared_this_);
                AZ_Assert(pd != 0, "Deleter wrapper is NULL");

                pd->set_deleter(*ppx);

                ppx->reset(shared_this_, ppx->get());
                shared_this_.reset();
            }
        }
    };
} // namespace AZStd

#endif  // #ifndef AZSTD_ENABLE_SHARED_FROM_THIS2_H
#pragma once

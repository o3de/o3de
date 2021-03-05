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
#ifndef AZSTD_SMART_PTR_ENABLE_SHARED_FROM_THIS_H
#define AZSTD_SMART_PTR_ENABLE_SHARED_FROM_THIS_H

//
//  enable_shared_from_this.hpp
//
//  Copyright 2002, 2009 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  http://www.boost.org/libs/smart_ptr/enable_shared_from_this.html
//

#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZStd
{
    template<class T>
    class enable_shared_from_this
    {
    protected:

        enable_shared_from_this()
        {
        }

        enable_shared_from_this(enable_shared_from_this const&)
        {
        }

        enable_shared_from_this& operator=(enable_shared_from_this const&)
        {
            return *this;
        }

        ~enable_shared_from_this()
        {
        }

    public:

        shared_ptr<T> shared_from_this()
        {
            shared_ptr<T> p(weak_this_);
            AZ_Assert(p.get() == this, "Pointer mismatch");
            return p;
        }

        shared_ptr<T const> shared_from_this() const
        {
            shared_ptr<T const> p(weak_this_);
            AZ_Assert(p.get() == this, "Pointer mismatch");
            return p;
        }

    public: // actually private, but avoids compiler template friendship issues

        // Note: invoked automatically by shared_ptr; do not call
        template<class X, class Y>
        void _internal_accept_owner(shared_ptr<X> const* ppx, Y* py) const
        {
            if (weak_this_.expired())
            {
                weak_this_ = shared_ptr<T>(*ppx, py);
            }
        }

    private:

        mutable weak_ptr<T> weak_this_;
    };
} // namespace AZStd

#endif  // #ifndef AZSTD_SMART_PTR_ENABLE_SHARED_FROM_THIS_H
#pragma once

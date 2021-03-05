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
#pragma once

#include <AzCore/std/base.h>

namespace AZStd
{
    struct intrusive_default_delete
    {
        template <typename U>
        void operator () (U* p) const
        {
            delete p;
        }
    };

    /**
     * An intrusive reference counting base class that is compliant with AZStd::intrusive_ptr. Explicit
     * friending of intrusive_ptr is used; the user must use intrusive_ptr to take a reference on the object.
     */
    template <typename refcount_t, typename Deleter = intrusive_default_delete>
    class intrusive_refcount
    {
    public:
        inline uint32_t use_count() const
        {
            return static_cast<uint32_t>(m_refCount);
        }

    protected:
        intrusive_refcount() = default;
        intrusive_refcount(Deleter deleter)
            : m_deleter{deleter}
        {}
        intrusive_refcount(const intrusive_refcount&) = delete;
        intrusive_refcount(intrusive_refcount&&) = delete;

        virtual ~intrusive_refcount()
        {
            AZ_Assert(m_refCount == 0, "The destructor was called on an object with active references.");
        }

        void add_ref() const
        {
            ++m_refCount;
        }

        void release() const
        {
            const int32_t refCount = static_cast<int32_t>(--m_refCount);
            AZ_Assert(refCount != -1, "Releasing an already released object");
            if (refCount == 0)
            {
                m_deleter(this);
            }
        }

        template <typename T>
        friend struct IntrusivePtrCountPolicy;

        friend struct intrusive_default_delete;

    private:
        mutable refcount_t m_refCount = {0};
        Deleter m_deleter;
    };

} // namespace AZStd
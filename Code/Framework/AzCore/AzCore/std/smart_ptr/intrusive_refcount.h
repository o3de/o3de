/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        intrusive_refcount();
        intrusive_refcount(Deleter deleter);
        intrusive_refcount(const intrusive_refcount&) = delete;
        intrusive_refcount(intrusive_refcount&&) = delete;
        virtual ~intrusive_refcount();

        void add_ref() const;
        void release() const;

        template <typename T>
        friend struct IntrusivePtrCountPolicy;

        friend struct intrusive_default_delete;

    private:
        mutable refcount_t m_refCount = {0};
        Deleter m_deleter;
    };

    template <typename refcount_t, typename Deleter>
    intrusive_refcount<refcount_t, Deleter>::intrusive_refcount() = default;

    template <typename refcount_t, typename Deleter>
    intrusive_refcount<refcount_t, Deleter>::intrusive_refcount(Deleter deleter)
        : m_deleter{ deleter }
    {
    }

    template <typename refcount_t, typename Deleter>
    intrusive_refcount<refcount_t, Deleter>::~intrusive_refcount()
    {
        AZ_Assert(m_refCount == 0, "The destructor was called on an object with active references.");
    }

    template <typename refcount_t, typename Deleter>
    void intrusive_refcount<refcount_t, Deleter>::add_ref() const
    {
        ++m_refCount;
    }

    template <typename refcount_t, typename Deleter>
    void intrusive_refcount<refcount_t, Deleter>::release() const
    {
        const int32_t refCount = static_cast<int32_t>(--m_refCount);
        AZ_Assert(refCount != -1, "Releasing an already released object");
        if (refCount == 0)
        {
            m_deleter(this);
        }
    }
} // namespace AZStd

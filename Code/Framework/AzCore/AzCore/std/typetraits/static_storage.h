/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZStd
{
    // Extension: static_storage: Used to initialize statics in a thread-safe manner
    // These are similar to default_delete/no_delete, except they just invoke the destructor (or not)
    template <class T>
    struct default_destruct
    {
        template <class U, class = typename AZStd::enable_if<AZStd::is_constructible<U*, T*>::value, void>::type>
        void operator()(U* ptr)
        {
            ptr->~T();
        }
    };

    template <class T>
    struct no_destruct
    {
        template <class U, class = typename AZStd::enable_if<AZStd::is_constructible<U*, T*>::value, void>::type>
        void operator()(U*)
        {
        }
    };

    template <typename T, class Destructor = default_destruct<T>>
    class static_storage
    {
    public:
        template <class ...Args>
        static_storage(Args&&... args)
        {
            // m_object is not initialized, it can only have 1 correct value, which
            // we can detect below in operator T&()
            m_object.store(new (&m_storage) T(AZStd::forward<Args>(args)...));
        }

        ~static_storage()
        {
            T* expected = reinterpret_cast<T*>(&m_storage);
            if (m_object.compare_exchange_strong(expected, nullptr))
            {
                Destructor()(reinterpret_cast<T*>(&m_storage));
            }
        }

        operator const T&() const
        {
            return operator T&();
        }

        operator T&()
        {
            // spin wait for m_object to have the correct value, someone must be constructing it
            T* obj = nullptr;
            do {
                obj = m_object.load();
            } while (obj != reinterpret_cast<T*>(&m_storage)); 
            return *obj;
        }

    private:
        typename aligned_storage<sizeof(T), alignment_of<T>::value>::type m_storage;
        AZStd::atomic<T*> m_object;
    };
}

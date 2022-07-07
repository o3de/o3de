/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "StlUtils.h"

#include <queue>
#include <set>
#include <algorithm>


namespace CryMT
{
    //////////////////////////////////////////////////////////////////////////
    // Thread Safe wrappers on the standard STL containers.
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Multi-Thread safe queue container, can be used instead of std::vector.
    //////////////////////////////////////////////////////////////////////////
    template <class T, class Alloc = std::allocator<T> >
    class queue
    {
    public:
        typedef T   value_type;
        typedef std::vector<T, Alloc>   container_type;
        typedef AZStd::lock_guard<AZStd::recursive_mutex> AutoLock;

        //////////////////////////////////////////////////////////////////////////
        // std::queue interface
        //////////////////////////////////////////////////////////////////////////
        const T& front() const      { AutoLock lock(m_cs); return v.front(); };
        const T& back() const { AutoLock lock(m_cs);    return v.back(); }
        void    push(const T& x)    { AutoLock lock(m_cs); return v.push_back(x); };
        void reserve(const size_t n) { AutoLock lock(m_cs); v.reserve(n); };

        AZStd::recursive_mutex& get_lock() const { return m_cs; }

        bool   empty() const { AutoLock lock(m_cs); return v.empty(); }
        int    size() const  { AutoLock lock(m_cs); return v.size(); }
        void   clear() { AutoLock lock(m_cs); v.clear(); }
        void   free_memory() { AutoLock lock(m_cs); stl::free_container(v); }

        template <class Func>
        void   sort(const Func& compare_less) { AutoLock lock(m_cs); std::sort(v.begin(), v.end(), compare_less); }

        //////////////////////////////////////////////////////////////////////////
        bool try_pop(T& returnValue)
        {
            AutoLock lock(m_cs);
            if (!v.empty())
            {
                returnValue = v.front();
                v.erase(v.begin());
                return true;
            }
            return false;
        };

    private:
        container_type v;
        mutable AZStd::recursive_mutex m_cs;
    };
}; // namespace CryMT

namespace stl
{
    template <typename T>
    void free_container(CryMT::queue<T>& v)
    {
        v.free_memory();
    }
}

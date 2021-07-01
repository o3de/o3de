/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_MULTITHREAD_CONTAINERS_H
#define CRYINCLUDE_CRYCOMMON_MULTITHREAD_CONTAINERS_H
#pragma once

#include "StlUtils.h"
#include "BitFiddling.h"

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
        typedef CryAutoCriticalSection AutoLock;

        //////////////////////////////////////////////////////////////////////////
        // std::queue interface
        //////////////////////////////////////////////////////////////////////////
        const T& front() const      { AutoLock lock(m_cs); return v.front(); };
        const T& back() const { AutoLock lock(m_cs);    return v.back(); }
        void    push(const T& x)    { AutoLock lock(m_cs); return v.push_back(x); };
        void reserve(const size_t n) { AutoLock lock(m_cs); v.reserve(n); };
        // classic pop function of queue should not be used for thread safety, use try_pop instead
        //void  pop()                           { AutoLock lock(m_cs); return v.erase(v.begin()); };

        CryCriticalSection& get_lock() const { return m_cs; }

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

        //////////////////////////////////////////////////////////////////////////
        bool try_remove(const T& value)
        {
            AutoLock lock(m_cs);
            if (!v.empty())
            {
                typename container_type::iterator it = std::find(v.begin(), v.end(), value);
                if (it != v.end())
                {
                    v.erase(it);
                    return true;
                }
            }
            return false;
        };

        template<typename Sizer>
        void GetMemoryUsage(Sizer* pSizer) const
        {
            pSizer->AddObject(v);
        }
    private:
        container_type v;
        mutable CryCriticalSection m_cs;
    };

    //////////////////////////////////////////////////////////////////////////
    // Multi-Thread safe vector container, can be used instead of std::vector.
    //////////////////////////////////////////////////////////////////////////
    template <class T>
    class vector
    {
    public:
        typedef T   value_type;
        typedef CryAutoCriticalSection AutoLock;

        CryCriticalSection& get_lock() const { return m_cs; }

        void free_memory() { AutoLock lock(m_cs); stl::free_container(v); }

        //////////////////////////////////////////////////////////////////////////
        // std::vector interface
        //////////////////////////////////////////////////////////////////////////
        bool   empty() const { AutoLock lock(m_cs); return v.empty(); }
        int    size() const  { AutoLock lock(m_cs); return v.size(); }
        void   resize(int sz) { AutoLock lock(m_cs); v.resize(sz); }
        void   reserve(int sz) { AutoLock lock(m_cs); v.reserve(sz); }
        size_t capacity() const { AutoLock lock(m_cs); return v.size(); }
        void   clear() { AutoLock lock(m_cs); v.clear(); }
        T&     operator[](size_t pos) { AutoLock lock(m_cs); return v[pos]; }
        const T& operator[](size_t pos) const { AutoLock lock(m_cs); return v[pos]; }
        const T& front() const { AutoLock lock(m_cs); return v.front(); }
        const T& back() const { AutoLock lock(m_cs);    return v.back(); }
        T& back() { AutoLock lock(m_cs);    return v.back(); }

        void push_back(const T& x) { AutoLock lock(m_cs); return v.push_back(x); }
        void pop_back() { AutoLock lock(m_cs);  return v.pop_back(); }
        //////////////////////////////////////////////////////////////////////////

        template <class Func>
        void sort(const Func& compare_less) { AutoLock lock(m_cs); std::sort(v.begin(), v.end(), compare_less); }

        template <class Iter>
        void append(const Iter& startRange, const Iter& endRange) { AutoLock lock(m_cs); v.insert(v.end(), startRange, endRange); }

        void swap(std::vector<T>& vec) { AutoLock lock(m_cs); v.swap(vec); }

        //////////////////////////////////////////////////////////////////////////
        bool try_pop_front(T& returnValue)
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
        bool try_pop_back(T& returnValue)
        {
            AutoLock lock(m_cs);
            if (!v.empty())
            {
                returnValue = v.back();
                v.pop_back();
                return true;
            }
            return false;
        };

        //////////////////////////////////////////////////////////////////////////
        template <typename FindFunction, typename KeyType>
        bool find_and_copy(FindFunction findFunc, const KeyType& key, T& foundValue) const
        {
            AutoLock lock(m_cs);
            if (!v.empty())
            {
                typename std::vector<T>::const_iterator it;
                for (it = v.begin(); it != v.end(); ++it)
                {
                    if (findFunc(key, *it))
                    {
                        foundValue = *it;
                        return true;
                    }
                }
            }
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        bool try_remove(const T& value)
        {
            AutoLock lock(m_cs);
            if (!v.empty())
            {
                typename std::vector<T>::iterator it = std::find(v.begin(), v.end(), value);
                if (it != v.end())
                {
                    v.erase(it);
                    return true;
                }
            }
            return false;
        };

        //////////////////////////////////////////////////////////////////////////
        template <typename Predicate>
        bool try_remove_and_erase_if(Predicate predicateFunction)
        {
            AutoLock lock(m_cs);
            if (!v.empty())
            {
                typename std::vector<T>::iterator it = std::remove_if(v.begin(), v.end(), predicateFunction);
                if (it != v.end())
                {
                    v.erase(it, v.end());
                    return true;
                }
            }
            return false;
        };


        //////////////////////////////////////////////////////////////////////////
        bool try_remove_at(size_t idx)
        {
            AutoLock lock(m_cs);
            if (idx < v.size())
            {
                v.erase(v.begin() + idx);
                return true;
            }
            return false;
        }


        //////////////////////////////////////////////////////////////////////////
        //Fast remove - just move last elem over deleted element - order is not preserved
        bool try_remove_unordered(const T& value)
        {
            AutoLock lock(m_cs);
            if (!v.empty())
            {
                typename std::vector<T>::iterator it = std::find(v.begin(), v.end(), value);
                if (it != v.end())
                {
                    if (v.size() > 1)
                    {
                        typename std::vector<T>::iterator it_back = v.end() - 1;

                        if (it != it_back)
                        {
                            *it = *it_back;
                        }

                        v.erase(it_back);
                    }
                    else
                    {
                        v.erase(it);
                    }
                    return true;
                }
            }
            return false;
        };

        vector() {}

        vector(const vector<T>& rOther)
        {
            AutoLock lock1(m_cs);
            AutoLock lock2(rOther.m_cs);

            v = rOther.v;
        }

        vector& operator=(const vector<T>& rOther)
        {
            if (this == &rOther)
            {
                return *this;
            }

            AutoLock lock1(m_cs);
            AutoLock lock2(rOther.m_cs);

            v = rOther.v;

            return *this;
        }
    private:
        std::vector<T> v;
        mutable CryCriticalSection m_cs;
    };


    //////////////////////////////////////////////////////////////////////////
    // Multi-Thread safe set container, can be used instead of std::set.
    // It has limited functionality, but most of it is there.
    //////////////////////////////////////////////////////////////////////////
    template <class T>
    class set
    {
    public:
        typedef T   value_type;
        typedef T Key;
        typedef typename std::set<T>::size_type size_type;
        typedef CryAutoCriticalSection  AutoLock;

        //////////////////////////////////////////////////////////////////////////
        // Methods
        //////////////////////////////////////////////////////////////////////////
        void                                clear()                                                                         { AutoLock lock(m_cs); s.clear(); }
        size_type                       count(const Key& _Key) const                                { AutoLock lock(m_cs); return s.count(_Key); }
        bool                                empty() const                                                               { AutoLock lock(m_cs); return s.empty(); }
        size_type                       erase(const Key& _Key)                                          { AutoLock lock(m_cs); return s.erase(_Key); }

        bool                                find(const Key& _Key)                                               { AutoLock lock(m_cs); return (s.find(_Key) != s.end()); }

        bool                                pop_front(value_type&   rFrontElement)
        {
            AutoLock lock(m_cs);
            if (s.empty())
            {
                return false;
            }
            rFrontElement = *s.begin();
            s.erase(s.begin());
            return true;
        }
        bool                                pop_front()
        {
            AutoLock lock(m_cs);
            if (s.empty())
            {
                return false;
            }
            s.erase(s.begin());
            return true;
        }

        bool                                front(value_type&   rFrontElement)
        {
            AutoLock lock(m_cs);
            if (s.empty())
            {
                return false;
            }
            rFrontElement = *s.begin();
            return true;
        }

        bool                                insert(const value_type& _Val)                          { AutoLock lock(m_cs); return s.insert(_Val).second; }
        size_type                       max_size() const                                                        { AutoLock lock(m_cs); return s.max_size(); }
        size_type                       size() const                                                                { AutoLock lock(m_cs); return s.size(); }
        void                                swap(set& _Right)                                                       { AutoLock lock(m_cs); s.swap(_Right); }

        CryCriticalSection& get_lock()                                                                  { return m_cs; }

    private:
        std::set<value_type>                s;
        mutable CryCriticalSection  m_cs;
    };


    ///////////////////////////////////////////////////////////////////////////////
    //
    // Multi-thread safe lock-less FIFO queue container for passing pointers between threads.
    // The queue only stores pointers to T, it does not copy the contents of T.
    //
    //////////////////////////////////////////////////////////////////////////
    template <class T, class Alloc = std::allocator<T> >
    class CLocklessPointerQueue
    {
    public:
        explicit CLocklessPointerQueue(size_t reserve = 32) { m_lockFreeQueue.reserve(reserve); };
        ~CLocklessPointerQueue() {};

        // Check's if queue is empty.
        bool    empty() const;

        // Pushes item to the queue, only pointer is stored, T contents are not copied.
        void    push(T* ptr);
        // pop can return NULL, always check for it before use.
        T*    pop();

    private:
        queue<T*, typename std::allocator_traits<Alloc>::template rebind_alloc<T*>> m_lockFreeQueue;
    };

    //////////////////////////////////////////////////////////////////////////
    template <class T, class Alloc>
    inline bool CLocklessPointerQueue<T, Alloc>::empty() const
    {
        return m_lockFreeQueue.empty();
    }

    //////////////////////////////////////////////////////////////////////////
    template <class T, class Alloc>
    inline void CLocklessPointerQueue<T, Alloc>::push(T* ptr)
    {
        m_lockFreeQueue.push(ptr);
    }

    //////////////////////////////////////////////////////////////////////////
    template <class T, class Alloc>
    inline T* CLocklessPointerQueue<T, Alloc>::pop()
    {
        T* val = NULL;
        m_lockFreeQueue.try_pop(val);
        return val;
    }
}; // namespace CryMT

namespace stl
{
    template <typename T>
    void free_container(CryMT::vector<T>& v)
    {
        v.free_memory();
    }
    template <typename T>
    void free_container(CryMT::queue<T>& v)
    {
        v.free_memory();
    }
}


#endif // CRYINCLUDE_CRYCOMMON_MULTITHREAD_CONTAINERS_H

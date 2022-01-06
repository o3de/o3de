/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/PlatformDef.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/static_storage.h>
#include <AzCore/std/function/function_template.h>

#include <list>
#include <vector>
#include <set>
#include <map>

template <class T>
class StaticInstanceSpecialization
{
};

// Specializations for std::vector and std::map which allows us to modify the
// least amount of legacy code by mirroring the std APIs that are in use
// These are not intended to be complete, just enough to shim existing legacy code
template <typename U, class A>
class StaticInstanceSpecialization<std::vector<U, A>>
{
public:
    using Container = std::vector<U, A>;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using pointer = typename Container::pointer;
    using const_pointer = typename Container::const_pointer;
    using size_type = typename Container::size_type;

    template <class Integral>
    AZ_FORCE_INLINE
    typename AZStd::enable_if<AZStd::is_integral<Integral>::value, reference>::type
    operator[](Integral index)
    {
        return get()[size_type(index)];
    }

    template <class Integral>
    AZ_FORCE_INLINE
    typename AZStd::enable_if<AZStd::is_integral<Integral>::value, const_reference>::type
    operator[](Integral index) const
    {
        return get()[size_type(index)];
    }

    AZ_FORCE_INLINE iterator begin()
    {
        return get().begin();
    }

    AZ_FORCE_INLINE const_iterator begin() const
    {
        return get().begin();
    }

    AZ_FORCE_INLINE iterator end()
    {
        return get().end();
    }

    AZ_FORCE_INLINE const_iterator end() const
    {
        return get().end();
    }

    AZ_FORCE_INLINE iterator erase(iterator it)
    {
        return get().erase(it);
    }

    AZ_FORCE_INLINE iterator erase(iterator first, iterator last)
    {
        return get().erase(first, last);
    }

    AZ_FORCE_INLINE void resize(size_t size)
    {
        get().resize(size);
    }

    AZ_FORCE_INLINE void reserve(size_t size)
    {
        get().reserve(size);
    }

    AZ_FORCE_INLINE void clear()
    {
        get().clear();
    }

    AZ_FORCE_INLINE size_type size()
    {
        return get().size();
    }

    AZ_FORCE_INLINE void push_back(U&& val)
    {
        get().push_back(val);
    }

    AZ_FORCE_INLINE void push_back(const U& val)
    {
        get().push_back(val);
    }

    AZ_FORCE_INLINE bool empty() const
    {
        return get().empty();
    }

    AZ_FORCE_INLINE iterator insert(U&& val)
    {
        return get().insert(val);
    }

    AZ_FORCE_INLINE iterator insert(iterator it, U&& val)
    {
        return get().insert(it, val);
    }

    AZ_FORCE_INLINE reference front()
    {
        return get().front();
    }

    AZ_FORCE_INLINE const_reference front() const
    {
        return get().front();
    }

    AZ_FORCE_INLINE reference back()
    {
        return get().back();
    }

    AZ_FORCE_INLINE const_reference back() const
    {
        return get().back();
    }

    AZ_FORCE_INLINE void pop_back()
    {
        get().pop_back();
    }

    AZ_FORCE_INLINE pointer data()
    {
        return get().data();
    }

    AZ_FORCE_INLINE const_pointer data() const
    {
        return get().data();
    }

    void swap(Container& other)
    {
        get().swap(other);
    }

private:
    Container& get() const;
};


template <typename U, class A>
class StaticInstanceSpecialization<std::list<U, A>>
{
public:
    using Container = std::list<U, A>;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using pointer = typename Container::pointer;
    using const_pointer = typename Container::const_pointer;
    using size_type = typename Container::size_type;

    AZ_FORCE_INLINE iterator begin()
    {
        return get().begin();
    }

    AZ_FORCE_INLINE const_iterator begin() const
    {
        return get().begin();
    }

    AZ_FORCE_INLINE iterator end()
    {
        return get().end();
    }

    AZ_FORCE_INLINE const_iterator end() const
    {
        return get().end();
    }

    AZ_FORCE_INLINE iterator erase(iterator it)
    {
        return get().erase(it);
    }

    AZ_FORCE_INLINE iterator erase(iterator first, iterator last)
    {
        return get().erase(first, last);
    }

    AZ_FORCE_INLINE void resize(size_t size)
    {
        get().resize(size);
    }

    AZ_FORCE_INLINE void reserve(size_t size)
    {
        get().reserve(size);
    }

    AZ_FORCE_INLINE void clear()
    {
        get().clear();
    }

    AZ_FORCE_INLINE size_type size()
    {
        return get().size();
    }

    AZ_FORCE_INLINE void push_back(U&& val)
    {
        get().push_back(val);
    }

    AZ_FORCE_INLINE void push_back(const U& val)
    {
        get().push_back(val);
    }

    AZ_FORCE_INLINE void push_front(U&& val)
    {
        get().push_front(val);
    }

    AZ_FORCE_INLINE void push_front(const U& val)
    {
        get().push_front(val);
    }

    AZ_FORCE_INLINE bool empty() const
    {
        return get().empty();
    }

    AZ_FORCE_INLINE iterator insert(U&& val)
    {
        return get().insert(val);
    }

    AZ_FORCE_INLINE iterator insert(iterator it, U&& val)
    {
        return get().insert(it, val);
    }

    AZ_FORCE_INLINE reference front()
    {
        return get().front();
    }

    AZ_FORCE_INLINE const_reference front() const
    {
        return get().front();
    }

    AZ_FORCE_INLINE reference back()
    {
        return get().back();
    }

    AZ_FORCE_INLINE const_reference back() const
    {
        return get().back();
    }

    AZ_FORCE_INLINE void pop_back()
    {
        get().pop_back();
    }

    AZ_FORCE_INLINE pointer data()
    {
        return get().data();
    }

    AZ_FORCE_INLINE const_pointer data() const
    {
        return get().data();
    }

    void swap(Container& other)
    {
        get().swap(other);
    }

private:
    Container& get() const;
};

template <class K, class V, class Less, class Allocator>
class StaticInstanceSpecialization<std::map<K, V, Less, Allocator>>
{
public:
    using Container = std::map<K, V, Less, Allocator>;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using key_type = typename Container::key_type;
    using mapped_type = typename Container::mapped_type;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;

    using pair_iter_bool = std::pair<iterator, bool>;


    AZ_FORCE_INLINE iterator begin()
    {
        return get().begin();
    }

    AZ_FORCE_INLINE const_iterator begin() const
    {
        return get().begin();
    }

    AZ_FORCE_INLINE iterator end()
    {
        return get().end();
    }

    AZ_FORCE_INLINE const_iterator end() const
    {
        return get().end();
    }

    AZ_FORCE_INLINE iterator erase(iterator it)
    {
        return get().erase(it);
    }

    AZ_FORCE_INLINE size_t erase(const key_type& key)
    {
        return get().erase(key);
    }

    AZ_FORCE_INLINE mapped_type& operator[](const key_type& key)
    {
        return get()[key];
    }

    template <class K2>
    AZ_FORCE_INLINE
    typename AZStd::enable_if<AZStd::is_constructible<key_type, K2>::value, mapped_type&>::type
    operator[](const K2& keylike)
    {
        return get()[key_type(keylike)];
    }

    AZ_FORCE_INLINE iterator find(const key_type& key)
    {
        return get().find(key);
    }

    AZ_FORCE_INLINE const_iterator find(const key_type& key) const
    {
        return get().find(key);
    }

    AZ_FORCE_INLINE void clear()
    {
        get().clear();
    }

    AZ_FORCE_INLINE bool empty() const
    {
        return get().empty();
    }

    AZ_FORCE_INLINE iterator insert(iterator hint, const value_type& kv)
    {
        return get().insert(hint, kv);
    }

    AZ_FORCE_INLINE pair_iter_bool insert(const value_type& kv)
    {
        return get().insert(kv);
    }

    AZ_FORCE_INLINE size_type size() const
    {
        return get().size();
    }

    void swap(Container& cont)
    {
        get().swap(cont);
    }

private:
    Container& get() const;
};

template <class K, class V, class Less, class Allocator>
class StaticInstanceSpecialization<std::multimap<K, V, Less, Allocator>>
{
public:
    using Container = std::multimap<K, V, Less, Allocator>;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using key_type = typename Container::key_type;
    using mapped_type = typename Container::mapped_type;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;

    AZ_FORCE_INLINE iterator begin()
    {
        return get().begin();
    }

    AZ_FORCE_INLINE const_iterator begin() const
    {
        return get().begin();
    }

    AZ_FORCE_INLINE iterator end()
    {
        return get().end();
    }

    AZ_FORCE_INLINE const_iterator end() const
    {
        return get().end();
    }

    AZ_FORCE_INLINE iterator erase(iterator it)
    {
        return get().erase(it);
    }

    AZ_FORCE_INLINE size_t erase(const key_type& key)
    {
        return get().erase(key);
    }

    AZ_FORCE_INLINE iterator find(const key_type& key)
    {
        return get().find(key);
    }

    AZ_FORCE_INLINE const_iterator find(const key_type& key) const
    {
        return get().find(key);
    }

    AZ_FORCE_INLINE void clear()
    {
        get().clear();
    }

    AZ_FORCE_INLINE bool empty() const
    {
        return get().empty();
    }

    AZ_FORCE_INLINE iterator insert(value_type&& kv)
    {
        return get().insert(kv);
    }

    AZ_FORCE_INLINE iterator insert(const value_type& kv)
    {
        return get().insert(kv);
    }

    AZ_FORCE_INLINE size_type size() const
    {
        return get().size();
    }

private:
    Container& get() const;
};

template <class T, class Less, class Allocator>
class StaticInstanceSpecialization<std::set<T, Less, Allocator>>
{
public:
    using Container = std::set<T, Less, Allocator>;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;

    using pair_iter_bool = std::pair<iterator, bool>;

    AZ_FORCE_INLINE iterator begin()
    {
        return get().begin();
    }

    AZ_FORCE_INLINE const_iterator begin() const
    {
        return get().begin();
    }

    AZ_FORCE_INLINE iterator end()
    {
        return get().end();
    }

    AZ_FORCE_INLINE const_iterator end() const
    {
        return get().end();
    }

    AZ_FORCE_INLINE iterator erase(iterator it)
    {
        return get().erase(it);
    }

    AZ_FORCE_INLINE size_t erase(const value_type& key)
    {
        return get().erase(key);
    }

    AZ_FORCE_INLINE iterator find(const value_type& key)
    {
        return get().find(key);
    }

    AZ_FORCE_INLINE const_iterator find(const value_type& key) const
    {
        return get().find(key);
    }

    AZ_FORCE_INLINE void clear()
    {
        get().clear();
    }

    AZ_FORCE_INLINE bool empty() const
    {
        return get().empty();
    }

    AZ_FORCE_INLINE pair_iter_bool insert(value_type&& kv)
    {
        return get().insert(kv);
    }

    AZ_FORCE_INLINE pair_iter_bool insert(const value_type& kv)
    {
        return get().insert(kv);
    }

    AZ_FORCE_INLINE size_type size() const
    {
        return get().size();
    }

private:
    Container& get() const;
};

// This is a non-thread safe version of AZStd::static_storage, used purely to lazy
// initialize various Cry DLL statics/singletons. They are created on first access.
template <class T, class Destructor=AZStd::default_destruct<T>>
class StaticInstance
    : public StaticInstanceSpecialization<T>
{
public:
    template <class ...Args>
    StaticInstance(Args&& ...args)
    {
        // We need to pass args as a copy to avoid taking references to values that dont support it.
        // For example, if an enum value is being passed in args, we would lost the value inside the lambda
        m_ctor = [=]()
        {
            return Create(args...);
        };
    }

    ~StaticInstance()
    {
        if (m_instance)
        {
            Destructor()(m_instance);
        }
    }

    AZ_FORCE_INLINE T* operator->() const
    {
        return Get();
    }

    AZ_FORCE_INLINE operator T*() const
    {
        return Get();
    }

    AZ_FORCE_INLINE operator T&() const
    {
        return *Get();
    }

    AZ_FORCE_INLINE T* operator&() const
    {
        return Get();
    }

    AZ_FORCE_INLINE operator bool() const
    {
        return m_instance != nullptr;
    }

private:
    StaticInstance(const StaticInstance&) = delete;
    StaticInstance(StaticInstance&&) = delete;
    StaticInstance& operator=(const StaticInstance&) = delete;

    template <class ...Args>
    T* Create(Args&& ...args) const
    {
        return new((void*)&m_storage) T(AZStd::forward<Args>(args)...);
    }

    T* Get() const
    {
        if (!m_instance)
        {
            m_instance = m_ctor();
        }
        return m_instance;
    }

    mutable T* m_instance = nullptr;
    mutable typename AZStd::aligned_storage<sizeof(T), AZStd::alignment_of<T>::value>::type m_storage;
    AZStd::function<T*()> m_ctor;
};

template <typename U, class A>
std::vector<U, A>& StaticInstanceSpecialization<std::vector<U, A>>::get() const
{
    return *(*static_cast<const StaticInstance<std::vector<U, A>>*>(this));
}

template <typename U, class A>
std::list<U, A>& StaticInstanceSpecialization<std::list<U, A>>::get() const
{
    return *(*static_cast<const StaticInstance<std::list<U, A>>*>(this));
}

template <class K, class V, class Less, class Allocator>
std::map<K, V, Less, Allocator>& StaticInstanceSpecialization<std::map<K, V, Less, Allocator>>::get() const
{
    return *(*static_cast<const StaticInstance<std::map<K, V, Less, Allocator>>*>(this));
}

template <class K, class V, class Less, class Allocator>
std::multimap<K, V, Less, Allocator>& StaticInstanceSpecialization<std::multimap<K, V, Less, Allocator>>::get() const
{
    return *(*static_cast<const StaticInstance<std::multimap<K, V, Less, Allocator>>*>(this));
}

template <class T, class Less, class Allocator>
std::set<T, Less, Allocator>& StaticInstanceSpecialization<std::set<T, Less, Allocator>>::get() const
{
    return *(*static_cast<const StaticInstance<std::set<T, Less, Allocator>>*>(this));
}

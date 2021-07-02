/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Various convenience utility functions for STL and alike
//               Used in Animation subsystem, and in some tools
#ifndef CRYINCLUDE_CRYCOMMON_STLUTILS_H
#define CRYINCLUDE_CRYCOMMON_STLUTILS_H
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

#if (defined(LINUX) || defined(APPLE))
    #include "platform.h"
#endif

#define STATIC_ASSERT(condition, errMessage) static_assert(condition, errMessage)

#include <map>
#include <set>
#include <algorithm>
#include <deque>
#include <vector>

#undef std__hash_map
#define std__hash_map AZStd::unordered_map
#define std__unordered_set AZStd::unordered_set
#define std__hash_multimap AZStd::unordered_multimap
#define std__hash AZStd::hash
#define std__unordered_map AZStd::unordered_map

template <class T, class Destructor>
class StaticInstance;

// auto-cleaner: upon destruction, calls the clear() method
template <class T>
class CAutoClear
{
public:
    CAutoClear (T* p)
        : m_p(p) {}
    ~CAutoClear () {m_p->clear(); }
protected:
    T* m_p;
};


template <class Container>
unsigned sizeofArray (const Container& arr)
{
    return (unsigned)(sizeof(typename Container::value_type) * arr.size());
}

template <class Container>
unsigned sizeofVector (const Container& arr)
{
    return (unsigned)(sizeof(typename Container::value_type) * arr.capacity());
}

template <class Container>
unsigned sizeofArray (const Container& arr, unsigned nSize)
{
    return arr.empty() ? 0u : (unsigned)(sizeof(typename Container::value_type) * nSize);
}

template <class Container>
unsigned capacityofArray (const Container& arr)
{
    return (unsigned)(arr.capacity() * sizeof(arr[0]));
}

template <class T>
unsigned countElements (const std::vector<T>& arrT, const T& x)
{
    unsigned nSum = 0;
    for (typename std::vector<T>::const_iterator iter = arrT.begin(); iter != arrT.end(); ++iter)
    {
        if (x == *iter)
        {
            ++nSum;
        }
    }
    return nSum;
}

// [Timur]
/** Contain extensions for STL library.
*/
namespace stl
{
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Compare member of class/struct.
    //
    // e.g. Sort Vec3s by x component
    //
    //           std::sort(vec3s.begin(), vec3s.end(), stl::member_compare<Vec3, float, &Vec3::x>());
    //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename OWNER_TYPE, typename MEMBER_TYPE, MEMBER_TYPE OWNER_TYPE::* MEMBER_PTR, typename EQUALITY = std::less<MEMBER_TYPE> >
    struct member_compare
    {
        inline bool operator () (const OWNER_TYPE& lhs, const OWNER_TYPE& rhs) const
        {
            return EQUALITY()(lhs.*MEMBER_PTR, rhs.*MEMBER_PTR);
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Compare member of class/struct against parameter.
    //
    // e.g. Find Vec3 with x component less than 1.0
    //
    //           std::find_if(vec3s.begin(), vec3s.end(), stl::member_compare_param<Vec3, float, &Vec3::x>(1.0f));
    //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename OWNER_TYPE, typename MEMBER_TYPE, MEMBER_TYPE OWNER_TYPE::* MEMBER_PTR, typename EQUALITY = std::less<MEMBER_TYPE> >
    struct member_compare_param
    {
        inline member_compare_param(const MEMBER_TYPE& _value)
            : value(_value)
        {
        }

        inline bool operator () (const OWNER_TYPE& rhs) const
        {
            return EQUALITY()(rhs.*MEMBER_PTR, value);
        }

        const MEMBER_TYPE& value;
    };

    //////////////////////////////////////////////////////////////////////////
    //! Searches the given entry in the map by key, and if there is none, returns the default value
    //////////////////////////////////////////////////////////////////////////
    template <typename Map>
    inline typename Map::mapped_type find_in_map(const Map& mapKeyToValue, const typename Map::key_type& key, typename Map::mapped_type valueDefault)
    {
        typename Map::const_iterator it = mapKeyToValue.find (key);
        if (it == mapKeyToValue.end())
        {
            return valueDefault;
        }
        else
        {
            return it->second;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //! Inserts and returns a reference to the given value in the map, or returns the current one if it's already there.
    //////////////////////////////////////////////////////////////////////////
    template <typename Map>
    inline typename Map::mapped_type& map_insert_or_get(Map& mapKeyToValue, const typename Map::key_type& key, const typename Map::mapped_type& defValue = typename Map::mapped_type())
    {
        auto&& iresult = mapKeyToValue.insert(typename Map::value_type(key, defValue));
        return iresult.first->second;
    }

    // searches the given entry in the map by key, and if there is none, returns the default value
    // The values are taken/returned in REFERENCEs rather than values
    template <typename Key, typename mapped_type, typename Traits, typename Allocator>
    inline mapped_type& find_in_map_ref(std::map<Key, mapped_type, Traits, Allocator>& mapKeyToValue, const Key& key, mapped_type& valueDefault)
    {
        typedef std::map<Key, mapped_type, Traits, Allocator> Map;
        typename Map::iterator it = mapKeyToValue.find (key);
        if (it == mapKeyToValue.end())
        {
            return valueDefault;
        }
        else
        {
            return it->second;
        }
    }

    template <typename Key, typename mapped_type, typename Traits, typename Allocator>
    inline const mapped_type& find_in_map_ref(const std::map<Key, mapped_type, Traits, Allocator>& mapKeyToValue, const Key& key, const mapped_type& valueDefault)
    {
        typedef std::map<Key, mapped_type, Traits, Allocator> Map;
        typename Map::const_iterator it = mapKeyToValue.find (key);
        if (it == mapKeyToValue.end())
        {
            return valueDefault;
        }
        else
        {
            return it->second;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //! Fills vector with contents of map.
    //////////////////////////////////////////////////////////////////////////
    template <class Map, class Vector>
    inline void map_to_vector(const Map& theMap, Vector& array)
    {
        array.resize(0);
        array.reserve(theMap.size());
        for (typename Map::const_iterator it = theMap.begin(); it != theMap.end(); ++it)
        {
            array.push_back(it->second);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //! Fills vector with contents of set.
    //////////////////////////////////////////////////////////////////////////
    template <class Set, class Vector>
    inline void set_to_vector(const Set& theSet, Vector& array)
    {
        array.resize(0);
        array.reserve(theSet.size());
        for (typename Set::const_iterator it = theSet.begin(); it != theSet.end(); ++it)
        {
            array.push_back(*it);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //! Find and erase element from container.
    // @return true if item was find and erased, false if item not found.
    //////////////////////////////////////////////////////////////////////////
    template <class Container, class Value>
    inline bool find_and_erase(Container& container, const Value& value)
    {
        typename Container::iterator it = AZStd::find(container.begin(), container.end(), value);
        if (it != container.end())
        {
            container.erase(it);
            return true;
        }
        return false;
    }

    template <typename K, typename P, typename A>
    inline bool find_and_erase(std::set<K, P, A>& container, const K& value)
    {
        return container.erase(value) > 0;
    }

    //////////////////////////////////////////////////////////////////////////
    //! Find and erase element from container.
    // @return true if item was find and erased, false if item not found.
    //////////////////////////////////////////////////////////////////////////
    template <class CONTAINER, class PREDICATE>
    inline bool find_and_erase_if(CONTAINER& container, const PREDICATE& predicate)
    {
        typename CONTAINER::iterator end = container.end(), i = std::find_if(container.begin(), end, predicate);

        if (i != end)
        {
            container.erase(i);

            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    //! Find and erase all elements matching value from container.
    // Assume that this will invalidate any exiting iterators.
    // Commonly used for removing NULL pointers from collections.
    //////////////////////////////////////////////////////////////////////////
    template <class Container>
    inline void find_and_erase_all(Container& container, const typename Container::value_type& value)
    {
        // Shuffles all elements != value to the front and returns the start of the removed elements.
        typename Container::iterator endIter(container.end());
        typename Container::iterator newEndIter(std::remove(container.begin(), endIter, value));

        // Delete the removed range at the back of the container (low-cost for vector).
        container.erase(newEndIter, endIter);
    }

    //////////////////////////////////////////////////////////////////////////
    //! Find and erase element from map.
    // @return true if item was find and erased, false if item not found.
    //////////////////////////////////////////////////////////////////////////
    template <class Container, class Key>
    inline bool member_find_and_erase(Container& container, const Key& key)
    {
        typename Container::iterator it = container.find (key);
        if (it != container.end())
        {
            container.erase(it);
            return true;
        }
        return false;
    }


    //////////////////////////////////////////////////////////////////////////
    //! Push back to container unique element.
    // @return true if item added, false overwise.
    template <class Container, class Value>
    inline bool push_back_unique(Container& container, const Value& value)
    {
        if (AZStd::find(container.begin(), container.end(), value) == container.end())
        {
            container.push_back(value);
            return true;
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    //! Push back to container unique element.
    // @return true if item added, false overwise.
    template <class CONTAINER, class PREDICATE, typename VALUE>
    inline bool push_back_unique_if(CONTAINER& container, const PREDICATE& predicate, const VALUE& value)
    {
        typename CONTAINER::iterator    end = container.end();

        if (AZStd::find_if(container.begin(), end, predicate) == end)
        {
            container.push_back(value);

            return true;
        }
        else
        {
            return false;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //! Push back to container contents of another container
    template <class Container, class Iter>
    inline void push_back_range(Container& container, Iter begin, Iter end)
    {
        for (Iter it = begin; it != end; ++it)
        {
            container.push_back(*it);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //! Push back to container contents of another container, if not already present
    template <class Container, class Iter>
    inline void push_back_range_unique(Container& container, Iter begin, Iter end)
    {
        for (Iter it = begin; it != end; ++it)
        {
            push_back_unique(container, *it);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //! Find element in container.
    // @return true if item found.
    template <class Container, class Value>
    inline bool find(Container& container, const Value& value)
    {
        return std::find(container.begin(), container.end(), value) != container.end();
    }

    //////////////////////////////////////////////////////////////////////////
    //! Find element in a sorted container using binary search with logarithmic efficiency.
    //
    template <class Iterator, class T>
    inline Iterator binary_find(Iterator first, Iterator last, const T& value)
    {
        Iterator it = std::lower_bound(first, last, value);
        return (it == last || value != *it) ? last : it;
    }

    //////////////////////////////////////////////////////////////////////////
    //! Find element in a sorted container using binary search with logarithmic efficiency.
    // @return true if item was inserted.
    template <class Container, class Value>
    inline bool binary_insert_unique(Container& container, const Value& value)
    {
        typename Container::iterator it = std::lower_bound(container.begin(), container.end(), value);
        if (it != container.end())
        {
            if (*it == value)
            {
                return false;
            }
            container.insert(it, value);
        }
        else
        {
            container.insert(container.end(), value);
        }
        return true;
    }
    //////////////////////////////////////////////////////////////////////////
    //! Find element in a sorted container using binary search with logarithmic efficiency.
    // and erases if element found.
    // @return true if item was erased.
    template <class Container, class Value>
    inline bool binary_erase(Container& container, const Value& value)
    {
        typename Container::iterator it = std::lower_bound(container.begin(), container.end(), value);
        if (it != container.end() && *it == value)
        {
            container.erase(it);
            return true;
        }
        return false;
    }

    template <typename ItT, typename Func>
    ItT remove_from_heap(ItT begin, ItT end, ItT at, Func order)
    {
        using std::swap;

        --end;
        if (at == end)
        {
            return at;
        }

        size_t idx = std::distance(begin, at);
        swap(*end, *at);

        size_t length = std::distance(begin, end);
        size_t parent, child;

        if (idx > 0 && order(*(begin + idx / 2), *(begin + idx)))
        {
            do
            {
                parent = idx / 2;
                swap(*(begin + idx), *(begin + parent));
                idx = parent;

                if (idx == 0 || order(*(begin + idx), *(begin + idx / 2)))
                {
                    return end;
                }
            }
            while (true);
        }
        else
        {
            do
            {
                child = idx * 2 + 1;
                if (child >= length)
                {
                    return end;
                }

                ItT left = begin + child;
                ItT right = begin + child + 1;

                if (right < end && order(*left, *right))
                {
                    ++child;
                }

                if (order(*(begin + child), *(begin + idx)))
                {
                    return end;
                }

                swap(*(begin + child), *(begin + idx));
                idx = child;
            }
            while (true);
        }

        return end;
    }

    struct container_object_deleter
    {
        template<typename T>
        void operator()(const T* ptr) const
        {
            delete ptr;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    //! Convert arbitary class to const char*
    //////////////////////////////////////////////////////////////////////////
    template <class Type>
    inline const char* constchar_cast(const Type& type)
    {
        return type;
    }

    //! Specialization of string to const char cast.
    template <>
    inline const char* constchar_cast(const string& type)
    {
        return type.c_str();
    }

    //////////////////////////////////////////////////////////////////////////
    //! Case sensetive less key for any type convertable to const char*.
    //////////////////////////////////////////////////////////////////////////
    template <class Type>
    struct less_strcmp
    {
        bool operator()(const Type& left, const Type& right) const
        {
            return strcmp(constchar_cast(left), constchar_cast(right)) < 0;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    //! Case insensetive less key for any type convertable to const char*.
    template <class Type>
    struct less_stricmp
    {
        bool operator()(const Type& left, const Type& right) const
        {
            return _stricmp(constchar_cast(left), constchar_cast(right)) < 0;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Hash map usage:
    // typedef AZStd::unordered_map<string,int, stl::hash_string_insensitve<string>, stl::equality_string_insensitive<string> > StringToIntHash;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // useful when the key is already the result of an hash function
    // key needs to be convertible to size_t
    //////////////////////////////////////////////////////////////////////////
    template <class Key>
    class hash_simple
    {
    public:
        enum    // parameters for hash table
        {
            bucket_size = 4,    // 0 < bucket_size
            min_buckets = 8
        };// min_buckets = 2 ^^ N, 0 < N

        size_t operator()(const Key& key) const
        {
            return size_t(key);
        };
        bool operator()(const Key& key1, const Key& key2) const
        {
            return key1 < key2;
        }
    };

    // simple hash class that has the avalanche property (a change in one bit affects all others)
    // ... use this if you have uint32 key values!
    class hash_uint32
    {
    public:
        enum    // parameters for hash table
        {
            bucket_size = 4,    // 0 < bucket_size
            min_buckets = 8  // min_buckets = 2 ^^ N, 0 < N
        };

        ILINE size_t operator()(uint32 a) const
        {
            a = (a + 0x7ed55d16) + (a << 12);
            a = (a ^ 0xc761c23c) ^ (a >> 19);
            a = (a + 0x165667b1) + (a << 5);
            a = (a + 0xd3a2646c) ^ (a << 9);
            a = (a + 0xfd7046c5) + (a << 3);
            a = (a ^ 0xb55a4f09) ^ (a >> 16);
            return a;
        };
        bool operator()(uint32 key1, uint32 key2) const
        {
            return key1 < key2;
        }
    };


    //////////////////////////////////////////////////////////////////////////
    //! Case sensitive string hash
    //////////////////////////////////////////////////////////////////////////
    template <class Key>
    class hash_string
    {
    public:
        enum    // parameters for hash table
        {
            bucket_size = 4,    // 0 < bucket_size
            min_buckets = 8
        };// min_buckets = 2 ^^ N, 0 < N

        size_t operator()(const Key& key) const
        {
            unsigned int h = 0;
            const char* s = constchar_cast(key);
            for (; *s; ++s)
            {
                h = 5 * h + *(unsigned char*)s;
            }
            return size_t(h);
        };
    };

    //////////////////////////////////////////////////////////////////////////
    //! Case sensitive string equality
    //////////////////////////////////////////////////////////////////////////
    template <class Key>
    class equality_string
    {
    public:
        bool operator()(const Key& key1, const Key& key2) const
        {
            return strcmp(constchar_cast(key1), constchar_cast(key2)) == 0;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    //! Case insensitive string hasher
    //////////////////////////////////////////////////////////////////////////
    template <class Key>
    class hash_string_caseless
    {
    public:
        enum    // parameters for hash table
        {
            bucket_size = 4,    // 0 < bucket_size
            min_buckets = 8
        };// min_buckets = 2 ^^ N, 0 < N

        size_t operator()(const Key& key) const
        {
            unsigned int h = 0;
            const char* s = constchar_cast(key);
            for (; *s; ++s)
            {
                h = 5 * h + tolower(*(unsigned char*)s);
            }
            return size_t(h);
        };
    };

    //////////////////////////////////////////////////////////////////////////
    //! Case insensitive string comparer
    //////////////////////////////////////////////////////////////////////////
    template <class Key>
    class equality_string_caseless
    {
    public:
        bool operator()(const Key& key1, const Key& key2) const
        {
            return _stricmp(constchar_cast(key1), constchar_cast(key2)) == 0;
        }
    };


    // Support for both Microsoft and SGI kind of hash_map.

#if defined(_STLP_HASH_MAP) || defined(APPLE) || defined(LINUX)
    // STL Port
    template <class _Key, class _Predicate = std::less<_Key> >
    struct hash_compare
    {
        enum
        {   // parameters for hash table
            bucket_size = 4,    // 0 < bucket_size
            min_buckets = 8   // min_buckets = 2 ^^ N, 0 < N
        };

        size_t operator()(const _Key& _Keyval) const
        {
            // return hash value.
            uint32 a = _Keyval;
            a = (a + 0x7ed55d16) + (a << 12);
            a = (a ^ 0xc761c23c) ^ (a >> 19);
            a = (a + 0x165667b1) + (a << 5);
            a = (a + 0xd3a2646c) ^ (a << 9);
            a = (a + 0xfd7046c5) + (a << 3);
            a = (a ^ 0xb55a4f09) ^ (a >> 16);
            return a;
        }

        // Less then function.
        bool operator()(const _Key& _Keyval1, const _Key& _Keyval2) const
        {   // test if _Keyval1 ordered before _Keyval2
            _Predicate comp;
            return (comp(_Keyval1, _Keyval2));
        }
    };

    template <class Key, class HashFunc>
    struct stlport_hash_equal
    {
        // Equal function.
        bool operator()(const Key& k1, const Key& k2) const
        {
            HashFunc less;
            // !(k1 < k2) && !(k2 < k1)
            return !less(k1, k2) && !less(k2, k1);
        }
    };

    template <class Key, class Value, class HashFunc = hash_compare<Key>, class Alloc = std::allocator< std::pair<const Key, Value> > >
    struct hash_map
        : public std__hash_map<Key, Value, HashFunc, stlport_hash_equal<Key, HashFunc>, Alloc>
    {
        hash_map()
            : std__hash_map<Key, Value, HashFunc, stlport_hash_equal<Key, HashFunc>, Alloc>(HashFunc::min_buckets) {}
    };

    template <class Key, class Value, class HashFunc = hash_compare<Key>, class Alloc = std::allocator< std::pair<const Key, Value> > >
    struct hash_multimap
        : public std__hash_multimap<Key, Value, HashFunc, stlport_hash_equal<Key, HashFunc>, Alloc>
    {
        hash_multimap()
            : std__hash_multimap<Key, Value, HashFunc, stlport_hash_equal<Key, HashFunc>, Alloc>(HashFunc::min_buckets) {}
    };
#endif

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    class intrusive_linked_list_node
    {
    public:
        intrusive_linked_list_node()  { link_to_intrusive_list(static_cast<T*>(this)); }
        // Not virtual by design
        ~intrusive_linked_list_node() { unlink_from_intrusive_list(static_cast<T*>(this)); }

        static T* get_intrusive_list_root() { return m_root_intrusive; };

        static void link_to_intrusive_list(T* pNode)
        {
            if (m_root_intrusive)
            {
                // Add to the beginning of the list.
                T* head = m_root_intrusive;
                pNode->m_prev_intrusive = 0;
                pNode->m_next_intrusive = head;
                head->m_prev_intrusive = pNode;
                m_root_intrusive = pNode;
            }
            else
            {
                m_root_intrusive = pNode;
                pNode->m_prev_intrusive = 0;
                pNode->m_next_intrusive = 0;
            }
        }
        static void unlink_from_intrusive_list(T* pNode)
        {
            if (pNode == m_root_intrusive) // if head of list.
            {
                m_root_intrusive = pNode->m_next_intrusive;
                if (m_root_intrusive)
                {
                    m_root_intrusive->m_prev_intrusive = 0;
                }
            }
            else
            {
                if (pNode->m_prev_intrusive)
                {
                    pNode->m_prev_intrusive->m_next_intrusive = pNode->m_next_intrusive;
                }
                if (pNode->m_next_intrusive)
                {
                    pNode->m_next_intrusive->m_prev_intrusive = pNode->m_prev_intrusive;
                }
            }
            pNode->m_next_intrusive = 0;
            pNode->m_prev_intrusive = 0;
        }

    public:
        static T* m_root_intrusive;
        T* m_next_intrusive;
        T* m_prev_intrusive;
    };

    template <class T>
    inline void reconstruct(T& t)
    {
        t.~T();
        new(&t)T;
    }

    template <class T, class D>
    inline void reconstruct(StaticInstance<T, D>& instance)
    {
        reconstruct(*instance);
    }

    template <typename T, typename A1>
    inline void reconstruct(T& t, const A1& a1)
    {
        t.~T();
        new (&t)T(a1);
    }

    template <typename T, typename A1, typename A2>
    inline void reconstruct(T& t, const A1& a1, const A2& a2)
    {
        t.~T();
        new (&t)T(a1, a2);
    }

    template <typename T, typename A1, typename A2, typename A3>
    inline void reconstruct(T& t, const A1& a1, const A2& a2, const A3& a3)
    {
        t.~T();
        new (&t)T(a1, a2, a3);
    }

    template <typename T, typename A1, typename A2, typename A3, typename A4>
    inline void reconstruct(T& t, const A1& a1, const A2& a2, const A3& a3, const A4& a4)
    {
        t.~T();
        new (&t)T(a1, a2, a3, a4);
    }

    template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
    inline void reconstruct(T& t, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5)
    {
        t.~T();
        new (&t)T(a1, a2, a3, a4, a5);
    }

    template <class T>
    inline void free_container(T& t)
    {
        reconstruct(t);
    }

    template <class T, class A>
    inline void free_container(std::deque<T, A>& t)
    {
        reconstruct(t);
    }

    template <class K, class D, class H, class A>
    inline void free_container(std__hash_map<K, D, H, A>& t)
    {
        reconstruct(t);
    }

    template <class T, class D>
    inline void free_container(StaticInstance<T, D>& instance)
    {
        reconstruct(*instance);
    }

    struct container_freer
    {
        template <typename T>
        void operator () (T& container) const
        {
            stl::free_container(container);
        }
    };

    template <typename T>
    struct scoped_set
    {
        scoped_set(T& ref, T val)
            : m_ref(&ref)
            , m_oldVal(ref)
        {
            ref = val;
        }

        ~scoped_set()
        {
            (*m_ref) = m_oldVal;
        }

    private:
        scoped_set(const scoped_set<T>& other);
        scoped_set<T>& operator = (const scoped_set<T>& other);

    private:
        T* m_ref;
        T m_oldVal;
    };

    template <typename T, size_t Length, typename Func>
    inline void for_each_array(T (&buffer)[Length], Func func)
    {
        std::for_each(&buffer[0], &buffer[Length], func);
    }

    template <typename T, typename D, size_t Length, typename Func>
    inline void for_each_array(StaticInstance<T, D>(&buffer)[Length], Func func)
    {
        for (size_t idx = 0; idx < Length; ++idx)
        {
            func(*buffer[idx]);
        }
    }

    template <typename T>
    inline void destruct(T* p)
    {
        p->~T();
    }
}

#define DEFINE_INTRUSIVE_LINKED_LIST(Class) \
    template<>                              \
    Class * stl::intrusive_linked_list_node<Class>::m_root_intrusive = nullptr;


// Performs a less-than compare on a serial sequence space, such that earlier values compare less-than later values.
// Unlike a normal integral value, this accounts for overflowing the limit of the underlying type.
// For example, assuming a 2-bit unsigned underlying type (with possible values 0, 1, 2 and 3), the following will hold: 0 < 1 && 1 < 2 && 2 < 3 && 3 < 0
// Assuming two equal values V1 and V2, V2 can be incremented up to "(2 ^ (bits - 1) - 1)" times and V1 < V2 will continue to hold.
// See also RFC-1982 that documents this http://tools.ietf.org/html/rfc1982
template<typename T>
struct SSerialCompare
{
    static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value, "T must be an unsigned integral type");

    static const T limit = (T(1) << (sizeof(T) * 8 - 1));

    bool operator()(T lhs, T rhs)
    {
        return ((lhs < rhs) && (rhs - lhs < limit)) || ((lhs > rhs) && (lhs - rhs > limit));
    }
};

template <class Container>
unsigned sizeOfVP(Container& arr)
{
    int i;
    unsigned size = 0;
    for (i = 0; i < (int)arr.size(); i++)
    {
        typename Container::value_type& T = arr[i];
        size += T->Size();
    }
    size += (arr.capacity() - arr.size()) * sizeof(typename Container::value_type);
    return size;
}

template <class Container>
unsigned sizeOfV(Container& arr)
{
    int i;
    unsigned size = 0;
    for (i = 0; i < (int)arr.size(); i++)
    {
        typename Container::value_type& T = arr[i];
        size += T.Size();
    }
    size += (arr.capacity() - arr.size()) * sizeof(typename Container::value_type);
    return size;
}
template <class Container>
unsigned sizeOfA(Container& arr)
{
    int i;
    unsigned size = 0;
    for (i = 0; i < arr.size(); i++)
    {
        typename Container::value_type& T = arr[i];
        size += T.Size();
    }
    return size;
}
// define the maplikestruct, used to approximate the memory requirements for a map node
namespace stl
{
    struct MapLikeStruct
    {
        bool color;
        void* parent;
        void* left;
        void* right;
    };
}
template <class Map>
unsigned sizeOfMap(Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += T.Size();
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}
template <class Map>
unsigned sizeOfMapStr(Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += T.capacity();
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}
template <class Map>
unsigned sizeOfMapP(Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += T->Size();
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}
template <class Map>
unsigned sizeOfMapS(Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += sizeof(T);
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}

#endif // CRYINCLUDE_CRYCOMMON_STLUTILS_H

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_CRYARRAY_H
#define CRYINCLUDE_CRYCOMMON_CRYARRAY_H
#pragma once

#include "CryLegacyAllocator.h"

//---------------------------------------------------------------------------
// Convenient iteration macros
#define for_iter(IT, it, b, e)          for (IT it = (b), _e = (e); it != _e; ++it)
#define for_container(CT, it, cont) for_iter (CT::iterator, it, (cont).begin(), (cont).end())

#define for_ptr(T, it, b, e)                for (T* it = (b), * _e = (e); it != _e; ++it)
#define for_array_ptr(T, it, arr)       for_ptr (T, it, (arr).begin(), (arr).end())

#define for_array(i, arr)                       for (int i = 0, _e = (arr).size(); i < _e; i++)
#define for_all(cont)                               for_array (_i, cont) cont[_i]

//---------------------------------------------------------------------------
// Stack array helper
#define ALIGNED_STACK_ARRAY(T, name, size, alignment)           \
    PREFAST_SUPPRESS_WARNING(6255)                              \
    T * name = (T*) alloca((size) * sizeof(T) + alignment - 1); \
    name = Align(name, alignment);

#define STACK_ARRAY(T, name, size)                 \
    ALIGNED_STACK_ARRAY(T, name, size, alignof(T)) \

//---------------------------------------------------------------------------
// Specify semantics for moving objects.
// If raw_movable() is true, objects will be moved with memmove().
// If false, with the templated move_init() function.
template<class T>
bool raw_movable([[maybe_unused]] T const& dest)
{
    return false;
}

// This container was written before C++11 and move semantics. As a result, it attempts
// to fake move semantics where possible by constructing over top of existing instances.
// This works great, unless the type being operated on has internal pointers to its own
// memory space (for instance a string with SSO, or COW semantics).
template <class T>
struct fake_move_helper
{
    static void move(T& dest, T& source)
    {
        ::new(&dest) T(source);
        source.~T();
    }
};

// Override for string to ensure proper construction
template <>
struct fake_move_helper<string>
{
    static void move(string& dest, string& source)
    {
        ::new((void*)&dest) string(); 
        dest = source;
        source.~string();
    }
};

// Generic move function: transfer an existing source object to uninitialized dest address.
// Addresses must not overlap (requirement on caller).
// May be specialized for specific types, to provide a more optimal move.
// For types that can be trivially moved (memcpy), do not specialize move_init, rather specialize raw_movable to return true.
template<class T>
void move_init(T& dest, T& source)
{
    assert(&dest != &source);
    fake_move_helper<T>::move(dest, source);
}

/*---------------------------------------------------------------------------
Public classes:

        Array<T, [I, STORAGE]>
        StaticArray<T, nSIZE, [I]>
        DynArray<T, [I, STORAGE, ALLOC]>
        StaticDynArray<T, nSIZE, [I]>

Support classes are placed in namespaces NArray and NAlloc to reduce global name usage.
---------------------------------------------------------------------------*/

namespace NArray
{
    // We should never have these defined as macros.
    #undef min
    #undef max

    // Define our own min/max here, to avoid including entire <algorithm>.
    template<class T>
    inline T min(T a, T b)
    { return a < b ? a : b; }
    template<class T>
    inline T max(T a, T b)
    { return a > b ? a : b; }

    // Automatic inference of signed from unsigned int type.
    template<class T>
    struct IntTraits
    {
        typedef T TSigned;
    };

    template<>
    struct IntTraits<uint>
    {
        typedef int TSigned;
    };
    template<>
    struct IntTraits<uint64>
    {
        typedef int64 TSigned;
    };
#if !defined(LINUX) && !defined(APPLE)
    template<>
    struct IntTraits<unsigned long>
    {
        typedef long TSigned;
    };
#endif

    /*---------------------------------------------------------------------------
    // STORAGE prototype for Array<T,I,STORAGE>.
    struct Storage
    {
        template<class T, class I>
        struct Store
        {
            [const] T* begin() [const];
            I size() const;
        };
    };
    ---------------------------------------------------------------------------*/

    //---------------------------------------------------------------------------
    // ArrayStorage: Default STORAGE Array<T,I,STORAGE>.
    // Simply contains a pointer and count to an existing array,
    // performs no allocation or deallocation.

    struct ArrayStorage
    {
        template<class T, class I = int>
        struct Store
        {
            // Construction.
            Store()
                : m_aElems(0)
                , m_nCount(0)
            {}
            Store(T* elems, I count)
                : m_aElems(elems)
                , m_nCount(count)
            {}
            Store(T* start, T* finish)
                : m_aElems(start)
                , m_nCount(check_cast<I>(finish - start))
            {}

            void set(T* elems, I count)
            {
                m_aElems = elems;
                m_nCount = count;
            }

            // Basic storage.
            CONST_VAR_FUNCTION(T * begin(),
                { return m_aElems;
                })
            inline I size() const
            { return m_nCount; }

            // Modifiers, alter range in place.
            void erase_front(I count = 1)
            {
                assert(count >= 0 && count <= m_nCount);
                m_nCount -= count;
                m_aElems += count;
            }

            void erase_back(I count = 1)
            {
                assert(count >= 0 && count <= m_nCount);
                m_nCount -= count;
            }

            void resize(I count)
            {
                assert(count >= 0 && count <= m_nCount);
                m_nCount = count;
            }

        protected:
            T*  m_aElems;
            I       m_nCount;
        };
    };

    //---------------------------------------------------------------------------
    // StaticArrayStorage: STORAGE scheme with a statically sized member array.

    template<int nSIZE>
    struct StaticArrayStorage
    {
        template<class T, class I = int>
        struct Store
        {
            // Basic storage.
            CONST_VAR_FUNCTION(T * begin(),
                { return m_aElems;
                })
            inline static I size()
            { return (I)nSIZE; }

        protected:
            T               m_aElems[nSIZE];
        };
    };
};

//---------------------------------------------------------------------------
// Array<T,I,S>: Non-growing array.
// S serves as base class, and implements storage scheme: begin(), size()

template< class T, class I = int, class STORE = NArray::ArrayStorage >
struct Array
    : STORE::template Store<T, I>
{
    typedef typename STORE::template Store<T, I> S;

    // Tedious redundancy.
    using S::size;
    using S::begin;

    // STL-compatible typedefs.
    typedef T                           value_type;
    typedef T*                      pointer;
    typedef const T*            const_pointer;
    typedef T&                      reference;
    typedef const T&            const_reference;

    typedef T*                      iterator;
    typedef const T*            const_iterator;

    typedef I                           size_type;
    typedef typename NArray::IntTraits<size_type>::TSigned
        difference_type;

    typedef Array<T, I>              array;
    typedef Array<const T, I>    const_array;

    // Construction.
    Array()
    {}

    // Forward single- and double-argument constructors.
    template<class In>
    explicit Array(const In& i)
        : S(i)
    {}

    template<class In1, class In2>
    Array(const In1& i1, const In2& i2)
        : S(i1, i2)
    {}

    // Accessors.
    inline bool empty() const
    { return size() == 0; }
    inline size_type size_mem() const
    { return size() * sizeof(T); }

    CONST_VAR_FUNCTION(T * data(),
        { return begin();
        })

    CONST_VAR_FUNCTION(T * end(),
        { return begin() + size();
        })

    CONST_VAR_FUNCTION(T * rbegin(),
        { return begin() + size() - 1;
        })

    CONST_VAR_FUNCTION(T * rend(),
        { return begin() - 1;
        })

    CONST_VAR_FUNCTION(T & front(),
        {
            assert(!empty());
            return *begin();
        })

    CONST_VAR_FUNCTION(T & back(),
        {
            assert(!empty());
            return *rbegin();
        })

    CONST_VAR_FUNCTION(T & at(size_type i),
        {
            CRY_ASSERT_TRACE(i >= 0 && i < size(), ("Index %lld is out of range (array size is %lld)", (long long int) i, (long long int) size()));
            return begin()[i];
        })

    CONST_VAR_FUNCTION(T & operator [](size_type i),
        {
            CRY_ASSERT_TRACE(i >= 0 && i < size(), ("Index %lld is out of range (array size is %lld)", (long long int) i, (long long int) size()));
            return begin()[i];
        })

    // Conversion to canonical array type.
    operator array()
    { return array(begin(), size()); }
    operator const_array() const
    {
        return const_array(begin(), size());
    }

    // Additional conversion via operator() to full or sub array.
    array operator ()(size_type i, size_type count)
    {
        assert(i >= 0 && i + count <= size());
        return array(begin() + i, count);
    }
    const_array operator ()(size_type i, size_type count) const
    {
        assert(i >= 0 && i + count <= size());
        return const_array(begin() + i, count);
    }

    array operator ()(size_type i = 0)
    { return (*this)(i, size() - i); }
    const_array operator ()(size_type i = 0) const
    { return (*this)(i, size() - i); }

    // Basic element assignment functions.

    // Copy values to existing elements.
    void fill(const T& val)
    {
        for_array_ptr (T, it, *this)
        * it = val;
    }

    void copy(const_array source)
    {
        assert(source.size() >= size());
        const T* s = source.begin();
        for_array_ptr (T, it, *this)
        * it = *s++;
    }

    // Raw element construct/destruct functions.
    iterator init()
    {
        for_array_ptr (T, it, *this)
        new(it) T;
        return begin();
    }
    iterator init(const T& val)
    {
        for_array_ptr (T, it, *this)
        new(it) T(val);
        return begin();
    }
    iterator init(const_array source)
    {
        assert(source.size() >= size());
        assert(source.end() <= begin() || source.begin() >= end());
        const_iterator s = source.begin();
        for_array_ptr (T, it, *this)
        new(it) T(*s++);
        return begin();
    }

    iterator move_init(array source)
    {
        assert(source.size() >= size());
        iterator s = source.begin();
        if (s != begin())
        {
            if (raw_movable(*s))
            {
                memmove(begin(), s, size_mem());
            }
            else if (s > begin() || source.end() <= begin())
            {
                for_array_ptr (T, it, *this)
                    ::move_init(*it, *s++);
            }
            else
            {
                s += size();
                for (iterator it = end(); it > begin(); )
                {
                    ::move_init(*--it, *--s);
                }
            }
        }
        return begin();
    }

    void destroy()
    {
        // Destroy in reverse order, to complement construction order.
        for (iterator it = rbegin(); it > rend(); --it)
        {
            it->~T();
        }
    }
};

// Type-inferring constructor.

template<class T, class I>
inline Array<T, I> ArrayT(T* elems, I count)
{
    return Array<T, I>(elems, count);
}

template<class T>
inline Array<T, size_t> ArrayT(T* start, T* finish)
{
    return Array<T, size_t>(start, finish);
}

// StaticArray<T,I,nSIZE>
// A superior alternative to static C arrays.
// Provides standard STL-like Array interface, including bounds-checking.
//      standard:       Type array[256];
//      structured: StaticArray<Type,256> array;

template<class T, int nSIZE, class I = int>
struct StaticArray
    : Array< T, I, NArray::StaticArrayStorage<nSIZE> >
{
};

//---------------------------------------------------------------------------
// Specify allocation for dynamic arrays

namespace NAlloc
{
    // Multi-purpose allocation function prototype
    // pMem = 0, nSize != 0:        allocate new mem, nSize = actual amount alloced
    // pMem != 0, nSize = 0:        deallcate mem
    // pMem != 0, nSize != 0:       nSize = actual amount allocated
    typedef void* (* Allocator)(void* pMem, size_t& nSize, size_t nAlign, bool bSlack);

    //
    // Allocation utilities
    //

    inline size_t realloc_size(size_t nMinSize)
    {
        // Choose an efficient realloc size, when growing an existing (non-zero) block.
        // Find the next power-of-two, minus a bit of presumed system alloc overhead.
        static const size_t nMinAlloc = 32;
        static const size_t nOverhead = 16;
        static const size_t nDoubleLimit =
            sizeof(size_t) < 8 ? 1 << 12      // 32-bit system
            : 1 << 16;                                            // >= 64-bit system

        nMinSize += nOverhead;
        size_t nAlloc = nMinAlloc;
        while (nAlloc < nMinSize)
        {
            nAlloc <<= 1;
        }
        if (nAlloc > nDoubleLimit)
        {
            size_t nAlign = NArray::max(nAlloc >> 3, nDoubleLimit);
            nAlloc = Align(nMinSize, nAlign);
        }
        return nAlloc - nOverhead;
    }

    template<class A, class T, class I>
    T* reallocate(A& allocator, T* old_elems, I old_size, I& new_size, size_t alignment = 1, bool allow_slack = false)
    {
        T* new_elems;
        if (new_size)
        {
            size_t new_bytes = new_size * sizeof(T);
            new_elems = (T*) allocator.alloc(0, new_bytes, alignment, allow_slack);
            assert(IsAligned(new_elems, alignment));
            assert(new_bytes >= new_size * sizeof(T));
            new_size = check_cast<I>(new_bytes / sizeof(T));
        }
        else
        {
            new_elems = 0;
        }

        if (old_elems)
        {
            Array<T, I> old_elems_array(old_elems, old_size);
            if (new_elems)
            {
                // Move elements.
                ArrayT(new_elems, NArray::min(old_size, new_size)).move_init(old_elems_array);
            }

            // Dealloc old.
            old_elems_array.destroy(); // call destructors
            size_t zero = 0;
            allocator.alloc(old_elems, zero, alignment);
        }

        return new_elems;
    }

    template<class A>
    inline size_t get_alloc_size(const A& allocator, const void* pMem, size_t nSize, size_t nAlign)
    {
        non_const(allocator).alloc((void*)pMem, nSize, nAlign);
        return nSize;
    }

    struct AllocFunction
    {
        Allocator       m_Function;

        void* alloc(void* pMem, size_t& nSize, size_t nAlign, bool bSlack = false)
        {
            return m_Function(pMem, nSize, nAlign, bSlack);
        }
    };

    // Adds prefix bytes to allocation, preserving alignment
    template<class A, class Prefix, int nSizeAlign = 1>
    struct AllocPrefix
        : A
    {
        void* alloc(void* pMem, size_t& nSize, size_t nAlign, bool bSlack = false)
        {
            // Adjust pointer and size for prefix bytes
            nAlign = NArray::max(nAlign, alignof(Prefix));
            size_t nPrefixSize = Align(sizeof(Prefix), nAlign);

            if (pMem)
            {
                pMem = (char*)pMem - nPrefixSize;
            }
            if (nSize)
            {
                nSize = Align(nSize, nSizeAlign);
                nSize += nPrefixSize;
            }

            pMem = A::alloc(pMem, nSize, nAlign, bSlack);

            if (nSize)
            {
                nSize -= nPrefixSize;
            }
            if (pMem)
            {
                pMem = (char*)pMem + nPrefixSize;
            }
            return pMem;
        }
    };

    // Stores and retrieves allocator function in memory, for compatibility with diverse allocators
    template<class A>
    struct AllocCompatible
    {
        void* alloc(void* pMem, size_t& nSize, size_t nAlign, bool bSlack = false)
        {
            nAlign = NArray::max(nAlign, alignof(Allocator));
            if (pMem)
            {
                // Retrieve original allocation function, for dealloc or size query
                AllocPrefix<AllocFunction, Allocator> alloc_prefix;
                alloc_prefix.m_Function = ((Allocator*)pMem)[-1];
                return alloc_prefix.alloc(pMem, nSize, nAlign, bSlack);
            }
            else if (nSize)
            {
                // Allocate new with this module's base_allocator, storing pointer to function
                AllocPrefix<A, Allocator> alloc_prefix;
                pMem = alloc_prefix.alloc(pMem, nSize, nAlign, bSlack);
                if (pMem)
                {
                    ((Allocator*)pMem)[-1] = &A::alloc;
                }
            }
            return pMem;
        }
    };

    //---------------------------------------------------------------------------
    // Allocators for DynArray.

    // Standard CryModule memory allocation, using aligned versions
    struct ModuleAlloc
    {
        static void* alloc(void* pMem, size_t& nSize, size_t nAlign, bool bSlack = false)
        {
            if (pMem)
            {
                if (nSize)
                {
                    // Return memory usage, adding presumed alignment padding
                    if (nAlign > sizeof(size_t))
                    {
                        nSize += nAlign - sizeof(size_t);
                    }
                }
                else
                {
                    // Dealloc
                    CryModuleMemalignFree(pMem);
                }
            }
            else if (nSize)
            {
                // Alloc
                if (bSlack)
                {
                    nSize = realloc_size(nSize);
                }
                return CryModuleMemalign(nSize, nAlign);
            }
            return 0;
        }
    };

    // Standard allocator for DynArray stores a compatibility pointer in the memory
    typedef AllocCompatible<ModuleAlloc> StandardAlloc;
};

//---------------------------------------------------------------------------
// Storage schemes for dynamic arrays
namespace NArray
{
    //---------------------------------------------------------------------------
    // SmallDynStorage: STORAGE scheme for DynArray<T,I,STORAGE,ALLOC>.
    // Array is just a single pointer, size and capacity information stored before the array data.

    template<class A = NAlloc::StandardAlloc>
    struct SmallDynStorage
    {
        template<class T, class I>
        struct Store
            : private A
        {
            struct Header
            {
                static const I nCAP_BIT = I(1) << (sizeof(I) * 8 - 1);

                ILINE char* data() const
                {
                    assert(IsAligned(this, sizeof(I)));
                    return (char*)(this + 1);
                }
                ILINE bool is_null() const
                { return m_nSizeCap == 0; }
                ILINE I size() const
                { return m_nSizeCap & ~nCAP_BIT; }

                I   capacity() const
                {
                    I aligned_bytes = Align(size() * sizeof(T), sizeof(I));
                    if (m_nSizeCap & nCAP_BIT)
                    {
                        // Capacity stored in word following data
                        return *(I*)(data() + aligned_bytes);
                    }
                    else
                    {
                        // Capacity - size < sizeof(I)
                        return aligned_bytes / sizeof(T);
                    }
                }

                void set_sizes(I s, I c)
                {
                    // Store size, and assert against overflow.
                    assert(s <= c);
                    m_nSizeCap = s;
                    I aligned_bytes = Align(s * sizeof(T), sizeof(I));
                    if (c * sizeof(T) >= aligned_bytes + sizeof(I))
                    {
                        // Has extra capacity, more than word-alignment
                        m_nSizeCap |= nCAP_BIT;
                        *(I*)(data() + aligned_bytes) = c;
                    }
                    assert(size() == s);
                    assert(capacity() == c);
                }

            protected:
                I       m_nSizeCap;             // Store allocation size, with last bit indicating extra capacity.

            public:

                static T* null_header()
                {
                    // m_aElems is never 0, for empty array points to a static empty header.
                    // Declare a big enough static var to account for alignment.
                    struct EmptyHeader
                    {
                        Header  head;
                        char        pad[alignof(T)];
                    };
                    static EmptyHeader s_EmptyHeader;

                    // The actual header pointer can be anywhere in the struct, it's all initialized to 0.
                    static T* s_EmptyElems = (T*)Align(s_EmptyHeader.pad, alignof(T));

                    return s_EmptyElems;
                }
            };

            // Construction.
            Store()
            {
                set_null();
            }

            Store(const A& a)
                : A(a)
            {
                set_null();
            }

            // Basic storage.
            CONST_VAR_FUNCTION(T * begin(),
                { return m_aElems;
                })
            inline I size() const
            { return header()->size(); }
            inline I capacity() const
            { return header()->capacity(); }
            size_t get_alloc_size() const
            { return is_null() ? 0 : NAlloc::get_alloc_size(allocator(), begin(), capacity() * sizeof(T), alignof(T)); }

            void resize_raw(I new_size, bool allow_slack = false)
            {
                I new_cap = capacity();
                if (allow_slack ? new_size > new_cap : new_size != new_cap)
                {
                    new_cap = new_size;
                    m_aElems = NAlloc::reallocate(allocator(), header()->is_null() ? 0 : m_aElems, size(), new_cap, alignof(T), allow_slack);
                    if (!m_aElems)
                    {
                        set_null();
                        return;
                    }
                }
                header()->set_sizes(new_size, new_cap);
            }

        protected:

            T*              m_aElems;

            CONST_VAR_FUNCTION(Header * header(),
                {
                    assert(m_aElems);
                    return ((Header*)m_aElems) - 1;
                })

            void set_null()
            { m_aElems = Header::null_header(); }
            bool is_null() const
            { return header()->is_null(); }

            typedef NAlloc::AllocPrefix<A, Header, sizeof(I)> AP;

            AP& allocator()
            {
                COMPILE_TIME_ASSERT(sizeof(AP) == sizeof(A));
                return *(AP*)this;
            }
            const AP& allocator() const
            {
                return *(const AP*)this;
            }
        };
    };

    //---------------------------------------------------------------------------
    // StaticDynStorage: STORAGE scheme with a statically sized member array.

    template<int nSIZE>
    struct StaticDynStorage
    {
        template<class T, class I = int>
        struct Store
            : ArrayStorage::Store<T, I>
        {
            Store()
                : ArrayStorage::Store<T, I>((T*)Align(m_aData, alignof(T)), 0) {}

            static I capacity()
            { return (I)nSIZE; }
            static size_t get_alloc_size()
            { return 0; }

            void resize_raw(I new_size, [[maybe_unused]] bool allow_slack = false)
            {
                // cannot realloc, just set size
                assert(new_size >= 0 && new_size <= capacity());
                this->m_nCount = new_size;
            }

        protected:

            char        m_aData[nSIZE * sizeof(T) + alignof(T) - 1];      // Storage for elems, deferred construction
        };
    };
};

// Legacy base class of DynArray, only used for read-only access
#define DynArrayRef DynArray

//---------------------------------------------------------------------------
// DynArray<T,I,S>: Extension of Array allowing dynamic allocation.
// S specifies storage scheme, as with Array, but adds resize(), capacity(), ...
// A specifies the actual memory allocation function: alloc()

// NOTE: This version has been re-based on AZStd::vector for correctness and performance
// The original implementation has been retained as LegacyDynArray below, for the few
// cases where we must retain the old internal behavior
template< class T, class I = int, class STORE = NArray::SmallDynStorage<> >
struct DynArray
    : public AZStd::vector<T, AZ::StdLegacyAllocator>
{
    typedef AZStd::vector<T, AZ::StdLegacyAllocator> vector_base;
    using value_type = typename vector_base::value_type;
    using size_type = I;
    using iterator = typename vector_base::iterator;
    using const_iterator = typename vector_base::const_iterator;

    using vector_base::begin;
    using vector_base::end;

    DynArray()
        : vector_base::vector()
    {
    }

    explicit DynArray(size_type numElements)
        : vector_base::vector(numElements)
    {
    }

    DynArray(size_type numElements, const T& value)
        : vector_base::vector(numElements, value)
    {
    }

    // Ignore any specialized allocators, they all pull from the LegacyAllocator now
    // Because we mandate that all DynArrays use the StdLegacyAllocator, matching allocators
    // isn't meaningful here, so it's factored out
    // Note that the STORE/S& is ignored entirely, because we do not support sharing storage
    // between 2 DynArrays anymore. LegacyDynArray can still do that, but this feature is no longer used
    template <typename S, typename = AZStd::enable_if_t<AZStd::is_base_of<S, typename STORE::template Store<T, I>>::value>>
    explicit DynArray(const S&)
        : vector_base::vector()
    {
    }

    size_type capacity() const
    {
        return static_cast<size_type>(vector_base::capacity());
    }

    size_type size() const
    {
        return static_cast<size_type>(vector_base::size());
    }

    size_type available() const
    {
        return capacity() - size();
    }

    size_type get_alloc_size() const
    {
        return capacity() * sizeof(T);
    }

    // Grow array, return iterator to new raw elems.
    iterator grow_raw(size_type count = 1, bool /*allow_slack*/ = true)
    {
        vector_base::resize(size() + count);
        return end() - count;
    }
    
    iterator grow(size_type count)
    {
        return grow_raw(count);
    }
    iterator grow(size_type count, const T& val)
    {
        vector_base::reserve(size() + count);
        for (size_type idx = 0; idx < count; ++idx)
        {
            vector_base::push_back(val);
        }
        return end() - count;
    }

    void shrink()
    {
        // Realloc memory to exact array size.
        vector_base::shrink_to_fit();
    }

    void resize(size_type newSize)
    {
        vector_base::resize(newSize);
    }

    void resize(size_type new_size, const T& val)
    {
        size_type s = size();
        if (new_size > s)
        {
            grow(new_size - s, val);
        }
        else
        {
            pop_back(s - new_size);
        }
    }

    void assign(const_iterator first, const_iterator last)
    {
        vector_base::assign(first, last);
    }

    void assign(size_type n, const T& val)
    {
        clear();
        grow(n, val);
    }

    iterator push_back()
    {
        return grow(1);
    }
    iterator push_back(const T& val)
    {
        vector_base::push_back(val);
        return vector_base::end() - 1;
    }
    iterator push_back(const DynArray& other)
    {
        return insert(end(), other.begin(), other.end());
    }

    iterator insert_raw(iterator pos, size_type count = 1)
    {
        // Grow array, return iterator to inserted raw elems.
        assert(pos >= begin() && pos <= end());
        vector_base::insert(pos, count, T());
        return pos;
    }

    iterator insert(iterator it, const T& val)
    {
        vector_base::insert(it, 1, val);
        return it;
    }
    iterator insert(iterator it, size_type count, const T& val)
    {
        vector_base::insert(it, count, val);
        return it;
    }
    iterator insert(iterator it, const_iterator start, const_iterator finish)
    {
        vector_base::insert(it, start, finish);
        return it;
    }

    iterator insert(size_type pos)
    {
        return insert_raw(begin() + pos);
    }
    iterator insert(size_type pos, const T& val)
    {
        iterator it = insert_raw(begin() + pos);
        *it = val;
        return it;
    }

    void pop_back(size_type count = 1, [[maybe_unused]] bool allow_slack = true)
    {
        // Destroy erased elems, change size without reallocing.
        assert(count >= 0 && count <= size());
        for (size_type idx = 0; idx < count; ++idx)
        {
            vector_base::pop_back();
        }
    }

    iterator erase(iterator pos)
    {
        return vector_base::erase(pos);
    }

    iterator erase(iterator start, iterator finish)
    {
        AZ_Assert(start >= begin() && finish >= start && finish <= end(), "DynArray: Erasure range out of bounds");

        // Copy over erased elems, destroy those at end.
        iterator it = start, e = end();
        while (finish < e)
        {
            *it++ = *finish++;
        }
        pop_back(check_cast<size_type>(finish - it));
        return it;
    }

    iterator erase(size_type pos, size_type count = 1)
    {
        return erase(begin() + pos, begin() + pos + count);
    }

    void clear()
    {
        vector_base::clear();
        vector_base::shrink_to_fit();
    }
};

//---------------------------------------------------------------------------
// Original Cry DynArray
//---------------------------------------------------------------------------
template< class T, class I = int, class STORE = NArray::SmallDynStorage<> >
struct LegacyDynArray
    : Array< T, I, STORE >
{
    typedef LegacyDynArray<T, I, STORE> self_type;
    typedef Array<T, I, STORE> super_type;
    typedef typename STORE::template Store<T, I> S;

    // Tedious redundancy for GCC.
    using_type(super_type, size_type);
    using_type(super_type, iterator);
    using_type(super_type, const_iterator);
    using_type(super_type, array);
    using_type(super_type, const_array);

    using super_type::size;
    using super_type::capacity;
    using super_type::begin;
    using super_type::end;
    using super_type::at;
    using super_type::copy;
    using super_type::init;
    using super_type::destroy;

    //
    // Construction.
    //
    LegacyDynArray()
    {}

    LegacyDynArray(size_type count)
    {
        grow(count);
    }
    LegacyDynArray(size_type count, const T& val)
    {
        grow(count, val);
    }

#if !defined(_DISALLOW_INITIALIZER_LISTS)
    // Initializer-list
    LegacyDynArray(std::initializer_list<T> l)
    {
        push_back(Array<const T, I>(l.begin(), l.size()));
    }
#endif

    // Copying from a generic array type.
    LegacyDynArray(const_array a)
    {
        push_back(a);
    }
    self_type& operator =(const_array a)
    {
        if (a.begin() >= begin() && a.end() <= end())
        {
            // Assigning from (partial) self; remove undesired elements.
            erase((T*)a.end(), end());
            erase(begin(), (T*)a.begin());
        }
        else
        {
            // Assert no overlap.
            assert(a.end() <= begin() || a.begin() >= end());
            if (a.size() == size())
            {
                // If same size, perform element copy.
                copy(a);
            }
            else
            {
                // If different sizes, destroy then copy init elements.
                pop_back(size());
                push_back(a);
            }
        }
        return *this;
    }

    // Copy init/assign.
    inline LegacyDynArray(const self_type& a)
    {
        push_back(a());
    }
    inline self_type& operator =(const self_type& a)
    {
        return *this = a();
    }

    // Init/assign from basic storage type.
    inline LegacyDynArray(const S& a)
    {
        push_back(const_array(a.begin(), a.size()));
    }
    inline self_type& operator =(const S& a)
    {
        return *this = const_array(a.begin(), a.size());
    }

    inline ~LegacyDynArray()
    {
        destroy();
        S::resize_raw(0);
    }

    void swap(self_type& a)
    {
        // Swap storage structures, no element copying
        S temp = static_cast<S&>(*this);
        static_cast<S&>(*this) = static_cast<S&>(a);
        static_cast<S&>(a) = temp;
    }

    inline size_type available() const
    {
        return capacity() - size();
    }

    //
    // Allocation modifiers.
    //

    void reserve(size_type count)
    {
        if (count > capacity())
        {
            I s = size();
            S::resize_raw(count, false);
            S::resize_raw(s, true);
        }
    }

    // Grow array, return iterator to new raw elems.
    iterator grow_raw(size_type count = 1, bool allow_slack = true)
    {
        S::resize_raw(size() + count, allow_slack);
        return end() - count;
    }
    Array<T, I> append_raw(size_type count = 1, bool allow_slack = true)
    {
        return Array<T, I>(grow_raw(count, allow_slack), count);
    }

    iterator grow(size_type count)
    {
        return append_raw(count).init();
    }
    iterator grow(size_type count, const T& val)
    {
        return append_raw(count).init(val);
    }

    void shrink()
    {
        // Realloc memory to exact array size.
        S::resize_raw(size());
    }

    void resize(size_type new_size)
    {
        size_type s = size();
        if (new_size > s)
        {
            append_raw(new_size - s, false).init();
        }
        else
        {
            pop_back(s - new_size, false);
        }
    }
    void resize(size_type new_size, const T& val)
    {
        size_type s = size();
        if (new_size > s)
        {
            append_raw(new_size - s, false).init(val);
        }
        else
        {
            pop_back(s - new_size, false);
        }
    }

    void assign(size_type n, const T& val)
    {
        resize(n);
        fill(val);
    }

    void assign(const_iterator start, const_iterator finish)
    {
        *this = const_array(start, finish);
    }

    iterator push_back()
    {
        return grow(1);
    }
    iterator push_back(const T& val)
    {
        return grow(1, val);
    }
    iterator push_back(const_array a)
    {
        return append_raw(a.size(), false).init(a);
    }

    array insert_raw(iterator pos, size_type count = 1)
    {
        // Grow array, return iterator to inserted raw elems.
        assert(pos >= begin() && pos <= end());
        size_t i = pos - begin();
        append_raw(count);
        (*this)(i + count).move_init((*this)(i));
        return (*this)(i, count);
    }

    iterator insert(iterator it, const T& val)
    {
        return insert_raw(it, 1).init(val);
    }
    iterator insert(iterator it, size_type count, const T& val)
    {
        return insert_raw(it, count).init(val);
    }
    iterator insert(iterator it, const_iterator start, const_iterator finish)
    {
        return insert(it, const_array(start, finish));
    }
    iterator insert(iterator it, const_array a)
    {
        return insert_raw(it, a.size()).init(a);
    }

    iterator insert(size_type pos)
    {
        return insert_raw(&at(pos)).init();
    }
    iterator insert(size_type pos, const T& val)
    {
        return insert_raw(&at(pos)).init(val);
    }
    iterator insert(size_type pos, const_array a)
    {
        return insert_raw(&at(pos), a.size()).init(a);
    }

    void pop_back(size_type count = 1, bool allow_slack = true)
    {
        // Destroy erased elems, change size without reallocing.
        assert(count >= 0 && count <= size());
        size_type new_size = size() - count;
        (*this)(new_size).destroy();
        S::resize_raw(new_size, allow_slack);
    }

    iterator erase(iterator start, iterator finish)
    {
        assert(start >= begin() && finish >= start && finish <= end());

        // Copy over erased elems, destroy those at end.
        iterator it = start, e = end();
        while (finish < e)
        {
            *it++ = *finish++;
        }
        pop_back(check_cast<size_type>(finish - it));
        return it;
    }

    iterator erase(iterator it)
    {
        return erase(it, it + 1);
    }

    iterator erase(size_type pos, size_type count = 1)
    {
        return erase(begin() + pos, begin() + pos + count);
    }

    void clear()
    {
        destroy();
        S::resize_raw(0);
    }
};

template<class T, int nSIZE, class I = int>
struct StaticDynArray
    : LegacyDynArray< T, I, NArray::StaticDynStorage<nSIZE> >
{
};



    #include "CryPodArray.h"


#endif // CRYINCLUDE_CRYCOMMON_CRYARRAY_H

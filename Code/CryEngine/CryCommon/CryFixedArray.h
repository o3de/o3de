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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

/*
CryFixedArray.h
    - no longer support being created on the stack (since the alignment code was changed to support adding CryFixedArrays into stl::vectors)
    - performs construction or destruction only on elements as they become live/dead or are moved around during the RemoveAt() reshuffle
    - just a range checked equivelant of a standard array
    - for now only allows push_back() population of array
    - if using as a class member variable ensure to put the CryFixedArrays after all other member variables at the bottom of your class
      to ensure all members stay on the same cacheline
*/

#ifndef CRYINCLUDE_CRYCOMMON_CRYFIXEDARRAY_H
#define CRYINCLUDE_CRYCOMMON_CRYFIXEDARRAY_H
#pragma once

#define DEBUG_CRYFIXED_ARRAY _DEBUG

template<
    unsigned int align >
struct CryFixedArrayDatum
{
};

template<>
struct CryFixedArrayDatum< 4 >
{
    typedef uint32          TDatum;
};

template<>
struct CryFixedArrayDatum< 8 >
{
    typedef uint64          TDatum;
};

template <class T, unsigned int N>
class CryFixedArray
{
protected:
    enum
    {
        ALIGN = MAX(alignof(T), sizeof(unsigned int))
    };                                                        // ALIGN at least sizeof(unsigned int)

    typedef typename CryFixedArrayDatum< ALIGN >::TDatum    TDatum;

    uint32          m_curSize[ sizeof (TDatum) / sizeof (uint32) ];     // Padded for alignment

    TDatum          m_data[(N * sizeof(T) + (sizeof(TDatum) - 1)) / sizeof(TDatum)]; // simple debugging - in VS: just add to a watch as "(T*)m_data, <N>" to see the array. ie. "(int*)m_data, 5" - the size of the array has to be a literal int

public:
    typedef T*       iterator;
    typedef const T* const_iterator;

    CryFixedArray()
    {
#if DEBUG_CRYFIXED_ARRAY
        if (((uintptr_t)m_data & (ALIGN - 1)) != 0)
        {
            CryLogAlways("CryFixedArray() error - data is not aligned. This may happen if you are creating a CryFixedArray on the stack, which isn't supported.");
        }
#endif
        CRY_ASSERT_MESSAGE(((uintptr_t)m_data & (ALIGN - 1)) == 0, "CryFixedArray() error - data is not aligned. This may happen if you are creating a CryFixedArray on the stack, which isn't supported.");
        m_curSize[0] = 0;
    }

    CryFixedArray(const CryFixedArray& other)
    {
        // doesn't require clear() this is newly constructed
        m_curSize[0] = other.m_curSize[0];

        int size = m_curSize[0];
        for (int i = 0; i < size; i++)
        {
            T& ele = operator[](i);
            const T& otherEle = other.operator[](i);
            new (&ele)T(otherEle);      // placement new
        }
    }

    CryFixedArray& operator=(const CryFixedArray& other)
    {
        if (this != &other)
        {
            clear(); // necessary to avoid potentially leaking within existing elements

            m_curSize[0] = other.m_curSize[0];

            int size = m_curSize[0];
            for (int i = 0; i < size; i++)
            {
                T& ele = operator[](i);
                const T& otherEle = other.operator[](i);
                //ele = otherEle;       // assignment instead of placement new to keep type of operation consistent - this cannot be done until this is rewritten to assign over existing elements and deconstruct any left overs, and placement new any new elements
                new (&ele)T(otherEle);      // placement new
            }
        }

        return *this;
    }

    virtual ~CryFixedArray()
    {
        clear();
    }

    ILINE T& at(unsigned int i)
    {
#if DEBUG_CRYFIXED_ARRAY
        if (i < size())
        {
            return alias_cast<T*>(m_data)[i];
        }
        else
        {
            // Log is required now as its possible to turn off assert output logging, yet you really want to know if this is happening!!!!
            CryLogAlways("CryFixedArray::at(i=%d) failed as i is out of range of curSize=%d (maxSize=%d) - forcing a crash", i, m_curSize[0], N);
            CRY_ASSERT_MESSAGE(0, string().Format("CryFixedArray::at(i=%d) failed as i is out of range of curSize=%d (maxSize=%d)", i, m_curSize[0], N));
            abort(); // better option than dereferncing a nullptr?
        }
#else
        return alias_cast<T*>(m_data)[i];
#endif
    }

    ILINE const T& at(unsigned int i) const
    {
#if DEBUG_CRYFIXED_ARRAY
        if (i < size())
        {
            return alias_cast<const T*>(m_data)[i];
        }
        else
        {
            // Log is required now as its possible to turn off assert output logging, yet you really want to know if this is happening!!!!
            CryLogAlways("CryFixedArray::at(i=%d) failed as i is out of range of curSize=%d (maxSize=%d) - forcing a crash", i, m_curSize[0], N);
            CRY_ASSERT_MESSAGE(0, string().Format("CryFixedArray::at(i=%d) failed as i is out of range of curSize=%d (maxSize=%d)", i, m_curSize[0], N));
            abort(); // better option than dereferncing a nullptr?
        }
#else
        return alias_cast<const T*>(m_data)[i];
#endif
    }

    ILINE const T& operator[](unsigned int i) const
    {
        return at(i);
    }

    ILINE T& operator[](unsigned int i)
    {
        return at(i);
    }

    ILINE void clear()
    {
        for (uint32 i = 0; i < m_curSize[0]; i++)
        {
            T& ele = operator[](i);
            ele.~T();
        }
        m_curSize[0] = 0;
#if DEBUG_CRYFIXED_ARRAY
        memset(m_data, 0, N * sizeof(T));
#endif
    }

    ILINE iterator begin()
    {
        return alias_cast<T*>(m_data);
    }
    ILINE const_iterator begin() const
    {
        return alias_cast<T*>(m_data);
    }
    ILINE iterator end()
    {
        return &(alias_cast<T*>(m_data))[m_curSize[0]];
    }
    ILINE const_iterator end() const
    {
        return &(alias_cast<T*>(m_data))[m_curSize[0]];
    }

    ILINE unsigned int max_size() const { return N; }
    ILINE unsigned int size() const { return m_curSize[0]; }
    ILINE bool empty() const { return size() == 0; }
    ILINE unsigned int isfull() const { return (size() == max_size()); }

    // allows you to push back default constructed elements
    ILINE void push_back ()
    {
        unsigned int curSize = size();
        if (curSize < N)
        {
            T* newT = &(alias_cast<T*>(m_data))[curSize];
            new (newT) T();

            m_curSize[0]++;
        }
        else
        {
            CryLogAlways("CryFixedArray::push_back() failing as array of size %u is full - NOT adding element", N);
            CRY_ASSERT_TRACE(0, ("CryFixedArray::push_back() failing as array of size %u is full - NOT adding element", N));
        }
    }

    ILINE void push_back (const T& ele)
    {
        unsigned int curSize = size();
        if (curSize < N)
        {
            T* newT = &(alias_cast<T*>(m_data))[curSize];
            new (newT) T(ele);  // placement new copy constructor - setup vtable etc

            m_curSize[0]++;
        }
        else
        {
            CryLogAlways("CryFixedArray::push_back() failing as array of size %u is full - NOT adding element", N);
            CRY_ASSERT_TRACE(0, ("CryFixedArray::push_back() failing as array of size %u is full - NOT adding element", N));
        }
    }

    ILINE void pop_back()
    {
        if (size() > 0)
        {
            back().~T();        // destruct back
            m_curSize[0]--;
        }
        else
        {
            CryLogAlways("CryFixedArray::pop_back() failed as array is empty");
            CRY_ASSERT_MESSAGE(0, "CryFixedArray::pop_back() failed as array is empty");
        }
    }

protected:
    ILINE const T& backEx() const
    {
#if DEBUG_CRYFIXED_ARRAY
        if (m_curSize[0] > 0)
        {
            return (alias_cast<T*>(m_data))[m_curSize[0] - 1];
        }
        else
        {
            CryLogAlways("CryFixedArray::back() failed as array is empty");
            CRY_ASSERT_MESSAGE(0, "CryFixedArray::back() failed as array is empty");
            abort(); // better option than dereferncing a nullptr?
        }
#else
        return (alias_cast<T*>(m_data))[m_curSize[0] - 1];
#endif
    }

public:
    ILINE const T& back() const
    {
        return backEx();
    }

    ILINE T& back()
    {
        return (T&)(backEx());
    }

    // if returns true then an element has been swapped into the new element[i] and as such may need updating to reflect its new location in memory
    ILINE bool removeAt(uint32 i)
    {
        bool swappedElement = false;

        if (i < m_curSize[0])
        {
            if (i != m_curSize[0] - 1)
            {
                operator[](i).~T(); // destruct element being removed

                // copy back() into element i
                T* newT = &(alias_cast<T*>(m_data))[i];
                new (newT) T(back());   // placement new copy constructor - setup vtable etc

                swappedElement = true;
            }
            pop_back();     // will destruct back()
        }
        else
        {
            CryLog("CryFixedArray::removeAt() failed as i=%d is out of range of curSize=%d", i, m_curSize[0]);
            CRY_ASSERT_MESSAGE(0, string().Format("CryFixedArray::removeAt() failed as i=%d is out of range of curSize=%d", i, m_curSize[0]));
        }
        return swappedElement;
    }
};

#endif // CRYINCLUDE_CRYCOMMON_CRYFIXEDARRAY_H

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Simple POD types container

#ifndef CRYINCLUDE_CRYCOMMON_CRYPODARRAY_H
#define CRYINCLUDE_CRYCOMMON_CRYPODARRAY_H
#pragma once

#include <AzCore/std/containers/vector.h>

//////////////////////////////////////////////////////////////////////////
// POD Array
// vector like class (random access O(1)) without construction/destructor/copy constructor/assignment handling
// over-allocation allows safe prefetching where required without worrying about memory page boundaries
//////////////////////////////////////////////////////////////////////////
template <class T, size_t overAllocBytes = 0>
class PodArray
{
    AZStd::vector<T> m_elements;

public:
    typedef T        value_type;
    typedef T*       iterator;
    typedef const T* const_iterator;

    //////////////////////////////////////////////////////////////////////////
    // STL compatible interface
    //////////////////////////////////////////////////////////////////////////
    void resize(size_t numElements)
    {
        m_elements.resize(numElements);
    }
    //////////////////////////////////////////////////////////////////////////
    ILINE void reserve(unsigned numElements) { m_elements.reserve(numElements); }
    ILINE void push_back(const T& rElement) { m_elements.push_back(rElement); }
    ILINE size_t size() const   { return m_elements.size(); }
    ILINE size_t capacity() const { return m_elements.capacity(); }

    ILINE void clear() { m_elements.clear(); }
    ILINE T* begin() { return m_elements.begin(); }
    ILINE T* end() { return m_elements.end(); }
    ILINE const T* begin() const { return m_elements.begin(); }
    ILINE const T* end() const { return m_elements.end(); }
    ILINE bool empty() const { return m_elements.empty(); }

    ILINE const T& front() const { return m_elements.front(); }
    ILINE T& front() { return m_elements.front(); }

    ILINE const T& back() const { return m_elements.back(); }
    ILINE T& back() { return m_elements.back(); }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    PodArray()
        : m_elements()
    {
    }
    PodArray(int elem_count, int nNewCount = 0)
    {
        m_elements.reserve(elem_count);
        m_elements.resize(nNewCount);
    }
    PodArray(const PodArray<T>& from)
        : m_elements(from.m_elements)
    {
    }
    ~PodArray()
    {
    }

    void Reset() { m_elements.clear(); }
    void Free()
    {
        m_elements.clear();
        m_elements.shrink_to_fit();
    }

    ILINE void Clear()
    {
        m_elements.clear();
    }

    int Find(const T& p)
    {
        const auto it = AZStd::find(m_elements.begin(), m_elements.end(), p);
        if (it != m_elements.end())
        {
            return static_cast<int>(AZStd::distance(m_elements.begin(), it));
        }
        return -1;
    }

    inline void AddList(const PodArray<T>& lstAnother)
    {
        AZStd::copy(lstAnother.m_elements.begin(), lstAnother.m_elements.end(), AZStd::back_inserter(m_elements));
    }

    inline void AddList(T* pAnotherArray, int nAnotherCount)
    {
        AZStd::copy(pAnotherArray, pAnotherArray + nAnotherCount, AZStd::back_inserter(m_elements));
    }

    ILINE void Add(const T& p)
    {
        m_elements.push_back(p);
    }

    ILINE T& AddNew()
    {
        m_elements.emplace_back();
        return m_elements.back();
    }

    void InsertBefore(const T& p, const unsigned int nBefore)
    {
        m_elements.insert(m_elements.begin() + nBefore, p);
    }

    void CheckAllocated(int elem_count)
    {
        if (m_elements.size() < elem_count)
        {
            m_elements.resize(elem_count);
        }
    }

    void PreAllocate(int elem_count, int nNewCount = -1)
    {
        m_elements.reserve(elem_count);
        if (nNewCount >= 0)
        {
            m_elements.resize(nNewCount);
        }
    }

    inline void Delete(const int nElemId, const int nElemCount = 1)
    {
        AZ_Assert(nElemId >= 0 && nElemId + nElemCount <= size(), "Index out of bounds");
        m_elements.erase(m_elements.begin() + nElemId, m_elements.begin() + nElemId + nElemCount);
    }

    inline void DeleteFastUnsorted(const int nElemId, const int nElemCount = 1)
    {
        AZ_Assert(nElemId >= 0 && nElemId + nElemCount <= size(), "Index out of bounds");
        m_elements.erase(m_elements.begin() + nElemId, m_elements.begin() + nElemId + nElemCount);
    }

    inline bool Delete(const T& del)
    {
        const size_t numElements = m_elements.size();
        m_elements.erase(std::remove(m_elements.begin(), m_elements.end(), del), m_elements.end());
        return numElements != m_elements.size();
    }

    ILINE  int Count() const { return m_elements.size(); }
    ILINE  unsigned int Size() const { return m_elements.size(); }

    ILINE  int IsEmpty() const { return m_elements.empty(); }

    ILINE  const T& operator [] (int i) const { return m_elements[i]; }
    ILINE  T& operator [] (int i) { return m_elements[i]; }
    ILINE  const T& GetAt(int i) const { return m_elements[i]; }
    ILINE  T& GetAt(int i) { return m_elements[i]; }
    ILINE  const T* Get(int i) const { return &m_elements[i]; }
    ILINE  T* Get(int i) { return &m_elements[i]; }
    ILINE  const T* GetElements() const { return m_elements.data(); }
    ILINE  T* GetElements() { return m_elements.data(); }
    
    ILINE  unsigned int GetDataSize() const { return m_elements.size() * sizeof(T); }

    const T& Last() const { return m_elements.back(); }
    T& Last() { return m_elements.back(); }

    ILINE void DeleteLast()
    {
        assert(!m_elements.empty());
        m_elements.pop_back();
    }

    PodArray<T>& operator=(const PodArray<T>& source_list)
    {
        m_elements = source_list.m_elements;
        return *this;
    }

    //////////////////////////////////////////////////////////////////////////
    // Return true if arrays have the same data.
    bool Compare(const PodArray<T>& l) const
    {
        return m_elements == l.m_elements;
    }

    // for statistics
    ILINE size_t ComputeSizeInMemory() const
    {
        return (sizeof(*this) + sizeof(T) * m_elements.capacity()) + overAllocBytes;
    }

    ILINE void RemoveIf(AZStd::function<bool(const T&)> testFunc)
    {
        m_elements.erase(AZStd::remove_if(m_elements.begin(), m_elements.end(), testFunc), m_elements.end());
    }
};

#endif // CRYINCLUDE_CRYCOMMON_CRYPODARRAY_H

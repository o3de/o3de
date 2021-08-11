/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "StandardHeaders.h"
#include "MCoreSystem.h"
#include "Algorithms.h"
#include "MemoryManager.h"


namespace MCore
{
    /**
     * Dynamic array template, using aligned memory allocations.
     * This array template allows dynamic sizing. It also stores the memory category of the data.
     * It can theoretically store 18446744073709551614 items (maximum size_t value - 1 for the invalid index).
     */
    template <typename T, uint32 alignment>
    class AlignedArray
    {
    public:
        /**
         * The memory block ID, used inside the memory manager.
         * This will make all arrays remain in the same memory blocks, which is more efficient in a lot of cases.
         * However, array data can still remain in other blocks.
         */
        enum
        {
            MEMORYBLOCK_ID = 3
        };

        /**
         * Default constructor.
         * Initializes the array so it's empty and has no memory allocated.
         */
        MCORE_INLINE AlignedArray()
            : m_data(nullptr)
            , m_length(0)
            , m_maxLength(0)
            , m_memCategory(MCORE_MEMCATEGORY_ARRAY)              {}

        /**
         * Constructor which creates a given number of elements.
         * @param elems The element data.
         * @param num The number of elements in 'elems'.
         * @param memCategory The memory category the array is in.
         */
        MCORE_INLINE explicit AlignedArray(T* elems, size_t num, uint16 memCategory = MCORE_MEMCATEGORY_ARRAY)
            : m_length(num)
            , m_maxLength(AllocSize(num))
            , m_memCategory(memCategory)
        {
            m_data = (T*)AlignedAllocate(m_maxLength * sizeof(T), alignment, m_memCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            for (size_t i = 0; i < m_length; ++i)
            {
                Construct(i, elems[i]);
            }
        }

        /**
         * Constructor which initializes the length of the array on a given number.
         * @param initSize The number of ellements to allocate space for.
         * @param memCategory The memory category the array is in.
         */
        MCORE_INLINE explicit AlignedArray(size_t initSize, uint16 memCategory = MCORE_MEMCATEGORY_ARRAY)
            : m_data(nullptr)
            , m_length(initSize)
            , m_maxLength(initSize)
            , m_memCategory(memCategory)
        {
            if (m_maxLength > 0)
            {
                m_data = (T*)AlignedAllocate(m_maxLength * sizeof(T), alignment, m_memCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
                for (size_t i = 0; i < m_length; ++i)
                {
                    Construct(i);
                }
            }
        }

        /**
         * Copy constructor.
         * @param other The other array to copy the data from.
         */
        AlignedArray(const AlignedArray<T, alignment>& other)
            : m_data(nullptr)
            , m_length(0)
            , m_maxLength(0)
            , m_memCategory(MCORE_MEMCATEGORY_ARRAY)              { *this = other; }

        /**
         * Move constructor.
         * @param other The array to move the data from.
         */
        AlignedArray(AlignedArray<T, alignment>&& other)                                                                { m_data = other.m_data; m_length = other.m_length; m_maxLength = other.m_maxLength; m_memCategory = other.m_memCategory; other.m_data = nullptr; other.m_length = 0; other.m_maxLength = 0; }

        /**
         * Destructor. Deletes all entry data.
         * However, if you store pointers to objects, these objects won't be deleted.<br>
         * Example:<br>
         * <pre>
         * AlignedArray< Object*, 16 > data;
         * for (size_t i=0; i<10; i++)
         *    data.Add( new Object() );
         * </pre>
         * Now when the array 'data' will be destructed, it will NOT free up the memory of the integers which you allocated by hand, using new.
         * In order to free up this memory, you can do this:
         * <pre>
         * for (size_t i=0; i<data.GetLength(); ++i)
         *      delete data[i];
         * data.Clear();
         * </pre>
         */
        ~AlignedArray()
        {
            for (size_t i = 0; i < m_length; ++i)
            {
                Destruct(i);
            }
            if (m_data)
            {
                AlignedFree(m_data);
            }
        }

        /**
         * Get the memory category ID where allocations made by this array belong to.
         * On default the memory category is 0, which means unknown.
         * @result The memory category ID.
         */
        MCORE_INLINE uint16 GetMemoryCategory() const                           { return m_memCategory; }

        /**
         * Set the memory category ID, where allocations made by this array will belong to.
         * On default, after construction of the array, the category ID is 0, which means it is unknown.
         * @param categoryID The memory category ID where this arrays allocations belong to.
         */
        MCORE_INLINE void SetMemoryCategory(uint16 categoryID)                  { m_memCategory = categoryID; }

        /**
         * Get a pointer to the first element.
         * @result A pointer to the first element.
         */
        MCORE_INLINE T* GetPtr()                                                { return m_data; }

        /**
         * Get a pointer to the first element.
         * @result A pointer to the first element.
         */
        MCORE_INLINE T* GetPtr() const                                          { return m_data; }

        /**
         * Get a given item/element.
         * @param pos The item/element number.
         * @result A reference to the element.
         */
        MCORE_INLINE T& GetItem(size_t pos)                                     { return m_data[pos]; }

        /**
         * Get the first element.
         * @result A reference to the first element.
         */
        MCORE_INLINE T& GetFirst()                                              { return m_data[0]; }

        /**
         * Get the last element.
         * @result A reference to the last element.
         */
        MCORE_INLINE T& GetLast()                                               { return m_data[m_length - 1]; }

        /**
         * Get a read-only pointer to the first element.
         * @result A read-only pointer to the first element.
         */
        MCORE_INLINE const T* GetReadPtr() const                                { return m_data; }

        /**
         * Get a read-only reference to a given element number.
         * @param pos The element number.
         * @result A read-only reference to the given element.
         */
        MCORE_INLINE const T& GetItem(size_t pos) const                         { return m_data[pos]; }

        /**
         * Get a read-only reference to the first element.
         * @result A read-only reference to the first element.
         */
        MCORE_INLINE const T& GetFirst() const                                  { return m_data[0]; }

        /**
         * Get a read-only reference to the last element.
         * @result A read-only reference to the last element.
         */
        MCORE_INLINE const T& GetLast() const                                   { return m_data[m_length - 1]; }

        /**
         * Check if the array is empty or not.
         * @result Returns true when there are no elements in the array, otherwise false is returned.
         */
        MCORE_INLINE bool GetIsEmpty() const                                    { return (m_length == 0); }

        /**
         * Checks if the passed index is in the array's range.
         * @param index The index to check.
         * @return True if the passed index is valid, false if not.
         */
        MCORE_INLINE bool GetIsValidIndex(size_t index) const                   { return (index < m_length); }

        /**
         * Get the number of elements in the array.
         * @result The number of elements in the array.
         */
        MCORE_INLINE size_t GetLength() const                                   { return m_length; }

        /**
         * Get the maximum number of elements. This is the number of elements there currently is space for to store.
         * However, never use this to make for-loops to iterate through all elements. Use GetLength() instead for that.
         * This purely has to do with pre-allocating, to reduce the number of reallocs.
         * @result The maximum array length.
         */
        MCORE_INLINE size_t GetMaxLength() const                                { return m_maxLength; }

        /**
         * Calculates the memory usage used by this array.
         * @param includeMembers Include the class members in the calculation? (default=true).
         * @result The number of bytes allocated by this array.
         */
        MCORE_INLINE size_t CalcMemoryUsage(bool includeMembers = true) const
        {
            size_t result = m_maxLength * sizeof(T);
            if (includeMembers)
            {
                result += sizeof(AlignedArray<T, alignment>);
            }
            return result;
        }

        /**
         * Set a given element to a given value.
         * @param pos The element number.
         * @param value The value to store at that element number.
         */
        MCORE_INLINE void SetElem(size_t pos, const T& value)                   { m_data[pos] = value; }

        /**
         * Add a given element to the back of the array.
         * @param x The element to add.
         */
        MCORE_INLINE void Add(const T& x)                                       { Grow(++m_length); Construct(m_length - 1, x); }

        /**
         * Add a given element to the back of the array, but without pre-allocation caching.
         * @param x The element to add.
         */
        MCORE_INLINE void AddExact(const T& x)                                  { GrowExact(++m_length); Construct(m_length - 1, x); }

        /**
         * Add a given array to the back of this array.
         * @param a The array to add.
         */
        MCORE_INLINE void Add(const AlignedArray<T, alignment>& a)
        {
            size_t l = m_length;
            Grow(m_length + a.m_length);
            for (size_t i = 0; i < a.GetLength(); ++i)
            {
                Construct(l + i, a[i]);
            }
        }                                                                                                                                                                                   // TODO: a.GetLength() can be precaled before loop?

        /**
         * Add an empty (default constructed) element to the back of the array.
         */
        MCORE_INLINE void AddEmpty()                                            { Grow(++m_length); Construct(m_length - 1); }

        /**
         * Add an empty (default constructed) element to the back of the array, but without pre-allocation caching.
         */
        MCORE_INLINE void AddEmptyExact()                                       { GrowExact(++m_length); Construct(m_length - 1); }

        /**
         * Remove the first array element.
         */
        MCORE_INLINE void RemoveFirst()
        {
            if (m_length > 0)
            {
                Remove(0);
            }
        }

        /**
         * Remove the last array element.
         */
        MCORE_INLINE void RemoveLast()
        {
            if (m_length > 0)
            {
                Destruct(--m_length);
            }
        }

        /**
         * Insert an empty element (default constructed) at a given position in the array.
         * @param pos The position to create the empty element.
         */
        MCORE_INLINE void Insert(size_t pos)                                    { Grow(m_length + 1); MoveElements(pos + 1, pos, m_length - pos - 1); Construct(pos); }

        /**
         * Insert a given element at a given position in the array.
         * @param pos The position to insert the empty element.
         * @param x The element to store at this position.
         */
        MCORE_INLINE void Insert(size_t pos, const T& x)                        { Grow(m_length + 1); MoveElements(pos + 1, pos, m_length - pos - 1); Construct(pos, x); }

        /**
         * Remove an element at a given position.
         * @param pos The element number to remove.
         */
        MCORE_INLINE void Remove(size_t pos)
        {
            Destruct(pos);
            if (m_length > 1)
            {
                MoveElements(pos, pos + 1, m_length - pos - 1);
            }
            m_length--;
        }

        /**
         * Remove a given number of elements starting at a given position in the array.
         * @param pos The start element, so to start removing from.
         * @param num The number of elements to remove from this position.
         */
        MCORE_INLINE void Remove(size_t pos, size_t num)
        {
            for (size_t i = pos; i < pos + num; ++i)
            {
                Destruct(i);
            }
            MoveElements(pos, pos + num, m_length - pos - num);
            m_length -= num;
        }

        /**
         * Remove a given element with a given value.
         * Only the first element with the given value will be removed.
         * @param item The item/element to remove.
         */
        MCORE_INLINE bool RemoveByValue(const T& item)
        {
            size_t index = Find(item);
            if (index == InvalidIndex)
            {
                return false;
            }
            Remove(index);
            return true;
        }

        /**
         * Remove a given element in the array and place the last element in the array at the created empty position.
         * So if we have an array with the following characters : ABCDEFG<br>
         * And we perform a SwapRemove(2), we will remove element C and place the last element (G) at the empty created position where C was located.
         * So we will get this:<br>
         * AB.DEFG [where . is empty, after we did the SwapRemove(2)]<br>
         * ABGDEF [this is the result. G has been moved to the empty position].
         */
        MCORE_INLINE void SwapRemove(size_t pos)
        {
            Destruct(pos);
            if (pos != m_length - 1)
            {
                Construct(pos, m_data[m_length - 1]);
                Destruct(m_length - 1);
            }
            m_length--;
        }                                                                                                                                                                                      // remove element at <pos> and place the last element of the array in that position

        /**
         * Swap two elements.
         * @param pos1 The first element number.
         * @param pos2 The second element number.
         */
        MCORE_INLINE void Swap(size_t pos1, size_t pos2)
        {
            if (pos1 != pos2)
            {
                Swap(GetItem(pos1), GetItem(pos2));
            }
        }

        /**
         * Clear the array contents. So GetLength() will return 0 after performing this method.
         * @param clearMem If set to true (default) the allocated memory will also be released. If set to false, GetMaxLength() will still return the number of elements
         * which the array contained before calling the Clear() method.
         */
        MCORE_INLINE void Clear(bool clearMem = true)
        {
            for (size_t i = 0; i < m_length; ++i)
            {
                Destruct(i);
            }
            m_length = 0;
            if (clearMem)
            {
                this->Free();
            }
        }

        /**
         * Make sure the array has enough space to store a given number of elements.
         * @param newLength The number of elements we want to make sure that will fit in the array.
         */
        MCORE_INLINE void AssureSize(size_t newLength)
        {
            if (m_length >= newLength)
            {
                return;
            }
            size_t oldLen = m_length;
            Grow(newLength);
            for (size_t i = oldLen; i < newLength; ++i)
            {
                Construct(i);
            }
        }

        /**
         * Make sure this array has enough allocated storage to grow to a given number of elements elements without having to realloc.
         * @param minLength The minimum length the array should have (actually the minimum maxLength, because this has no influence on what GetLength() will return).
         */
        MCORE_INLINE void Reserve(size_t minLength)
        {
            if (m_maxLength < minLength)
            {
                Realloc(minLength);
            }
        }

        /**
         * Make the array as small as possible. So remove all extra pre-allocated data, so that the array consumes the least possible amount of memory.
         */
        MCORE_INLINE void Shrink()
        {
            if (m_length == m_maxLength)
            {
                return;
            }
            MCORE_ASSERT(m_maxLength >= m_length);
            Realloc(m_length);
        }

        /**
         * Check if the array contains a given element.
         * @param x The element to check.
         * @result Returns true when the array contains the element, otherwise false is returned.
         */
        MCORE_INLINE bool Contains(const T& x) const                            { return (Find(x) != InvalidIndex); }

        /**
         * Find the position of a given element.
         * @param x The element to find.
         * @result Returns the index in the array, ranging from [0 to GetLength()-1] when found, otherwise InvalidIndex is returned.
         */
        MCORE_INLINE size_t Find(const T& x) const
        {
            for (size_t i = 0; i < m_length; ++i)
            {
                if (m_data[i] == x)
                {
                    return i;
                }
            }
            return InvalidIndex;
        }

        /**
         * Copy the contents of another array into this one using a direct memory copy.
         * This does not call copy constructors of the objects, but just copies the raw memory data.
         * This resizes this array to be the exact length of the array we will copy the data from.
         * @param other The array to copy the data from.
         */
        MCORE_INLINE void MemCopyContentsFrom(const AlignedArray<T, alignment>& other)  { Resize(other.GetLength()); MemCopy((uint8*)m_data, (uint8*)other.m_data, sizeof(T) * other.m_length); }

        // sort function and standard sort function
        typedef int32   (MCORE_CDECL * CmpFunc)(const T& itemA, const T& itemB);
        static  int32   MCORE_CDECL StdCmp(const T& itemA, const T& itemB)
        {
            if (itemA < itemB)
            {
                return -1;
            }
            else if (itemA == itemB)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
        static  int32   MCORE_CDECL StdPtrObjCmp(const T& itemA, const T& itemB)
        {
            if (*itemA < *itemB)
            {
                return -1;
            }
            else if (*itemA == *itemB)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }

        /**
         * Sort the complete array using a given sort function.
         * @param cmp The sort function to use.
         */
        MCORE_INLINE void Sort(CmpFunc cmp)                                     { InnerSort(0, m_length - 1, cmp); }

        /**
         * Sort a given part of the array using a given sort function.
         * The default parameters are set so that it will sort the compelete array with a default compare function (which uses the < and > operators).
         * The method will sort all elements between the given 'first' and 'last' element (first and last are also included in the sort).
         * @param first The first element to start sorting.
         * @param last The last element to sort (when set to InvalidIndex, GetLength()-1 will be used).
         * @param cmp The compare function.
         */
        MCORE_INLINE void Sort(size_t first = 0, size_t last = InvalidIndex, CmpFunc cmp = StdCmp)
        {
            if (last == InvalidIndex)
            {
                last = m_length - 1;
            }
            InnerSort(first, last, cmp);
        }

        /**
         * Performs a sort on a given part of the array.
         * @param first The first element to start the sorting at.
         * @param last The last element to end the sorting.
         * @param cmp The compare function.
         */
        MCORE_INLINE void InnerSort(int32 first, int32 last, CmpFunc cmp)
        {
            if (first >= last)
            {
                return;
            }
            int32 split = Partition(first, last, cmp);
            InnerSort(first, split - 1, cmp);
            InnerSort(split + 1, last, cmp);
        }

        // resize in a fast way that doesn't call constructors or destructors
        void ResizeFast(size_t newLength)
        {
            if (m_length == newLength)
            {
                return;
            }

            if (newLength > m_length)
            {
                GrowExact(newLength);
            }

            m_length = newLength;
        }

        /**
         * Resize the array to a given size.
         * This does not mean an actual realloc will be made. This will only happen when the new length is bigger than the maxLength of the array.
         * @param newLength The new length the array should be.
         */
        void Resize(size_t newLength)
        {
            if (m_length == newLength)
            {
                return;
            }

            // check for growing or shrinking array
            if (newLength > m_length)
            {
                // growing array, construct empty elements at end of array
                const size_t oldLen = m_length;
                GrowExact(newLength);
                for (size_t i = oldLen; i < newLength; ++i)
                {
                    Construct(i);
                }
            }
            else
            {
                // shrinking array, destruct elements at end of array
                for (size_t i = newLength; i < m_length; ++i)
                {
                    Destruct(i);
                }

                m_length = newLength;
            }
        }

        /**
         * Move "numElements" elements starting from the source index, to the dest index.
         * Please note thate the array has to be large enough. You can't move data past the end of the array.
         * @param destIndex The destination index.
         * @param sourceIndex The source index, where the source elements start.
         * @param numElements The number of elements to move.
         */
        MCORE_INLINE void MoveElements(size_t destIndex, size_t sourceIndex, size_t numElements)
        {
            if (numElements > 0)
            {
                MemMove(m_data + destIndex, m_data + sourceIndex, numElements * sizeof(T));
            }
        }

        // operators
        bool                            operator==(const AlignedArray<T, alignment>& other) const
        {
            if (m_length != other.m_length)
            {
                return false;
            }
            for (size_t i = 0; i < m_length; ++i)
            {
                if (m_data[i] != other.m_data[i])
                {
                    return false;
                }
            }
            return true;
        }
        AlignedArray<T, alignment>&     operator= (const AlignedArray<T, alignment>& other)
        {
            if (&other != this)
            {
                Clear(false);
                m_memCategory = other.m_memCategory;
                Grow(other.m_length);
                for (size_t i = 0; i < m_length; ++i)
                {
                    Construct(i, other.m_data[i]);
                }
            }
            return *this;
        }
        AlignedArray<T, alignment>&     operator= (AlignedArray<T, alignment>&& other)
        {
            MCORE_ASSERT(&other != this);
            if (m_data)
            {
                AlignedFree(m_data);
            }
            m_data = other.m_data;
            m_memCategory = other.m_memCategory;
            m_length = other.m_length;
            m_maxLength = other.m_maxLength;
            other.m_data = nullptr;
            other.m_length = 0;
            other.m_maxLength = 0;
            return *this;
        }
        AlignedArray<T, alignment>&     operator+=(const T& other)                                      { Add(other); return *this; }
        AlignedArray<T, alignment>&     operator+=(const AlignedArray<T, alignment>& other)             { Add(other); return *this; }
        MCORE_INLINE T&         operator[](size_t index)                            { MCORE_ASSERT(index < m_length); return m_data[index]; }
        MCORE_INLINE const T&   operator[](size_t index) const                      { MCORE_ASSERT(index < m_length); return m_data[index]; }

    private:
        T*      m_data;          /**< The element data. */
        size_t  m_length;        /**< The number of used elements in the array. */
        size_t  m_maxLength;     /**< The number of elements that we have allocated memory for. */
        uint16  m_memCategory;   /**< The memory category ID. */

        // private functions
        MCORE_INLINE void   Grow(size_t newLength)
        {
            m_length = newLength;
            if (m_maxLength >= newLength)
            {
                return;
            }
            Realloc(AllocSize(newLength));
        }
        MCORE_INLINE void   GrowExact(size_t newLength)
        {
            m_length = newLength;
            if (m_maxLength < newLength)
            {
                Realloc(newLength);
            }
        }
        MCORE_INLINE size_t AllocSize(size_t num)                                   { return 1 + num /*+num/8*/; }
        MCORE_INLINE void   Alloc(size_t num)                                       { m_data = (T*)AlignedAllocate(num * sizeof(T), alignment, m_memCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE); }
        MCORE_INLINE void   Realloc(size_t newSize)
        {
            if (newSize == 0)
            {
                this->Free();
                return;
            }
            if (m_data)
            {
                m_data = (T*)AlignedRealloc(m_data, newSize * sizeof(T), m_maxLength * sizeof(T), alignment, m_memCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            }
            else
            {
                m_data = (T*)AlignedAllocate(newSize * sizeof(T), alignment, m_memCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            }

            m_maxLength = newSize;
        }
        void Free()
        {
            m_length = 0;
            m_maxLength = 0;
            if (m_data)
            {
                AlignedFree(m_data);
                m_data = nullptr;
            }
        }
        MCORE_INLINE void   Construct(size_t index, const T& original)              { ::new(m_data + index)T(original); }         // copy-construct an element at <index> which is a copy of <original>
        MCORE_INLINE void   Construct(size_t index)                                 { ::new(m_data + index)T; }                   // construct an element at place <index>
        MCORE_INLINE void   Destruct(size_t index)
        {
            #if (MCORE_COMPILER == MCORE_COMPILER_MSVC)
            MCORE_UNUSED(index);        // work around an MSVC compiler bug, where it triggers a warning that parameter 'index' is unused
            #endif
            (m_data + index)->~T();
        }

        // partition part of array (for sorting)
        int32 Partition(int32 left, int32 right, CmpFunc cmp)
        {
            ::MCore::Swap(m_data[left], m_data[ (left + right) >> 1 ]);

            T& target = m_data[right];
            int32 i = left - 1;
            int32 j = right;

            bool neverQuit = true;  // workaround to disable a "warning C4127: conditional expression is constant"
            while (neverQuit)
            {
                while (i < j)
                {
                    if (cmp(m_data[++i], target) >= 0)
                    {
                        break;
                    }
                }
                while (j > i)
                {
                    if (cmp(m_data[--j], target) <= 0)
                    {
                        break;
                    }
                }
                if (i >= j)
                {
                    break;
                }
                ::MCore::Swap(m_data[i], m_data[j]);
            }

            ::MCore::Swap(m_data[i], m_data[right]);
            return i;
        }
    };
}   // namespace MCore

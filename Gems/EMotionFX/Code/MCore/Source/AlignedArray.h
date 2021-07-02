/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
     * It can theoretically store 4294967296 items (maximum uint32 value).
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
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0)
            , mMemCategory(MCORE_MEMCATEGORY_ARRAY)              {}

        /**
         * Constructor which creates a given number of elements.
         * @param elems The element data.
         * @param num The number of elements in 'elems'.
         * @param memCategory The memory category the array is in.
         */
        MCORE_INLINE explicit AlignedArray(T* elems, uint32 num, uint16 memCategory = MCORE_MEMCATEGORY_ARRAY)
            : mLength(num)
            , mMaxLength(AllocSize(num))
            , mMemCategory(memCategory)
        {
            mData = (T*)AlignedAllocate(mMaxLength * sizeof(T), alignment, mMemCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            for (uint32 i = 0; i < mLength; ++i)
            {
                Construct(i, elems[i]);
            }
        }

        /**
         * Constructor which initializes the length of the array on a given number.
         * @param initSize The number of ellements to allocate space for.
         * @param memCategory The memory category the array is in.
         */
        MCORE_INLINE explicit AlignedArray(uint32 initSize, uint16 memCategory = MCORE_MEMCATEGORY_ARRAY)
            : mData(nullptr)
            , mLength(initSize)
            , mMaxLength(initSize)
            , mMemCategory(memCategory)
        {
            if (mMaxLength > 0)
            {
                mData = (T*)AlignedAllocate(mMaxLength * sizeof(T), alignment, mMemCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
                for (uint32 i = 0; i < mLength; ++i)
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
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0)
            , mMemCategory(MCORE_MEMCATEGORY_ARRAY)              { *this = other; }

        /**
         * Move constructor.
         * @param other The array to move the data from.
         */
        AlignedArray(AlignedArray<T, alignment>&& other)                                                                { mData = other.mData; mLength = other.mLength; mMaxLength = other.mMaxLength; mMemCategory = other.mMemCategory; other.mData = nullptr; other.mLength = 0; other.mMaxLength = 0; }

        /**
         * Destructor. Deletes all entry data.
         * However, if you store pointers to objects, these objects won't be deleted.<br>
         * Example:<br>
         * <pre>
         * AlignedArray< Object*, 16 > data;
         * for (uint32 i=0; i<10; i++)
         *    data.Add( new Object() );
         * </pre>
         * Now when the array 'data' will be destructed, it will NOT free up the memory of the integers which you allocated by hand, using new.
         * In order to free up this memory, you can do this:
         * <pre>
         * for (uint32 i=0; i<data.GetLength(); ++i)
         *      delete data[i];
         * data.Clear();
         * </pre>
         */
        ~AlignedArray()
        {
            for (uint32 i = 0; i < mLength; ++i)
            {
                Destruct(i);
            }
            if (mData)
            {
                AlignedFree(mData);
            }
        }

        /**
         * Get the memory category ID where allocations made by this array belong to.
         * On default the memory category is 0, which means unknown.
         * @result The memory category ID.
         */
        MCORE_INLINE uint16 GetMemoryCategory() const                           { return mMemCategory; }

        /**
         * Set the memory category ID, where allocations made by this array will belong to.
         * On default, after construction of the array, the category ID is 0, which means it is unknown.
         * @param categoryID The memory category ID where this arrays allocations belong to.
         */
        MCORE_INLINE void SetMemoryCategory(uint16 categoryID)                  { mMemCategory = categoryID; }

        /**
         * Get a pointer to the first element.
         * @result A pointer to the first element.
         */
        MCORE_INLINE T* GetPtr()                                                { return mData; }

        /**
         * Get a pointer to the first element.
         * @result A pointer to the first element.
         */
        MCORE_INLINE T* GetPtr() const                                          { return mData; }

        /**
         * Get a given item/element.
         * @param pos The item/element number.
         * @result A reference to the element.
         */
        MCORE_INLINE T& GetItem(uint32 pos)                                     { return mData[pos]; }

        /**
         * Get the first element.
         * @result A reference to the first element.
         */
        MCORE_INLINE T& GetFirst()                                              { return mData[0]; }

        /**
         * Get the last element.
         * @result A reference to the last element.
         */
        MCORE_INLINE T& GetLast()                                               { return mData[mLength - 1]; }

        /**
         * Get a read-only pointer to the first element.
         * @result A read-only pointer to the first element.
         */
        MCORE_INLINE const T* GetReadPtr() const                                { return mData; }

        /**
         * Get a read-only reference to a given element number.
         * @param pos The element number.
         * @result A read-only reference to the given element.
         */
        MCORE_INLINE const T& GetItem(uint32 pos) const                         { return mData[pos]; }

        /**
         * Get a read-only reference to the first element.
         * @result A read-only reference to the first element.
         */
        MCORE_INLINE const T& GetFirst() const                                  { return mData[0]; }

        /**
         * Get a read-only reference to the last element.
         * @result A read-only reference to the last element.
         */
        MCORE_INLINE const T& GetLast() const                                   { return mData[mLength - 1]; }

        /**
         * Check if the array is empty or not.
         * @result Returns true when there are no elements in the array, otherwise false is returned.
         */
        MCORE_INLINE bool GetIsEmpty() const                                    { return (mLength == 0); }

        /**
         * Checks if the passed index is in the array's range.
         * @param index The index to check.
         * @return True if the passed index is valid, false if not.
         */
        MCORE_INLINE bool GetIsValidIndex(uint32 index) const                   { return (index < mLength); }

        /**
         * Get the number of elements in the array.
         * @result The number of elements in the array.
         */
        MCORE_INLINE uint32 GetLength() const                                   { return mLength; }

        /**
         * Get the maximum number of elements. This is the number of elements there currently is space for to store.
         * However, never use this to make for-loops to iterate through all elements. Use GetLength() instead for that.
         * This purely has to do with pre-allocating, to reduce the number of reallocs.
         * @result The maximum array length.
         */
        MCORE_INLINE uint32 GetMaxLength() const                                { return mMaxLength; }

        /**
         * Calculates the memory usage used by this array.
         * @param includeMembers Include the class members in the calculation? (default=true).
         * @result The number of bytes allocated by this array.
         */
        MCORE_INLINE uint32 CalcMemoryUsage(bool includeMembers = true) const
        {
            uint32 result = mMaxLength * sizeof(T);
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
        MCORE_INLINE void SetElem(uint32 pos, const T& value)                   { mData[pos] = value; }

        /**
         * Add a given element to the back of the array.
         * @param x The element to add.
         */
        MCORE_INLINE void Add(const T& x)                                       { Grow(++mLength); Construct(mLength - 1, x); }

        /**
         * Add a given element to the back of the array, but without pre-allocation caching.
         * @param x The element to add.
         */
        MCORE_INLINE void AddExact(const T& x)                                  { GrowExact(++mLength); Construct(mLength - 1, x); }

        /**
         * Add a given array to the back of this array.
         * @param a The array to add.
         */
        MCORE_INLINE void Add(const AlignedArray<T, alignment>& a)
        {
            uint32 l = mLength;
            Grow(mLength + a.mLength);
            for (uint32 i = 0; i < a.GetLength(); ++i)
            {
                Construct(l + i, a[i]);
            }
        }                                                                                                                                                                                   // TODO: a.GetLength() can be precaled before loop?

        /**
         * Add an empty (default constructed) element to the back of the array.
         */
        MCORE_INLINE void AddEmpty()                                            { Grow(++mLength); Construct(mLength - 1); }

        /**
         * Add an empty (default constructed) element to the back of the array, but without pre-allocation caching.
         */
        MCORE_INLINE void AddEmptyExact()                                       { GrowExact(++mLength); Construct(mLength - 1); }

        /**
         * Remove the first array element.
         */
        MCORE_INLINE void RemoveFirst()
        {
            if (mLength > 0)
            {
                Remove((uint32)0);
            }
        }

        /**
         * Remove the last array element.
         */
        MCORE_INLINE void RemoveLast()
        {
            if (mLength > 0)
            {
                Destruct(--mLength);
            }
        }

        /**
         * Insert an empty element (default constructed) at a given position in the array.
         * @param pos The position to create the empty element.
         */
        MCORE_INLINE void Insert(uint32 pos)                                    { Grow(mLength + 1); MoveElements(pos + 1, pos, mLength - pos - 1); Construct(pos); }

        /**
         * Insert a given element at a given position in the array.
         * @param pos The position to insert the empty element.
         * @param x The element to store at this position.
         */
        MCORE_INLINE void Insert(uint32 pos, const T& x)                        { Grow(mLength + 1); MoveElements(pos + 1, pos, mLength - pos - 1); Construct(pos, x); }

        /**
         * Remove an element at a given position.
         * @param pos The element number to remove.
         */
        MCORE_INLINE void Remove(uint32 pos)
        {
            Destruct(pos);
            if (mLength > 1)
            {
                MoveElements(pos, pos + 1, mLength - pos - 1);
            }
            mLength--;
        }

        /**
         * Remove a given number of elements starting at a given position in the array.
         * @param pos The start element, so to start removing from.
         * @param num The number of elements to remove from this position.
         */
        MCORE_INLINE void Remove(uint32 pos, uint32 num)
        {
            for (uint32 i = pos; i < pos + num; ++i)
            {
                Destruct(i);
            }
            MoveElements(pos, pos + num, mLength - pos - num);
            mLength -= num;
        }

        /**
         * Remove a given element with a given value.
         * Only the first element with the given value will be removed.
         * @param item The item/element to remove.
         */
        MCORE_INLINE bool RemoveByValue(const T& item)
        {
            uint32 index = Find(item);
            if (index == MCORE_INVALIDINDEX32)
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
        MCORE_INLINE void SwapRemove(uint32 pos)
        {
            Destruct(pos);
            if (pos != mLength - 1)
            {
                Construct(pos, mData[mLength - 1]);
                Destruct(mLength - 1);
            }
            mLength--;
        }                                                                                                                                                                                      // remove element at <pos> and place the last element of the array in that position

        /**
         * Swap two elements.
         * @param pos1 The first element number.
         * @param pos2 The second element number.
         */
        MCORE_INLINE void Swap(uint32 pos1, uint32 pos2)
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
            for (uint32 i = 0; i < mLength; ++i)
            {
                Destruct(i);
            }
            mLength = 0;
            if (clearMem)
            {
                this->Free();
            }
        }

        /**
         * Make sure the array has enough space to store a given number of elements.
         * @param newLength The number of elements we want to make sure that will fit in the array.
         */
        MCORE_INLINE void AssureSize(uint32 newLength)
        {
            if (mLength >= newLength)
            {
                return;
            }
            uint32 oldLen = mLength;
            Grow(newLength);
            for (uint32 i = oldLen; i < newLength; ++i)
            {
                Construct(i);
            }
        }

        /**
         * Make sure this array has enough allocated storage to grow to a given number of elements elements without having to realloc.
         * @param minLength The minimum length the array should have (actually the minimum maxLength, because this has no influence on what GetLength() will return).
         */
        MCORE_INLINE void Reserve(uint32 minLength)
        {
            if (mMaxLength < minLength)
            {
                Realloc(minLength);
            }
        }

        /**
         * Make the array as small as possible. So remove all extra pre-allocated data, so that the array consumes the least possible amount of memory.
         */
        MCORE_INLINE void Shrink()
        {
            if (mLength == mMaxLength)
            {
                return;
            }
            MCORE_ASSERT(mMaxLength >= mLength);
            Realloc(mLength);
        }

        /**
         * Check if the array contains a given element.
         * @param x The element to check.
         * @result Returns true when the array contains the element, otherwise false is returned.
         */
        MCORE_INLINE bool Contains(const T& x) const                            { return (Find(x) != MCORE_INVALIDINDEX32); }

        /**
         * Find the position of a given element.
         * @param x The element to find.
         * @result Returns the index in the array, ranging from [0 to GetLength()-1] when found, otherwise MCORE_INVALIDINDEX32 is returned.
         */
        MCORE_INLINE uint32 Find(const T& x) const
        {
            for (uint32 i = 0; i < mLength; ++i)
            {
                if (mData[i] == x)
                {
                    return i;
                }
            }
            return MCORE_INVALIDINDEX32;
        }

        /**
         * Copy the contents of another array into this one using a direct memory copy.
         * This does not call copy constructors of the objects, but just copies the raw memory data.
         * This resizes this array to be the exact length of the array we will copy the data from.
         * @param other The array to copy the data from.
         */
        MCORE_INLINE void MemCopyContentsFrom(const AlignedArray<T, alignment>& other)  { Resize(other.GetLength()); MemCopy((uint8*)mData, (uint8*)other.mData, sizeof(T) * other.mLength); }

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
        MCORE_INLINE void Sort(CmpFunc cmp)                                     { InnerSort(0, mLength - 1, cmp); }

        /**
         * Sort a given part of the array using a given sort function.
         * The default parameters are set so that it will sort the compelete array with a default compare function (which uses the < and > operators).
         * The method will sort all elements between the given 'first' and 'last' element (first and last are also included in the sort).
         * @param first The first element to start sorting.
         * @param last The last element to sort (when set to MCORE_INVALIDINDEX32, GetLength()-1 will be used).
         * @param cmp The compare function.
         */
        MCORE_INLINE void Sort(uint32 first = 0, uint32 last = MCORE_INVALIDINDEX32, CmpFunc cmp = StdCmp)
        {
            if (last == MCORE_INVALIDINDEX32)
            {
                last = mLength - 1;
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
        void ResizeFast(uint32 newLength)
        {
            if (mLength == newLength)
            {
                return;
            }

            if (newLength > mLength)
            {
                GrowExact(newLength);
            }

            mLength = newLength;
        }

        /**
         * Resize the array to a given size.
         * This does not mean an actual realloc will be made. This will only happen when the new length is bigger than the maxLength of the array.
         * @param newLength The new length the array should be.
         */
        void Resize(uint32 newLength)
        {
            if (mLength == newLength)
            {
                return;
            }

            // check for growing or shrinking array
            if (newLength > mLength)
            {
                // growing array, construct empty elements at end of array
                const uint32 oldLen = mLength;
                GrowExact(newLength);
                for (uint32 i = oldLen; i < newLength; ++i)
                {
                    Construct(i);
                }
            }
            else
            {
                // shrinking array, destruct elements at end of array
                for (uint32 i = newLength; i < mLength; ++i)
                {
                    Destruct(i);
                }

                mLength = newLength;
            }
        }

        /**
         * Move "numElements" elements starting from the source index, to the dest index.
         * Please note thate the array has to be large enough. You can't move data past the end of the array.
         * @param destIndex The destination index.
         * @param sourceIndex The source index, where the source elements start.
         * @param numElements The number of elements to move.
         */
        MCORE_INLINE void MoveElements(uint32 destIndex, uint32 sourceIndex, uint32 numElements)
        {
            if (numElements > 0)
            {
                MemMove(mData + destIndex, mData + sourceIndex, numElements * sizeof(T));
            }
        }

        // operators
        bool                            operator==(const AlignedArray<T, alignment>& other) const
        {
            if (mLength != other.mLength)
            {
                return false;
            }
            for (uint32 i = 0; i < mLength; ++i)
            {
                if (mData[i] != other.mData[i])
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
                mMemCategory = other.mMemCategory;
                Grow(other.mLength);
                for (uint32 i = 0; i < mLength; ++i)
                {
                    Construct(i, other.mData[i]);
                }
            }
            return *this;
        }
        AlignedArray<T, alignment>&     operator= (AlignedArray<T, alignment>&& other)
        {
            MCORE_ASSERT(&other != this);
            if (mData)
            {
                AlignedFree(mData);
            }
            mData = other.mData;
            mMemCategory = other.mMemCategory;
            mLength = other.mLength;
            mMaxLength = other.mMaxLength;
            other.mData = nullptr;
            other.mLength = 0;
            other.mMaxLength = 0;
            return *this;
        }
        AlignedArray<T, alignment>&     operator+=(const T& other)                                      { Add(other); return *this; }
        AlignedArray<T, alignment>&     operator+=(const AlignedArray<T, alignment>& other)             { Add(other); return *this; }
        MCORE_INLINE T&         operator[](uint32 index)                            { MCORE_ASSERT(index < mLength); return mData[index]; }
        MCORE_INLINE const T&   operator[](uint32 index) const                      { MCORE_ASSERT(index < mLength); return mData[index]; }

    private:
        T*      mData;          /**< The element data. */
        uint32  mLength;        /**< The number of used elements in the array. */
        uint32  mMaxLength;     /**< The number of elements that we have allocated memory for. */
        uint16  mMemCategory;   /**< The memory category ID. */

        // private functions
        MCORE_INLINE void   Grow(uint32 newLength)
        {
            mLength = newLength;
            if (mMaxLength >= newLength)
            {
                return;
            }
            Realloc(AllocSize(newLength));
        }
        MCORE_INLINE void   GrowExact(uint32 newLength)
        {
            mLength = newLength;
            if (mMaxLength < newLength)
            {
                Realloc(newLength);
            }
        }
        MCORE_INLINE uint32 AllocSize(uint32 num)                                   { return 1 + num /*+num/8*/; }
        MCORE_INLINE void   Alloc(uint32 num)                                       { mData = (T*)AlignedAllocate(num * sizeof(T), alignment, mMemCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE); }
        MCORE_INLINE void   Realloc(uint32 newSize)
        {
            if (newSize == 0)
            {
                this->Free();
                return;
            }
            if (mData)
            {
                mData = (T*)AlignedRealloc(mData, newSize * sizeof(T), mMaxLength * sizeof(T), alignment, mMemCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            }
            else
            {
                mData = (T*)AlignedAllocate(newSize * sizeof(T), alignment, mMemCategory, MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            }

            mMaxLength = newSize;
        }
        void Free()
        {
            mLength = 0;
            mMaxLength = 0;
            if (mData)
            {
                AlignedFree(mData);
                mData = nullptr;
            }
        }
        MCORE_INLINE void   Construct(uint32 index, const T& original)              { ::new(mData + index)T(original); }         // copy-construct an element at <index> which is a copy of <original>
        MCORE_INLINE void   Construct(uint32 index)                                 { ::new(mData + index)T; }                   // construct an element at place <index>
        MCORE_INLINE void   Destruct(uint32 index)
        {
            #if (MCORE_COMPILER == MCORE_COMPILER_MSVC)
            MCORE_UNUSED(index);        // work around an MSVC compiler bug, where it triggers a warning that parameter 'index' is unused
            #endif
            (mData + index)->~T();
        }

        // partition part of array (for sorting)
        int32 Partition(int32 left, int32 right, CmpFunc cmp)
        {
            ::MCore::Swap(mData[left], mData[ (left + right) >> 1 ]);

            T& target = mData[right];
            int32 i = left - 1;
            int32 j = right;

            bool neverQuit = true;  // workaround to disable a "warning C4127: conditional expression is constant"
            while (neverQuit)
            {
                while (i < j)
                {
                    if (cmp(mData[++i], target) >= 0)
                    {
                        break;
                    }
                }
                while (j > i)
                {
                    if (cmp(mData[--j], target) <= 0)
                    {
                        break;
                    }
                }
                if (i >= j)
                {
                    break;
                }
                ::MCore::Swap(mData[i], mData[j]);
            }

            ::MCore::Swap(mData[i], mData[right]);
            return i;
        }
    };
}   // namespace MCore

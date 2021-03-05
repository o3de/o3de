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

#pragma once

#include "StandardHeaders.h"
#include "Array.h"
#include "LogManager.h" // used for LogContents

namespace MCore
{
    /**
     * A dynamic 2D array template.
     * This would be a better solution than "Array< Array< T > >", because the Array inside Array will perform many allocations,
     * while this specialized 2D array will only perform two similar allocations.
     * What it does is keep one big array of data elements, and maintain a table that indices inside this big array.
     * We advise you to call the Shrink function after you performed a number of operations on the array, to maximize its memory usage efficiency.
     *
     * The layout of the array is as following:
     *
     * <pre>
     *
     * [ROW0]: [E0][E1][E2]
     * [ROW1]: [E0][E1]
     * [ROW2]: [E0][E1][E2][E3]
     * [ROW3]: [E0]
     *
     * </pre>
     *
     * Where E0, E1, E2, etc are elements of the specified type T.
     * Each row can have a different amount of elements that can be added or removed dynamically. Also rows can be deleted
     * or added when desired.
     */
    template <class T>
    class Array2D
    {
    public:
        /**
         * An index table entry.
         * Each row in the 2D array will get a table entry, which tells us where in the data array
         * the element data starts for the given row, and how many elements will follow for the given row.
         */
        struct TableEntry
        {
            uint32  mStartIndex;        /**< The index offset where the data for this row starts. */
            uint32  mNumElements;       /**< The number of elements to follow. */
        };

        /**
         * The default constructor.
         * The number of pre-cached/allocated elements per row is set to a value of 2 on default.
         * You can use the SetNumPreCachedElements(...) method to adjust this value. Make sure you adjust this value
         * before you call the Resize method though, otherwise it will have no immediate effect.
         */
        MCORE_INLINE Array2D()
            : mNumPreCachedElements(2) { }

        /**
         * Extended constructor which will automatically initialize the array dimensions.
         * Basically this will initialize the array dimensions at (numRows x numPreAllocatedElemsPerRow) elements.
         * Please note though, that this will NOT add actual elements. So you can't get values from the elements yet.
         * This would just pre-allocate data. You have to use the Add method to actually fill the items.
         * @param numRows The number of rows the array should have.
         * @param numPreAllocatedElemsPerRow The number of pre-cached/allocated elements per row.
         *
         */
        MCORE_INLINE Array2D(uint32 numRows, uint32 numPreAllocatedElemsPerRow = 2)
            : mNumPreCachedElements(numPreAllocatedElemsPerRow)   { Resize(numRows); }

        /**
         * The copy constructor.
         * This copies over all data from another specified 2D array.
         * @param other The array to copy the data from.
         */
        MCORE_INLINE Array2D(const Array2D<T>& other)     { *this = other;  }

        /**
         * The destructor.
         * If you have arrays of pointers to allocated objects, you have to delete those objects by hand.
         */
        MCORE_INLINE ~Array2D()     { Clear(); }

        /**
         * Set the memory category ID, where allocations made by this array will belong to.
         * On default, after construction of the array, the category ID is 0, which means it is unknown.
         * @param categoryID The memory category ID where this arrays allocations belong to.
         */
        MCORE_INLINE void SetMemoryCategory(uint16 categoryID)                      { mData.SetMemoryCategory(categoryID); mIndexTable.SetMemoryCategory(categoryID); }

        /**
         * Resize the array in one dimension (the number of rows).
         * Rows that will be added willl automatically get [n] number of elements pre-allocated.
         * The number of [n] can be set with the SetNumPreCachedElements(...) method.
         * Please note that the pre-allocated/cached elements are not valid to be used yet. You have to use the Add method first.
         * @param numRows The number of rows you wish to init for.
         * @param autoShrink When set to true, after execution of this method the Shrink method will automatically be called in order
         *                   to optimize the memory usage. This only happens when resizing to a lower amount of rows, so when making the array smaller.
         */
        void Resize(uint32 numRows, bool autoShrink = false);

        /**
         * Add an element to the list of elements in a given row.
         * @param rowIndex The row number to add the element to.
         * @param element The value of the element to add.
         */
        void Add(uint32 rowIndex, const T& element);

        /**
         * Remove an element from the array.
         * @param rowIndex The row number where the element is stored.
         * @param elementIndex The element number inside this row to remove.
         */
        void Remove(uint32 rowIndex, uint32 elementIndex);

        /**
         * Remove a given row, including all its elements.
         * This will decrease the number of rows.
         * @param rowIndex The row number to remove.
         * @param autoShrink When set to true, the array's memory usage will be optimized and minimized as much as possible.
         */
        void RemoveRow(uint32 rowIndex, bool autoShrink = false);

        /**
         * Remove a given range of rows and all their elements.
         * All rows from the specified start row until the end row will be removed, with the start and end rows included.
         * @param startRow The start row number to start removing from (so this one will also be removed).
         * @param endRow The end row number (which will also be removed).
         * @param autoShrink When set to true, the array's memory usage will be optimized and minimized as much as possible.
         */
        void RemoveRows(uint32 startRow, uint32 endRow, bool autoShrink = false);

        /**
         * Optimize (minimize) the memory usage of the array.
         * This will move all elements around, removing all gaps and unused pre-cached/allocated items.
         * It is advised to call this method after you applied some heavy modifications to the array, such as
         * removing rows or many elements. When your array data is fairly static, and you won't be adding or removing
         * data from it very frequently, you should definitely call this method after you have filled the array with data.
         */
        void Shrink();

        /**
         * Set the number of elements per row that should be pre-allocated/cached when creating / adding new rows.
         * This doesn't actually increase the number of elements for a given row, but just reserves memory for the elements, which can
         * speedup adding of new elements and prevent memory reallocs. The default value is set to 2 when creating an array, unless specified differently.
         * @param numElemsPerRow The number of elements per row that should be pre-allocated.
         */
        MCORE_INLINE void SetNumPreCachedElements(uint32 numElemsPerRow)            { mNumPreCachedElements = numElemsPerRow; }

        /**
         * Get the number of pre-cached/allocated elements per row, when creating new rows.
         * See the SetNumPreCachedElements for more information.
         * @result The number of elements per row that will be pre-allocated/cached when adding a new row.
         * @see SetNumPreCachedElements.
         */
        MCORE_INLINE uint32 GetNumPreCachedElements() const                         { return mNumPreCachedElements; }

        /**
         * Get the number of stored elements inside a given row.
         * @param rowIndex The row number.
         * @result The number of elements stored inside this row.
         */
        MCORE_INLINE uint32 GetNumElements(uint32 rowIndex) const                   { return mIndexTable[rowIndex].mNumElements; }

        /**
         * Get a pointer to the element data stored in a given row.
         * Use this method with care as it can easily overwrite data from other elements.
         * All element data for a given row is stored sequential, so right after eachother in one continuous piece of memory.
         * The next row's element data however might not be connected to the memory of row before that!
         * Also only use this method when the GetNumElements(...) method for this row returns a value greater than zero.
         * @param rowIndex the row number.
         * @result A pointer to the element data for the given row.
         */
        MCORE_INLINE T* GetElements(uint32 rowIndex)                                { return &mData[ mIndexTable[rowIndex].mStartIndex ]; }

        /**
         * Get the data of a given element.
         * @param rowIndex The row number where the element is stored.
         * @param elementNr The element number inside this row to retrieve.
         * @result A reference to the element data.
         */
        MCORE_INLINE T& GetElement(uint32 rowIndex, uint32 elementNr)               { return mData[ mIndexTable[rowIndex].mStartIndex + elementNr ]; }

        /**
         * Get the data of a given element.
         * @param rowIndex The row number where the element is stored.
         * @param elementNr The element number inside this row to retrieve.
         * @result A const reference to the element data.
         */
        MCORE_INLINE const T& GetElement(uint32 rowIndex, uint32 elementNr) const   { return mData[ mIndexTable[rowIndex].mStartIndex + elementNr ]; }

        /**
         * Set the value for a given element in the array.
         * @param rowIndex The row where the element is stored in.
         * @param elementNr The element number to set the value for.
         * @param value The value to set the element to.
         */
        MCORE_INLINE void SetElement(uint32 rowIndex, uint32 elementNr, const T& value)     { MCORE_ASSERT(rowIndex < mIndexTable.GetLength()); MCORE_ASSERT(elementNr < mIndexTable[rowIndex].mNumElements); mData[ mIndexTable[rowIndex].mStartIndex + elementNr ] = value; }

        /**
         * Get the number of rows in the 2D array.
         * @result The number of rows.
         */
        MCORE_INLINE uint32 GetNumRows() const                                      { return mIndexTable.GetLength(); }

        /**
         * Calculate the percentage of memory that is filled with element data.
         * When this is 100%, then all allocated element data is filled and used.
         * When it would be 25% then only 25% of all allocated element data is used. This is an indication that
         * you should most likely use the Shrink method, which will ensure that the memory usage will become 100% again, which
         * would be most optimal.
         * @result The percentage (in range of 0..100) of used element memory.
         */
        MCORE_INLINE float CalcUsedElementMemoryPercentage() const                  { return (mData.GetLength() ? (CalcTotalNumElements() / (float)mData.GetLength()) * 100.0f : 0); }

        /**
         * Swap the element data of two rows.
         * Beware, this is pretty slow!
         * @param rowA The first row.
         * @param rowB The second row.
         */
        void Swap(uint32 rowA, uint32 rowB);

        /**
         * Calculate the total number of used elements.
         * A used element is an element that has been added and that has a valid value stored.
         * This excludes pre-allocated/cached elements.
         * @result The total number of elements stored in the array.
         */
        uint32 CalcTotalNumElements() const;

        /**
         * Clear all contents.
         * This deletes all rows and clears all their their elements as well.
         * Please keep in mind though, that when you have an array of pointers to objects you allocated, that
         * you still have to delete those objects by hand! The Clear function will not delete those.
         * @param freeMem When set to true, all memory used by the array internally will be deleted. If set to false, the memory
         *                will not be deleted and can be reused later on again without doing any memory realloc when possible.
         */
        MCORE_INLINE void Clear(bool freeMem = true)                                  { mIndexTable.Clear(freeMem); mData.Clear(freeMem); }

        /**
         * The assignment operator.
         * This copies over all data from another 2D array.
         * All current contents will be cleared before copying over the data from the other array.
         * @param other The other array to copy the data from.
         */
        Array2D<T>& operator = (const Array2D<T>& other);

        /**
         * Log all array contents.
         * This will log the number of rows, number of elements, used element memory percentage, as well
         * as some details about each row.
         */
        void LogContents();

        /**
         * Get the index table.
         * This table describes for each row the start index and number of elements for the row.
         * The length of the array equals the value returned by GetNumRows().
         * @result The array of index table entries, which specify the start indices and number of entries per row.
         */
        MCORE_INLINE Array<TableEntry>& GetIndexTable()                             { return mIndexTable; }

        /**
         * Get the data array.
         * This contains the data array in which the index table points.
         * Normally you shouldn't be using this method. However it is useful in some specific cases.
         * @result The data array that contains all elements.
         */
        MCORE_INLINE Array<T>& GetData()                                            { return mData; }

    private:
        Array<T>            mData;                  /**< The element data. */
        Array<TableEntry>   mIndexTable;            /**< The index table that let's us know where what data is inside the element data array. */
        uint32              mNumPreCachedElements;  /**< The number of elements per row to pre-allocate when resizing this array. This prevents some re-allocs. */
    };


    // include inline code
#include "Array2D.inl"
}   // namespace MCore

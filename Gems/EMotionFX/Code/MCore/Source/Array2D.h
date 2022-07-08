/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

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
            size_t  m_startIndex;        /**< The index offset where the data for this row starts. */
            size_t  m_numElements;       /**< The number of elements to follow. */
        };

        /**
         * The default constructor.
         * The number of pre-cached/allocated elements per row is set to a value of 2 on default.
         * You can use the SetNumPreCachedElements(...) method to adjust this value. Make sure you adjust this value
         * before you call the Resize method though, otherwise it will have no immediate effect.
         */
        Array2D() = default;

        /**
         * Extended constructor which will automatically initialize the array dimensions.
         * Basically this will initialize the array dimensions at (numRows x numPreAllocatedElemsPerRow) elements.
         * Please note though, that this will NOT add actual elements. So you can't get values from the elements yet.
         * This would just pre-allocate data. You have to use the Add method to actually fill the items.
         * @param numRows The number of rows the array should have.
         * @param numPreAllocatedElemsPerRow The number of pre-cached/allocated elements per row.
         *
         */
        Array2D(size_t numRows, size_t numPreAllocatedElemsPerRow = 2)
            : m_numPreCachedElements(numPreAllocatedElemsPerRow)   { Resize(numRows); }

        /**
         * Resize the array in one dimension (the number of rows).
         * Rows that will be added willl automatically get [n] number of elements pre-allocated.
         * The number of [n] can be set with the SetNumPreCachedElements(...) method.
         * Please note that the pre-allocated/cached elements are not valid to be used yet. You have to use the Add method first.
         * @param numRows The number of rows you wish to init for.
         * @param autoShrink When set to true, after execution of this method the Shrink method will automatically be called in order
         *                   to optimize the memory usage. This only happens when resizing to a lower amount of rows, so when making the array smaller.
         */
        void Resize(size_t numRows, bool autoShrink = false);

        /**
         * Add an element to the list of elements in a given row.
         * @param rowIndex The row number to add the element to.
         * @param element The value of the element to add.
         */
        void Add(size_t rowIndex, const T& element);

        /**
         * Remove an element from the array.
         * @param rowIndex The row number where the element is stored.
         * @param elementIndex The element number inside this row to remove.
         */
        void Remove(size_t rowIndex, size_t elementIndex);

        /**
         * Remove a given row, including all its elements.
         * This will decrease the number of rows.
         * @param rowIndex The row number to remove.
         * @param autoShrink When set to true, the array's memory usage will be optimized and minimized as much as possible.
         */
        void RemoveRow(size_t rowIndex, bool autoShrink = false);

        /**
         * Remove a given range of rows and all their elements.
         * All rows from the specified start row until the end row will be removed, with the start and end rows included.
         * @param startRow The start row number to start removing from (so this one will also be removed).
         * @param endRow The end row number (which will also be removed).
         * @param autoShrink When set to true, the array's memory usage will be optimized and minimized as much as possible.
         */
        void RemoveRows(size_t startRow, size_t endRow, bool autoShrink = false);

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
        void SetNumPreCachedElements(size_t numElemsPerRow)            { m_numPreCachedElements = numElemsPerRow; }

        /**
         * Get the number of pre-cached/allocated elements per row, when creating new rows.
         * See the SetNumPreCachedElements for more information.
         * @result The number of elements per row that will be pre-allocated/cached when adding a new row.
         * @see SetNumPreCachedElements.
         */
        size_t GetNumPreCachedElements() const                         { return m_numPreCachedElements; }

        /**
         * Get the number of stored elements inside a given row.
         * @param rowIndex The row number.
         * @result The number of elements stored inside this row.
         */
        size_t GetNumElements(size_t rowIndex) const                   { return m_indexTable[rowIndex].m_numElements; }

        /**
         * Get a pointer to the element data stored in a given row.
         * Use this method with care as it can easily overwrite data from other elements.
         * All element data for a given row is stored sequential, so right after eachother in one continuous piece of memory.
         * The next row's element data however might not be connected to the memory of row before that!
         * Also only use this method when the GetNumElements(...) method for this row returns a value greater than zero.
         * @param rowIndex the row number.
         * @result A pointer to the element data for the given row.
         */
        T* GetElements(size_t rowIndex)                                { return &m_data[ m_indexTable[rowIndex].m_startIndex ]; }

        /**
         * Get the data of a given element.
         * @param rowIndex The row number where the element is stored.
         * @param elementNr The element number inside this row to retrieve.
         * @result A reference to the element data.
         */
        T& GetElement(size_t rowIndex, size_t elementNr)               { return m_data[ m_indexTable[rowIndex].m_startIndex + elementNr ]; }

        /**
         * Get the data of a given element.
         * @param rowIndex The row number where the element is stored.
         * @param elementNr The element number inside this row to retrieve.
         * @result A const reference to the element data.
         */
        const T& GetElement(size_t rowIndex, size_t elementNr) const   { return m_data[ m_indexTable[rowIndex].m_startIndex + elementNr ]; }

        /**
         * Set the value for a given element in the array.
         * @param rowIndex The row where the element is stored in.
         * @param elementNr The element number to set the value for.
         * @param value The value to set the element to.
         */
        void SetElement(size_t rowIndex, size_t elementNr, const T& value)     { MCORE_ASSERT(rowIndex < m_indexTable.GetLength()); MCORE_ASSERT(elementNr < m_indexTable[rowIndex].m_numElements); m_data[ m_indexTable[rowIndex].m_startIndex + elementNr ] = value; }

        /**
         * Get the number of rows in the 2D array.
         * @result The number of rows.
         */
        size_t GetNumRows() const                                      { return m_indexTable.size(); }

        /**
         * Calculate the percentage of memory that is filled with element data.
         * When this is 100%, then all allocated element data is filled and used.
         * When it would be 25% then only 25% of all allocated element data is used. This is an indication that
         * you should most likely use the Shrink method, which will ensure that the memory usage will become 100% again, which
         * would be most optimal.
         * @result The percentage (in range of 0..100) of used element memory.
         */
        float CalcUsedElementMemoryPercentage() const                  { return (m_data.GetLength() ? (CalcTotalNumElements() / (float)m_data.GetLength()) * 100.0f : 0); }

        /**
         * Swap the element data of two rows.
         * Beware, this is pretty slow!
         * @param rowA The first row.
         * @param rowB The second row.
         */
        void Swap(size_t rowA, size_t rowB);

        /**
         * Calculate the total number of used elements.
         * A used element is an element that has been added and that has a valid value stored.
         * This excludes pre-allocated/cached elements.
         * @result The total number of elements stored in the array.
         */
        size_t CalcTotalNumElements() const;

        /**
         * Clear all contents.
         * This deletes all rows and clears all their their elements as well.
         * Please keep in mind though, that when you have an array of pointers to objects you allocated, that
         * you still have to delete those objects by hand! The Clear function will not delete those.
         * @param freeMem When set to true, all memory used by the array internally will be deleted. If set to false, the memory
         *                will not be deleted and can be reused later on again without doing any memory realloc when possible.
         */
        void Clear(bool freeMem = true)
        {
            m_indexTable.clear();
            m_data.clear();
            if (freeMem)
            {
                m_indexTable.shrink_to_fit();
                m_data.shrink_to_fit();
            }
        }

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
        AZStd::vector<TableEntry>& GetIndexTable()                             { return m_indexTable; }

        /**
         * Get the data array.
         * This contains the data array in which the index table points.
         * Normally you shouldn't be using this method. However it is useful in some specific cases.
         * @result The data array that contains all elements.
         */
        AZStd::vector<T>& GetData()                                            { return m_data; }

    private:
        AZStd::vector<T>            m_data;                  /**< The element data. */
        AZStd::vector<TableEntry>   m_indexTable;            /**< The index table that let's us know where what data is inside the element data array. */
        size_t              m_numPreCachedElements = 2;  /**< The number of elements per row to pre-allocate when resizing this array. This prevents some re-allocs. */
    };


    // include inline code
#include "Array2D.inl"
}   // namespace MCore

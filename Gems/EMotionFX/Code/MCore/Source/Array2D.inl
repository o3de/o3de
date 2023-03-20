/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// resize the array's number of rows
template <class T>
void Array2D<T>::Resize(size_t numRows, bool autoShrink)
{
    // get the current (old) number of rows
    const size_t oldNumRows = m_indexTable.size();

    // don't do anything when we don't need to
    if (numRows == oldNumRows)
    {
        return;
    }

    // resize the index table
    m_indexTable.resize(numRows);

    // check if we decreased the number of rows or not
    if (numRows < oldNumRows)
    {
        // pack memory as tight as possible
        if (autoShrink)
        {
            Shrink();
        }
    }
    else    // we added new entries
    {
        // init the new table entries
        for (size_t i = oldNumRows; i < numRows; ++i)
        {
            m_indexTable[i].m_startIndex  = m_data.size() + (i * m_numPreCachedElements);
            m_indexTable[i].m_numElements = 0;
        }

        // grow the data array
        const size_t numNewRows = numRows - oldNumRows;
        m_data.resize(m_data.size() + numNewRows * m_numPreCachedElements);
    }
}


// add an element
template <class T>
void Array2D<T>::Add(size_t rowIndex, const T& element)
{
    AZ_Assert(rowIndex < m_indexTable.size(), "Array index out of bounds");

    // find the insert location inside the data array
    size_t insertPos = m_indexTable[rowIndex].m_startIndex + m_indexTable[rowIndex].m_numElements;
    if (insertPos >= m_data.size())
    {
        m_data.resize(insertPos + 1);
    }

    // check if we need to insert for real
    bool needRealInsert = true;
    if (rowIndex < m_indexTable.size() - 1)                 // if there are still entries coming after the one we have to add to
    {
        if (insertPos < m_indexTable[rowIndex + 1].m_startIndex)    // if basically there are empty unused element we can use
        {
            needRealInsert = false;                             // then we don't need to do any reallocs
        }
    }
    else
    {
        // if we're dealing with the last row
        if (rowIndex == m_indexTable.size() - 1)
        {
            if (insertPos < m_data.size())  // if basically there are empty unused element we can use
            {
                needRealInsert = false;
            }
        }
    }

    // perform the insertion
    if (needRealInsert)
    {
        // insert the element inside the data array
        m_data.insert(AZStd::next(begin(m_data), insertPos), element);

        // adjust the index table entries
        const size_t numRows = m_indexTable.size();
        for (size_t i = rowIndex + 1; i < numRows; ++i)
        {
            m_indexTable[i].m_startIndex++;
        }
    }
    else
    {
        m_data[insertPos] = element;
    }

    // increase the number of elements in the index table
    m_indexTable[rowIndex].m_numElements++;
}


// remove a given element
template <class T>
void Array2D<T>::Remove(size_t rowIndex, size_t elementIndex)
{
    AZ_Assert(rowIndex < m_indexTable.size(), "Array2D<>::Remove: array index out of bounds");
    AZ_Assert(elementIndex < m_indexTable[rowIndex].m_numElements, "Array2D<>::Remove: element index out of bounds");
    AZ_Assert(m_indexTable[rowIndex].m_numElements > 0, "Array2D<>::Remove: array index out of bounds");

    const size_t startIndex         = m_indexTable[rowIndex].m_startIndex;
    const size_t maxElementIndex    = m_indexTable[rowIndex].m_numElements - 1;

    // swap the last element with the one to be removed
    if (elementIndex != maxElementIndex)
    {
        m_data[startIndex + elementIndex] = m_data[startIndex + maxElementIndex];
    }

    // decrease the number of elements
    m_indexTable[rowIndex].m_numElements--;
}


// remove a given row
template <class T>
void Array2D<T>::RemoveRow(size_t rowIndex, bool autoShrink)
{
    AZ_Assert(rowIndex < m_indexTable.GetLength(), "Array2D<>::RemoveRow: rowIndex out of bounds");
    m_indexTable.Remove(rowIndex);

    // optimize memory usage when desired
    if (autoShrink)
    {
        Shrink();
    }
}


// remove a set of rows
template <class T>
void Array2D<T>::RemoveRows(size_t startRow, size_t endRow, bool autoShrink)
{
    AZ_Assert(startRow < m_indexTable.size(), "Array2D<>::RemoveRows: startRow out of bounds");
    AZ_Assert(endRow < m_indexTable.size(), "Array2D<>::RemoveRows: endRow out of bounds");

    // check if the start row is smaller than the end row
    if (startRow < endRow)
    {
        const size_t numToRemove = (endRow - startRow) + 1;
        m_indexTable.erase(AZStd::next(begin(m_indexTable), startRow), AZStd::next(AZStd::next(begin(m_indexTable), startRow), numToRemove));
    }
    else    // if the end row is smaller than the start row
    {
        const size_t numToRemove = (startRow - endRow) + 1;
        m_indexTable.erase(AZStd::next(begin(m_indexTable), endRow), AZStd::next(AZStd::next(begin(m_indexTable), endRow), numToRemove));
    }

    // optimize memory usage when desired
    if (autoShrink)
    {
        Shrink();
    }
}


// optimize memory usage
template <class T>
void Array2D<T>::Shrink()
{
    // for all attributes, except for the last one
    const size_t numRows = m_indexTable.size();
    if (numRows == 0)
    {
        return;
    }

    // remove all unused items between the rows (unused element data per row)
    const size_t numRowsMinusOne = numRows - 1;
    for (size_t a = 0; a < numRowsMinusOne; ++a)
    {
        const size_t firstUnusedIndex  = m_indexTable[a  ].m_startIndex + m_indexTable[a].m_numElements;
        const size_t numUnusedElements = m_indexTable[a + 1].m_startIndex - firstUnusedIndex;

        // if we have pre-cached/unused elements, remove those by moving memory to remove the "holes"
        if (numUnusedElements > 0)
        {
            // remove the unused elements from the array
            m_data.erase(AZStd::next(begin(m_data), firstUnusedIndex), AZStd::next(AZStd::next(begin(m_data), firstUnusedIndex), numUnusedElements));

            // change the start indices for all the rows coming after the current one
            const size_t numTotalRows = m_indexTable.size();
            for (size_t i = a + 1; i < numTotalRows; ++i)
            {
                m_indexTable[i].m_startIndex -= numUnusedElements;
            }
        }
    }

    // now move all start index values and all data to the front of the data array as much as possible
    // like data on row 0 starting at data element 7, would be moved to data element 0
    size_t dataPos = 0;
    for (size_t row = 0; row < numRows; ++row)
    {
        // if the data starts after the place where it could start, move it to the place where it could start
        if (m_indexTable[row].m_startIndex > dataPos)
        {
            AZStd::move(AZStd::next(begin(m_data), this->m_indexTable[row].m_startIndex), AZStd::next(AZStd::next(begin(m_data), this->m_indexTable[row].m_startIndex), this->m_indexTable[row].m_numElements), AZStd::next(begin(m_data), dataPos));
            m_indexTable[row].m_startIndex = dataPos;
        }

        // increase the data pos
        dataPos += m_indexTable[row].m_numElements;
    }

    // remove all unused data items
    if (dataPos < m_data.size())
    {
        m_data.erase(AZStd::next(begin(m_data), dataPos), end(m_data));
    }

    // shrink the arrays
    m_data.shrink_to_fit();
    m_indexTable.shrink_to_fit();
}


// calculate the number of used elements
template <class T>
size_t Array2D<T>::CalcTotalNumElements() const
{
    size_t totalElements = 0;

    // add all number of row elements together
    const size_t numRows = m_indexTable.size();
    for (size_t i = 0; i < numRows; ++i)
    {
        totalElements += m_indexTable[i].m_numElements;
    }

    return totalElements;
}


// swap the contents of two rows
template <class T>
void Array2D<T>::Swap(size_t rowA, size_t rowB)
{
    // get the original number of elements from both rows
    const size_t numElementsA = m_indexTable[rowA].m_numElements;
    const size_t numElementsB = m_indexTable[rowB].m_numElements;

    // move the element data of rowA into a temp buffer
    AZStd::vector<T> tempData(numElementsA);
    AZStd::move(
        AZStd::next(m_data.begin(), m_indexTable[rowA].m_startIndex),
        AZStd::next(m_data.begin(), m_indexTable[rowA].m_startIndex + numElementsA),
        tempData.begin()
    );

    // remove the elements from rowA
    while (GetNumElements(rowA))
    {
        Remove(rowA, 0);
    }

    // add all elements of row B
    size_t i;
    for (i = 0; i < numElementsB; ++i)
    {
        Add(rowA, GetElement(rowB, i));
    }

    // remove all elements from B
    while (GetNumElements(rowB))
    {
        Remove(rowB, 0);
    }

    // add all elements from the original A
    for (i = 0; i < numElementsA; ++i)
    {
        Add(rowB, tempData[i]);
    }
}

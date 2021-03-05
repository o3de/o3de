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

// resize the array's number of rows
template <class T>
void Array2D<T>::Resize(uint32 numRows, bool autoShrink)
{
    // get the current (old) number of rows
    const uint32 oldNumRows = mIndexTable.GetLength();

    // don't do anything when we don't need to
    if (numRows == oldNumRows)
    {
        return;
    }

    // resize the index table
    mIndexTable.Resize(numRows);

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
        for (uint32 i = oldNumRows; i < numRows; ++i)
        {
            mIndexTable[i].mStartIndex  = mData.GetLength() + (i * mNumPreCachedElements);
            mIndexTable[i].mNumElements = 0;
        }

        // grow the data array
        const uint32 numNewRows = numRows - oldNumRows;
        mData.Resize(mData.GetLength() + numNewRows * mNumPreCachedElements);
    }
}


// add an element
template <class T>
void Array2D<T>::Add(uint32 rowIndex, const T& element)
{
    MCORE_ASSERT(rowIndex < mIndexTable.GetLength());

    // find the insert location inside the data array
    uint32 insertPos = mIndexTable[rowIndex].mStartIndex + mIndexTable[rowIndex].mNumElements;
    if (insertPos >= mData.GetLength())
    {
        mData.Resize(insertPos + 1);
    }

    // check if we need to insert for real
    bool needRealInsert = true;
    if (rowIndex < mIndexTable.GetLength() - 1)                 // if there are still entries coming after the one we have to add to
    {
        if (insertPos < mIndexTable[rowIndex + 1].mStartIndex)    // if basically there are empty unused element we can use
        {
            needRealInsert = false;                             // then we don't need to do any reallocs
        }
    }
    else
    {
        // if we're dealing with the last row
        if (rowIndex == mIndexTable.GetLength() - 1)
        {
            if (insertPos < mData.GetLength())  // if basically there are empty unused element we can use
            {
                needRealInsert = false;
            }
        }
    }

    // perform the insertion
    if (needRealInsert)
    {
        // insert the element inside the data array
        mData.Insert(insertPos, element);

        // adjust the index table entries
        const uint32 numRows = mIndexTable.GetLength();
        for (uint32 i = rowIndex + 1; i < numRows; ++i)
        {
            mIndexTable[i].mStartIndex++;
        }
    }
    else
    {
        mData[insertPos] = element;
    }

    // increase the number of elements in the index table
    mIndexTable[rowIndex].mNumElements++;
}


// remove a given element
template <class T>
void Array2D<T>::Remove(uint32 rowIndex, uint32 elementIndex)
{
    MCORE_ASSERT(rowIndex < mIndexTable.GetLength());
    MCORE_ASSERT(elementIndex < mIndexTable[rowIndex].mNumElements);
    MCORE_ASSERT(mIndexTable[rowIndex].mNumElements > 0);

    const uint32 startIndex         = mIndexTable[rowIndex].mStartIndex;
    const uint32 maxElementIndex    = mIndexTable[rowIndex].mNumElements - 1;

    // swap the last element with the one to be removed
    if (elementIndex != maxElementIndex)
    {
        mData[startIndex + elementIndex] = mData[startIndex + maxElementIndex];
    }

    // decrease the number of elements
    mIndexTable[rowIndex].mNumElements--;
}


// remove a given row
template <class T>
void Array2D<T>::RemoveRow(uint32 rowIndex, bool autoShrink)
{
    MCORE_ASSERT(rowIndex < mIndexTable.GetLength());
    mIndexTable.Remove(rowIndex);

    // optimize memory usage when desired
    if (autoShrink)
    {
        Shrink();
    }
}


// remove a set of rows
template <class T>
void Array2D<T>::RemoveRows(uint32 startRow, uint32 endRow, bool autoShrink)
{
    MCORE_ASSERT(startRow < mIndexTable.GetLength());
    MCORE_ASSERT(endRow < mIndexTable.GetLength());

    // check if the start row is smaller than the end row
    if (startRow < endRow)
    {
        const uint32 numToRemove = (endRow - startRow) + 1;
        mIndexTable.Remove(startRow, numToRemove);
    }
    else    // if the end row is smaller than the start row
    {
        const uint32 numToRemove = (startRow - endRow) + 1;
        mIndexTable.Remove(endRow, numToRemove);
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
    const uint32 numRows = mIndexTable.GetLength();
    if (numRows == 0)
    {
        return;
    }

    // remove all unused items between the rows (unused element data per row)
    const uint32 numRowsMinusOne = numRows - 1;
    for (uint32 a = 0; a < numRowsMinusOne; ++a)
    {
        const uint32 firstUnusedIndex  = mIndexTable[a  ].mStartIndex + mIndexTable[a].mNumElements;
        const uint32 numUnusedElements = mIndexTable[a + 1].mStartIndex - firstUnusedIndex;

        // if we have pre-cached/unused elements, remove those by moving memory to remove the "holes"
        if (numUnusedElements > 0)
        {
            // remove the unused elements from the array
            mData.Remove(firstUnusedIndex, numUnusedElements);

            // change the start indices for all the rows coming after the current one
            const uint32 numTotalRows = mIndexTable.GetLength();
            for (uint32 i = a + 1; i < numTotalRows; ++i)
            {
                mIndexTable[i].mStartIndex -= numUnusedElements;
            }
        }
    }

    // now move all start index values and all data to the front of the data array as much as possible
    // like data on row 0 starting at data element 7, would be moved to data element 0
    uint32 dataPos = 0;
    for (uint32 row = 0; row < numRows; ++row)
    {
        // if the data starts after the place where it could start, move it to the place where it could start
        if (mIndexTable[row].mStartIndex > dataPos)
        {
            mData.MoveElements(dataPos, mIndexTable[row].mStartIndex, mIndexTable[row].mNumElements);
            mIndexTable[row].mStartIndex = dataPos;
        }

        // increase the data pos
        dataPos += mIndexTable[row].mNumElements;
    }

    // remove all unused data items
    if (dataPos < mData.GetLength())
    {
        mData.Remove(dataPos, mData.GetLength() - dataPos);
    }

    // shrink the arrays
    mData.Shrink();
    mIndexTable.Shrink();
}


// calculate the number of used elements
template <class T>
uint32 Array2D<T>::CalcTotalNumElements() const
{
    uint32 totalElements = 0;

    // add all number of row elements together
    const uint32 numRows = mIndexTable.GetLength();
    for (uint32 i = 0; i < numRows; ++i)
    {
        totalElements += mIndexTable[i].mNumElements;
    }

    return totalElements;
}



// assignment operator
template <class T>
Array2D<T>& Array2D<T>::operator = (const Array2D<T>& other)
{
    // don't allow copying itself
    if (&other == this)
    {
        return *this;
    }

    // get rid of existing data
    Clear();

    // copy over the data
    mNumPreCachedElements   = other.mNumPreCachedElements;
    mData                   = other.mData;
    mIndexTable             = other.mIndexTable;

    // return itself
    return *this;
}


// log some details on the contents
template <class T>
void Array2D<T>::LogContents()
{
    const uint32 numElements = CalcTotalNumElements();

    LogDetailedInfo("--[ MCore::Array2D object 0x%x ]----------------------------------------------------", this);
    LogDetailedInfo("Num rows = %d", mIndexTable.GetLength());
    LogDetailedInfo("Num data elements = %d [%d in data]", numElements, mData.GetLength());
    LogDetailedInfo("Used element memory = %.1f percent", CalcUsedElementMemoryPercentage());
    LogDetailedInfo("Rows:");
    const uint32 numRows = mIndexTable.GetLength();
    for (uint32 r = 0; r < numRows; ++r)
    {
        LogDetailedInfo("  + Row #%d - startIndex=%d  numElements=%d", r, mIndexTable[r].mStartIndex, mIndexTable[r].mNumElements);
    }
    LogDetailedInfo("----------------------------------------------------------------------------------------");
}


// swap the contents of two rows
template <class T>
void Array2D<T>::Swap(uint32 rowA, uint32 rowB)
{
    // get the original number of elements from both rows
    const uint32 numElementsA = mIndexTable[rowA].mNumElements;
    const uint32 numElementsB = mIndexTable[rowB].mNumElements;

    // copy the element data of rowA into a temp buffer
    T* tempData = (T*)MCore::Allocate(numElementsA * sizeof(T), 0);
    MCore::MemCopy((uint8*)tempData, GetElements(rowA), sizeof(T) * numElementsA);

    // remove the elements from rowA
    while (GetNumElements(rowA))
    {
        Remove(rowA, 0);
    }

    // add all elements of row B
    uint32 i;
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

    // delete the temp buffer
    MCore::Free(tempData);
}

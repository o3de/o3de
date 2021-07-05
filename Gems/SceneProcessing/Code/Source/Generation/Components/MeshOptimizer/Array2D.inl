/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// resize the array's number of rows
template <class T>
void Array2D<T>::Resize(size_t numRows, bool autoShrink)
{
    // get the current (old) number of rows
    const size_t oldNumRows = mIndexTable.size();

    // don't do anything when we don't need to
    if (numRows == oldNumRows)
    {
        return;
    }

    // resize the index table
    mIndexTable.resize(numRows);

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
            mIndexTable[i].mStartIndex  = mData.size() + (i * mNumPreCachedElements);
            mIndexTable[i].mNumElements = 0;
        }

        // grow the data array
        const size_t numNewRows = numRows - oldNumRows;
        mData.resize(mData.size() + numNewRows * mNumPreCachedElements);
    }
}


// add an element
template <class T>
void Array2D<T>::Add(size_t rowIndex, const T& element)
{
    AZ_Assert(rowIndex < mIndexTable.size(), "Array index out of bounds");

    // find the insert location inside the data array
    size_t insertPos = mIndexTable[rowIndex].mStartIndex + mIndexTable[rowIndex].mNumElements;
    if (insertPos >= mData.size())
    {
        mData.resize(insertPos + 1);
    }

    // check if we need to insert for real
    bool needRealInsert = true;
    if (rowIndex < mIndexTable.size() - 1)                 // if there are still entries coming after the one we have to add to
    {
        if (insertPos < mIndexTable[rowIndex + 1].mStartIndex)    // if basically there are empty unused element we can use
        {
            needRealInsert = false;                             // then we don't need to do any reallocs
        }
    }
    else
    {
        // if we're dealing with the last row
        if (rowIndex == mIndexTable.size() - 1)
        {
            if (insertPos < mData.size())  // if basically there are empty unused element we can use
            {
                needRealInsert = false;
            }
        }
    }

    // perform the insertion
    if (needRealInsert)
    {
        // insert the element inside the data array
        mData.insert(AZStd::next(begin(mData), insertPos), element);

        // adjust the index table entries
        const size_t numRows = mIndexTable.size();
        for (size_t i = rowIndex + 1; i < numRows; ++i)
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
void Array2D<T>::Remove(size_t rowIndex, size_t elementIndex)
{
    AZ_Assert(rowIndex < mIndexTable.size(), "Array2D<>::Remove: array index out of bounds");
    AZ_Assert(elementIndex < mIndexTable[rowIndex].mNumElements, "Array2D<>::Remove: element index out of bounds");
    AZ_Assert(mIndexTable[rowIndex].mNumElements > 0, "Array2D<>::Remove: array index out of bounds");

    const size_t startIndex         = mIndexTable[rowIndex].mStartIndex;
    const size_t maxElementIndex    = mIndexTable[rowIndex].mNumElements - 1;

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
void Array2D<T>::RemoveRow(size_t rowIndex, bool autoShrink)
{
    AZ_Assert(rowIndex < mIndexTable.GetLength(), "Array2D<>::RemoveRow: rowIndex out of bounds");
    mIndexTable.Remove(rowIndex);

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
    AZ_Assert(startRow < mIndexTable.size(), "Array2D<>::RemoveRows: startRow out of bounds");
    AZ_Assert(endRow < mIndexTable.size(), "Array2D<>::RemoveRows: endRow out of bounds");

    // check if the start row is smaller than the end row
    if (startRow < endRow)
    {
        const size_t numToRemove = (endRow - startRow) + 1;
        mIndexTable.erase(AZStd::next(begin(mIndexTable), startRow), AZStd::next(AZStd::next(begin(mIndexTable), startRow), numToRemove));
    }
    else    // if the end row is smaller than the start row
    {
        const size_t numToRemove = (startRow - endRow) + 1;
        mIndexTable.erase(AZStd::next(begin(mIndexTable), endRow), AZStd::next(AZStd::next(begin(mIndexTable), endRow), numToRemove));
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
    const size_t numRows = mIndexTable.size();
    if (numRows == 0)
    {
        return;
    }

    // remove all unused items between the rows (unused element data per row)
    const size_t numRowsMinusOne = numRows - 1;
    for (size_t a = 0; a < numRowsMinusOne; ++a)
    {
        const size_t firstUnusedIndex  = mIndexTable[a  ].mStartIndex + mIndexTable[a].mNumElements;
        const size_t numUnusedElements = mIndexTable[a + 1].mStartIndex - firstUnusedIndex;

        // if we have pre-cached/unused elements, remove those by moving memory to remove the "holes"
        if (numUnusedElements > 0)
        {
            // remove the unused elements from the array
            mData.erase(AZStd::next(begin(mData), firstUnusedIndex), AZStd::next(AZStd::next(begin(mData), firstUnusedIndex), numUnusedElements));

            // change the start indices for all the rows coming after the current one
            const size_t numTotalRows = mIndexTable.size();
            for (size_t i = a + 1; i < numTotalRows; ++i)
            {
                mIndexTable[i].mStartIndex -= numUnusedElements;
            }
        }
    }

    // now move all start index values and all data to the front of the data array as much as possible
    // like data on row 0 starting at data element 7, would be moved to data element 0
    size_t dataPos = 0;
    for (size_t row = 0; row < numRows; ++row)
    {
        // if the data starts after the place where it could start, move it to the place where it could start
        if (mIndexTable[row].mStartIndex > dataPos)
        {
            AZStd::move(AZStd::next(begin(mData), this->mIndexTable[row].mStartIndex), AZStd::next(AZStd::next(begin(mData), this->mIndexTable[row].mStartIndex), this->mIndexTable[row].mNumElements), AZStd::next(begin(mData), dataPos));
            mIndexTable[row].mStartIndex = dataPos;
        }

        // increase the data pos
        dataPos += mIndexTable[row].mNumElements;
    }

    // remove all unused data items
    if (dataPos < mData.size())
    {
        mData.erase(AZStd::next(begin(mData), dataPos), end(mData));
    }

    // shrink the arrays
    mData.shrink_to_fit();
    mIndexTable.shrink_to_fit();
}


// calculate the number of used elements
template <class T>
size_t Array2D<T>::CalcTotalNumElements() const
{
    size_t totalElements = 0;

    // add all number of row elements together
    const size_t numRows = mIndexTable.size();
    for (size_t i = 0; i < numRows; ++i)
    {
        totalElements += mIndexTable[i].mNumElements;
    }

    return totalElements;
}


// swap the contents of two rows
template <class T>
void Array2D<T>::Swap(size_t rowA, size_t rowB)
{
    // get the original number of elements from both rows
    const size_t numElementsA = mIndexTable[rowA].mNumElements;
    const size_t numElementsB = mIndexTable[rowB].mNumElements;

    // move the element data of rowA into a temp buffer
    AZStd::vector<T> tempData(numElementsA);
    AZStd::move(
        AZStd::next(mData.begin(), mIndexTable[rowA].mStartIndex),
        AZStd::next(mData.begin(), mIndexTable[rowA].mStartIndex + numElementsA),
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

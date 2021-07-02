/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// default constructor
template <class Key, class Value>
HashTable<Key, Value>::HashTable()
{
    mElements.SetMemoryCategory(MCORE_MEMCATEGORY_HASHTABLE);
    mTotalNumEntries = 0;
}


// extended constructor
template <class Key, class Value>
HashTable<Key, Value>::HashTable(uint32 maxElements)
{
    mElements.SetMemoryCategory(MCORE_MEMCATEGORY_HASHTABLE);
    mTotalNumEntries = 0;
    Init(maxElements);
}


// copy constructor
template <class Key, class Value>
HashTable<Key, Value>::HashTable(const HashTable<Key, Value>& other)
    : mTotalNumEntries(0)
{
    *this = other;
}


// destructor
template <class Key, class Value>
HashTable<Key, Value>::~HashTable()
{
    Clear();
}


// clear the table
template <class Key, class Value>
void HashTable<Key, Value>::Clear()
{
    // get rid of existing elements
    const uint32 numElems = mElements.GetLength();
    for (uint32 i = 0; i < numElems; ++i)
    {
        if (mElements[i])
        {
            delete mElements[i];
        }
    }

    // clear the array
    mElements.Clear();

    mTotalNumEntries = 0;
}


// find the entry with a given key
template <class Key, class Value>
bool HashTable<Key, Value>::FindEntry(const Key& key, uint32* outElementNr, uint32* outEntryNr) const
{
    // calculate the hash value
    uint32 hashResult = Hash<Key>(key) % mElements.GetLength();

    // check if the we have an entry at this hash position
    if (mElements[hashResult] == nullptr)
    {
        return false;
    }

    // search inside the array of entries
    const uint32 numElements = mElements[hashResult]->GetLength();
    for (uint32 i = 0; i < numElements; ++i)
    {
        // if we found the one we are searching for
        if (mElements[hashResult]->GetItem(i).mKey == key)
        {
            *outElementNr = hashResult;
            *outEntryNr   = i;
            return true;
        }
    }

    return false;
}


// initialize the table at a given maximum amount of elements
template <class Key, class Value>
void HashTable<Key, Value>::Init(uint32 maxElements)
{
    // get rid of existing elements
    Clear();

    // resize the array
    mElements.Resize(maxElements);
    mElements.Shrink();

    // reset all the elements
    for (uint32 i = 0; i < maxElements; ++i)
    {
        mElements[i] = nullptr;
    }
}


// add an entry to the table
template <class Key, class Value>
void HashTable<Key, Value>::Add(const Key& key, const Value& value)
{
    // calculate the hash value
    uint32 hashResult = Hash<Key>(key) % mElements.GetLength();

    // make sure there isn't already an element with this key
    MCORE_ASSERT(Contains(key) == false);

    // if the array isn't allocated yet, do so
    if (mElements[hashResult] == nullptr)
    {
        mElements[hashResult] = new MCore::Array< Entry >();
        mElements[hashResult]->SetMemoryCategory(MCORE_MEMCATEGORY_HASHTABLE);
    }

    // add the entry to the array
    mElements[hashResult]->Add(Entry(key, value));

    // increase the total number of entries
    mTotalNumEntries++;
}


// get a value
template <class Key, class Value>
bool HashTable<Key, Value>::GetValue(const Key& inKey, Value* outValue) const
{
    // try to find the element
    uint32 elementNr, entryNr;
    if (FindEntry(inKey, &elementNr, &entryNr))
    {
        *outValue = mElements[elementNr]->GetItem(entryNr).mValue;
        return true;
    }

    // nothing found
    return false;
}


// check if there is an entry using the specified key
template <class Key, class Value>
bool HashTable<Key, Value>::Contains(const Key& key) const
{
    uint32 elementNr, entryNr;
    return FindEntry(key, &elementNr, &entryNr);
}


template <class Key, class Value>
bool HashTable<Key, Value>::Remove(const Key& key)
{
    uint32 elementNr, entryNr;
    if (FindEntry(key, &elementNr, &entryNr))
    {
        // remove the element
        mElements[elementNr]->Remove(entryNr);

        // remove the array if it is empty
        if (mElements[elementNr]->GetLength() == 0)
        {
            delete mElements[elementNr];
            mElements[elementNr] = nullptr;
        }

        // decrease the total number of entries
        mTotalNumEntries--;

        // yeah, we successfully removed it
        return true;
    }

    // the element wasn't found, so cannot be removed
    return false;
}


// get the number of table elements
template <class Key, class Value>
uint32 HashTable<Key, Value>::GetNumTableElements() const
{
    return mElements.GetLength();
}


// get the get the number of entries for a given table element
template <class Key, class Value>
uint32 HashTable<Key, Value>::GetNumEntries(uint32 tableElementNr) const
{
    if (mElements[tableElementNr] == nullptr)
    {
        return 0;
    }

    return mElements[tableElementNr]->GetLength();
}


// get the number of entries in the table
template <class Key, class Value>
uint32 HashTable<Key, Value>::GetTotalNumEntries() const
{
    return mTotalNumEntries;
}


// calculate the load balance
template <class Key, class Value>
float HashTable<Key, Value>::CalcLoadBalance() const
{
    uint32 numUsedElements = 0;

    // traverse all elements
    const uint32 numElements = mElements.GetLength();
    for (uint32 i = 0; i < numElements; ++i)
    {
        if (mElements[i])
        {
            numUsedElements++;
        }
    }

    if (numUsedElements == 0)
    {
        return 0;
    }

    return (numUsedElements / (float)numElements) * 100.0f;
}


template <class Key, class Value>
float HashTable<Key, Value>::CalcAverageNumEntries() const
{
    uint32 numEntries = 0;
    uint32 numUsedElements = 0;

    // traverse all elements
    const uint32 numElements = mElements.GetLength();
    for (uint32 i = 0; i < numElements; ++i)
    {
        if (mElements[i])
        {
            numUsedElements++;
            numEntries += mElements[i]->GetLength();
        }
    }

    if (numEntries == 0)
    {
        return 0;
    }

    return numEntries / (float)numUsedElements;
}


// update the value of the entry with a given key
template <class Key, class Value>
bool HashTable<Key, Value>::SetValue(const Key& key, const Value& value)
{
    // try to find the element
    uint32 elementNr, entryNr;
    if (FindEntry(key, &elementNr, &entryNr))
    {
        mElements[elementNr]->GetItem(entryNr).mValue = value;
        return true;
    }

    // nothing found
    return false;
}


// get a given entry
template <class Key, class Value>
MCORE_INLINE typename HashTable<Key, Value>::Entry& HashTable<Key, Value>::GetEntry(uint32 tableElementNr, uint32 entryNr)
{
    MCORE_ASSERT(tableElementNr < mElements.GetLength());   // make sure the values are in range
    MCORE_ASSERT(mElements[tableElementNr]);                                // this table element must have entries
    MCORE_ASSERT(entryNr < mElements[tableElementNr]->GetLength());     //

    return mElements[tableElementNr]->GetItem(entryNr);
}


// operator =
template <class Key, class Value>
HashTable<Key, Value>& HashTable<Key, Value>::operator = (const HashTable<Key, Value>& other)
{
    if (&other == this)
    {
        return *this;
    }

    // get rid of old data
    Clear();

    // copy the number of entries
    mTotalNumEntries = other.mTotalNumEntries;

    // resize the element array
    mElements.Resize(other.mElements.GetLength());

    // copy the element entries
    const uint32 numElements = mElements.GetLength();
    for (uint32 i = 0; i < numElements; ++i)
    {
        if (other.mElements[i])
        {
            // create the array and copy the entries
            mElements[i]  = new MCore::Array< Entry >();
            *mElements[i] = *other.mElements[i];
        }
        else
        {
            mElements[i] = nullptr;
        }
    }

    return *this;
}

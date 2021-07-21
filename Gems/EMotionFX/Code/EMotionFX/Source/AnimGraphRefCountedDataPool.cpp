/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AnimGraphRefCountedDataPool.h"


namespace EMotionFX
{
    // constructor
    AnimGraphRefCountedDataPool::AnimGraphRefCountedDataPool()
    {
        mItems.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA);
        mFreeItems.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA);
        mItems.Reserve(32);
        mFreeItems.Reserve(32);
        Resize(16);
        mMaxUsed = 0;
    }


    // destructor
    AnimGraphRefCountedDataPool::~AnimGraphRefCountedDataPool()
    {
        // delete all items
        const uint32 numItems = mItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            delete mItems[i];
        }
        mItems.Clear();

        // clear the free array
        mFreeItems.Clear();
    }


    // resize the number of items in the pool
    void AnimGraphRefCountedDataPool::Resize(uint32 numItems)
    {
        const uint32 numOldItems = mItems.GetLength();

        // if we will remove Items
        int32 difference = numItems - numOldItems;
        if (difference < 0)
        {
            // remove the last Items
            difference = abs(difference);
            for (int32 i = 0; i < difference; ++i)
            {
                AnimGraphRefCountedData* item = mItems[mFreeItems.GetLength() - 1];
                MCORE_ASSERT(mFreeItems.Contains(item)); // make sure the Item is not already in use
                delete item;
                mItems.Remove(mFreeItems.GetLength() - 1);
            }
        }
        else // we want to add new Items
        {
            for (int32 i = 0; i < difference; ++i)
            {
                AnimGraphRefCountedData* newItem = new AnimGraphRefCountedData();
                mItems.Add(newItem);
                mFreeItems.Add(newItem);
            }
        }
    }


    // request an item
    AnimGraphRefCountedData* AnimGraphRefCountedDataPool::RequestNew()
    {
        // if we have no free items left, allocate a new one
        if (mFreeItems.GetLength() == 0)
        {
            AnimGraphRefCountedData* newItem = new AnimGraphRefCountedData();
            mItems.Add(newItem);
            mMaxUsed = MCore::Max<uint32>(mMaxUsed, GetNumUsedItems());
            return newItem;
        }

        // request the last free item
        AnimGraphRefCountedData* item = mFreeItems[mFreeItems.GetLength() - 1];
        mFreeItems.RemoveLast(); // remove it from the list of free Items
        mMaxUsed = MCore::Max<uint32>(mMaxUsed, GetNumUsedItems());
        return item;
    }


    // free the item again
    void AnimGraphRefCountedDataPool::Free(AnimGraphRefCountedData* item)
    {
        MCORE_ASSERT(mItems.Contains(item));
        mFreeItems.Add(item);
    }
}   // namespace EMotionFX

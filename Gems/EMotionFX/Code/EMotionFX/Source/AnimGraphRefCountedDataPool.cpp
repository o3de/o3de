/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AnimGraphRefCountedDataPool.h"
#include <MCore/Source/FastMath.h>
#include <MCore/Source/Algorithms.h>


namespace EMotionFX
{
    // constructor
    AnimGraphRefCountedDataPool::AnimGraphRefCountedDataPool()
    {
        mItems.reserve(32);
        mFreeItems.reserve(32);
        Resize(16);
        mMaxUsed = 0;
    }


    // destructor
    AnimGraphRefCountedDataPool::~AnimGraphRefCountedDataPool()
    {
        // delete all items
        const uint32 numItems = mItems.size();
        for (uint32 i = 0; i < numItems; ++i)
        {
            delete mItems[i];
        }
        mItems.clear();

        // clear the free array
        mFreeItems.clear();
    }


    // resize the number of items in the pool
    void AnimGraphRefCountedDataPool::Resize(uint32 numItems)
    {
        const uint32 numOldItems = mItems.size();

        // if we will remove Items
        int32 difference = numItems - numOldItems;
        if (difference < 0)
        {
            // remove the last Items
            difference = abs(difference);
            for (int32 i = 0; i < difference; ++i)
            {
                AnimGraphRefCountedData* item = mItems.back();
                MCORE_ASSERT(AZStd::find(begin(mFreeItems), end(mFreeItems), item) != end(mFreeItems)); // make sure the Item is not already in use
                delete item;
                mItems.erase(mItems.end() - 1);
            }
        }
        else // we want to add new Items
        {
            for (int32 i = 0; i < difference; ++i)
            {
                AnimGraphRefCountedData* newItem = new AnimGraphRefCountedData();
                mItems.emplace_back(newItem);
                mFreeItems.emplace_back(newItem);
            }
        }
    }


    // request an item
    AnimGraphRefCountedData* AnimGraphRefCountedDataPool::RequestNew()
    {
        // if we have no free items left, allocate a new one
        if (mFreeItems.size() == 0)
        {
            AnimGraphRefCountedData* newItem = new AnimGraphRefCountedData();
            mItems.emplace_back(newItem);
            mMaxUsed = MCore::Max<uint32>(mMaxUsed, GetNumUsedItems());
            return newItem;
        }

        // request the last free item
        AnimGraphRefCountedData* item = mFreeItems[mFreeItems.size() - 1];
        mFreeItems.pop_back(); // remove it from the list of free Items
        mMaxUsed = MCore::Max<uint32>(mMaxUsed, GetNumUsedItems());
        return item;
    }


    // free the item again
    void AnimGraphRefCountedDataPool::Free(AnimGraphRefCountedData* item)
    {
        MCORE_ASSERT(AZStd::find(begin(mItems), end(mItems), item) != end(mItems));
        mFreeItems.emplace_back(item);
    }
}   // namespace EMotionFX

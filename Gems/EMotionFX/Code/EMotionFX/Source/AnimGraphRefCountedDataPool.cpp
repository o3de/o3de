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
        for (AnimGraphRefCountedData*& mItem : mItems)
        {
            delete mItem;
        }
        mItems.clear();

        // clear the free array
        mFreeItems.clear();
    }


    // resize the number of items in the pool
    void AnimGraphRefCountedDataPool::Resize(size_t numItems)
    {
        const size_t numOldItems = mItems.size();

        // if we will remove Items
        if (numItems < numOldItems)
        {
            // remove the last Items
            const size_t numToRemove = numOldItems - numItems;
            for (size_t i = 0; i < numToRemove; ++i)
            {
                AnimGraphRefCountedData* item = mItems.back();
                MCORE_ASSERT(AZStd::find(begin(mFreeItems), end(mFreeItems), item) != end(mFreeItems)); // make sure the Item is not already in use
                delete item;
                mItems.erase(mItems.end() - 1);
            }
        }
        else // we want to add new Items
        {
            const size_t numToAdd = numItems - numOldItems;
            for (size_t i = 0; i < numToAdd; ++i)
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
        if (mFreeItems.empty())
        {
            AnimGraphRefCountedData* newItem = new AnimGraphRefCountedData();
            mItems.emplace_back(newItem);
            mMaxUsed = AZStd::max(mMaxUsed, GetNumUsedItems());
            return newItem;
        }

        // request the last free item
        AnimGraphRefCountedData* item = mFreeItems[mFreeItems.size() - 1];
        mFreeItems.pop_back(); // remove it from the list of free Items
        mMaxUsed = AZStd::max(mMaxUsed, GetNumUsedItems());
        return item;
    }


    // free the item again
    void AnimGraphRefCountedDataPool::Free(AnimGraphRefCountedData* item)
    {
        MCORE_ASSERT(AZStd::find(begin(mItems), end(mItems), item) != end(mItems));
        mFreeItems.emplace_back(item);
    }
}   // namespace EMotionFX

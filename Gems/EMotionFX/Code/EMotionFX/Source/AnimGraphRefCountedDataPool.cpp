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
        m_items.reserve(32);
        m_freeItems.reserve(32);
        Resize(16);
        m_maxUsed = 0;
    }


    // destructor
    AnimGraphRefCountedDataPool::~AnimGraphRefCountedDataPool()
    {
        // delete all items
        for (AnimGraphRefCountedData*& item : m_items)
        {
            delete item;
        }
        m_items.clear();

        // clear the free array
        m_freeItems.clear();
    }


    // resize the number of items in the pool
    void AnimGraphRefCountedDataPool::Resize(size_t numItems)
    {
        const size_t numOldItems = m_items.size();

        // if we will remove Items
        if (numItems < numOldItems)
        {
            // remove the last Items
            const size_t numToRemove = numOldItems - numItems;
            for (size_t i = 0; i < numToRemove; ++i)
            {
                AnimGraphRefCountedData* item = m_items.back();
                MCORE_ASSERT(AZStd::find(begin(m_freeItems), end(m_freeItems), item) != end(m_freeItems)); // make sure the Item is not already in use
                delete item;
                m_items.erase(m_items.end() - 1);
            }
        }
        else // we want to add new Items
        {
            const size_t numToAdd = numItems - numOldItems;
            for (size_t i = 0; i < numToAdd; ++i)
            {
                AnimGraphRefCountedData* newItem = new AnimGraphRefCountedData();
                m_items.emplace_back(newItem);
                m_freeItems.emplace_back(newItem);
            }
        }
    }


    // request an item
    AnimGraphRefCountedData* AnimGraphRefCountedDataPool::RequestNew()
    {
        // if we have no free items left, allocate a new one
        if (m_freeItems.empty())
        {
            AnimGraphRefCountedData* newItem = new AnimGraphRefCountedData();
            m_items.emplace_back(newItem);
            m_maxUsed = AZStd::max(m_maxUsed, GetNumUsedItems());
            return newItem;
        }

        // request the last free item
        AnimGraphRefCountedData* item = m_freeItems[m_freeItems.size() - 1];
        m_freeItems.pop_back(); // remove it from the list of free Items
        m_maxUsed = AZStd::max(m_maxUsed, GetNumUsedItems());
        return item;
    }


    // free the item again
    void AnimGraphRefCountedDataPool::Free(AnimGraphRefCountedData* item)
    {
        MCORE_ASSERT(AZStd::find(begin(m_items), end(m_items), item) != end(m_items));
        m_freeItems.emplace_back(item);
    }
}   // namespace EMotionFX

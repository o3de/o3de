/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "EMotionFXConfig.h"
#include "AnimGraphRefCountedData.h"
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphRefCountedDataPool
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphRefCountedDataPool, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA);

    public:
        AnimGraphRefCountedDataPool();
        ~AnimGraphRefCountedDataPool();

        void Resize(size_t numItems);

        AnimGraphRefCountedData* RequestNew();
        void Free(AnimGraphRefCountedData* item);

        MCORE_INLINE size_t GetNumFreeItems() const             { return m_freeItems.size(); }
        MCORE_INLINE size_t GetNumItems() const                 { return m_items.size(); }
        MCORE_INLINE size_t GetNumUsedItems() const             { return m_items.size() - m_freeItems.size(); }
        MCORE_INLINE size_t GetNumMaxUsedItems() const          { return m_maxUsed; }
        MCORE_INLINE void ResetMaxUsedItems()                   { m_maxUsed = 0; }

    private:
        AZStd::vector<AnimGraphRefCountedData*> m_items;
        AZStd::vector<AnimGraphRefCountedData*> m_freeItems;
        size_t                                  m_maxUsed;
    };
}   // namespace EMotionFX

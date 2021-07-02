/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "EMotionFXConfig.h"
#include "AnimGraphRefCountedData.h"
#include <MCore/Source/Array.h>


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

        void Resize(uint32 numItems);

        AnimGraphRefCountedData* RequestNew();
        void Free(AnimGraphRefCountedData* item);

        MCORE_INLINE uint32 GetNumFreeItems() const             { return mFreeItems.GetLength(); }
        MCORE_INLINE uint32 GetNumItems() const                 { return mItems.GetLength(); }
        MCORE_INLINE uint32 GetNumUsedItems() const             { return (mItems.GetLength() - mFreeItems.GetLength()); }
        MCORE_INLINE uint32 GetNumMaxUsedItems() const          { return mMaxUsed; }
        MCORE_INLINE void ResetMaxUsedItems()                   { mMaxUsed = 0; }

    private:
        MCore::Array<AnimGraphRefCountedData*> mItems;
        MCore::Array<AnimGraphRefCountedData*> mFreeItems;
        uint32                                  mMaxUsed;
    };
}   // namespace EMotionFX

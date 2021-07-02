/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include <MCore/Source/Array.h>
#include <MCore/Source/MultiThreadManager.h>


namespace EMotionFX
{
    // forward declarations
    class MotionInstance;
    class ActorInstance;
    class Motion;

    /**
     *
     *
     */
    class EMFX_API MotionInstancePool
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class MotionInstance;

    public:
        enum EPoolType
        {
            POOLTYPE_STATIC,
            POOLTYPE_DYNAMIC
        };

        static MotionInstancePool* Create();

        void Init(uint32 numInitialInstances = 256, EPoolType poolType = POOLTYPE_DYNAMIC, uint32 subPoolSize = 512); // auto called on EMotion FX init

        // with lock
        MotionInstance* RequestNew(Motion* motion, ActorInstance* actorInstance);
        void Free(MotionInstance* motionInstance);

        // without lock
        MotionInstance* RequestNewWithoutLock(Motion* motion, ActorInstance* actorInstance);
        void FreeWithoutLock(MotionInstance* motionInstance);

        // misc
        void LogMemoryStats();  // locks internally
        void Shrink();  // minimize memory usage

        // manual locking and unlocking for threading
        void Lock();
        void Unlock();

    private:
        class EMFX_API SubPool
        {
            MCORE_MEMORYOBJECTCATEGORY(SubPool, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL)

        public:
            SubPool();
            ~SubPool();

            uint8*      mData;
            uint32      mNumInstances;
            uint32      mNumInUse;
        };

        struct EMFX_API MemLocation
        {
            void*       mAddress;
            SubPool*    mSubPool;
        };

        class EMFX_API Pool
        {
            MCORE_MEMORYOBJECTCATEGORY(Pool, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL)

        public:
            Pool();
            ~Pool();

            uint8*                      mData;
            uint32                      mNumInstances;
            uint32                      mNumUsedInstances;
            uint32                      mSubPoolSize;
            MCore::Array<MemLocation>   mFreeList;
            MCore::Array<SubPool*>      mSubPools;
            EPoolType                   mPoolType;
        };

        Pool*           mPool;
        MCore::Mutex    mLock;

        MotionInstancePool();
        ~MotionInstancePool();
    };
}   // namespace EMotionFX

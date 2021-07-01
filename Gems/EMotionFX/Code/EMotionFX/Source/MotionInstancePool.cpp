/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MotionInstancePool.h"
#include "MotionInstance.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionInstancePool, MotionAllocator, 0)


    // constructor
    MotionInstancePool::SubPool::SubPool()
        : mData(nullptr)
        , mNumInstances(0)
        , mNumInUse(0)
    {
    }


    // destructor
    MotionInstancePool::SubPool::~SubPool()
    {
        MCore::Free(mData);
        mData = nullptr;
    }


    //--------------------------------------------------------------------------------------------------
    // class MotionInstancePool::Pool
    //--------------------------------------------------------------------------------------------------

    // constructor
    MotionInstancePool::Pool::Pool()
    {
        mFreeList.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);
        mSubPools.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);
        mPoolType           = POOLTYPE_DYNAMIC;
        mData               = nullptr;
        mNumInstances       = 0;
        mNumUsedInstances   = 0;
        mSubPoolSize        = 0;
    }


    // destructor
    MotionInstancePool::Pool::~Pool()
    {
        if (mPoolType == POOLTYPE_STATIC)
        {
            MCore::Free(mData);
            mData = nullptr;
            mFreeList.Clear();
        }
        else
        if (mPoolType == POOLTYPE_DYNAMIC)
        {
            MCORE_ASSERT(mData == nullptr);

            // delete all subpools
            const uint32 numSubPools = mSubPools.GetLength();
            for (uint32 s = 0; s < numSubPools; ++s)
            {
                delete mSubPools[s];
            }
            mSubPools.Clear();

            mFreeList.Clear();
        }
        else
        {
            MCORE_ASSERT(false);
        }
    }



    //--------------------------------------------------------------------------------------------------
    // class MotionInstancePool
    //--------------------------------------------------------------------------------------------------

    // constructor
    MotionInstancePool::MotionInstancePool()
        : BaseObject()
    {
        mPool = nullptr;
    }


    // destructor
    MotionInstancePool::~MotionInstancePool()
    {
        if (mPool->mNumUsedInstances > 0)
        {
            MCore::LogError("EMotionFX::~MotionInstancePool() - There are still %d unfreed motion instances, please use the Free function in the MotionInstancePool to free them, just like you would delete the object.", mPool->mNumUsedInstances);
        }

        delete mPool;
    }


    // create
    MotionInstancePool* MotionInstancePool::Create()
    {
        return aznew MotionInstancePool();
    }


    // init the motion instance pool
    void MotionInstancePool::Init(uint32 numInitialInstances, EPoolType poolType, uint32 subPoolSize)
    {
        if (mPool)
        {
            MCore::LogError("EMotionFX::MotionInstancePool::Init() - We have already initialized the pool, ignoring new init call.");
            return;
        }

        // check if we use numInitialInstances==0 with a static pool, as that isn't allowed of course
        if (poolType == POOLTYPE_STATIC && numInitialInstances == 0)
        {
            MCore::LogError("EMotionFX::MotionInstancePool::Init() - The number of initial motion instances cannot be 0 when using a static pool. Please set the dynamic parameter to true, or increase the value of numInitialInstances.");
            MCORE_ASSERT(false);
            return;
        }

        // create the subpool
        mPool = new Pool();
        mPool->mNumInstances    = numInitialInstances;
        mPool->mPoolType        = poolType;
        mPool->mSubPoolSize     = subPoolSize;

        // if we have a static pool
        if (poolType == POOLTYPE_STATIC)
        {
            mPool->mData    = (uint8*)MCore::Allocate(numInitialInstances * sizeof(MotionInstance), EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);// alloc space
            mPool->mFreeList.ResizeFast(numInitialInstances);
            for (uint32 i = 0; i < numInitialInstances; ++i)
            {
                void* memLocation = (void*)(mPool->mData + i * sizeof(MotionInstance));
                mPool->mFreeList[i].mAddress = memLocation;
                mPool->mFreeList[i].mSubPool = nullptr;
            }
        }
        else // if we have a dynamic pool
        if (poolType == POOLTYPE_DYNAMIC)
        {
            mPool->mSubPools.Reserve(32);

            SubPool* subPool = new SubPool();
            subPool->mData              = (uint8*)MCore::Allocate(numInitialInstances * sizeof(MotionInstance), EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);// alloc space
            subPool->mNumInstances      = numInitialInstances;

            mPool->mFreeList.ResizeFast(numInitialInstances);
            for (uint32 i = 0; i < numInitialInstances; ++i)
            {
                mPool->mFreeList[i].mAddress = (void*)(subPool->mData + i * sizeof(MotionInstance));
                mPool->mFreeList[i].mSubPool = subPool;
            }

            mPool->mSubPools.Add(subPool);
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
        }
    }


    // return a new motion instance
    MotionInstance* MotionInstancePool::RequestNewWithoutLock(Motion* motion, ActorInstance* actorInstance)
    {
        // check if we already initialized
        if (mPool == nullptr)
        {
            MCore::LogWarning("EMotionFX::MotionInstancePool::RequestNew() - We have not yet initialized the pool, initializing it to a dynamic pool");
            Init();
        }

        // if there is are free items left
        if (mPool->mFreeList.GetLength() > 0)
        {
            const MemLocation& location = mPool->mFreeList.GetLast();
            MotionInstance* result = MotionInstance::Create(location.mAddress, motion, actorInstance);

            if (location.mSubPool)
            {
                location.mSubPool->mNumInUse++;
            }
            result->SetSubPool(location.mSubPool);

            mPool->mFreeList.RemoveLast(); // remove it from the free list
            mPool->mNumUsedInstances++;
            return result;
        }

        // we have no more free attributes left
        if (mPool->mPoolType == POOLTYPE_DYNAMIC) // we're dynamic, so we can just create new ones
        {
            const uint32 numInstances = mPool->mSubPoolSize;
            mPool->mNumInstances += numInstances;

            SubPool* subPool = new SubPool();
            subPool->mData              = (uint8*)MCore::Allocate(numInstances * sizeof(MotionInstance), EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);// alloc space
            subPool->mNumInstances      = numInstances;

            const uint32 startIndex = mPool->mFreeList.GetLength();
            //mPool->mFreeList.Reserve( numInstances * 2 );
            if (mPool->mFreeList.GetMaxLength() < mPool->mNumInstances)
            {
                mPool->mFreeList.Reserve(mPool->mNumInstances + mPool->mFreeList.GetMaxLength() / 2);
            }

            mPool->mFreeList.ResizeFast(startIndex + numInstances);
            for (uint32 i = 0; i < numInstances; ++i)
            {
                void* memAddress = (void*)(subPool->mData + i * sizeof(MotionInstance));
                mPool->mFreeList[i + startIndex].mAddress = memAddress;
                mPool->mFreeList[i + startIndex].mSubPool = subPool;
            }

            mPool->mSubPools.Add(subPool);

            const MemLocation& location = mPool->mFreeList.GetLast();
            MotionInstance* result = MotionInstance::Create(location.mAddress, motion, actorInstance);
            if (location.mSubPool)
            {
                location.mSubPool->mNumInUse++;
            }
            result->SetSubPool(location.mSubPool);
            mPool->mFreeList.RemoveLast(); // remove it from the free list
            mPool->mNumUsedInstances++;
            return result;
        }
        else // we are static and ran out of free attributes
        if (mPool->mPoolType == POOLTYPE_STATIC)
        {
            MCore::LogError("EMotionFX::MotionInstancePool::RequestNew() - There are no free motion instance in the static pool. Please increase the size of the pool or make it dynamic when calling Init.");
            MCORE_ASSERT(false); // we ran out of free motion instances
            return nullptr;
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
            return nullptr;
        }
    }


    // free a given motion instance
    void MotionInstancePool::FreeWithoutLock(MotionInstance* motionInstance)
    {
        if (motionInstance == nullptr)
        {
            return;
        }

        if (mPool == nullptr)
        {
            MCore::LogWarning("EMotionFX::MotionInstancePool::Free() - The pool has not yet been initialized, please call Init first.");
            MCORE_ASSERT(false);
            return;
        }

        // add it back to the free list
        if (motionInstance->GetSubPool())
        {
            motionInstance->GetSubPool()->mNumInUse--;
        }

        mPool->mFreeList.AddEmpty();
        mPool->mFreeList.GetLast().mAddress = motionInstance;
        mPool->mFreeList.GetLast().mSubPool = motionInstance->GetSubPool();
        mPool->mNumUsedInstances--;

        motionInstance->DecreaseReferenceCount();
        motionInstance->~MotionInstance(); // call the destructor
    }


    // log the memory stats
    void MotionInstancePool::LogMemoryStats()
    {
        Lock();
        MCore::LogInfo("EMotionFX::MotionInstancePool::LogMemoryStats() - Logging motion instance pool info");

        const uint32 numFree    = mPool->mFreeList.GetLength();
        uint32 numUsed          = mPool->mNumUsedInstances;
        uint32 memUsage         = 0;
        uint32 usedMemUsage     = 0;
        uint32 totalMemUsage    = 0;
        uint32 totalUsedInstancesMemUsage = 0;

        if (mPool->mPoolType == POOLTYPE_STATIC)
        {
            if (mPool->mNumInstances > 0)
            {
                memUsage = mPool->mNumInstances * sizeof(MotionInstance);
                usedMemUsage = numUsed * sizeof(MotionInstance);
            }
        }
        else
        if (mPool->mPoolType == POOLTYPE_DYNAMIC)
        {
            if (mPool->mNumInstances > 0)
            {
                memUsage = mPool->mNumInstances * sizeof(MotionInstance);
                usedMemUsage = numUsed * sizeof(MotionInstance);
            }
        }

        totalUsedInstancesMemUsage  += usedMemUsage;
        totalMemUsage += memUsage;
        totalMemUsage += sizeof(Pool);
        totalMemUsage += mPool->mFreeList.CalcMemoryUsage(false);

        MCore::LogInfo("Pool:");
        if (mPool->mPoolType == POOLTYPE_DYNAMIC)
        {
            MCore::LogInfo("   - Num SubPools:          %d", mPool->mSubPools.GetLength());
        }
        MCore::LogInfo("   - Num Instances:         %d", mPool->mNumInstances);
        MCore::LogInfo("   - Num Free:              %d", numFree);
        MCore::LogInfo("   - Num Used:              %d", numUsed);
        MCore::LogInfo("   - PoolType:              %s", (mPool->mPoolType == POOLTYPE_STATIC) ? "Static" : "Dynamic");
        MCore::LogInfo("   - Total Instances Mem:   %d bytes (%d k)", memUsage, memUsage / 1000);
        MCore::LogInfo("   - Used Instances Mem:    %d (%d k)", totalUsedInstancesMemUsage, totalUsedInstancesMemUsage / 1000);
        MCore::LogInfo("   - Total Mem Usage:       %d (%d k)", totalMemUsage, totalMemUsage / 1000);
        Unlock();
    }


    // request a new one including lock
    MotionInstance* MotionInstancePool::RequestNew(Motion* motion, ActorInstance* actorInstance)
    {
        Lock();
        MotionInstance* result = RequestNewWithoutLock(motion, actorInstance);
        Unlock();

        return result;
    }


    // free including lock
    void MotionInstancePool::Free(MotionInstance* motionInstance)
    {
        Lock();
        FreeWithoutLock(motionInstance);
        Unlock();
    }


    // wait with execution until we can set the lock
    void MotionInstancePool::Lock()
    {
        mLock.Lock();
    }


    // release the lock again
    void MotionInstancePool::Unlock()
    {
        mLock.Unlock();
    }


    // shrink the pool
    void MotionInstancePool::Shrink()
    {
        Lock();

        for (uint32 i = 0; i < mPool->mSubPools.GetLength(); )
        {
            SubPool* subPool = mPool->mSubPools[i];
            if (subPool->mNumInUse == 0)
            {
                // remove all free allocations
                for (uint32 a = 0; a < mPool->mFreeList.GetLength(); )
                {
                    if (mPool->mFreeList[a].mSubPool == subPool)
                    {
                        mPool->mFreeList.Remove(a);
                    }
                    else
                    {
                        ++a;
                    }
                }
                mPool->mNumInstances -= subPool->mNumInstances;

                mPool->mSubPools.Remove(i);
                delete subPool;
            }
            else
            {
                ++i;
            }
        }

        mPool->mSubPools.Shrink();
        //mPool->mFreeList.Shrink();
        if ((mPool->mFreeList.GetMaxLength() - mPool->mFreeList.GetLength()) > 4096)
        {
            mPool->mFreeList.ReserveExact(mPool->mFreeList.GetLength() + 4096);
        }

        Unlock();
    }
}   // namespace MCore

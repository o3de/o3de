/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ_CLASS_ALLOCATOR_IMPL(MotionInstancePool, MotionAllocator)


    // constructor
    MotionInstancePool::SubPool::SubPool()
        : m_data(nullptr)
        , m_numInstances(0)
        , m_numInUse(0)
    {
    }


    // destructor
    MotionInstancePool::SubPool::~SubPool()
    {
        MCore::Free(m_data);
        m_data = nullptr;
    }


    //--------------------------------------------------------------------------------------------------
    // class MotionInstancePool::Pool
    //--------------------------------------------------------------------------------------------------

    // constructor
    MotionInstancePool::Pool::Pool()
    {
        m_poolType           = POOLTYPE_DYNAMIC;
        m_data               = nullptr;
        m_numInstances       = 0;
        m_numUsedInstances   = 0;
        m_subPoolSize        = 0;
    }


    // destructor
    MotionInstancePool::Pool::~Pool()
    {
        if (m_poolType == POOLTYPE_STATIC)
        {
            MCore::Free(m_data);
            m_data = nullptr;
            m_freeList.clear();
        }
        else
        if (m_poolType == POOLTYPE_DYNAMIC)
        {
            MCORE_ASSERT(m_data == nullptr);

            // delete all subpools
            for (SubPool* subPool : m_subPools)
            {
                delete subPool;
            }
            m_subPools.clear();

            m_freeList.clear();
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
        : MCore::RefCounted()
    {
        m_pool = nullptr;
    }


    // destructor
    MotionInstancePool::~MotionInstancePool()
    {
        if (m_pool->m_numUsedInstances > 0)
        {
            MCore::LogError("EMotionFX::~MotionInstancePool() - There are still %d unfreed motion instances, please use the Free function in the MotionInstancePool to free them, just like you would delete the object.", m_pool->m_numUsedInstances);
        }

        delete m_pool;
    }


    // create
    MotionInstancePool* MotionInstancePool::Create()
    {
        return aznew MotionInstancePool();
    }


    // init the motion instance pool
    void MotionInstancePool::Init(size_t numInitialInstances, EPoolType poolType, size_t subPoolSize)
    {
        if (m_pool)
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
        m_pool = new Pool();
        m_pool->m_numInstances    = numInitialInstances;
        m_pool->m_poolType        = poolType;
        m_pool->m_subPoolSize     = subPoolSize;

        // if we have a static pool
        if (poolType == POOLTYPE_STATIC)
        {
            m_pool->m_data    = (uint8*)MCore::Allocate(numInitialInstances * sizeof(MotionInstance), EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);// alloc space
            m_pool->m_freeList.resize_no_construct(numInitialInstances);
            for (size_t i = 0; i < numInitialInstances; ++i)
            {
                void* memLocation = (void*)(m_pool->m_data + i * sizeof(MotionInstance));
                m_pool->m_freeList[i].m_address = memLocation;
                m_pool->m_freeList[i].m_subPool = nullptr;
            }
        }
        else // if we have a dynamic pool
        if (poolType == POOLTYPE_DYNAMIC)
        {
            m_pool->m_subPools.reserve(32);

            SubPool* subPool = new SubPool();
            subPool->m_data              = (uint8*)MCore::Allocate(numInitialInstances * sizeof(MotionInstance), EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);// alloc space
            subPool->m_numInstances      = numInitialInstances;

            m_pool->m_freeList.resize_no_construct(numInitialInstances);
            for (size_t i = 0; i < numInitialInstances; ++i)
            {
                m_pool->m_freeList[i].m_address = (void*)(subPool->m_data + i * sizeof(MotionInstance));
                m_pool->m_freeList[i].m_subPool = subPool;
            }

            m_pool->m_subPools.emplace_back(subPool);
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
        if (m_pool == nullptr)
        {
            MCore::LogWarning("EMotionFX::MotionInstancePool::RequestNew() - We have not yet initialized the pool, initializing it to a dynamic pool");
            Init();
        }

        // if there is are free items left
        if (m_pool->m_freeList.size() > 0)
        {
            const MemLocation& location = m_pool->m_freeList.back();
            MotionInstance* result = MotionInstance::Create(location.m_address, motion, actorInstance);

            if (location.m_subPool)
            {
                location.m_subPool->m_numInUse++;
            }
            result->SetSubPool(location.m_subPool);

            m_pool->m_freeList.pop_back(); // remove it from the free list
            m_pool->m_numUsedInstances++;
            return result;
        }

        // we have no more free attributes left
        if (m_pool->m_poolType == POOLTYPE_DYNAMIC) // we're dynamic, so we can just create new ones
        {
            const size_t numInstances = m_pool->m_subPoolSize;
            m_pool->m_numInstances += numInstances;

            SubPool* subPool = new SubPool();
            subPool->m_data              = (uint8*)MCore::Allocate(numInstances * sizeof(MotionInstance), EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);// alloc space
            subPool->m_numInstances      = numInstances;

            const size_t startIndex = m_pool->m_freeList.size();
            if (m_pool->m_freeList.capacity() < m_pool->m_numInstances)
            {
                m_pool->m_freeList.reserve(m_pool->m_numInstances + m_pool->m_freeList.capacity() / 2);
            }

            m_pool->m_freeList.resize_no_construct(startIndex + numInstances);
            for (size_t i = 0; i < numInstances; ++i)
            {
                void* memAddress = (void*)(subPool->m_data + i * sizeof(MotionInstance));
                m_pool->m_freeList[i + startIndex].m_address = memAddress;
                m_pool->m_freeList[i + startIndex].m_subPool = subPool;
            }

            m_pool->m_subPools.emplace_back(subPool);

            const MemLocation& location = m_pool->m_freeList.back();
            MotionInstance* result = MotionInstance::Create(location.m_address, motion, actorInstance);
            if (location.m_subPool)
            {
                location.m_subPool->m_numInUse++;
            }
            result->SetSubPool(location.m_subPool);
            m_pool->m_freeList.pop_back(); // remove it from the free list
            m_pool->m_numUsedInstances++;
            return result;
        }
        else // we are static and ran out of free attributes
        if (m_pool->m_poolType == POOLTYPE_STATIC)
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

        if (m_pool == nullptr)
        {
            MCore::LogWarning("EMotionFX::MotionInstancePool::Free() - The pool has not yet been initialized, please call Init first.");
            MCORE_ASSERT(false);
            return;
        }

        // add it back to the free list
        if (motionInstance->GetSubPool())
        {
            motionInstance->GetSubPool()->m_numInUse--;
        }

        m_pool->m_freeList.emplace_back();
        m_pool->m_freeList.back().m_address = motionInstance;
        m_pool->m_freeList.back().m_subPool = motionInstance->GetSubPool();
        m_pool->m_numUsedInstances--;

        motionInstance->DecreaseReferenceCount();
        motionInstance->~MotionInstance(); // call the destructor
    }


    // log the memory stats
    void MotionInstancePool::LogMemoryStats()
    {
        Lock();
        MCore::LogInfo("EMotionFX::MotionInstancePool::LogMemoryStats() - Logging motion instance pool info");

        const size_t numFree    = m_pool->m_freeList.size();
        size_t numUsed          = m_pool->m_numUsedInstances;
        size_t memUsage         = 0;
        size_t usedMemUsage     = 0;
        size_t totalMemUsage    = 0;
        size_t totalUsedInstancesMemUsage = 0;

        if (m_pool->m_poolType == POOLTYPE_STATIC)
        {
            if (m_pool->m_numInstances > 0)
            {
                memUsage = m_pool->m_numInstances * sizeof(MotionInstance);
                usedMemUsage = numUsed * sizeof(MotionInstance);
            }
        }
        else
        if (m_pool->m_poolType == POOLTYPE_DYNAMIC)
        {
            if (m_pool->m_numInstances > 0)
            {
                memUsage = m_pool->m_numInstances * sizeof(MotionInstance);
                usedMemUsage = numUsed * sizeof(MotionInstance);
            }
        }

        totalUsedInstancesMemUsage  += usedMemUsage;
        totalMemUsage += memUsage;
        totalMemUsage += sizeof(Pool);
        totalMemUsage += m_pool->m_freeList.capacity() * sizeof(decltype(m_pool->m_freeList)::value_type);

        MCore::LogInfo("Pool:");
        if (m_pool->m_poolType == POOLTYPE_DYNAMIC)
        {
            MCore::LogInfo("   - Num SubPools:          %d", m_pool->m_subPools.size());
        }
        MCore::LogInfo("   - Num Instances:         %d", m_pool->m_numInstances);
        MCore::LogInfo("   - Num Free:              %d", numFree);
        MCore::LogInfo("   - Num Used:              %d", numUsed);
        MCore::LogInfo("   - PoolType:              %s", (m_pool->m_poolType == POOLTYPE_STATIC) ? "Static" : "Dynamic");
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
        m_lock.Lock();
    }


    // release the lock again
    void MotionInstancePool::Unlock()
    {
        m_lock.Unlock();
    }


    // shrink the pool
    void MotionInstancePool::Shrink()
    {
        Lock();

        for (size_t i = 0; i < m_pool->m_subPools.size(); )
        {
            SubPool* subPool = m_pool->m_subPools[i];
            if (subPool->m_numInUse == 0)
            {
                // remove all free allocations
                for (size_t a = 0; a < m_pool->m_freeList.size(); )
                {
                    if (m_pool->m_freeList[a].m_subPool == subPool)
                    {
                        m_pool->m_freeList.erase(AZStd::next(begin(m_pool->m_freeList), a));
                    }
                    else
                    {
                        ++a;
                    }
                }
                m_pool->m_numInstances -= subPool->m_numInstances;

                m_pool->m_subPools.erase(AZStd::next(begin(m_pool->m_subPools), i));
                delete subPool;
            }
            else
            {
                ++i;
            }
        }

        m_pool->m_subPools.shrink_to_fit();
        if ((m_pool->m_freeList.capacity() - m_pool->m_freeList.size()) > 4096)
        {
            m_pool->m_freeList.reserve(m_pool->m_freeList.size() + 4096);
        }

        Unlock();
    }
}   // namespace MCore

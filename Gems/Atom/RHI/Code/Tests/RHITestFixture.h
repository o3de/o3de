/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Component/TickBus.h>

#include <Atom/RHI.Reflect/Base.h>

namespace UnitTest
{
    inline constexpr bool EnableLeakTracking = false;

    class RHITestFixture
        : public ScopedAllocatorSetupFixture
    {
        AZStd::unique_ptr<AZ::ReflectionManager> m_reflectionManager;

    public:
        RHITestFixture()
        {
            {
                AZ::PoolAllocator::Descriptor desc;

                if constexpr (EnableLeakTracking)
                {
                    desc.m_allocationRecords = true;
                    desc.m_stackRecordLevels = 5;
                    desc.m_isMemoryGuards = true;
                    desc.m_isMarkUnallocatedMemory = true;
                }

                AZ::AllocatorInstance<AZ::PoolAllocator>::Create(desc);
            }

            {
                AZ::ThreadPoolAllocator::Descriptor desc;

                if constexpr (EnableLeakTracking)
                {
                    desc.m_allocationRecords = true;
                    desc.m_stackRecordLevels = 5;
                    desc.m_isMemoryGuards = true;
                    desc.m_isMarkUnallocatedMemory = true;
                }

                AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create(desc);
            }

            if constexpr (EnableLeakTracking)
            {
                AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::GetAllocator().GetRecords();
                if (records)
                {
                    records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
                }
            }
        }


        AZ::SerializeContext* GetSerializeContext()
        {
            return m_reflectionManager ? m_reflectionManager->GetReflectContext<AZ::SerializeContext>() : nullptr;
        }

        virtual ~RHITestFixture()
        {
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        }

        void SetUp() override
        {
            AZ::RHI::Validation::s_isEnabled = true;

            m_reflectionManager = AZStd::make_unique<AZ::ReflectionManager>();
            m_reflectionManager->AddReflectContext<AZ::SerializeContext>();

            AZ::NameDictionary::Create();
        }

        void TearDown() override
        {
            // Flushing the tick bus queue since AZ::RHI::Factory:Register queues a function
            AZ::SystemTickBus::ClearQueuedEvents();

            AZ::NameDictionary::Destroy();

            m_reflectionManager->Clear();
            m_reflectionManager.reset();
        }
    };
}

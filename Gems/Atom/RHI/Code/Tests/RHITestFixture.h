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
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Component/TickBus.h>

#include <Atom/RHI.Reflect/Base.h>

namespace UnitTest
{
    class RHITestFixture
        : public LeakDetectionFixture
    {
        AZStd::unique_ptr<AZ::ReflectionManager> m_reflectionManager;

    public:

        AZ::SerializeContext* GetSerializeContext()
        {
            return m_reflectionManager ? m_reflectionManager->GetReflectContext<AZ::SerializeContext>() : nullptr;
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

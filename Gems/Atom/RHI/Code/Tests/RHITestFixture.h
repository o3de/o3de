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
#include <Atom/RHI/RHISystem.h>
#include <Tests/Device.h>
#include <Tests/Factory.h>

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

    class MultiDeviceRHITestFixture : public RHITestFixture
    {
    public:
        void SetUp() override
        {
            RHITestFixture::SetUp();

            m_factory.reset(aznew Factory());

            m_rhiSystem.reset(aznew AZ::RHI::RHISystem);

            m_rhiSystem->InitDevices(DeviceCount);
            m_rhiSystem->Init();
        }

        void TearDown() override
        {
            m_rhiSystem->Shutdown();
            m_rhiSystem.reset();
            m_factory.reset();

            RHITestFixture::TearDown();
        }

    private:
        AZStd::unique_ptr<AZ::RHI::RHISystem> m_rhiSystem;
        AZStd::unique_ptr<Factory> m_factory;
    };
}

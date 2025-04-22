/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetManagerTestFixture.h"

#include <AzCore/Interface/Interface.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

extern "C" void CleanUpRpiPublicGenericClassInfo();
extern "C" void CleanUpRpiEditGenericClassInfo();

namespace UnitTest
{
    void AssetManagerTestFixture::SetUp()
    {
        using namespace AZ::Data;

        LeakDetectionFixture::SetUp();

        AssetManager::Descriptor desc;
        AssetManager::Create(desc);

        m_reflectionManager = AZStd::make_unique<AZ::ReflectionManager>();
        m_reflectionManager->AddReflectContext<AZ::SerializeContext>();
        m_reflectionManager->AddReflectContext<AZ::BehaviorContext>();

        AZ::ComponentApplicationBus::Handler::BusConnect();
        AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
    }

    void AssetManagerTestFixture::TearDown()
    {
        AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
        AZ::ComponentApplicationBus::Handler::BusDisconnect();
        using namespace AZ::Data;

        AZ::Data::AssetManager::Destroy();

        m_reflectionManager->Clear();
        m_reflectionManager.reset();

        CleanUpRpiPublicGenericClassInfo();
        CleanUpRpiEditGenericClassInfo();

        LeakDetectionFixture::TearDown();
    }

    AZ::BehaviorContext* AssetManagerTestFixture::GetBehaviorContext()
    {
        return m_reflectionManager ? m_reflectionManager->GetReflectContext<AZ::BehaviorContext>() : nullptr;
    }

    AZ::SerializeContext* AssetManagerTestFixture::GetSerializeContext()
    {
        return m_reflectionManager ? m_reflectionManager->GetReflectContext<AZ::SerializeContext>() : nullptr;
    }
}


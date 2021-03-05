/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "AssetManagerTestFixture.h"
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace UnitTest
{
    void AssetManagerTestFixture::SetUp()
    {
        using namespace AZ::Data;

        AllocatorsTestFixture::SetUp();

        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        AssetManager::Descriptor desc;
        AssetManager::Create(desc);

        m_reflectionManager = AZStd::make_unique<AZ::ReflectionManager>();
        m_reflectionManager->AddReflectContext<AZ::SerializeContext>();

        AZ::ComponentApplicationBus::Handler::BusConnect();
    }

    void AssetManagerTestFixture::TearDown()
    {
        AZ::ComponentApplicationBus::Handler::BusDisconnect();
        using namespace AZ::Data;

        AZ::Data::AssetManager::Destroy();

        m_reflectionManager->Clear();
        m_reflectionManager.reset();

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

        AllocatorsTestFixture::TearDown();
    }
}


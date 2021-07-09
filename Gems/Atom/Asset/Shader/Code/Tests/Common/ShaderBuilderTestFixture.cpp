/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShaderBuilderTestFixture.h"

#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Name/NameDictionary.h>

namespace UnitTest
{
    void ShaderBuilderTestFixture::SetUp()
    {
        AllocatorsTestFixture::SetUp();

        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        AZ::NameDictionary::Create();
    }

    void ShaderBuilderTestFixture::TearDown()
    {
        AZ::NameDictionary::Destroy();

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

        AllocatorsTestFixture::TearDown();
    }

}


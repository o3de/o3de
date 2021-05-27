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


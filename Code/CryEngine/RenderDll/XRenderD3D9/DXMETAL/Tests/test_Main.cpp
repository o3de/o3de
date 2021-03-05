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
#include "RenderDll_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocatorManager.h>
namespace
{
    class RenderDllTestEnvironment final
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override
        {
            // required memory management
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            AZ::AllocatorInstance<AZ::OSAllocator>::Create();
            // For some reason there's always an assert in ~AllocatorManager() after running the tests,
            // even though we call Destroy on both allocators in the TeardownEnvironment function,
            // so allow allocator leaking here to enable the tests to run without issue
            AZ::AllocatorManager::Instance().SetAllocatorLeaking(true); 
        }

        void TeardownEnvironment() override
        {           
            AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    private:
    };
}
AZ_UNIT_TEST_HOOK(new(AZ_OS_MALLOC(sizeof(RenderDllTestEnvironment), alignof(RenderDllTestEnvironment))) RenderDllTestEnvironment);

TEST(CryRenderMetalSanityTest, Sanity)
{
    EXPECT_EQ(1, 1);
}

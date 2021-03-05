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
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocatorManager.h>
namespace
{
    class RenderDllTestEnvironment final
        : public AZ::Test::ITestEnvironment
        , UnitTest::TraceBusRedirector // provide AZ_TEST_START_TRACE_SUPPRESSION
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(RenderDllTestEnvironment);

    protected:
        void SetupEnvironment() override
        {
             AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TeardownEnvironment() override
        {           
             AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }
    private:
    };
}
AZ_UNIT_TEST_HOOK(new RenderDllTestEnvironment);

TEST(CryRenderGL, Sanity)
{
    EXPECT_EQ(1, 1);
}

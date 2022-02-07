/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include "Tests.h"

// set up the memory allocators and assert interceptor
struct GridMateTestEnvironment
    : public AZ::Test::ITestEnvironment
    , public AZ::Debug::TraceMessageBus::Handler
{
    void SetupEnvironment() final
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Create();
        BusConnect();
    }
    void TeardownEnvironment() final
    {
        BusDisconnect();
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }
    bool OnAssert(const char* message) override
    {
        (void)message;
        AZ_TEST_ASSERT(false); // just forward
        return true;
    }
};

AZ_UNIT_TEST_HOOK(new GridMateTestEnvironment());

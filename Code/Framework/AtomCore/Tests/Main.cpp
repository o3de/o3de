/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/typetraits/typetraits.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzTest/AzTest.h>

#include <AzCore/Memory/OSAllocator.h>

DECLARE_AZ_UNIT_TEST_MAIN()

namespace AZ
{
    inline void* AZMemAlloc(AZStd::size_t byteSize, AZStd::size_t alignment, const char* name = "No name allocation")
    {
        (void)name;
        return AZ_OS_MALLOC(byteSize, alignment);
    }

    inline void AZFree(void* ptr, AZStd::size_t byteSize = 0, AZStd::size_t alignment = 0)
    {
        (void)byteSize;
        (void)alignment;
        AZ_OS_FREE(ptr);
    }
}
// END OF TEMP MEMORY ALLOCATIONS

using namespace AZ;

// Handle asserts
class TraceDrillerHook
    : public AZ::Test::ITestEnvironment
    , public UnitTest::TraceBusRedirector
{
public:
    void SetupEnvironment() override
    {
        AllocatorInstance<OSAllocator>::Create(); // used by the bus

        BusConnect();
    }

    void TeardownEnvironment() override
    {
        BusDisconnect();

        AllocatorInstance<OSAllocator>::Destroy(); // used by the bus
    }
};

AZ_UNIT_TEST_HOOK(new TraceDrillerHook());


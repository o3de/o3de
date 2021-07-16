/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/OverrunDetectionAllocator.h>

// Testing overruns with the debugger will cause breaks to occur. You can disable it with this macro.
//
// NOTE: This test currently requires compiling the project with structured exceptions /EHa which
// requires disabling a warning /wd4714. This would be better served with EXPECT_DEATH, except 
// asserts currently get in the way of this working.
#ifndef OVERRUN_DETECTION_TEST_OVERRUNS
#   define OVERRUN_DETECTION_TEST_OVERRUNS  0
#endif

#if OVERRUN_DETECTION_TEST_OVERRUNS
#   include <eh.h>
#endif


using namespace AZ;
using namespace AZ::Debug;

namespace UnitTest
{
    class OverrunDetectionAllocatorTest : public ::testing::Test
    {
    public:
        void RunTests()
        {
            AllocatorInstance<OverrunDetectionAllocator>::Create();
            auto& overrunAllocator = AllocatorInstance<OverrunDetectionAllocator>::Get();
            static const AZStd::pair<size_t, size_t> sizeAndAlignments[] =
            {
                { 16, 8 },
                { 15, 1 },
                { 17, 1 },
                { 1024, 16 },
                { 1023, 1 },
                { 1025, 1 },
                { 65536, 16 },
                { 65535, 1 },
                { 65537, 1 }
            };

            for (const auto& sizeAndAlignment : sizeAndAlignments)
            {
                const size_t size = sizeAndAlignment.first;
                const size_t alignment = sizeAndAlignment.second;
                void* p = overrunAllocator.Allocate(size, alignment, 0);

                // Touch every byte of the allocated memory
                memset(p, 0xAB, size);

#if OVERRUN_DETECTION_TEST_OVERRUNS
                // Attempt an overrun
                auto original = _set_se_translator([](unsigned int u, EXCEPTION_POINTERS *)
                {
                    GTEST_ASSERT_EQ(u, 0xC0000005);
                    throw std::exception("Access violation");
                });

                auto overrunFn = [p, size]()
                {
                    try
                    {
                        // If you have a debugger attached it will halt here. You can either choose Continue
                        // or disable this test by setting OVERRUN_DETECTION_TEST_OVERRUNS to 0.
                        *((char *)p + size) = 0x01;
                    }
                    catch (...)
                    {
                        return true;
                    }

                    return false;
                };

                EXPECT_TRUE(overrunFn());
                _set_se_translator(original);
#endif

                overrunAllocator.DeAllocate(p);
            }

            AllocatorInstance<OverrunDetectionAllocator>::Destroy();
        }
    };


    TEST_F(OverrunDetectionAllocatorTest, Test)
    {
        RunTests();
    }
}

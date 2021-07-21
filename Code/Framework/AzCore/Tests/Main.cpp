/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>


#if defined(HAVE_BENCHMARK)

namespace AzCore
{
    class AzCoreBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
    {
        void SetUpBenchmark() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }
        void TearDownBenchmark() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV, AzCore::AzCoreBenchmarkEnvironment)

#else

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

#endif // HAVE_BENCHMARK

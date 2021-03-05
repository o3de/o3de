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

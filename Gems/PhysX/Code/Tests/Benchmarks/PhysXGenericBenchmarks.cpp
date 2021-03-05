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

#ifdef HAVE_BENCHMARK

#include <AzTest/AzTest.h>
#include <Physics/PhysicsTests.h>

namespace PhysX::Benchmarks
{
    //! Used to measure how much overhead fixture adds to benchmark runs.
    class BenchmarkablePhysXBenchmarkFixture
        : public Physics::GenericPhysicsFixture
    {
    public:
        void SetUp()
        {
            Physics::GenericPhysicsFixture::SetUpInternal();
        }

        void TearDown()
        {
            Physics::GenericPhysicsFixture::TearDownInternal();
        }
    };

    void BM_PhysXBenchmarkFixture(benchmark::State& state)
    {
        for (auto _ : state)
        {
            auto fixture = std::make_unique<BenchmarkablePhysXBenchmarkFixture>();
            fixture->SetUp();
            fixture->TearDown();
            benchmark::DoNotOptimize(*fixture);
        }
    }

    BENCHMARK(BM_PhysXBenchmarkFixture)->Unit(benchmark::kMillisecond);
}
#endif //HAVE_BENCHMARK

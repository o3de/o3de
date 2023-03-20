/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <AzTest/AzTest.h>
#include <Tests/PhysXGenericTestFixture.h>

namespace PhysX::Benchmarks
{
    //! Used to measure how much overhead fixture adds to benchmark runs.
    class BenchmarkablePhysXBenchmarkFixture
        : public PhysX::GenericPhysicsFixture
    {
    public:
        void SetUp()
        {
            PhysX::GenericPhysicsFixture::SetUpInternal();
        }

        void TearDown()
        {
            PhysX::GenericPhysicsFixture::TearDownInternal();
        }
    };

    void BM_PhysXBenchmarkFixture(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
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

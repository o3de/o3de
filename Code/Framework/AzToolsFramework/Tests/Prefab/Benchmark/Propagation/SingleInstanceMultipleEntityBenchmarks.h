/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#pragma once

#include <Prefab/Benchmark/Propagation/PropagationBenchmarkFixture.h>

namespace Benchmark
{
    //! This class captures benchmarks for propagating changes to a single prefab instance with multiple entities that are side-by-side.
    class SingleInstanceMultipleEntityBenchmarks : public PropagationBenchmarkFixture
    {
    protected:
        void SetupHarness(const benchmark::State& state) override;
        void TeardownHarness(const benchmark::State& state) override;
    };
} // namespace Benchmark
#endif

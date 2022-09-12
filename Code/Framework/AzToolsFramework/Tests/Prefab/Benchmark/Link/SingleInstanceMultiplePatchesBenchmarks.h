/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#pragma once

#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>

namespace Benchmark
{
    //! This class captures benchmarks around CRUD operations on patches to a single prefab instance with multiple entities that are side-by-side.
    class SingleInstanceMultiplePatchesBenchmarks : public BM_Prefab
    {
    protected:
        void SetupHarness(const benchmark::State& state) override;
        void TeardownHarness(const benchmark::State& state) override;

        AZStd::unique_ptr<PrefabDom> m_linkDomToSet;
        AZStd::unique_ptr<Instance> m_parentInstance;
        LinkId m_linkId;
    };
} // namespace Benchmark
#endif

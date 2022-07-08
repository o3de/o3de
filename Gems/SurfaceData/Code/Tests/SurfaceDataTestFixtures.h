/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/GemTestEnvironment.h>

namespace UnitTest
{
    // SurfaceData needs to use the GemTestEnvironment to load the LmbrCentral Gem so that Shape components can be used
    // in the unit tests and benchmarks.
    class SurfaceDataTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    public:
        void AddGemsAndComponents() override;
    };

#ifdef HAVE_BENCHMARK
    //! The Benchmark environment is used for one time setup and tear down of shared resources
    class SurfaceDataBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
        , public SurfaceDataTestEnvironment

    {
    protected:
        void SetUpBenchmark() override
        {
            SetupEnvironment();
        }

        void TearDownBenchmark() override
        {
            TeardownEnvironment();
        }
    };
#endif

}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/GemTestEnvironment.h>
#include <FastNoiseGradientComponent.h>
#include <FastNoiseSystemComponent.h>

namespace UnitTest
{
    // The FastNoise unit tests need to use the GemTestEnvironment to load the GradientSignal and LmbrCentral Gems so that
    // GradientTransform components  can be used in the unit tests and benchmarks.
    class FastNoiseTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    public:
        void AddGemsAndComponents() override
        {
            AddDynamicModulePaths({ "GradientSignal", "LmbrCentral" });

            AddComponentDescriptors({
                AzFramework::TransformComponent::CreateDescriptor(),
                FastNoiseGem::FastNoiseSystemComponent::CreateDescriptor(),
                FastNoiseGem::FastNoiseGradientComponent::CreateDescriptor()
            });

            AddRequiredComponents({ FastNoiseGem::FastNoiseSystemComponent::TYPEINFO_Uuid() });
        }
    };

#ifdef HAVE_BENCHMARK
    //! The Benchmark environment is used for one time setup and tear down of shared resources
    class FastNoiseBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
        , public FastNoiseTestEnvironment

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


} // namespace UnitTest


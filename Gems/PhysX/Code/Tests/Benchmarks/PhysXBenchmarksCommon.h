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
#pragma once

#ifdef HAVE_BENCHMARK

#include <AzTest/AzTest.h>
#include <PhysXTestEnvironment.h>

#include <AzFramework/Physics/SystemBus.h>

namespace PhysX::Benchmarks
{
    static constexpr float DefaultTimeStep = 0.0166667f; // 0.0166667f (60fps)

    //! The Benchmark environment is used for one time setup and tear down of shared resources
    class PhysXBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
        , public PhysX::Environment

    {
    public:
        ~PhysXBenchmarkEnvironment();

    protected:
        void SetUpBenchmark() override;
        void TearDownBenchmark() override;
    };

    //! Base Fixture for running physX benchmarks
    class PhysXBaseBenchmarkFixture
        : public benchmark::Fixture
        , protected Physics::DefaultWorldBus::Handler
    {
    public:
        // Physics::DefaultWorldBus::Handler Interface -------------
        AzPhysics::SceneHandle GetDefaultSceneHandle() const override;
        // Physics::DefaultWorldBus::Handler Interface -------------

        //! Run the simulation for a set number of frames. This will execute as each frame as quickly as possible
        //! @param numFrames - The number of 'game' frames to run the simulation
        //! @param timeStep - The frame time of the 'game' frame. Default - 0.0166667f (60fps)
        void UpdateSimulation(unsigned int numFrames, float timeStep = DefaultTimeStep);

        void StepScene1Tick(float timeStep = DefaultTimeStep);
    protected:
        void SetUpInternal();
        void TearDownInternal();

        //! allows each fixture to setup and define the default World Config
        virtual AzPhysics::SceneConfiguration GetDefaultSceneConfiguration() = 0;

        //! Creates the default scene
        //!     - Calls GetWorldEventHandler() to attached an event handle if provided by the fixture
        AzPhysics::SceneHandle CreateDefaultTestScene();

        AzPhysics::Scene* m_defaultScene = nullptr;
        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        AZ::ComponentDescriptor* m_dummyTerrainComponentDescriptor = nullptr;
    };
} // namespace PhysX::Benchmarks
#endif //HAVE_BENCHMARK

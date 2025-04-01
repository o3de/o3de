/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#ifdef HAVE_BENCHMARK

#include <AzTest/AzTest.h>
#include <PhysXTestEnvironment.h>

#include <AzFramework/Physics/SystemBus.h>

namespace PhysX::Benchmarks
{
    inline constexpr float DefaultTimeStep = 0.0166667f; // 0.0166667f (60fps)

    //! Rigid body benchmark types: create entities with rigid body component or rigid bodies with no entities
    inline constexpr int RigidBodyApiObject = 0;
    inline constexpr int RigidBodyEntity = 1;

    //! The Benchmark environment is used for one time setup and tear down of shared resources
    class PhysXBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
        , public PhysX::Environment

    {
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
    };
} // namespace PhysX::Benchmarks
#endif //HAVE_BENCHMARK

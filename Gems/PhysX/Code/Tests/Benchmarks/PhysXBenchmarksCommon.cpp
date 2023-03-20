/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <Benchmarks/PhysXBenchmarksCommon.h>
#include <PhysXTestUtil.h>
#include <PhysXTestCommon.h>
#include <Scene/PhysXScene.h>

namespace PhysX::Benchmarks
{
    void PhysXBenchmarkEnvironment::SetUpBenchmark()
    {
        PhysX::Environment::SetupInternal();
    }
    void PhysXBenchmarkEnvironment::TearDownBenchmark()
    {
        PhysX::Environment::TeardownInternal();
    }

    AzPhysics::SceneHandle PhysXBaseBenchmarkFixture::GetDefaultSceneHandle() const
    {
        return m_testSceneHandle;
    }

    void PhysXBaseBenchmarkFixture::UpdateSimulation(unsigned int numFrames, float timeStep /*= DefaultTimeStep*/)
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            for (AZ::u32 i = 0; i < numFrames; i++)
            {
                physicsSystem->Simulate(timeStep);
            }
        }
    }

    void PhysXBaseBenchmarkFixture::StepScene1Tick(float timeStep /*= DefaultTimeStep*/)
    {
        m_defaultScene->StartSimulation(timeStep);
        m_defaultScene->FinishSimulation();
        static_cast<PhysX::PhysXScene*>(m_defaultScene)->FlushTransformSync();
    }

    void PhysXBaseBenchmarkFixture::SetUpInternal()
    {
        m_testSceneHandle = CreateDefaultTestScene(); //create the default scene
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            m_defaultScene = physicsSystem->GetScene(m_testSceneHandle);
        }

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void PhysXBaseBenchmarkFixture::TearDownInternal()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();

        //Clean up the test scene
        m_defaultScene = nullptr;
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
        m_testSceneHandle = AzPhysics::InvalidSceneHandle;

        TestUtils::ResetPhysXSystem();
    }

    AzPhysics::SceneHandle PhysXBaseBenchmarkFixture::CreateDefaultTestScene()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = GetDefaultSceneConfiguration();
            sceneConfiguration.m_sceneName = "BenchmarkWorld";
            AzPhysics::SceneHandle sceneHandle = physicsSystem->AddScene(sceneConfiguration);
            return sceneHandle;
        }
        return AzPhysics::InvalidSceneHandle;
    }
} //namespace PhysX::Benchmarks
#endif //HAVE_BENCHMARK

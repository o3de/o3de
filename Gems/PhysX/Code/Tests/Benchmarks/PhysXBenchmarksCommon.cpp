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

#include <Benchmarks/PhysXBenchmarksCommon.h>
#include <Material.h>
#include <PhysXTestUtil.h>
#include <PhysXTestCommon.h>

namespace PhysX::Benchmarks
{
    PhysXBenchmarkEnvironment::~PhysXBenchmarkEnvironment()
    {
        //within our scene queries we use thread_locals, as a result the allocator needs to be around until the module is cleaned up.
        //having the allocator cleaned up here rather then in TeardownInternal() allows it to be around long enough to clean up resource nicely.
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

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
    }

    void PhysXBaseBenchmarkFixture::SetUpInternal()
    {
        m_testSceneHandle = CreateDefaultTestScene(); //create the default scene
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            m_defaultScene = physicsSystem->GetScene(m_testSceneHandle);
        }

        m_dummyTerrainComponentDescriptor = DummyTestTerrainComponent::CreateDescriptor();
        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void PhysXBaseBenchmarkFixture::TearDownInternal()
    {
        //cleanup materials in case some where created
        PhysX::MaterialManagerRequestsBus::Broadcast(&PhysX::MaterialManagerRequestsBus::Events::ReleaseAllMaterials);
        Physics::DefaultWorldBus::Handler::BusDisconnect();

        //Clean up the test scene
        m_defaultScene = nullptr;
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
        m_testSceneHandle = AzPhysics::InvalidSceneHandle;

        TestUtils::ResetPhysXSystem();
        
        m_dummyTerrainComponentDescriptor->ReleaseDescriptor();
        m_dummyTerrainComponentDescriptor = nullptr;
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

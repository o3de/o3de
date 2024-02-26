/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/PhysXTestFixtures.h>
#include <Tests/PhysXTestUtil.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>

//hook in the test environments for Gem::PhysX.Tests
#include <PhysXTestEnvironment.h>

#if HAVE_BENCHMARK

#include <Benchmarks/PhysXBenchmarksCommon.h>

AZ_UNIT_TEST_HOOK(new PhysX::TestEnvironment, PhysX::Benchmarks::PhysXBenchmarkEnvironment)

#else

AZ_UNIT_TEST_HOOK(new PhysX::TestEnvironment)

#endif // HAVE_BENCHMARK

namespace PhysX
{
    void PhysXDefaultWorldTest::SetUp() 
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_sceneName = AzPhysics::DefaultPhysicsSceneName;
            m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            m_defaultScene = physicsSystem->GetScene(m_testSceneHandle);
        }

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void PhysXDefaultWorldTest::TearDown()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultScene = nullptr;

        //Clean up the Test scene
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
        m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        TestUtils::ResetPhysXSystem();
    }
    
    AzPhysics::SceneHandle PhysXDefaultWorldTest::GetDefaultSceneHandle() const
    {
        return m_testSceneHandle;
    }
}

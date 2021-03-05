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

#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AzPhysics
{
    class Scene;
}

namespace PhysX
{
    //! PhysXDefaultWorldTest is a test fixture which creates a default world,
    //! and implements common default world behavior.
    class PhysXDefaultWorldTest
        : public ::testing::Test
        , protected Physics::DefaultWorldBus::Handler
        , protected Physics::WorldEventHandler
    {
    protected:
        void SetUp() override;
        void TearDown() override;

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override;

        // WorldEventHandler
        void OnTriggerEnter(const Physics::TriggerEvent& triggerEvent) override;
        void OnTriggerExit(const Physics::TriggerEvent& triggerEvent) override;

        void OnCollisionBegin(const Physics::CollisionEvent& event);
        void OnCollisionPersist(const Physics::CollisionEvent& event);
        void OnCollisionEnd(const Physics::CollisionEvent& event);

        AZ::ComponentDescriptor* m_dummyTerrainComponentDescriptor = nullptr;
        AzPhysics::Scene* m_defaultScene = nullptr;
        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;
    };

    //! Extension of PhysXDefaultWorldTest to support parameterized tests.
    template <typename T>
    class PhysXDefaultWorldTestWithParam
        : public PhysXDefaultWorldTest
        , public ::testing::WithParamInterface<T>
    {
    };
}

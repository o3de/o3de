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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Physics/PhysicsTests.h>

#include <System/PhysXSystem.h>

namespace PhysXEditorTests
{
    using EntityPtr = AZStd::unique_ptr<AZ::Entity>;

    //! Creates a default editor entity in an inactive state.
    EntityPtr CreateInactiveEditorEntity(const char* entityName);

    //! Creates and activates a game entity from an editor entity.
    EntityPtr CreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity);

    //! Class used for loading system components from this gem.
    class PhysXEditorSystemComponentEntity
        : public AZ::Entity
    {
        friend class PhysXEditorFixture;
    };

    //! Test fixture which creates a tools application, loads the PhysX runtime gem and creates a default physics world.
    //! The application is created for the whole test case, rather than individually for each test, due to a known
    //! problem with buses when repeatedly loading and unloading gems. A new default world is created for each test.
    class PhysXEditorFixture
        : public UnitTest::AllocatorsTestFixture
        , public Physics::DefaultWorldBus::Handler
        , public Physics::WorldEventHandler
    {
    public:
        void SetUp() override;
        void TearDown() override;

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override;

        // WorldEventHandler
        void OnTriggerEnter(const Physics::TriggerEvent& triggerEvent) override;
        void OnTriggerExit(const Physics::TriggerEvent& triggerEvent) override;
        void OnCollisionBegin(const Physics::CollisionEvent& event) override;
        void OnCollisionPersist(const Physics::CollisionEvent& event) override;
        void OnCollisionEnd(const Physics::CollisionEvent& event) override;
       
        AZ::ComponentDescriptor* m_dummyTerrainComponentDescriptor = nullptr;
        AzPhysics::SceneHandle m_defaultSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::Scene* m_defaultScene = nullptr;

        // workaround for parameterized tests causing issues with this (and any derived) fixture
        void ValidateInvalidEditorShapeColliderComponentParams(float radius, float height);
    };
} // namespace PhysXEditorTests

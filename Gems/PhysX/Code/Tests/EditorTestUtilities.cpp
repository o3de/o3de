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

#include "PhysX_precompiled.h"

#include <Tests/EditorTestUtilities.h>

#include <EditorShapeColliderComponent.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <PhysX/PhysXLocks.h>
#include <RigidBodyStatic.h>
#include <StaticRigidBodyComponent.h>
#include <Tests/PhysXTestUtil.h>
#include <AzFramework/Physics/World.h>

#include <System/PhysXCookingParams.h>
#include <Tests/PhysXTestCommon.h>
#include <PhysXCharacters/Components/EditorCharacterControllerComponent.h>

namespace PhysXEditorTests
{
    EntityPtr CreateInactiveEditorEntity(const char* entityName)
    {
        AZ::Entity* entity = nullptr;
        UnitTest::CreateDefaultEditorEntity(entityName, &entity);
        entity->Deactivate();

        return AZStd::unique_ptr<AZ::Entity>(entity);
    }

    EntityPtr CreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity)
    {
        EntityPtr gameEntity = AZStd::make_unique<AZ::Entity>();
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::PreExportEntity, *editorEntity, *gameEntity);
        gameEntity->Init();
        gameEntity->Activate();
        return gameEntity;
    }

    void PhysXEditorFixture::SetUp()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            //in case a test modifies the default world config setup a config without getting the default(eg. SetWorldConfiguration_ForwardsConfigChangesToWorldRequestBus)
            AzPhysics::SceneConfiguration sceneConfiguration;
            sceneConfiguration.m_legacyId = Physics::DefaultPhysicsWorldId;
            m_defaultSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            if (m_defaultScene = physicsSystem->GetScene(m_defaultSceneHandle))
            {
                m_defaultScene->GetLegacyWorld()->SetEventHandler(this);
            }
        }
        Physics::DefaultWorldBus::Handler::BusConnect();
        m_dummyTerrainComponentDescriptor = PhysX::DummyTestTerrainComponent::CreateDescriptor();
    }

    void PhysXEditorFixture::TearDown()
    {
        m_dummyTerrainComponentDescriptor->ReleaseDescriptor();
        m_dummyTerrainComponentDescriptor = nullptr;

        Physics::DefaultWorldBus::Handler::BusDisconnect();

        // prevents warnings from the undo cache on subsequent tests
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::FlushUndo);

        //cleanup the created default scene. cannot remove all currently as the editor system component creates a editor scene that is never recreated.
        m_defaultScene = nullptr;
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_defaultSceneHandle);
        }
    }

    // DefaultWorldBus
    AZStd::shared_ptr<Physics::World> PhysXEditorFixture::GetDefaultWorld()
    {
        return m_defaultScene->GetLegacyWorld();
    }

    // WorldEventHandler
    void PhysXEditorFixture::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(),
            &Physics::TriggerNotifications::OnTriggerEnter, triggerEvent);
    }

    void PhysXEditorFixture::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(),
            &Physics::TriggerNotifications::OnTriggerExit, triggerEvent);
    }

    void PhysXEditorFixture::OnCollisionBegin(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionBegin, event);
    }

    void PhysXEditorFixture::OnCollisionPersist(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionPersist, event);
    }

    void PhysXEditorFixture::OnCollisionEnd(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionEnd, event);
    }

    void PhysXEditorFixture::ValidateInvalidEditorShapeColliderComponentParams([[maybe_unused]] const float radius, [[maybe_unused]] const float height)
    {
        // create an editor entity with a shape collider component and a cylinder shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorCylinderShapeComponentTypeId);
        editorEntity->Activate();
        
        {
            UnitTest::ErrorHandler warningHandler("Negative or zero cylinder dimensions are invalid");
            LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
                &LmbrCentral::CylinderShapeComponentRequests::SetRadius, radius);
        
            // expect a warning if the radius is invalid
            int expectedWarningCount = radius <= 0.f ? 1 : 0;
            EXPECT_EQ(warningHandler.GetWarningCount(), expectedWarningCount);
        }
        
        {
            UnitTest::ErrorHandler warningHandler("Negative or zero cylinder dimensions are invalid");
            LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
                &LmbrCentral::CylinderShapeComponentRequests::SetHeight, height);
        
            // expect a warning if the radius or height is invalid
            int expectedWarningCount = radius <= 0.f || height <= 0.f ? 1 : 0;
            EXPECT_EQ(warningHandler.GetWarningCount(), expectedWarningCount);
        }

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        
        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetStaticRigidBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());
        
        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());
        
        // there should be no shapes on the rigid body because the cylinder radius and/or height is invalid
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 0);
    }
} // namespace PhysXEditorTests

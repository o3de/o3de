/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/EditorTestUtilities.h>

#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <EditorShapeColliderComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysXCharacters/Components/EditorCharacterControllerComponent.h>
#include <RigidBodyStatic.h>
#include <StaticRigidBodyComponent.h>
#include <System/PhysXCookingParams.h>
#include <Tests/PhysXTestCommon.h>
#include <Tests/PhysXTestUtil.h>

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

    EntityPtr CreateBoxShapeColliderEditorEntity(
        const AZ::Transform& transform,
        const AZ::Vector3& nonUniformScale,
        const AZ::Vector3& boxDimensions,
        const AZ::Vector3& translationOffset)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        AZ::NonUniformScaleRequestBus::Event(editorEntityId, &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::ShapeComponentRequests::SetTranslationOffset, translationOffset);

        return editorEntity;
    }

    EntityPtr CreateCapsuleShapeColliderEditorEntity(
        const AZ::Transform& transform, float radius, float height, const AZ::Vector3& translationOffset)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorCapsuleShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::CapsuleShapeComponentRequests::SetRadius, radius);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::CapsuleShapeComponentRequests::SetHeight, height);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::ShapeComponentRequests::SetTranslationOffset, translationOffset);

        return editorEntity;
    }

    EntityPtr CreateSphereShapeColliderEditorEntity(const AZ::Transform& transform, float radius, const AZ::Vector3& translationOffset)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorSphereShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::SphereShapeComponentRequestsBus::Event(editorEntityId, &LmbrCentral::SphereShapeComponentRequests::SetRadius, radius);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::ShapeComponentRequests::SetTranslationOffset, translationOffset);

        return editorEntity;
    }

    AZ::Aabb GetSimulatedBodyAabb(AZ::EntityId entityId)
    {
        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, entityId, &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        if (simulatedBody)
        {
            const auto* pxActor = static_cast<const physx::PxActor*>(simulatedBody->GetNativePointer());
            PHYSX_SCENE_READ_LOCK(pxActor->getScene());
            return PxMathConvert(pxActor->getWorldBounds(1.0f));
        }
        return AZ::Aabb::CreateNull();
    }

    void PhysXEditorFixture::SetUp()
    {
        UnitTest::RegistryTestHelper::SetUp(LmbrCentral::ShapeComponentTranslationOffsetEnabled, true);

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            //in case a test modifies the default world config setup a config without getting the default(eg. SetWorldConfiguration_ForwardsConfigChangesToWorldRequestBus)
            AzPhysics::SceneConfiguration sceneConfiguration;
            sceneConfiguration.m_sceneName = AzPhysics::DefaultPhysicsSceneName;
            m_defaultSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            m_defaultScene = physicsSystem->GetScene(m_defaultSceneHandle);
        }
        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void PhysXEditorFixture::TearDown()
    {
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

        UnitTest::RegistryTestHelper::TearDown();
    }

    void PhysXEditorFixture::ConnectToPVD()
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (debug)
        {
            debug->ConnectToPvd();
        }
    }

    void PhysXEditorFixture::DisconnectFromPVD()
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (debug)
        {
            debug->DisconnectFromPvd();
        }
    }

    // DefaultWorldBus
    AzPhysics::SceneHandle PhysXEditorFixture::GetDefaultSceneHandle() const
    {
        return m_defaultSceneHandle;
    }

    void PhysXEditorFixture::ValidateInvalidEditorShapeColliderComponentParams([[maybe_unused]] const float radius, [[maybe_unused]] const float height)
    {
        // create an editor entity with a shape collider component and a cylinder shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorCylinderShapeComponentTypeId);
        editorEntity->Activate();

        {
            UnitTest::ErrorHandler dimensionWarningHandler("Negative or zero cylinder dimensions are invalid");
            UnitTest::ErrorHandler colliderWarningHandler("No Collider or Shape information found when creating Rigid body");
            LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
                &LmbrCentral::CylinderShapeComponentRequests::SetRadius, radius);
        
            // expect 2 warnings
                //1 if the radius is invalid
                //2 when re-creating the underlying simulated body
            int expectedWarningCount = radius <= 0.f ? 1 : 0;
            EXPECT_EQ(dimensionWarningHandler.GetExpectedWarningCount(), expectedWarningCount);
            EXPECT_EQ(colliderWarningHandler.GetExpectedWarningCount(), expectedWarningCount);
        }

        {
            UnitTest::ErrorHandler dimensionWarningHandler("Negative or zero cylinder dimensions are invalid");
            UnitTest::ErrorHandler colliderWarningHandler("No Collider or Shape information found when creating Rigid body");
            LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
                &LmbrCentral::CylinderShapeComponentRequests::SetHeight, height);
        
            // expect 2 warnings
                //1 if the radius or height is invalid
                //2 when re-creating the underlying simulated body
            int expectedWarningCount = radius <= 0.f || height <= 0.f ? 1 : 0;
            EXPECT_EQ(dimensionWarningHandler.GetExpectedWarningCount(), expectedWarningCount);
            EXPECT_EQ(colliderWarningHandler.GetExpectedWarningCount(), expectedWarningCount);
        }

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be no shapes on the rigid body because the cylinder radius and/or height is invalid
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 0);
    }
} // namespace PhysXEditorTests

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
#include <EditorColliderComponent.h>
#include <EditorMeshColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorStaticRigidBodyComponent.h>
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
    // This function reactivates the entity to cause the simulated body to be recreated.
    // This is necessary when modifying properties that affect a dynamic rigid body,
    // because it will delay applying the changes until the next simulation tick,
    // which happens automatically in the editor but not in test environment.
    static void ForceSimulatedBodyRecreation(AZ::Entity& entity)
    {
        entity.Deactivate();
        entity.Activate();
    }

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
        const AZ::Vector3& boxDimensions,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::ShapeComponentRequests::SetTranslationOffset, translationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    EntityPtr CreateCapsuleShapeColliderEditorEntity(
        float radius,
        float height,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorCapsuleShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::CapsuleShapeComponentRequests::SetRadius, radius);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::CapsuleShapeComponentRequests::SetHeight, height);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::ShapeComponentRequests::SetTranslationOffset, translationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    EntityPtr CreateSphereShapeColliderEditorEntity(
        float radius,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorSphereShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::SphereShapeComponentRequestsBus::Event(editorEntityId, &LmbrCentral::SphereShapeComponentRequests::SetRadius, radius);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            editorEntityId, &LmbrCentral::ShapeComponentRequests::SetTranslationOffset, translationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    EntityPtr CreateBoxPrimitiveColliderEditorEntity(
        const AZ::Vector3& boxDimensions,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        const AZ::Quaternion& rotationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Box);

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetBoxDimensions, boxDimensions);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, translationOffset);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, rotationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    EntityPtr CreateCapsulePrimitiveColliderEditorEntity(
        float radius,
        float height,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        const AZ::Quaternion& rotationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Capsule);

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCapsuleRadius, radius);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCapsuleHeight, height);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, translationOffset);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, rotationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    EntityPtr CreateSpherePrimitiveColliderEditorEntity(
        float radius,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        const AZ::Quaternion& rotationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Sphere);

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetSphereRadius, radius);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, translationOffset);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, rotationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    EntityPtr CreateCylinderPrimitiveColliderEditorEntity(
        float radius,
        float height,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        const AZ::Quaternion& rotationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("CylinderEntity");
        const auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());

        AZ::TransformBus::Event(editorEntity->GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Cylinder);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCylinderRadius, radius);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCylinderHeight, height);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, translationOffset);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, rotationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    EntityPtr CreateMeshColliderEditorEntity(
        AZ::Data::Asset<PhysX::Pipeline::MeshAsset> meshAsset,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        const AZ::Quaternion& rotationOffset,
        AZStd::optional<AZ::Vector3> nonUniformScale,
        RigidBodyType rigidBodyType)
    {
        Physics::ColliderConfiguration colliderConfiguration;
        Physics::PhysicsAssetShapeConfiguration assetShapeConfig;
        assetShapeConfig.m_asset = meshAsset;

        EntityPtr editorEntity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        auto* colliderComponent =
            editorEntity->CreateComponent<PhysX::EditorMeshColliderComponent>(colliderConfiguration, assetShapeConfig);
        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        }
        else
        {
            editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        }
        if (nonUniformScale.has_value())
        {
            editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        }
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());

        AZ::TransformBus::Event(editorEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, translationOffset);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, rotationOffset);

        if (nonUniformScale.has_value())
        {
            AZ::NonUniformScaleRequestBus::Event(editorEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, *nonUniformScale);
        }

        if (rigidBodyType == RigidBodyType::Dynamic)
        {
            ForceSimulatedBodyRecreation(*editorEntity);
        }

        return editorEntity;
    }

    AZ::Aabb GetSimulatedBodyAabb(AZ::EntityId entityId)
    {
        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, entityId, &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        if (simulatedBody)
        {
            return simulatedBody->GetAabb();
        }
        return AZ::Aabb::CreateNull();
    }

    AZ::Aabb GetDebugDrawAabb(AZ::EntityId entityId)
    {
        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(
            entityId,
            &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 },
            testDebugDisplayRequests);
        return testDebugDisplayRequests.GetAabb();
    }

    void PhysXEditorFixture::SetUp()
    {
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

    void PhysXEditorFixture::ValidateInvalidEditorShapeColliderComponentParams(const float radius, const float height)
    {
        // create an editor entity with a shape collider component and a cylinder shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
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

        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be no shapes on the rigid body because the cylinder radius and/or height is invalid
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 0);
    }

    void PhysXEditorFixture::ValidateInvalidEditorColliderComponentParams(const float radius, const float height)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        editorEntity->Activate();

        // Set collider to be a cylinder
        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Cylinder);

        {
            UnitTest::ErrorHandler dimensionErrorHandler("SetCylinderRadius: radius must be greater than zero.");
            PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCylinderRadius, radius);
        
            int expectedErrorCount = radius <= 0.f ? 1 : 0;
            EXPECT_EQ(dimensionErrorHandler.GetExpectedErrorCount(), expectedErrorCount);
        }

        {
            UnitTest::ErrorHandler dimensionErrorHandler("SetCylinderHeight: height must be greater than zero.");
            PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCylinderHeight, height);
        
            int expectedErrorCount = height <= 0.f ? 1 : 0;
            EXPECT_EQ(dimensionErrorHandler.GetExpectedErrorCount(), expectedErrorCount);
        }

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should still be 1 valid shape (using default cylinder dimensions) since setting invalid radius and heights should not be applied
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
    }
} // namespace PhysXEditorTests

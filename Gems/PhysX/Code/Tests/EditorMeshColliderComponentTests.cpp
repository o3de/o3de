/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <EditorColliderComponent.h>
#include <EditorMeshColliderComponent.h>
#include <EditorShapeColliderComponent.h>
#include <EditorForceRegionComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorStaticRigidBodyComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <PhysX/ForceRegionComponentBus.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/MeshColliderComponentBus.h>
#include <PhysX/MeshAsset.h>
#include <RigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <ShapeColliderComponent.h>
#include <MeshColliderComponent.h>
#include <StaticRigidBodyComponent.h>
#include <Tests/EditorTestUtilities.h>
#include <Tests/PhysXTestCommon.h>
#include <Tests/PhysXMeshTestData.h>

namespace PhysXEditorTests
{
    static bool MeshColliderHasOnePhysicsAssetShapeType(PhysX::MeshColliderComponent* meshColliderComponent)
    {
        if (!meshColliderComponent)
        {
            return false;
        }

        const AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        if (shapeConfigList.size() != 1)
        {
            return false;
        }

        return shapeConfigList[0].second->GetShapeType() == Physics::ShapeType::PhysicsAsset;
    }

    static physx::PxGeometryType::Enum GetSimulatedBodyFirstPxGeometryType(const AZ::EntityId& entityId)
    {
        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, entityId, &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        if (!simulatedBody)
        {
            return physx::PxGeometryType::eINVALID;
        }

        const auto* pxRigidActor = static_cast<const physx::PxRigidActor*>(simulatedBody->GetNativePointer());
        if (!pxRigidActor)
        {
            return physx::PxGeometryType::eINVALID;
        }

        PHYSX_SCENE_READ_LOCK(pxRigidActor->getScene());

        if (pxRigidActor->getNbShapes() == 0)
        {
            return physx::PxGeometryType::eINVALID;
        }

        physx::PxShape* shape = nullptr;
        pxRigidActor->getShapes(&shape, 1, 0);

        if (!shape)
        {
            return physx::PxGeometryType::eINVALID;
        }

        return shape->getGeometryType();
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_RigidBodyDependencySatisfied_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorMeshColliderComponent>();
        entity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();

        // the entity should be in a valid state because the component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_RigidBodyDependencyMissing_EntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorMeshColliderComponent>();

        // the entity should not be in a valid state because the collider component requires a rigid body
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_MultipleColliderComponents_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorMeshColliderComponent>();
        entity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();

        // adding a second collider component should not make the entity invalid
        entity->CreateComponent<PhysX::EditorMeshColliderComponent>();

        // the entity should be in a valid state because the component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_WithOtherColliderComponents_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorMeshColliderComponent>();
        entity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();

        // the collider component should be compatible with multiple collider components
        entity->CreateComponent<PhysX::EditorColliderComponent>();
        entity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);

        // the entity should be in a valid state because the component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_ColliderWithBox_CorrectRuntimeComponents)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorMeshColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::MeshColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>() != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_ColliderWithBoxAndRigidBody_CorrectRuntimeComponents)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorMeshColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::MeshColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::RigidBodyComponent>() != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_ColliderWithNoMesh_GeneratesNoShapes)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("MeshColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorMeshColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* rigidBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(rigidBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidDynamic->getScene());

        // there should be no shapes
        EXPECT_EQ(pxRigidDynamic->getNbShapes(), 0);
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithPrimitive_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SpherePrimitive.data(), PhysXMeshTestData::SpherePrimitive.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(meshAssetData->CreateMeshAsset());

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eSPHERE);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.5f, -0.5f, -0.5f), 1e-3f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(0.5f, 0.5f, 0.5f), 1e-3f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithPrimitive_AssetScale_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SpherePrimitive.data(), PhysXMeshTestData::SpherePrimitive.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        const AZ::Vector3 meshAssetScale(2.0f, 1.1f, 3.5f);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(meshAssetData->CreateMeshAsset());

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), editorEntity->FindComponent<PhysX::EditorMeshColliderComponent>()->GetId());
        PhysX::EditorMeshColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, meshAssetScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            // because there is a non-uniform scale applied, the geometry type used should be a convex mesh
            // rather than a primitive type
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eCONVEXMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.55f, -1.75f), 1e-3f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.55f, 1.75f), 1e-3f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithPrimitive_AssetScale_NonUniformScale_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SpherePrimitive.data(), PhysXMeshTestData::SpherePrimitive.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        const AZ::Vector3 nonUniformScale(1.0f, 1.5f, 1.0f);
        const AZ::Vector3 meshAssetScale(2.0f, 1.1f, 3.5f);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            meshAssetData->CreateMeshAsset(),
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            nonUniformScale);

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), editorEntity->FindComponent<PhysX::EditorMeshColliderComponent>()->GetId());
        PhysX::EditorMeshColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, meshAssetScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            // because there is a non-uniform scale applied, the geometry type used should be a convex mesh
            // rather than a primitive type
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eCONVEXMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.825f, -1.75f), 1e-3f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.825f, 1.75f), 1e-3f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithConvex_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereConvex.data(), PhysXMeshTestData::SphereConvex.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(meshAssetData->CreateMeshAsset());

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eCONVEXMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            // convex shapes used to export the sphere mesh requires a higher toleance
            // when checking its aabb due to the lower tesselation it does to cover the sphere.
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.5f, -0.5f, -0.5f), 1e-1f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(0.5f, 0.5f, 0.5f), 1e-1f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithConvex_AssetScale_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereConvex.data(), PhysXMeshTestData::SphereConvex.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        const AZ::Vector3 meshAssetScale(2.0f, 1.1f, 3.5f);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(meshAssetData->CreateMeshAsset());

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), editorEntity->FindComponent<PhysX::EditorMeshColliderComponent>()->GetId());
        PhysX::EditorMeshColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, meshAssetScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eCONVEXMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            // convex shapes used to export the sphere mesh requires a higher toleance
            // when checking its aabb due to the lower tesselation it does to cover the sphere.
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.55f, -1.75f), 1e-1f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.55f, 1.75f), 1e-1f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithConvex_AssetScale_NonUniformScale_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereConvex.data(), PhysXMeshTestData::SphereConvex.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        const AZ::Vector3 nonUniformScale(1.0f, 1.5f, 1.0f);
        const AZ::Vector3 meshAssetScale(2.0f, 1.1f, 3.5f);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            meshAssetData->CreateMeshAsset(),
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            nonUniformScale);

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), editorEntity->FindComponent<PhysX::EditorMeshColliderComponent>()->GetId());
        PhysX::EditorMeshColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, meshAssetScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eCONVEXMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            // convex shapes used to export the sphere mesh requires a higher toleance
            // when checking its aabb due to the lower tesselation it does to cover the sphere.
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.825f, -1.75f), 1e-1f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.825f, 1.75f), 1e-1f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithTriangleMesh_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereTriangleMesh.data(), PhysXMeshTestData::SphereTriangleMesh.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(meshAssetData->CreateMeshAsset());

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eTRIANGLEMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.5f, -0.5f, -0.5f), 1e-3f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(0.5f, 0.5f, 0.5f), 1e-3f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithTriangleMesh_AssetScale_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereTriangleMesh.data(), PhysXMeshTestData::SphereTriangleMesh.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        const AZ::Vector3 meshAssetScale(2.0f, 1.1f, 3.5f);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(meshAssetData->CreateMeshAsset());

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), editorEntity->FindComponent<PhysX::EditorMeshColliderComponent>()->GetId());
        PhysX::EditorMeshColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, meshAssetScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eTRIANGLEMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.55f, -1.75f), 1e-3f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.55f, 1.75f), 1e-3f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithTriangleMesh_AssetScale_NonUniformScale_CorrectShapeTypeGeometryTypeAndAabb)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereTriangleMesh.data(), PhysXMeshTestData::SphereTriangleMesh.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        const AZ::Vector3 nonUniformScale(1.0f, 1.5f, 1.0f);
        const AZ::Vector3 meshAssetScale(2.0f, 1.1f, 3.5f);

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            meshAssetData->CreateMeshAsset(),
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            nonUniformScale);

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), editorEntity->FindComponent<PhysX::EditorMeshColliderComponent>()->GetId());
        PhysX::EditorMeshColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorMeshColliderComponentRequests::SetAssetScale, meshAssetScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        for (const auto& entityId : { editorEntity->GetId(), gameEntity->GetId() })
        {
            EXPECT_EQ(GetSimulatedBodyFirstPxGeometryType(entityId), physx::PxGeometryType::eTRIANGLEMESH);

            const AZ::Aabb aabb = GetSimulatedBodyAabb(entityId);
            EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.825f, -1.75f), 1e-3f));
            EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.825f, 1.75f), 1e-3f));
        }

        EXPECT_TRUE(MeshColliderHasOnePhysicsAssetShapeType(gameEntity->FindComponent<PhysX::MeshColliderComponent>()));

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithTriangleMeshAndDynamicRigidBody_Errors)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereTriangleMesh.data(), PhysXMeshTestData::SphereTriangleMesh.size());
        ASSERT_TRUE(meshAssetData != nullptr);

        UnitTest::ErrorHandler errorHandler("Cannot use triangle mesh geometry on a dynamic object");

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            meshAssetData->CreateMeshAsset(),
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(), 
            AZStd::nullopt,
            RigidBodyType::Dynamic);

        // The error appears twice because the CreateMeshColliderEditorEntity helper activates
        // the entity twice when using dynamic rigid bodies.
        EXPECT_EQ(errorHandler.GetExpectedErrorCount(), 2);

        delete meshAssetData;
    }

} // namespace PhysXEditorTests

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eSPHERE);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.5f, -0.5f, -0.5f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(0.5f, 0.5f, 0.5f), 1e-3f));

        delete meshAssetData;
    }

    // [o3de/o3de#14907]
    // Asset Scale (with non-uniform value) does not work when the asset contains primitives and there
    // is no non-uniform scale component present.
    TEST_F(PhysXEditorFixture, DISABLED_EditorMeshColliderComponent_AssetWithPrimitive_AssetScale_CorrectShapeTypeGeometryTypeAndAabb)
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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        // because there is a non-uniform scale applied, the runtime entity should have a MeshColliderComponent with a
        // cooked mesh shape configuration, rather than a Sphere shape configuration.
        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            // because there is a non-uniform scale applied, the geometry type used should be a convex mesh
            // rather than a primitive type
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.55f, -1.75f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.55f, 1.75f), 1e-3f));

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        // because there is a non-uniform scale applied, the runtime entity should have a MeshColliderComponent with a
        // cooked mesh shape configuration, rather than a Sphere shape configuration.
        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            // because there is a non-uniform scale applied, the geometry type used should be a convex mesh
            // rather than a primitive type
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.825f, -1.75f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.825f, 1.75f), 1e-3f));

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        // convex shapes used to export the sphere mesh requires a higher toleance
        // when checking its aabb due to the lower tesselation it does to cover the sphere.
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.5f, -0.5f, -0.5f), 1e-1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(0.5f, 0.5f, 0.5f), 1e-1f));

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        // convex shapes used to export the sphere mesh requires a higher toleance
        // when checking its aabb due to the lower tesselation it does to cover the sphere.
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.55f, -1.75f), 1e-1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.55f, 1.75f), 1e-1f));

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        // convex shapes used to export the sphere mesh requires a higher toleance
        // when checking its aabb due to the lower tesselation it does to cover the sphere.
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.825f, -1.75f), 1e-1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.825f, 1.75f), 1e-1f));

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eTRIANGLEMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.5f, -0.5f, -0.5f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(0.5f, 0.5f, 0.5f), 1e-3f));

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eTRIANGLEMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.55f, -1.75f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.55f, 1.75f), 1e-3f));

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

        // check that the runtime entity has the expected components
        auto* meshColliderComponent = gameEntity->FindComponent<PhysX::MeshColliderComponent>();
        auto* staticRigidBodyComponent = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();
        EXPECT_TRUE(meshColliderComponent != nullptr);
        EXPECT_TRUE(staticRigidBodyComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = meshColliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::PhysicsAsset);
        }

        const auto* simulatedBody = staticRigidBodyComponent->GetSimulatedBody();
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eTRIANGLEMESH);
        }

        const AZ::Aabb aabb = simulatedBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.0f, -0.825f, -1.75f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.0f, 0.825f, 1.75f), 1e-3f));

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

        EXPECT_EQ(errorHandler.GetExpectedErrorCount(), 1);

        delete meshAssetData;
    }

} // namespace PhysXEditorTests

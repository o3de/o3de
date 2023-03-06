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

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithPrimitive_CorrectRuntimeEntity)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SpherePrimitive.data(), PhysXMeshTestData::SpherePrimitive.size());
        EXPECT_TRUE(meshAssetData != nullptr);
        if (!meshAssetData)
        {
            return;
        }

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            AZ::Transform::CreateIdentity(), meshAssetData->CreateMeshAsset(), AZ::Vector3::CreateZero(), AZStd::nullopt, RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* rigidBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(rigidBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidDynamic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidDynamic->getNbShapes(), 1);
        if (pxRigidDynamic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidDynamic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eSPHERE);
        }

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithConvex_CorrectRuntimeEntity)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereConvex.data(), PhysXMeshTestData::SphereConvex.size());
        EXPECT_TRUE(meshAssetData != nullptr);
        if (!meshAssetData)
        {
            return;
        }

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            AZ::Transform::CreateIdentity(), meshAssetData->CreateMeshAsset(), AZ::Vector3::CreateZero(), AZStd::nullopt, RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* rigidBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(rigidBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidDynamic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidDynamic->getNbShapes(), 1);
        if (pxRigidDynamic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidDynamic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithTriangleMesh_CorrectRuntimeEntity)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereTriangleMesh.data(), PhysXMeshTestData::SphereTriangleMesh.size());
        EXPECT_TRUE(meshAssetData != nullptr);
        if (!meshAssetData)
        {
            return;
        }

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            AZ::Transform::CreateIdentity(), meshAssetData->CreateMeshAsset(), AZ::Vector3::CreateZero(), AZStd::nullopt, RigidBodyType::Static);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* simulatedBody = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody();
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

        delete meshAssetData;
    }

    TEST_F(PhysXEditorFixture, EditorMeshColliderComponent_AssetWithTriangleMeshAndDynamicRigidBody_Errors)
    {
        auto* meshAssetData = AZ::Utils::LoadObjectFromBuffer<PhysX::Pipeline::MeshAssetData>(
            PhysXMeshTestData::SphereTriangleMesh.data(), PhysXMeshTestData::SphereTriangleMesh.size());
        EXPECT_TRUE(meshAssetData != nullptr);
        if (!meshAssetData)
        {
            return;
        }

        UnitTest::ErrorHandler errorHandler("Cannot use triangle mesh geometry on a dynamic object");

        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateMeshColliderEditorEntity(
            AZ::Transform::CreateIdentity(), meshAssetData->CreateMeshAsset(), AZ::Vector3::CreateZero(), AZStd::nullopt, RigidBodyType::Dynamic);

        EXPECT_EQ(errorHandler.GetExpectedErrorCount(), 1);

        delete meshAssetData;
    }

} // namespace PhysXEditorTests

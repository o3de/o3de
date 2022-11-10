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
#include <EditorColliderComponent.h>
#include <EditorForceRegionComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorShapeColliderComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <PhysX/ForceRegionComponentBus.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/SystemComponentBus.h>
#include <RigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <ShapeColliderComponent.h>
#include <StaticRigidBodyComponent.h>
#include <Tests/EditorTestUtilities.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysXEditorTests
{
    namespace
    {
        struct TestData
        {
            const AZStd::vector<AZ::Vector2> polygonHShape = {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(0.0f, 3.0f),
                AZ::Vector2(1.0f, 3.0f),
                AZ::Vector2(1.0f, 2.0f),
                AZ::Vector2(2.0f, 2.0f),
                AZ::Vector2(2.0f, 3.0f),
                AZ::Vector2(3.0f, 3.0f),
                AZ::Vector2(3.0f, 0.0f),
                AZ::Vector2(2.0f, 0.0f),
                AZ::Vector2(2.0f, 1.0f),
                AZ::Vector2(1.0f, 1.0f),
                AZ::Vector2(1.0f, 0.0f)
            };
        };
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeDependencySatisfied_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);

        // the entity should be in a valid state because the shape component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeDependencyMissing_EntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorShapeColliderComponent>();

        // the entity should not be in a valid state because the shape collider component requires a shape component
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_MultipleShapeColliderComponents_EntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);

        // adding a second shape collider component should make the entity invalid
        entity->CreateComponent<PhysX::EditorShapeColliderComponent>();

        // the entity should be in a valid state because the shape component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::HasIncompatibleServices);
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderPlusMultipleColliderComponents_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);

        // the shape collider component should be compatible with multiple collider components
        entity->CreateComponent<PhysX::EditorColliderComponent>();
        entity->CreateComponent<PhysX::EditorColliderComponent>();

        // the entity should be in a valid state because the shape component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBox_CorrectRuntimeComponents)
    {
        // create an editor entity with a shape collider component and a box shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::ShapeColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(LmbrCentral::BoxShapeComponentTypeId) != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBox_CorrectRuntimeGeometry)
    {
        // create an editor entity with a shape collider component and a box shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorEntity->Activate();

        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(editorEntity->GetId(),
            &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        physx::PxShape* shape = nullptr;
        pxRigidStatic->getShapes(&shape, 1, 0);
        EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eBOX);

        // the bounding box of the rigid body should reflect the dimensions of the box set above
        AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(0.5f * boxDimensions));
        EXPECT_TRUE(aabb.GetMin().IsClose(-0.5f * boxDimensions));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBoxAndTranslationOffset_CorrectEditorStaticBodyGeometry)
    {
        AZ::Transform transform(AZ::Vector3(2.0f, -6.0f, 5.0f), AZ::Quaternion(0.3f, -0.3f, 0.1f, 0.9f), 1.6f);
        AZ::Vector3 nonUniformScale(2.0f, 2.5f, 0.5f);
        AZ::Vector3 boxDimensions(5.0f, 8.0f, 6.0f);
        AZ::Vector3 translationOffset(-4.0f, 3.0f, -1.0f);
        EntityPtr editorEntity = CreateBoxShapeColliderEditorEntity(transform, nonUniformScale, boxDimensions, translationOffset);

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, editorEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidStatic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-25.488f, -10.16f, -11.448f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(1.136f, 18.32f, 16.584f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBoxAndTranslationOffset_CorrectEditorDynamicBodyGeometry)
    {
        AZ::Transform transform(AZ::Vector3(-5.0f, -1.0f, 3.0f), AZ::Quaternion(0.7f, 0.5f, -0.1f, 0.5f), 1.2f);
        AZ::Vector3 nonUniformScale(1.5f, 1.5f, 4.0f);
        AZ::Vector3 boxDimensions(6.0f, 4.0f, 1.0f);
        AZ::Vector3 translationOffset(6.0f, -5.0f, -4.0f);
        EntityPtr editorEntity = CreateBoxShapeColliderEditorEntity(transform, nonUniformScale, boxDimensions, translationOffset);

        editorEntity->Deactivate();
        editorEntity->Activate();

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, editorEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidDynamic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-20.264f, 15.68f, -6.864f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(-7.592f, 26.0f, 6.672f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBoxAndTranslationOffset_CorrectRuntimeStaticBodyGeometry)
    {
        AZ::Transform transform(AZ::Vector3(7.0f, 2.0f, 4.0f), AZ::Quaternion(0.4f, -0.8f, 0.4f, 0.2f), 2.5f);
        AZ::Vector3 nonUniformScale(0.8f, 2.0f, 1.5f);
        AZ::Vector3 boxDimensions(1.0f, 4.0f, 7.0f);
        AZ::Vector3 translationOffset(6.0f, -1.0f, -2.0f);
        EntityPtr editorEntity = CreateBoxShapeColliderEditorEntity(transform, nonUniformScale, boxDimensions, translationOffset);
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidStatic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-4.8f, -14.14f, 5.265f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(12.4f, 15.02f, 31.895f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBoxAndTranslationOffset_CorrectRuntimeDynamicBodyGeometry)
    {
        AZ::Transform transform(AZ::Vector3(4.0f, 4.0f, 2.0f), AZ::Quaternion(0.1f, 0.3f, 0.9f, 0.3f), 0.8f);
        AZ::Vector3 nonUniformScale(2.5f, 1.0f, 2.0f);
        AZ::Vector3 boxDimensions(4.0f, 2.0f, 7.0f);
        AZ::Vector3 translationOffset(-2.0f, 7.0f, -1.0f);
        EntityPtr editorEntity = CreateBoxShapeColliderEditorEntity(transform, nonUniformScale, boxDimensions, translationOffset);
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidDynamic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-1.664f, -8.352f, -0.88f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(9.536f, 2.848f, 9.04f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithSphereAndTranslationOffset_CorrectEditorStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateSphereShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(4.0f, 2.0f, -2.0f), AZ::Quaternion(-0.5f, -0.5f, 0.1f, 0.7f), 3.0f),
            1.6f,
            AZ::Vector3(2.0f, 3.0f, -7.0f)
        );

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, editorEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidStatic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(22.12f, -7.24f, -10.4f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(31.72f, 2.36f, -0.8f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithSphereAndTranslationOffset_CorrectEditorDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateSphereShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(2.0f, -5.0f, -6.0f), AZ::Quaternion(0.7f, 0.1f, 0.7f, 0.1f), 0.4f),
            3.5f,
            AZ::Vector3(1.0f, 3.0f, -1.0f)
        );

        editorEntity->Deactivate();
        editorEntity->Activate();

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, editorEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidDynamic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(0.2f, -7.44f, -6.68f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(3.0f, -4.64f, -3.88f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithSphereAndTranslationOffset_CorrectRuntimeStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateSphereShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(4.0f, 4.0f, -1.0f), AZ::Quaternion(0.8f, -0.2f, 0.4f, 0.4f), 2.4f),
            2.0f,
            AZ::Vector3(5.0f, 6.0f, -8.0f)
        );

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidStatic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-12.032f, 5.92f, 17.624f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(-2.432f, 15.52f, 27.224f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithSphereAndTranslationOffset_CorrectRuntimeDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateSphereShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(2.0f, 2.0f, -5.0f), AZ::Quaternion(0.9f, 0.3f, 0.3f, 0.1f), 5.0f),
            0.4f,
            AZ::Vector3(4.0f, 7.0f, 3.0f)
        );

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidDynamic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(38.6f, -16.0f, 3.2f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(42.6f, -12.0f, 7.2f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCapsuleAndTranslationOffset_CorrectEditorStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsuleShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(2.0f, 1.0f, -2.0f), AZ::Quaternion(-0.2f, -0.8f, -0.4f, 0.4f), 4.0f),
            2.0f,
            8.0f,
            AZ::Vector3(2.0f, 3.0f, 5.0f)
        );

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, editorEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidStatic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-16.56f, 9.8f, -7.92f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(7.12f, 38.6f, 13.84f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCapsuleAndTranslationOffset_CorrectEditorDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsuleShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(7.0f, -9.0f, 2.0f), AZ::Quaternion(0.7f, 0.1f, 0.7f, 0.1f), 0.2f),
            1.0f,
            5.0f,
            AZ::Vector3(2.0f, 9.0f, -5.0f)
        );

        editorEntity->Deactivate();
        editorEntity->Activate();

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, editorEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidDynamic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(5.5f, -10.816f, 2.688f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(6.5f, -10.416f, 3.088f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCapsuleAndTranslationOffset_CorrectRuntimeStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsuleShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(-4.0f, -3.0f, -1.0f), AZ::Quaternion(0.5f, -0.7f, -0.1f, 0.5f), 0.8f),
            2.0f,
            11.0f,
            AZ::Vector3(4.0f, 1.0f, -3.0f)
        );

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidStatic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-6.4f, -6.92f, -0.36f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(1.28f, -1.704f, 5.528f)));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCapsuleAndTranslationOffset_CorrectRuntimeDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsuleShapeColliderEditorEntity(
            AZ::Transform(AZ::Vector3(7.0f, 6.0f, -3.0f), AZ::Quaternion(-0.3f, -0.1f, -0.3f, 0.9f), 6.0f),
            0.4f,
            3.0f,
            AZ::Vector3(2.0f, -7.0f, 7.0f)
        );

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* simulatedBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            simulatedBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(simulatedBody->GetNativePointer());

        AZ::Aabb aabb = PxMathConvert(pxRigidDynamic->getWorldBounds(1.0f));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-11.0f, -7.8f, 47.4f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(-6.2f, 4.92f, 62.76f)));
    }

    void SetPolygonPrismVertices(AZ::EntityId entityId, const AZStd::vector<AZ::Vector2>& vertices)
    {
        AZ::PolygonPrismPtr polygonPrism;
        LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrism, entityId,
            &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);
        if (polygonPrism)
        {
            polygonPrism->m_vertexContainer.SetVertices(vertices);
        }
    }

    void SetPolygonPrismHeight(AZ::EntityId entityId, float height)
    {
        AZ::PolygonPrismPtr polygonPrism;
        LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrism, entityId,
            &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);
        if (polygonPrism)
        {
            polygonPrism->SetHeight(height);
        }
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithPolygonPrism_CorrectRuntimeGeometry)
    {
        // create an editor entity with a shape collider component and a polygon prism shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorPolygonPrismShapeComponentTypeId);

        // suppress the shape collider error that will be raised because the polygon prism vertices have not been set yet
        UnitTest::ErrorHandler polygonPrismErrorHandler("Invalid polygon prism");
        editorEntity->Activate();

        // modify the geometry of the polygon prism
        TestData testData;
        SetPolygonPrismVertices(editorEntity->GetId(), testData.polygonHShape);
        SetPolygonPrismHeight(editorEntity->GetId(), 2.0f);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // the input polygon prism was not convex, so should have been decomposed into multiple shapes
        const int numShapes = pxRigidStatic->getNbShapes();
        EXPECT_TRUE(numShapes > 1);

        // the shapes should all be convex meshes
        for (int shapeIndex = 0; shapeIndex < numShapes; shapeIndex++)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, shapeIndex);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }

        // the vertices of the input polygon prism ranged from (0, 0) to (3, 3) and the height was set to 2
        // the bounding box of the static rigid body should reflect those values
        AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(3.0f, 3.0f, 2.0f)));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithPolygonPrismAndNonUniformScale_CorrectRunAabb)
    {
        // create an editor entity with a shape collider component and a polygon prism shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorPolygonPrismShapeComponentTypeId);

        // add a non-uniform scale component
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();

        // suppress the shape collider error that will be raised because the polygon prism vertices have not been set yet
        UnitTest::ErrorHandler polygonPrismErrorHandler("Invalid polygon prism");
        editorEntity->Activate();

        // modify the geometry of the polygon prism
        TestData testData;
        AZ::EntityId entityId = editorEntity->GetId();
        SetPolygonPrismVertices(entityId, testData.polygonHShape);
        SetPolygonPrismHeight(entityId, 2.0f);

        // update the transform scale and non-uniform scale
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetLocalUniformScale, 2.0f);
        AZ::NonUniformScaleRequestBus::Event(entityId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.5f, 1.5f, 2.0f));

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        // the vertices of the input polygon prism ranged from (0, 0) to (3, 3) and the height was set to 2
        // the bounding box of the static rigid body should reflect those values combined with the scale values above
        AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(3.0f, 9.0f, 8.0f)));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinder_CorrectRuntimeComponents)
    {
        // create an editor entity with a shape collider component and a cylinder shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorCylinderShapeComponentTypeId);
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::ShapeColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(LmbrCentral::CylinderShapeComponentTypeId) != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinderWithValidRadiusAndValidHeight_CorrectRuntimeGeometry)
    {
        // create an editor entity with a shape collider component and a cylinder shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorCylinderShapeComponentTypeId);
        editorEntity->Activate();
        
        const float validRadius = 1.0f;
        const float validHeight = 1.0f;
        
        LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
            &LmbrCentral::CylinderShapeComponentRequests::SetRadius, validRadius);
        
        LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
            &LmbrCentral::CylinderShapeComponentRequests::SetHeight, validHeight);
        
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        
        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());
        
        // there should be a single shape on the rigid body and it should be a convex mesh
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        physx::PxShape* shape = nullptr;
        pxRigidStatic->getShapes(&shape, 1, 0);
        EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        
        // the bounding box of the rigid body should reflect the dimensions of the cylinder set above
        AZ::Aabb aabb = staticBody->GetAabb();
        
        // Check that the z positions of the bounding box match that of the cylinder
        EXPECT_NEAR(aabb.GetMin().GetZ(), -0.5f * validHeight, AZ::Constants::Tolerance);
        EXPECT_NEAR(aabb.GetMax().GetZ(), 0.5f * validHeight, AZ::Constants::Tolerance);
        
        // check that the xy points are not outside the radius of the cylinder
        AZ::Vector2 vecMin(aabb.GetMin().GetX(), aabb.GetMin().GetY());
        AZ::Vector2 vecMax(aabb.GetMax().GetX(), aabb.GetMax().GetY());
        EXPECT_TRUE(AZ::GetAbs(vecMin.GetX()) <= validRadius);
        EXPECT_TRUE(AZ::GetAbs(vecMin.GetY()) <= validRadius);
        EXPECT_TRUE(AZ::GetAbs(vecMax.GetX()) <= validRadius);
        EXPECT_TRUE(AZ::GetAbs(vecMax.GetX()) <= validRadius);        
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinderWithNullRadius_HandledGracefully)
    {
        ValidateInvalidEditorShapeColliderComponentParams(0.f, 1.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinderWithNullHeight_HandledGracefully)
    {
        ValidateInvalidEditorShapeColliderComponentParams(1.f, 0.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinderWithNullRadiusAndNullHeight_HandledGracefully)
    {
        ValidateInvalidEditorShapeColliderComponentParams(0.f, 0.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinderWithNegativeRadiusAndNullHeight_HandledGracefully)
    {
        ValidateInvalidEditorShapeColliderComponentParams(-1.f, 0.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinderWithNullRadiusAndNegativeHeight_HandledGracefully)
    {
        ValidateInvalidEditorShapeColliderComponentParams(0.f, -1.f);
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithCylinderSwitchingFromNullHeightToValidHeight_HandledGracefully)
    {
        // create an editor entity with a shape collider component and a cylinder shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorCylinderShapeComponentTypeId);
        editorEntity->Activate();

        const float validRadius = 1.0f;
        const float nullHeight = 0.0f;
        const float validHeight = 1.0f;

        LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
            &LmbrCentral::CylinderShapeComponentRequests::SetRadius, validRadius);

        {
            UnitTest::ErrorHandler dimensionWarningHandler("Negative or zero cylinder dimensions are invalid");
            UnitTest::ErrorHandler colliderWarningHandler("No Collider or Shape information found when creating Rigid body");

            LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
                &LmbrCentral::CylinderShapeComponentRequests::SetHeight, nullHeight);

            EXPECT_EQ(dimensionWarningHandler.GetExpectedWarningCount(), 1);
            EXPECT_EQ(colliderWarningHandler.GetExpectedWarningCount(), 1);
        }

        {
            UnitTest::ErrorHandler dimensionWarningHandler("Negative or zero cylinder dimensions are invalid");
            UnitTest::ErrorHandler colliderWarningHandler("No Collider or Shape information found when creating Rigid body");

            LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
                &LmbrCentral::CylinderShapeComponentRequests::SetHeight, validHeight);

            EXPECT_EQ(dimensionWarningHandler.GetExpectedWarningCount(), 0);
            EXPECT_EQ(colliderWarningHandler.GetExpectedWarningCount(), 0);
        }
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBoxAndRigidBody_CorrectRuntimeComponents)
    {
        // create an editor entity with a shape collider component and a box shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::ShapeColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(LmbrCentral::BoxShapeComponentTypeId) != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::RigidBodyComponent>() != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithBoxAndRigidBody_CorrectRuntimeEntity)
    {
        // create an editor entity with a shape collider component and a box shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorEntity->Activate();

        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(editorEntity->GetId(),
            &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* rigidBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(rigidBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidDynamic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidDynamic->getNbShapes(), 1);
        physx::PxShape* shape = nullptr;
        pxRigidDynamic->getShapes(&shape, 1, 0);
        EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eBOX);

        // the bounding box of the rigid body should reflect the dimensions of the box set above
        AZ::Aabb aabb = rigidBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(0.5f * boxDimensions));
        EXPECT_TRUE(aabb.GetMin().IsClose(-0.5f * boxDimensions));
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_TransformChanged_ColliderUpdated)
    {
        // create an editor entity with a shape collider component and a box shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorEntity->Activate();
        AZ::EntityId editorEntityId = editorEntity->GetId();
        AZ::Vector3 boxDimensions = AZ::Vector3::CreateOne();
        LmbrCentral::BoxShapeComponentRequestsBus::EventResult(boxDimensions, editorEntityId,
            &LmbrCentral::BoxShapeComponentRequests::GetBoxDimensions);

        // update the transform
        const float scale = 2.0f;
        AZ::TransformBus::Event(editorEntityId, &AZ::TransformInterface::SetLocalUniformScale, scale);
        const AZ::Vector3 translation(10.0f, 20.0f, 30.0f);
        AZ::TransformBus::Event(editorEntityId, &AZ::TransformInterface::SetWorldTranslation, translation);

        // make a game entity and check its bounding box is consistent with the changed transform
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(translation + 0.5f * scale * boxDimensions));
        EXPECT_TRUE(aabb.GetMin().IsClose(translation - 0.5f * scale * boxDimensions));
    }

    void SetBoolValueOnComponent(AZ::Component* component, AZ::Crc32 name, bool value)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AzToolsFramework::InstanceDataHierarchy instanceDataHierarchy;
        instanceDataHierarchy.AddRootInstance(component);
        instanceDataHierarchy.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_WRITE);
        AzToolsFramework::InstanceDataHierarchy::InstanceDataNode* instanceNode =
            instanceDataHierarchy.FindNodeByPartialAddress({ name });
        if (instanceNode)
        {
            instanceNode->Write<bool>(value);
        }
    }

    void SetTrigger(PhysX::EditorShapeColliderComponent* editorShapeColliderComponent, bool isTrigger)
    {
        SetBoolValueOnComponent(editorShapeColliderComponent, AZ_CRC_CE("Trigger"), isTrigger);
    }

    bool GetBoolValueFromComponent(AZ::Component* component, AZ::Crc32 name)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AzToolsFramework::InstanceDataHierarchy instanceDataHierarchy;
        instanceDataHierarchy.AddRootInstance(component);
        instanceDataHierarchy.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
        AzToolsFramework::InstanceDataHierarchy::InstanceDataNode* instanceNode =
            instanceDataHierarchy.FindNodeByPartialAddress({ name });
        bool value = false;
        instanceNode->Read<bool>(value);
        return value;
    }

    bool IsTrigger(PhysX::EditorShapeColliderComponent* editorShapeColliderComponent)
    {
        return GetBoolValueFromComponent(editorShapeColliderComponent, AZ_CRC_CE("Trigger"));
    }

    void SetSingleSided(PhysX::EditorShapeColliderComponent* editorShapeColliderComponent, bool singleSided)
    {
        SetBoolValueOnComponent(editorShapeColliderComponent, AZ_CRC_CE("SingleSided"), singleSided);
    }

    bool IsSingleSided(PhysX::EditorShapeColliderComponent* editorShapeColliderComponent)
    {
        return GetBoolValueFromComponent(editorShapeColliderComponent, AZ_CRC_CE("SingleSided"));
    }

    EntityPtr CreateRigidBox(const AZ::Vector3& boxDimensions, const AZ::Vector3& position)
    {
        EntityPtr rigidBodyEditorEntity = CreateInactiveEditorEntity("RigidBodyEditorEntity");
        rigidBodyEditorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        rigidBodyEditorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        rigidBodyEditorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        rigidBodyEditorEntity->Activate();
        LmbrCentral::BoxShapeComponentRequestsBus::Event(rigidBodyEditorEntity->GetId(),
            &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);
        AZ::TransformBus::Event(rigidBodyEditorEntity->GetId(), &AZ::TransformInterface::SetWorldTranslation, position);
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::RemoveDirtyEntity, rigidBodyEditorEntity->GetId());

        return CreateActiveGameEntityFromEditorEntity(rigidBodyEditorEntity.get());
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_PolygonPrismForceRegion_AppliesForceAtRuntime)
    {
        // create an editor entity with shape collider, polygon prism shape and force region components
        EntityPtr forceRegionEditorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        auto* shapeColliderComponent = forceRegionEditorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        SetTrigger(shapeColliderComponent, true);
        forceRegionEditorEntity->CreateComponent(LmbrCentral::EditorPolygonPrismShapeComponentTypeId);
        forceRegionEditorEntity->CreateComponent<PhysX::EditorForceRegionComponent>();

        // suppress the shape collider error that will be raised because the polygon prism vertices have not been set yet
        UnitTest::ErrorHandler polygonPrismErrorHandler("Invalid polygon prism");
        forceRegionEditorEntity->Activate();

        // modify the geometry of the polygon prism
        TestData testData;
        SetPolygonPrismVertices(forceRegionEditorEntity->GetId(), testData.polygonHShape);
        SetPolygonPrismHeight(forceRegionEditorEntity->GetId(), 2.0f);

        EntityPtr forceRegionGameEntity = CreateActiveGameEntityFromEditorEntity(forceRegionEditorEntity.get());

        // add a force to the force region
        PhysX::ForceRegionRequestBus::Event(forceRegionGameEntity->GetId(),
            &PhysX::ForceRegionRequests::AddForceWorldSpace, AZ::Vector3::CreateAxisX(), 100.0f);

        const AZ::Vector3 boxDimensions(0.5f, 0.5f, 0.5f);
        // create one box over the centre of the polygon prism
        const AZ::Vector3 box1Position(1.5f, 1.5f, 3.0f);
        // create another box over one of the notches in the H
        const AZ::Vector3 box2Position(1.5f, 0.5f, 3.0f);
        EntityPtr rigidBodyGameEntity1 = CreateRigidBox(boxDimensions, box1Position);
        EntityPtr rigidBodyGameEntity2 = CreateRigidBox(boxDimensions, box2Position);

        PhysX::TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 100);

        // the first rigid body should have been moved in the positive x direction by the force region
        EXPECT_TRUE(rigidBodyGameEntity1->GetTransform()->GetWorldTranslation().GetX() > box1Position.GetX() + AZ::Constants::FloatEpsilon);
        // the second rigid body should not have entered the force region and so its X position should not have been affected
        EXPECT_NEAR(rigidBodyGameEntity2->GetTransform()->GetWorldTranslation().GetX(), box2Position.GetX(), 1e-3f);
    }

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeColliderWithScaleSetToParentEntity_CorrectRuntimeScale)
    {
        // create an editor parent entity (empty, need transform component only)
        EntityPtr editorParentEntity = CreateInactiveEditorEntity("ParentEntity");
        editorParentEntity->Activate();

        // set some scale to parent entity
        const float parentScale = 2.0f;
        AZ::TransformBus::Event(editorParentEntity->GetId(), &AZ::TransformInterface::SetLocalUniformScale, parentScale);

        // create an editor child entity with a shape collider component and a box shape component
        EntityPtr editorChildEntity = CreateInactiveEditorEntity("ChildEntity");
        editorChildEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorChildEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorChildEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        editorChildEntity->Activate();

        // set some dimensions to child entity box component
        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(editorChildEntity->GetId(),
            &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions,
            boxDimensions);

        // set one entity as parent of another
        AZ::TransformBus::Event(editorChildEntity->GetId(),
            &AZ::TransformBus::Events::SetParentRelative,
            editorParentEntity->GetId());

        // build child game entity (parent will be built implicitly)
        EntityPtr gameChildEntity = CreateActiveGameEntityFromEditorEntity(editorChildEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const AzPhysics::RigidBody* rigidBody =
            gameChildEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();
        
        // the bounding box of the rigid body should reflect the dimensions of the box set above
        // and parent entity scale
        const AZ::Aabb aabb = rigidBody->GetAabb();
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(0.5f * boxDimensions * parentScale));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(-0.5f * boxDimensions * parentScale));
    }

    class PhysXEditorParamBoolFixture
        : public ::testing::WithParamInterface<bool>
        , public PhysXEditorFixture
    {
    };

    TEST_P(PhysXEditorParamBoolFixture, EditorShapeColliderComponent_ShapeColliderWithQuadShapeNonUniformlyScalesCorrectly)
    {
        // test both single and double-sided quad colliders
        bool singleSided = GetParam();

        EntityPtr editorEntity = CreateInactiveEditorEntity("QuadEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorQuadShapeComponentTypeId);
        auto* shapeColliderComponent = editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        SetSingleSided(shapeColliderComponent, singleSided);
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        const auto entityId = editorEntity->GetId();

        editorEntity->Activate();

        LmbrCentral::QuadShapeComponentRequestBus::Event(entityId, &LmbrCentral::QuadShapeComponentRequests::SetQuadWidth, 1.2f);
        LmbrCentral::QuadShapeComponentRequestBus::Event(entityId, &LmbrCentral::QuadShapeComponentRequests::SetQuadHeight, 0.8f);

        // update the transform scale and non-uniform scale
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetLocalUniformScale, 3.0f);
        AZ::NonUniformScaleRequestBus::Event(entityId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(1.5f, 0.5f, 1.0f));

        // make a game entity and check that its AABB is as expected
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        AZ::Aabb aabb = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetAabb();

        EXPECT_NEAR(aabb.GetMin().GetX(), -2.7f, 1e-3f);
        EXPECT_NEAR(aabb.GetMin().GetY(), -0.6f, 1e-3f);
        EXPECT_NEAR(aabb.GetMax().GetX(), 2.7f, 1e-3f);
        EXPECT_NEAR(aabb.GetMax().GetY(), 0.6f, 1e-3f);
    }

    TEST_P(PhysXEditorParamBoolFixture, EditorShapeColliderComponent_TriggerSettingIsRememberedWhenSwitchingToQuadAndBack)
    {
        bool initialTriggerSetting = GetParam();

        // create an editor entity with a box component (which does support trigger)
        EntityPtr editorEntity = CreateInactiveEditorEntity("QuadEntity");
        auto* boxShapeComponent = editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);
        auto* shapeColliderComponent = editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        SetTrigger(shapeColliderComponent, initialTriggerSetting);
        editorEntity->Activate();

        // the trigger setting should be what it was set to
        EXPECT_EQ(IsTrigger(shapeColliderComponent), initialTriggerSetting);

        // deactivate the entity and swap the box for a quad (which does not support trigger)
        editorEntity->Deactivate();
        editorEntity->RemoveComponent(boxShapeComponent);
        auto* quadShapeComponent = editorEntity->CreateComponent(LmbrCentral::EditorQuadShapeComponentTypeId);
        editorEntity->Activate();

        // the trigger setting should now be false, because quad shape does not support triggers
        EXPECT_FALSE(IsTrigger(shapeColliderComponent));

        // swap back to a box shape
        editorEntity->Deactivate();
        editorEntity->RemoveComponent(quadShapeComponent);
        editorEntity->AddComponent(boxShapeComponent);
        editorEntity->Activate();

        // the original trigger setting should have been remembered
        EXPECT_EQ(IsTrigger(shapeColliderComponent), initialTriggerSetting);

        // the quad shape component is no longer attached to the entity so won't be automatically cleared up
        delete quadShapeComponent;
    }

    TEST_P(PhysXEditorParamBoolFixture, EditorShapeColliderComponent_SingleSidedSettingIsRememberedWhenAddingAndRemovingRigidBody)
    {
        bool initialSingleSidedSetting = GetParam();

        // create an editor entity without a rigid body (that means both single-sided and double-sided quads are valid)
        EntityPtr editorEntity = CreateInactiveEditorEntity("QuadEntity");
        editorEntity->CreateComponent(LmbrCentral::EditorQuadShapeComponentTypeId);
        auto* shapeColliderComponent = editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        SetSingleSided(shapeColliderComponent, initialSingleSidedSetting);
        editorEntity->Activate();

        // verify that the single sided setting matches the initial value
        EXPECT_EQ(IsSingleSided(shapeColliderComponent), initialSingleSidedSetting);

        // add an editor rigid body component (this should mean single-sided quads are not supported)
        editorEntity->Deactivate();
        auto rigidBodyComponent = editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->Activate();

        EXPECT_FALSE(IsSingleSided(shapeColliderComponent));

        // remove the editor rigid body component (the previous single-sided setting should be restored)
        editorEntity->Deactivate();
        editorEntity->RemoveComponent(rigidBodyComponent);
        editorEntity->Activate();

        EXPECT_EQ(IsSingleSided(shapeColliderComponent), initialSingleSidedSetting);

        // the rigid body component is no longer attached to the entity so won't be automatically cleared up
        delete rigidBodyComponent;
    }

    INSTANTIATE_TEST_CASE_P(PhysXEditorTest, PhysXEditorParamBoolFixture, ::testing::Bool());

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_SingleSidedQuadDoesNotCollideFromBelow)
    {
        // create an editor entity without a rigid body (that means both single-sided and double-sided quads are valid), positioned at the origin
        EntityPtr editorQuadEntity = CreateInactiveEditorEntity("QuadEntity");
        editorQuadEntity->CreateComponent(LmbrCentral::EditorQuadShapeComponentTypeId);
        auto* shapeColliderComponent = editorQuadEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        SetSingleSided(shapeColliderComponent, true);
        editorQuadEntity->Activate();
        LmbrCentral::QuadShapeComponentRequestBus::Event(editorQuadEntity->GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadHeight, 10.0f);
        LmbrCentral::QuadShapeComponentRequestBus::Event(editorQuadEntity->GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadWidth, 10.0f);

        // add a second entity with a box collider and a rigid body, positioned below the quad
        EntityPtr editorBoxEntity = CreateInactiveEditorEntity("BoxEntity");
        editorBoxEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
        editorBoxEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorBoxEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorBoxEntity->Activate();
        AZ::TransformBus::Event(editorBoxEntity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, -AZ::Vector3::CreateAxisZ());

        EntityPtr gameQuadEntity = CreateActiveGameEntityFromEditorEntity(editorQuadEntity.get());
        EntityPtr gameBoxEntity = CreateActiveGameEntityFromEditorEntity(editorBoxEntity.get());

        // give the box enough upward velocity to rise above the level of the quad
        // simulate for enough time that the box would have reached the top of its trajectory and fallen back past the starting point if
        // it hadn't collided with the top of the quad
        const int numTimesteps = 100;
        Physics::RigidBodyRequestBus::Event(gameBoxEntity->GetId(), &Physics::RigidBodyRequests::SetLinearVelocity, AZ::Vector3::CreateAxisZ(6.0f));
        PhysX::TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, numTimesteps);

        // the box should travel through the base of the quad because it has no collision from that direction
        // and land on the top surface of the quad, which does have collision
        float finalHeight = 0.0f;
        AZ::TransformBus::EventResult(finalHeight, gameBoxEntity->GetId(), &AZ::TransformBus::Events::GetWorldZ);

        EXPECT_GT(finalHeight, 0.0f);
    }
} // namespace PhysXEditorTests

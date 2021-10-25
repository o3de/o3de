/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <Tests/EditorTestUtilities.h>
#include <EditorHeightfieldColliderComponent.h>
#include <HeightfieldColliderComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <StaticRigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <PhysX/PhysXLocks.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>

namespace UnitTest
{
    class MockTerrainPhysicsColliderComponent
        : public AZ::Component
        , protected Physics::HeightfieldProviderRequestsBus::Handler
    {
    public:
        AZ_COMPONENT(MockTerrainPhysicsColliderComponent, "{C5F7CCCF-FDB2-40DF-992D-CF028F4A1B59}");

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MockTerrainPhysicsColliderComponent, AZ::Component>()
                    ->Version(1)
                    ;
            }
        }

        void Activate() override
        {
            Physics::HeightfieldProviderRequestsBus::Handler::BusConnect(GetEntityId());
        }

        void Deactivate() override
        {
            Physics::HeightfieldProviderRequestsBus::Handler::BusDisconnect();
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
        }

        // HeightfieldProviderRequestsBus
        AZ::Vector2 GetHeightfieldGridSpacing() const override
        {
            return AZ::Vector2(1, 1);
        }
        void GetHeightfieldGridSize(int32_t& numColumns, int32_t& numRows) const override
        {
            numColumns = 3;
            numRows = 3;
        }
        AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const override
        {
            AZStd::vector<Physics::HeightMaterialPoint> samples{ { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { 2.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { -1.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { 0.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                                 { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight } };
            return samples;
        }

        void GetHeightfieldHeightBounds(float& x, float &y) const
        {
            x = 0;
            y = 0;
        }

        AZ::Transform GetHeightfieldTransform(void) const
        {
            return AZ::Transform::CreateTranslation({ 0, 0, 0 });
        }

        MOCK_CONST_METHOD0(GetMaterialList, AZStd::vector<Physics::MaterialId>());
        MOCK_CONST_METHOD0(GetHeights, AZStd::vector<float>());
        MOCK_CONST_METHOD1(UpdateHeights, AZStd::vector<float>(const AZ::Aabb& dirtyRegion));
        MOCK_CONST_METHOD1(UpdateHeightsAndMaterials, AZStd::vector<Physics::HeightMaterialPoint>(const AZ::Aabb& dirtyRegion));
        MOCK_CONST_METHOD0(GetHeightfieldAabb, AZ::Aabb());
    };
}


namespace PhysXEditorTests
{
    EntityPtr SetupHeightfieldComponent()
    {
        // create an editor entity with a shape collider component and a box shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        editorEntity->CreateComponent<UnitTest::MockTerrainPhysicsColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorAxisAlignedBoxShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();
        AZ::ComponentApplicationBus::Broadcast(
            &AZ::ComponentApplicationRequests::RegisterComponentDescriptor,
            UnitTest::MockTerrainPhysicsColliderComponent::CreateDescriptor());
        return editorEntity;
    }

    void CleanupHeightfieldComponent()
    {
        AZ::ComponentApplicationBus::Broadcast(
            &AZ::ComponentApplicationRequests::UnregisterComponentDescriptor,
            UnitTest::MockTerrainPhysicsColliderComponent::CreateDescriptor());
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentDependenciesSatisfiedEntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorAxisAlignedBoxShapeComponentTypeId);
        entity->CreateComponent<UnitTest::MockTerrainPhysicsColliderComponent>()->CreateDescriptor();

        // the entity should be in a valid state because the shape component and
        // the Terrain Physics Collider Component requirement is satisfied.
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentDependenciesMissingEntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();

        // the entity should not be in a valid state because the heightfield collider component requires
        // a shape component and the Terrain Physics Collider Component
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentMultipleHeightfieldColliderComponentsEntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorAxisAlignedBoxShapeComponentTypeId);

        // adding a second heightfield collider component should make the entity invalid
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();

        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::HasIncompatibleServices);
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentHeightfieldColliderWithCorrectComponentsCorrectRuntimeComponents)
    {
        EntityPtr editorEntity = SetupHeightfieldComponent();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<UnitTest::MockTerrainPhysicsColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::HeightfieldColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(LmbrCentral::AxisAlignedBoxShapeComponentTypeId) != nullptr);

        CleanupHeightfieldComponent();
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentHeightfieldColliderWithAABoxCorrectRuntimeGeometry)
    {
        EntityPtr editorEntity = SetupHeightfieldComponent();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* staticBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            staticBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a heightfield
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);

        physx::PxShape* shape = nullptr;
        pxRigidStatic->getShapes(&shape, 1, 0);
        EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eHEIGHTFIELD);

        CleanupHeightfieldComponent();
    }

} // namespace PhysXEditorTests


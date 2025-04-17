/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestMeshColliderComponent.h>

#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace UnitTest
{
    class PhysXMeshColliderComponentModeTest
        : public ToolsApplicationFixture<false>
    {
    protected:
        AZ::ComponentId m_meshColliderComponentId;

        AZ::Entity* CreateEntityWithTestMeshColliderComponent()
        {
            AZ::Entity* entity = nullptr;
            AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

            entity->Deactivate();

            // Add placeholder component which implements component mode.
            auto meshColliderComponent = entity->CreateComponent<TestMeshColliderComponent>();

            m_meshColliderComponentId = meshColliderComponent->GetId();

            entity->Activate();

            AzToolsFramework::SelectEntity(entityId);

            return entity;
        }

        // Needed to support ViewportUi request calls.
        ViewportManagerWrapper m_viewportManagerWrapper;

        void SetUpEditorFixtureImpl() override
        {
            m_viewportManagerWrapper.Create();
        }
        void TearDownEditorFixtureImpl() override
        {
            m_viewportManagerWrapper.Destroy();
        }
    };

    TEST_F(PhysXMeshColliderComponentModeTest, PressingKeyRShouldResetAssetScale)
    {
        // Given there is a mesh collider component in component mode.
        auto colliderEntity = CreateEntityWithTestMeshColliderComponent();
        AZ::Vector3 assetScale(10.0f, 10.0f, 10.0f);
        colliderEntity->FindComponent<TestMeshColliderComponent>()->SetAssetScale(assetScale);

        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestMeshColliderComponent>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        // Then the asset scale should be reset.
        assetScale = colliderEntity->FindComponent<TestMeshColliderComponent>()->GetAssetScale();
        EXPECT_THAT(assetScale, IsClose(AZ::Vector3::CreateOne()));
    }

    using PhysXMeshColliderComponentModeManipulatorTest =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<PhysXMeshColliderComponentModeTest>;

    TEST_F(PhysXMeshColliderComponentModeManipulatorTest, AssetScaleManipulatorsScaleInCorrectDirection)
    {
        auto colliderEntity = CreateEntityWithTestMeshColliderComponent();
        colliderEntity->FindComponent<TestMeshColliderComponent>()->SetAssetScale(AZ::Vector3::CreateOne());
        EnterComponentMode<TestMeshColliderComponent>();
        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // position the camera so the X axis manipulator will be flipped
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationZ(-AZ::Constants::QuarterPi), AZ::Vector3(-5.0f, -5.0f, 0.0f)));

        // select a point in world space slightly displaced from the position of the entity in the negative x direction
        // in order to grab the X manipulator
        const float x = 0.1f;
        const float xDelta = 0.1f;
        const AZ::Vector3 worldStart(-x, 0.0f, 0.0f);

        // position in world space to drag to
        const AZ::Vector3 worldEnd(-(x + xDelta), 0.0f, 0.0f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd);

        const auto worldToScreenMultiplier = 1.0f / AzToolsFramework::CalculateScreenToWorldMultiplier(worldStart, m_cameraState);
        const auto assetScale = colliderEntity->FindComponent<TestMeshColliderComponent>()->GetAssetScale();
        // need quite a large tolerance because using screen co-ordinates limits precision
        const float tolerance = 0.01f;
        EXPECT_THAT(assetScale.GetX(), ::testing::FloatNear(1.0f + xDelta * worldToScreenMultiplier, tolerance));
        EXPECT_THAT(assetScale.GetY(), ::testing::FloatNear(1.0f, tolerance));
        EXPECT_THAT(assetScale.GetZ(), ::testing::FloatNear(1.0f, tolerance));
    }
} // namespace UnitTest

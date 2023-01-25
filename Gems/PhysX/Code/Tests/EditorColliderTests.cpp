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
            const AZStd::vector<AZ::Vector2> polygonHShape = { AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 3.0f), AZ::Vector2(1.0f, 3.0f),
                                                               AZ::Vector2(1.0f, 2.0f), AZ::Vector2(2.0f, 2.0f), AZ::Vector2(2.0f, 3.0f),
                                                               AZ::Vector2(3.0f, 3.0f), AZ::Vector2(3.0f, 0.0f), AZ::Vector2(2.0f, 0.0f),
                                                               AZ::Vector2(2.0f, 1.0f), AZ::Vector2(1.0f, 1.0f), AZ::Vector2(1.0f, 0.0f) };
        };
    } // namespace

    TEST_F(PhysXEditorFixture, EditorShapeColliderComponent_ShapeDependencySatisfied_EntityIsValid)
    {
        PositionEntities();
        PositionCamera(m_cameraState);

        using ::testing::Eq;
        auto selectedEntitiesBefore = SelectedEntities();
        EXPECT_TRUE(selectedEntitiesBefore.empty());

        // calculate the position in screen space of the initial entity position
        const auto entity1ScreenPosition = AzFramework::WorldToScreen(Entity1WorldTranslation, m_cameraState);

        // click the entity in the viewport
        m_actionDispatcher->SetStickySelect(true)
            ->CameraState(m_cameraState)
            ->MousePosition(entity1ScreenPosition)
            ->MouseLButtonDown()
            ->MouseLButtonUp();

        // entity is selected
        auto selectedEntitiesAfter = SelectedEntities();
        EXPECT_THAT(selectedEntitiesAfter.size(), Eq(1));
        EXPECT_THAT(selectedEntitiesAfter.front(), Eq(m_entityId1));

        EntityPtr entity = CreateInactiveEditorEntity("ColliderComponnetEntitiy");
        entity->CreateComponent<PhysX::EditorColliderComponent>();

    }
} // namespace PhysXEditorTests

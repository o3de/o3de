/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include "Shape/EditorSphereShapeComponent.h"

namespace LmbrCentral
{
    using AzToolsFramework::ViewportInteraction::BuildMouseButtons;
    using AzToolsFramework::ViewportInteraction::BuildMouseInteraction;
    using AzToolsFramework::ViewportInteraction::BuildMousePick;

    class EditorSphereShapeComponentFixture : public UnitTest::ToolsApplicationFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            m_editorSphereShapeComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSphereShapeComponent::CreateDescriptor());
            m_editorSphereShapeComponentDescriptor->Reflect(serializeContext);

            m_entityId1 = UnitTest::CreateDefaultEditorEntity("Entity1");
        }

        void TearDownEditorFixtureImpl() override
        {
            bool entityDestroyed = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                entityDestroyed, &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entityId1);

            m_editorSphereShapeComponentDescriptor.reset();
        }

        AZ::EntityId m_entityId1;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSphereShapeComponentDescriptor;
    };

    struct IntersectionQueryOutcome
    {
        bool m_helpersVisible;
        bool m_expectedIntersection;
    };

    using ShapeComponentIndirectCallManipulatorViewportInteractionFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorSphereShapeComponentFixture>;

    class ShapeComponentIndirectCallManipulatorViewportInteractionFixtureParam
        : public ShapeComponentIndirectCallManipulatorViewportInteractionFixture
        , public ::testing::WithParamInterface<IntersectionQueryOutcome>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            ShapeComponentIndirectCallManipulatorViewportInteractionFixture::SetUpEditorFixtureImpl();

            auto* entity1 = AzToolsFramework::GetEntityById(m_entityId1);
            entity1->Deactivate();
            entity1->CreateComponent<EditorSphereShapeComponent>();
            entity1->Activate();

            AZ::TransformBus::Event(
                m_entityId1, &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 2.0f, 0.0f)));
            LmbrCentral::SphereShapeComponentRequestsBus::Event(
                m_entityId1, &LmbrCentral::SphereShapeComponentRequestsBus::Events::SetRadius, 1.0f);

            m_cameraState = AzFramework::CreateDefaultCamera(AZ::Transform::CreateIdentity(), AZ::Vector2(1024.0f, 768.0f));
        }
    };

    TEST_P(ShapeComponentIndirectCallManipulatorViewportInteractionFixtureParam, ShapeIntersectionOnlyHappensWithHelpersEnabled)
    {
        // given
        m_viewportManipulatorInteraction->GetViewportInteraction().SetHelpersVisible(GetParam().m_helpersVisible);

        const auto entity1ScreenPosition = AzFramework::WorldToScreen(AzToolsFramework::GetWorldTranslation(m_entityId1), m_cameraState);
        const auto viewportId = m_viewportManipulatorInteraction->GetViewportInteraction().GetViewportId();
        const auto mouseInteraction = BuildMouseInteraction(
            BuildMousePick(m_cameraState, entity1ScreenPosition),
            BuildMouseButtons(AzToolsFramework::ViewportInteraction::MouseButton::None),
            AzToolsFramework::ViewportInteraction::InteractionId(AZ::EntityId(), viewportId),
            AzToolsFramework::ViewportInteraction::KeyboardModifiers());

        // mimic mouse move
        m_actionDispatcher->CameraState(m_cameraState)->MousePosition(entity1ScreenPosition);

        // when
        float closestDistance = AZStd::numeric_limits<float>::max();
        const bool entityPicked = AzToolsFramework::PickEntity(m_entityId1, mouseInteraction, closestDistance, viewportId);

        // then
        EXPECT_THAT(entityPicked, ::testing::Eq(GetParam().m_expectedIntersection));
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        ShapeComponentIndirectCallManipulatorViewportInteractionFixtureParam,
        testing::Values(IntersectionQueryOutcome{ true, true }, IntersectionQueryOutcome{ false, false }));
} // namespace LmbrCentral

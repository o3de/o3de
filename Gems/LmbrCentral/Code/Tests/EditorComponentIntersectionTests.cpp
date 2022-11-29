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
#include "Shape/EditorSplineComponent.h"

namespace LmbrCentral
{
    using AzToolsFramework::ViewportInteraction::BuildMouseButtons;
    using AzToolsFramework::ViewportInteraction::BuildMouseInteraction;
    using AzToolsFramework::ViewportInteraction::BuildMousePick;

    class EditorIntersectionComponentFixture : public UnitTest::ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            m_editorSphereShapeComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSphereShapeComponent::CreateDescriptor());
            m_editorSphereShapeComponentDescriptor->Reflect(serializeContext);
            m_editorSplineComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSplineComponent::CreateDescriptor());
            m_editorSplineComponentDescriptor->Reflect(serializeContext);

            m_entityId1 = UnitTest::CreateDefaultEditorEntity("Entity1");
        }

        void TearDownEditorFixtureImpl() override
        {
            bool entityDestroyed = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                entityDestroyed, &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entityId1);

            m_editorSplineComponentDescriptor.reset();
            m_editorSphereShapeComponentDescriptor.reset();
        }

        AZ::EntityId m_entityId1;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSphereShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSplineComponentDescriptor;
    };

    struct IntersectionQueryOutcome
    {
        bool m_helpersVisible;
        bool m_expectedIntersection;
    };

    using EditorComponentIndirectCallManipulatorViewportInteractionFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorIntersectionComponentFixture>;

    class EditorComponentIndirectCallManipulatorViewportInteractionFixtureParam
        : public EditorComponentIndirectCallManipulatorViewportInteractionFixture
        , public ::testing::WithParamInterface<IntersectionQueryOutcome>
    {
    public:
        virtual void CreateEditorComponent(AZ::Entity* entity) = 0;
        virtual void SetupEditorComponent(AZ::EntityId entityId) = 0;

        void SetUpEditorFixtureImpl() override
        {
            EditorComponentIndirectCallManipulatorViewportInteractionFixture::SetUpEditorFixtureImpl();

            auto* entity1 = AzToolsFramework::GetEntityById(m_entityId1);
            AZ_Assert(entity1, "Entity1 could not be found");
            entity1->Deactivate();
            CreateEditorComponent(entity1);
            entity1->Activate();

            AZ::TransformBus::Event(
                m_entityId1, &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 2.0f, 0.0f)));

            SetupEditorComponent(m_entityId1);

            m_cameraState = AzFramework::CreateDefaultCamera(AZ::Transform::CreateIdentity(), AzFramework::ScreenSize(1024, 768));
        }

        void VerifySelectionIntersection()
        {
            // given
            m_viewportManipulatorInteraction->GetViewportInteraction().SetHelpersVisible(GetParam().m_helpersVisible);

            const auto entity1ScreenPosition =
                AzFramework::WorldToScreen(AzToolsFramework::GetWorldTranslation(m_entityId1), m_cameraState);
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
    };

    class ShapeEditorComponentIndirectCallManipulatorViewportInteractionFixtureParam
        : public EditorComponentIndirectCallManipulatorViewportInteractionFixtureParam
    {
    public:
        void CreateEditorComponent(AZ::Entity* entity) override
        {
            entity->CreateComponent<EditorSphereShapeComponent>();
        }

        void SetupEditorComponent(AZ::EntityId entityId) override
        {
            LmbrCentral::SphereShapeComponentRequestsBus::Event(
                entityId, &LmbrCentral::SphereShapeComponentRequestsBus::Events::SetRadius, 1.0f);
        }
    };

    class SplineEditorComponentIndirectCallManipulatorViewportInteractionFixtureParam
        : public EditorComponentIndirectCallManipulatorViewportInteractionFixtureParam
    {
    public:
        void CreateEditorComponent(AZ::Entity* entity) override
        {
            entity->CreateComponent<EditorSplineComponent>();
        }

        void SetupEditorComponent([[maybe_unused]] AZ::EntityId entityId) override
        {
            // unused
        }
    };

    TEST_P(ShapeEditorComponentIndirectCallManipulatorViewportInteractionFixtureParam, ShapeIntersectionOnlyHappensWithHelpersEnabled)
    {
        VerifySelectionIntersection();
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        ShapeEditorComponentIndirectCallManipulatorViewportInteractionFixtureParam,
        testing::Values(IntersectionQueryOutcome{ true, true }, IntersectionQueryOutcome{ false, false }));

    TEST_P(SplineEditorComponentIndirectCallManipulatorViewportInteractionFixtureParam, SplineIntersectionOnlyHappensWithHelpersEnabled)
    {
        VerifySelectionIntersection();
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        SplineEditorComponentIndirectCallManipulatorViewportInteractionFixtureParam,
        testing::Values(IntersectionQueryOutcome{ true, true }, IntersectionQueryOutcome{ false, false }));
} // namespace LmbrCentral

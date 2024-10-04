/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <CustomSerializeContextTestFixture.h>

namespace UnitTest
{
    class ManipulatorCoreFixture
        : public CustomSerializeContextTestFixture
    {
    public:
        void SetUp() override
        {
            CustomSerializeContextTestFixture::SetUp();

            m_transformComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AzToolsFramework::Components::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));

            m_lockComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AzToolsFramework::Components::EditorLockComponent::CreateDescriptor());
            m_lockComponentDescriptor->Reflect(&(*m_serializeContext));

            m_visibilityComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AzToolsFramework::Components::EditorVisibilityComponent::CreateDescriptor());
            m_visibilityComponentDescriptor->Reflect(&(*m_serializeContext));

            m_linearManipulator = AzToolsFramework::LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());

            m_entity = AZStd::make_unique<AZ::Entity>();
            // add required components for the Editor entity
            m_entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            m_entity->CreateComponent<AzToolsFramework::Components::EditorLockComponent>();
            m_entity->CreateComponent<AzToolsFramework::Components::EditorVisibilityComponent>();

            m_entity->Init();
            m_entity->Activate();

            if (const auto* transformComponent = m_entity->FindComponent<AzToolsFramework::Components::TransformComponent>())
            {
                m_transformComponentId = transformComponent->GetId();
                m_linearManipulator->AddEntityComponentIdPair(AZ::EntityComponentIdPair{ m_entityId, m_transformComponentId });
            }

            if (const auto* lockComponent = m_entity->FindComponent<AzToolsFramework::Components::EditorLockComponent>())
            {
                m_lockComponentId = lockComponent->GetId();
                m_linearManipulator->AddEntityComponentIdPair(AZ::EntityComponentIdPair{ m_entityId, m_lockComponentId });
            }

            if (const auto* visibilityComponent = m_entity->FindComponent<AzToolsFramework::Components::EditorVisibilityComponent>())
            {
                m_visibiltyComponentId = visibilityComponent->GetId();
                m_linearManipulator->AddEntityComponentIdPair(AZ::EntityComponentIdPair{ m_entityId, m_visibiltyComponentId });
            }

            m_editorEntityComponentChangeDetector = AZStd::make_unique<EditorEntityComponentChangeDetector>(m_entityId);
        }

        void TearDown() override
        {
            m_editorEntityComponentChangeDetector.reset();
            m_linearManipulator.reset();
            m_entity.reset();
            m_transformComponentDescriptor.reset();
            m_lockComponentDescriptor.reset();
            m_visibilityComponentDescriptor.reset();
            CustomSerializeContextTestFixture::TearDown();
        }

        AZ::EntityId m_entityId;
        AZStd::unique_ptr<AZ::Entity> m_entity;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_linearManipulator;
        AZStd::unique_ptr<EditorEntityComponentChangeDetector> m_editorEntityComponentChangeDetector;
        AZ::ComponentId m_transformComponentId;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZ::ComponentId m_lockComponentId;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_lockComponentDescriptor;
        AZ::ComponentId m_visibiltyComponentId;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_visibilityComponentDescriptor;
    };

    TEST_F(ManipulatorCoreFixture, AllEntityIdComponentPairsRemovedFromManipulatorAfterRemoveEntity)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // handled in Fixture::SetUp()
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        m_linearManipulator->RemoveEntityId(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(m_linearManipulator->HasEntityId(m_entityId));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ManipulatorCoreFixture, EntityIdComponentPairRemovedFromManipulatorAfterRemoveEntityComponentId)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const auto entityLockComponentIdPair = AZ::EntityComponentIdPair{ m_entityId, m_lockComponentId };
        const auto entityVisibiltyComponentIdPair = AZ::EntityComponentIdPair{ m_entityId, m_visibiltyComponentId };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        m_linearManipulator->RemoveEntityComponentIdPair(entityLockComponentIdPair);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(m_linearManipulator->HasEntityComponentIdPair(entityLockComponentIdPair));
        EXPECT_TRUE(m_linearManipulator->HasEntityComponentIdPair(entityVisibiltyComponentIdPair));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ManipulatorCoreFixture, EntityComponentsNotifiedAfterManipulatorAction)
    {
        using testing::UnorderedElementsAre;
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // handled in Fixture::SetUp()
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        m_linearManipulator->OnLeftMouseDown(AzToolsFramework::ViewportInteraction::MouseInteraction{}, 0.0f);

        m_linearManipulator->OnLeftMouseUp(AzToolsFramework::ViewportInteraction::MouseInteraction{});
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(
            m_editorEntityComponentChangeDetector->m_componentIds,
            UnorderedElementsAre(m_transformComponentId, m_lockComponentId, m_visibiltyComponentId));

        // note that manipulators talk to property editor components directly via the above call, which causes
        // an automatic invalidation of the property editor UI for that entity/component pair in all windows where
        // it is present.  It is not necessary to broadcast a message to invalidate anything else.
    }

    using ManipulatorCoreInteractionFixture = DirectCallManipulatorViewportInteractionFixtureMixin<ManipulatorCoreFixture>;

    TEST_F(ManipulatorCoreInteractionFixture, TestInteraction)
    {
        // setup viewport/camera
        m_cameraState.m_viewportSize = AzFramework::ScreenSize(1280, 720);
        AzFramework::SetCameraTransform(m_cameraState, AZ::Transform::CreateIdentity());

        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AzToolsFramework::CreateManipulatorViewSphere(
            // note: use a small radius for the manipulator view/bounds to ensure precise mouse movement
            AZ::Color{},
            0.001f,
            [](const AzToolsFramework::ViewportInteraction::MouseInteraction&, bool, const AZ::Color&)
            {
                return AZ::Color{};
            }));

        m_linearManipulator->SetViews(views);
        m_linearManipulator->Register(m_viewportManipulatorInteraction->GetManipulatorManagerId());

        // the initial starting position of the manipulator
        const auto initialTransformWorld = AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 10.0f, 0.0f));
        // where the linear manipulator should end up
        const auto finalTransformWorld = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 10.0f, 0.0f));

        // calculate the position in screen space of the initial position of the manipulator
        const auto initialPositionScreen = AzFramework::WorldToScreen(initialTransformWorld.GetTranslation(), m_cameraState);
        // calculate the position in screen space of the final position of the manipulator
        const auto finalPositionScreen = AzFramework::WorldToScreen(finalTransformWorld.GetTranslation(), m_cameraState);

        m_linearManipulator->SetSpace(AZ::Transform::CreateIdentity());
        m_linearManipulator->SetLocalTransform(initialTransformWorld);
        m_linearManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                // move the manipulator
                m_linearManipulator->SetLocalPosition(action.LocalPosition());
            });

        // press and drag the mouse (starting where the surface manipulator is)
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(initialPositionScreen)
            ->MouseLButtonDown()
            ->MousePosition(finalPositionScreen)
            ->MouseLButtonUp();

        // read back the position of the manipulator now
        const AZ::Transform finalManipulatorTransform = m_linearManipulator->GetLocalTransform();

        // ensure final world positions match
        EXPECT_THAT(finalManipulatorTransform, IsCloseTolerance(finalTransformWorld, 0.01f));
    }

    TEST_F(ManipulatorCoreInteractionFixture, MouseUpOfOtherMouseButtonDoesNotEndManipulatorInteraction)
    {
        // setup viewport/camera
        m_cameraState.m_viewportSize = AzFramework::ScreenSize(1280, 720);
        AzFramework::SetCameraTransform(m_cameraState, AZ::Transform::CreateIdentity());

        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AzToolsFramework::CreateManipulatorViewSphere(
            // note: use a small radius for the manipulator view/bounds to ensure precise mouse movement
            AZ::Color{},
            0.001f,
            [](const AzToolsFramework::ViewportInteraction::MouseInteraction&, bool, const AZ::Color&)
            {
                return AZ::Color{};
            }));

        m_linearManipulator->SetViews(views);
        m_linearManipulator->Register(m_viewportManipulatorInteraction->GetManipulatorManagerId());

        // the transform of the manipulator in world space
        const auto transformWorld = AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 10.0f, 0.0f));
        // the position of the manipulator in screen space
        const auto positionScreen = AzFramework::WorldToScreen(transformWorld.GetTranslation(), m_cameraState);

        m_linearManipulator->SetSpace(AZ::Transform::CreateIdentity());
        m_linearManipulator->SetLocalTransform(transformWorld);

        // press and drag the mouse (starting where the surface manipulator is)
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(positionScreen)
            ->MouseLButtonDown()
            ->MouseRButtonDown()
            ->MouseRButtonUp();

        bool interacting = false;
        AzToolsFramework::ManipulatorManagerRequestBus::EventResult(
            interacting,
            m_viewportManipulatorInteraction->GetManipulatorManagerId(),
            &AzToolsFramework::ManipulatorManagerRequestBus::Events::Interacting);

        EXPECT_THAT(interacting, ::testing::IsTrue());
    }
} // namespace UnitTest

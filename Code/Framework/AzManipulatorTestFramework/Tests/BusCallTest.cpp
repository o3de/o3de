/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include "AzManipulatorTestFrameworkTestFixtures.h"
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>

namespace UnitTest
{
    class AzManipulatorTestFrameworkBusCallTestFixture
        : public LinearManipulatorTestFixture
    {
    protected:
        AzManipulatorTestFrameworkBusCallTestFixture()
            : LinearManipulatorTestFixture(AzToolsFramework::g_mainManipulatorManagerId) {}

        bool IsManipulatorInteractingBusCall() const
        {
            bool manipulatorInteracting = false;
            AzToolsFramework::ManipulatorManagerRequestBus::EventResult(
                manipulatorInteracting, AzToolsFramework::g_mainManipulatorManagerId,
                &AzToolsFramework::ManipulatorManagerRequestBus::Events::Interacting);

            return manipulatorInteracting;
        }
    };

    TEST_F(AzManipulatorTestFrameworkBusCallTestFixture, ConsumeViewportLeftMouseClick)
    {
        // given a left mouse down ray in world space
        auto event = AzManipulatorTestFramework::CreateMouseInteractionEvent(
            m_interaction, AzToolsFramework::ViewportInteraction::MouseEvent::Down);

        // consume the mouse down and up events
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);
        event.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Up;
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);

        // expect the left mouse down and mouse up sanity flags to be set
        EXPECT_TRUE(m_receivedLeftMouseDown);
        EXPECT_TRUE(m_receivedLeftMouseUp);

        // do not expect the mouse move sanity flag to be set
        EXPECT_FALSE(m_receivedMouseMove);
    }

    TEST_F(AzManipulatorTestFrameworkBusCallTestFixture, ConsumeViewportMouseMoveHover)
    {
        // given a left mouse down ray in world space
        const auto event = AzManipulatorTestFramework::CreateMouseInteractionEvent(
            m_interaction, AzToolsFramework::ViewportInteraction::MouseEvent::Move);

        // consume the mouse move event
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);

        // do not expect the manipulator to be performing an action
        EXPECT_FALSE(m_linearManipulator->PerformingAction());
        EXPECT_FALSE(IsManipulatorInteractingBusCall());

        // do not expect the left mouse down/up and mouse move sanity flags to be set
        EXPECT_FALSE(m_receivedLeftMouseDown);
        EXPECT_FALSE(m_receivedMouseMove);
        EXPECT_FALSE(m_receivedLeftMouseUp);
    }

    TEST_F(AzManipulatorTestFrameworkBusCallTestFixture, ConsumeViewportMouseMoveActive)
    {
        // given a left mouse down ray in world space
        auto event = AzManipulatorTestFramework::CreateMouseInteractionEvent(
            m_interaction, AzToolsFramework::ViewportInteraction::MouseEvent::Down);

        // consume the mouse down event
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);

        // consume the mouse move event
        event.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Move;
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);

        // do not expect the mouse to be hovering over the manipulator
        EXPECT_FALSE(m_linearManipulator->MouseOver());

        // expect the manipulator to be performing an action
        EXPECT_TRUE(m_linearManipulator->PerformingAction());
        EXPECT_TRUE(IsManipulatorInteractingBusCall());

        // consume the mouse up event
        event.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Up;
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);

        // expect the left mouse down/up and mouse move sanity flags to be set
        EXPECT_TRUE(m_receivedLeftMouseDown);
        EXPECT_TRUE(m_receivedMouseMove);
        EXPECT_TRUE(m_receivedLeftMouseUp);
    }

    TEST_F(AzManipulatorTestFrameworkBusCallTestFixture, MoveManipulatorAlongAxis)
    {
        AZ::Vector3 movementAlongAxis = AZ::Vector3::CreateZero();
        const AZ::Vector3 expectedMovementAlongAxis = AZ::Vector3(-5.0f, 0.0f, 0.0f);

        const AZ::Vector3 initialManipulatorPosition = m_linearManipulator->GetLocalPosition();
        m_linearManipulator->InstallMouseMoveCallback(
            [&movementAlongAxis, this](const AzToolsFramework::LinearManipulator::Action& action)
        {
            movementAlongAxis = action.LocalPositionOffset();
            m_linearManipulator->SetLocalPosition(action.LocalPosition());
        });

        // given a left mouse down ray in world space
        auto event = AzManipulatorTestFramework::CreateMouseInteractionEvent(
            m_interaction, AzToolsFramework::ViewportInteraction::MouseEvent::Down);

        // consume the mouse down event
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);

        // move the mouse along the -x axis
        event.m_mouseInteraction.m_mousePick.m_rayOrigin += expectedMovementAlongAxis;
        event.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Move;
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);

        // expect the manipulator to be performing an action
        EXPECT_TRUE(m_linearManipulator->PerformingAction());
        EXPECT_TRUE(IsManipulatorInteractingBusCall());

        // consume the mouse up event
        event.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Up;
        AzManipulatorTestFramework::DispatchMouseInteractionEvent(event);
 
        // expect the left mouse down/up sanity flags to be set
        EXPECT_TRUE(m_receivedLeftMouseDown);
        EXPECT_TRUE(m_receivedLeftMouseUp);

        // expect the manipulator movement along the axis to match the mouse movement along the axis
        EXPECT_EQ(movementAlongAxis, expectedMovementAlongAxis);
        EXPECT_EQ(m_linearManipulator->GetLocalPosition(), initialManipulatorPosition + expectedMovementAlongAxis);
    }
} // namespace UnitTest

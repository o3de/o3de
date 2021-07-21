/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzManipulatorTestFrameworkTestFixtures.h"

namespace UnitTest
{
    class CustomManipulatorManager : public AzToolsFramework::ManipulatorManager
    {
        using ManagerBase = AzToolsFramework::ManipulatorManager;

    public:
        using ManagerBase::ManagerBase;

        size_t RegisteredManipulatorCount() const
        {
            return m_manipulatorIdToPtrMap.size();
        }
    };

    class AzManipulatorTestFrameworkCustomManagerTestFixture : public LinearManipulatorTestFixture
    {
    protected:
        AzManipulatorTestFrameworkCustomManagerTestFixture()
            : LinearManipulatorTestFixture(AzToolsFramework::ManipulatorManagerId(AZ::Crc32("TestManipulatorManagerId")))
        {
        }

        void SetUpEditorFixtureImpl() override
        {
            m_manipulatorManager = AZStd::make_shared<CustomManipulatorManager>(m_manipulatorManagerId);

            LinearManipulatorTestFixture::SetUpEditorFixtureImpl();
        }

        void TearDownEditorFixtureImpl() override
        {
            LinearManipulatorTestFixture::TearDownEditorFixtureImpl();
            m_manipulatorManager.reset();
        }

        AZStd::shared_ptr<CustomManipulatorManager> m_manipulatorManager;
    };

    TEST_F(AzManipulatorTestFrameworkCustomManagerTestFixture, RegisterManipulatorWithCustomManager)
    {
        // Expect the manager to contain one manipulator
        EXPECT_EQ(m_manipulatorManager->RegisteredManipulatorCount(), 1);
    }

    TEST_F(AzManipulatorTestFrameworkCustomManagerTestFixture, ConsumeViewportLeftMouseClick)
    {
        // consume the mouse down and up events
        m_manipulatorManager->ConsumeViewportMousePress(m_interaction);
        m_manipulatorManager->ConsumeViewportMouseRelease(m_interaction);

        // expect the left mouse down and mouse up sanity flags to be set
        EXPECT_TRUE(m_receivedLeftMouseDown);
        EXPECT_TRUE(m_receivedLeftMouseUp);

        // do not expect the mouse move sanity flag to be set
        EXPECT_FALSE(m_receivedMouseMove);
    }

    TEST_F(AzManipulatorTestFrameworkCustomManagerTestFixture, ConsumeViewportMouseMoveHover)
    {
        // consume the mouse move event
        m_manipulatorManager->ConsumeViewportMouseMove(m_interaction);

        // do not expect the manipulator to be performing an action
        EXPECT_FALSE(m_linearManipulator->PerformingAction());
        EXPECT_FALSE(m_manipulatorManager->Interacting());

        // do not expect the left mouse down/up and mouse move sanity flags to be set
        EXPECT_FALSE(m_receivedLeftMouseDown);
        EXPECT_FALSE(m_receivedMouseMove);
        EXPECT_FALSE(m_receivedLeftMouseUp);
    }

    TEST_F(AzManipulatorTestFrameworkCustomManagerTestFixture, ConsumeViewportMouseMoveActive)
    {
        // consume the mouse down and mouse move events
        m_manipulatorManager->ConsumeViewportMousePress(m_interaction);
        m_manipulatorManager->ConsumeViewportMouseMove(m_interaction);

        // do not expect the mouse to be hovering over the manipulator
        EXPECT_FALSE(m_linearManipulator->MouseOver());

        // expect the manipulator to be performing an action
        EXPECT_TRUE(m_linearManipulator->PerformingAction());
        EXPECT_TRUE(m_manipulatorManager->Interacting());

        // consume the mouse up event
        m_manipulatorManager->ConsumeViewportMouseRelease(m_interaction);

        // expect the left mouse down/up and mouse move sanity flags to be set
        EXPECT_TRUE(m_receivedLeftMouseDown);
        EXPECT_TRUE(m_receivedMouseMove);
        EXPECT_TRUE(m_receivedLeftMouseUp);
    }

    TEST_F(AzManipulatorTestFrameworkCustomManagerTestFixture, MoveManipulatorAlongAxis)
    {
        AZ::Vector3 movementAlongAxis = AZ::Vector3::CreateZero();
        const AZ::Vector3 expectedPositionAfterMovementAlongAxis = AZ::Vector3(-5.0f, 0.0f, 0.0f);

        m_linearManipulator->InstallMouseMoveCallback(
            [&movementAlongAxis](const AzToolsFramework::LinearManipulator::Action& action)
            {
                movementAlongAxis = action.m_current.m_localPositionOffset;
            });

        // consume the mouse down event
        m_manipulatorManager->ConsumeViewportMousePress(m_interaction);

        // move the mouse along the -x axis
        m_interaction.m_mousePick.m_rayOrigin += expectedPositionAfterMovementAlongAxis;
        m_manipulatorManager->ConsumeViewportMouseMove(m_interaction);

        // expect the manipulator to be performing an action
        EXPECT_TRUE(m_linearManipulator->PerformingAction());
        EXPECT_TRUE(m_manipulatorManager->Interacting());

        // consume the mouse up event
        m_manipulatorManager->ConsumeViewportMouseRelease(m_interaction);

        // expect the left mouse down/up sanity flags to be set
        EXPECT_TRUE(m_receivedLeftMouseDown);
        EXPECT_TRUE(m_receivedLeftMouseUp);

        // expect the manipulator movement along the axis to match the mouse movement along the axis
        EXPECT_EQ(movementAlongAxis, expectedPositionAfterMovementAlongAxis);
    }
} // namespace UnitTest

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace UnitTest
{
    class AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture : public ToolsApplicationFixture
    {
    protected:
        struct State
        {
            State(AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteraction> viewportManipulatorInteraction)
                : m_viewportManipulatorInteraction(viewportManipulatorInteraction.release())
                , m_actionDispatcher(
                      AZStd::make_unique<AzManipulatorTestFramework::ImmediateModeActionDispatcher>(*m_viewportManipulatorInteraction))
                , m_linearManipulator(AzManipulatorTestFramework::CreateLinearManipulator(
                      m_viewportManipulatorInteraction->GetManipulatorManager().GetId(),
                      /*position=*/AZ::Vector3(0.0f, 50.0f, 0.0f),
                      /*radius=*/m_boundsRadius))
            {
                // default sanity check call backs
                m_linearManipulator->InstallLeftMouseDownCallback(
                    [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
                    {
                        m_receivedLeftMouseDown = true;
                    });

                m_linearManipulator->InstallMouseMoveCallback(
                    [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
                    {
                        m_receivedMouseMove = true;
                    });

                m_linearManipulator->InstallLeftMouseUpCallback(
                    [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
                    {
                        m_receivedLeftMouseUp = true;
                    });
            }

            ~State() = default;

            // sanity flags for manipulator mouse callbacks
            bool m_receivedLeftMouseDown = false;
            bool m_receivedMouseMove = false;
            bool m_receivedLeftMouseUp = false;

        private:
            AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteraction> m_viewportManipulatorInteraction;

        public:
            AZStd::unique_ptr<AzManipulatorTestFramework::ImmediateModeActionDispatcher> m_actionDispatcher;
            AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_linearManipulator;
        };

        void ConsumeViewportLeftMouseClick(State& state);
        void ConsumeViewportMouseMoveHover(State& state);
        void ConsumeViewportMouseMoveActive(State& state);
        void MoveManipulatorAlongAxis(State& state);

    protected:
        void SetUpEditorFixtureImpl() override
        {
            m_directState =
                AZStd::make_unique<State>(AZStd::make_unique<AzManipulatorTestFramework::DirectCallManipulatorViewportInteraction>(
                    AZStd::make_shared<NullDebugDisplayRequests>()));
            m_busState =
                AZStd::make_unique<State>(AZStd::make_unique<AzManipulatorTestFramework::IndirectCallManipulatorViewportInteraction>(
                    AZStd::make_shared<NullDebugDisplayRequests>()));
            m_cameraState =
                AzFramework::CreateIdentityDefaultCamera(AZ::Vector3::CreateZero(), AzManipulatorTestFramework::DefaultViewportSize);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_directState.reset();
            m_busState.reset();
        }

    public:
        AZStd::unique_ptr<State> m_directState;
        AZStd::unique_ptr<State> m_busState;
        AzFramework::CameraState m_cameraState;
        static constexpr float m_boundsRadius = 2.0f;
    };

    void AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture::ConsumeViewportLeftMouseClick(State& state)
    {
        // given a left mouse down ray in world space
        // consume the mouse down and up events
        state.m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(AzManipulatorTestFramework::GetCameraStateViewportCenter(m_cameraState))
            ->MouseLButtonDown()
            ->Trace("Expecting left mouse button down")
            ->ExpectTrue(state.m_receivedLeftMouseDown)
            ->Trace("Not expecting left mouse button up")
            ->ExpectFalse(state.m_receivedLeftMouseUp)
            ->Trace("Expecting mouse move")
            // note: a zero delta mouse move is generated after every mouse up event in the
            // editor application - the manipulator test framework simulates this event to
            // ensure the tests are representative of what actually happens in the editor
            // therefore we do expect m_receivedMouseMove to be true after MouseLButtonDown
            ->ExpectTrue(state.m_receivedMouseMove)
            ->ExpectManipulatorBeingInteracted()
            ->MouseLButtonUp()
            ->Trace("Expecting left mouse button up")
            ->ExpectTrue(state.m_receivedLeftMouseDown)
            ->ExpectTrue(state.m_receivedLeftMouseUp)
            ->ExpectTrue(state.m_receivedMouseMove)
            ->ExpectFalse(state.m_linearManipulator->PerformingAction())
            ->ExpectManipulatorNotBeingInteracted();
    }

    void AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture::ConsumeViewportMouseMoveHover(State& state)
    {
        // given a left mouse down ray in world space
        // consume the mouse move event
        state.m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(AzManipulatorTestFramework::GetCameraStateViewportCenter(m_cameraState))
            ->ExpectFalse(state.m_linearManipulator->PerformingAction())
            ->ExpectManipulatorNotBeingInteracted()
            ->ExpectFalse(state.m_receivedLeftMouseDown)
            ->ExpectFalse(state.m_receivedMouseMove)
            ->ExpectFalse(state.m_receivedLeftMouseUp);
    }

    void AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture::ConsumeViewportMouseMoveActive(State& state)
    {
        // given a left mouse down ray in world space
        // consume the mouse move event
        state.m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(AzManipulatorTestFramework::GetCameraStateViewportCenter(m_cameraState))
            ->MouseLButtonDown()
            ->ExpectTrue(state.m_linearManipulator->PerformingAction())
            ->ExpectManipulatorBeingInteracted()
            ->MouseLButtonUp()
            ->ExpectTrue(state.m_receivedLeftMouseDown)
            ->ExpectTrue(state.m_receivedMouseMove)
            ->ExpectTrue(state.m_receivedLeftMouseUp);
    }

    void AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture::MoveManipulatorAlongAxis(State& state)
    {
        // the initial starting position of the manipulator (in front of the camera)
        const auto initialPositionWorld = state.m_linearManipulator->GetLocalPosition();
        // where the manipulator should end up (in front and to the left of the camera)
        const auto finalPositionWorld = AZ::Vector3(-10.0f, 50.0f, 0.0f);
        // perspective scale factor for manipulator distance to camera
        const auto scaledRadiusBound =
            AzToolsFramework::CalculateScreenToWorldMultiplier(initialPositionWorld, m_cameraState) * m_boundsRadius;
        // vector from camera to manipulator
        const auto vectorToInitialPositionWorld = (initialPositionWorld - m_cameraState.m_position).GetNormalized();
        // adjusted final world position taking into account the manipulator position relative to the camera
        const auto finalPositionWorldAdjusted = finalPositionWorld - (vectorToInitialPositionWorld * scaledRadiusBound);
        // calculate the position in screen space of the initial position of the manipulator
        const auto initialPositionScreen = AzFramework::WorldToScreen(initialPositionWorld, m_cameraState);
        // calculate the position in screen space of the final position of the manipulator
        const auto finalPositionScreen = AzFramework::WorldToScreen(finalPositionWorldAdjusted, m_cameraState);

        AZ::Vector3 movementAlongAxis = AZ::Vector3::CreateZero();

        state.m_linearManipulator->InstallMouseMoveCallback(
            [&movementAlongAxis](const AzToolsFramework::LinearManipulator::Action& action)
            {
                movementAlongAxis = action.LocalPosition();
            });

        state.m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(initialPositionScreen)
            ->MouseLButtonDown()
            ->ExpectTrue(state.m_linearManipulator->PerformingAction())
            ->ExpectManipulatorBeingInteracted()
            ->MousePosition(finalPositionScreen)
            ->MouseLButtonUp()
            ->ExpectTrue(state.m_receivedLeftMouseDown)
            ->ExpectTrue(state.m_receivedLeftMouseUp)
            ->ExpectTrue(movementAlongAxis.IsClose(finalPositionWorld, 0.01f));
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture, ConsumeViewportLeftMouseClick)
    {
        ConsumeViewportLeftMouseClick(*m_directState);
        ConsumeViewportLeftMouseClick(*m_busState);
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture, ConsumeViewportMouseMoveHover)
    {
        ConsumeViewportMouseMoveHover(*m_directState);
        ConsumeViewportMouseMoveHover(*m_busState);
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture, ConsumeViewportMouseMoveActive)
    {
        ConsumeViewportMouseMoveActive(*m_directState);
        ConsumeViewportMouseMoveActive(*m_busState);
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTestFixture, MoveManipulatorAlongAxis)
    {
        MoveManipulatorAlongAxis(*m_directState);
        MoveManipulatorAlongAxis(*m_busState);
    }
} // namespace UnitTest

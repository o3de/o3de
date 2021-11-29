/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzManipulatorTestFrameworkTestFixtures.h"

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace UnitTest
{
    class GridSnappingFixture : public ToolsApplicationFixture
    {
    public:
        GridSnappingFixture()
            : m_viewportManipulatorInteraction(AZStd::make_unique<AzManipulatorTestFramework::DirectCallManipulatorViewportInteraction>())
            , m_actionDispatcher(
                  AZStd::make_unique<AzManipulatorTestFramework::ImmediateModeActionDispatcher>(*m_viewportManipulatorInteraction))
        {
        }

    protected:
        void SetUpEditorFixtureImpl() override
        {
            m_cameraState =
                AzFramework::CreateIdentityDefaultCamera(AZ::Vector3::CreateZero(), AzManipulatorTestFramework::DefaultViewportSize);
        }

    public:
        const float m_boundsRadius = 1.0f;
        AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteraction> m_viewportManipulatorInteraction;
        AZStd::unique_ptr<AzManipulatorTestFramework::ImmediateModeActionDispatcher> m_actionDispatcher;
        AzFramework::CameraState m_cameraState;
    };

    TEST_F(GridSnappingFixture, MouseDownWithSnappingEnabledSnapsToClosestGridSize)
    {
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> linearManipulator(AzManipulatorTestFramework::CreateLinearManipulator(
            m_viewportManipulatorInteraction->GetManipulatorManager().GetId(),
            /*position=*/AZ::Vector3(0.0f, 50.0f, 0.0f),
            /*radius=*/m_boundsRadius));

        // the initial starting position of the manipulator (in front of the camera)
        const auto initialPositionWorld = linearManipulator->GetLocalPosition();
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

        // callback to update the manipulator's current position
        linearManipulator->InstallMouseMoveCallback(
            [linearManipulator](const AzToolsFramework::LinearManipulator::Action& action)
            {
                linearManipulator->SetLocalPosition(action.LocalPosition());
            });

        m_actionDispatcher->SetSnapToGrid(true)
            ->GridSize(5.0f)
            ->CameraState(m_cameraState)
            ->MousePosition(initialPositionScreen)
            ->MouseLButtonDown()
            ->ExpectManipulatorBeingInteracted()
            ->MousePosition(finalPositionScreen)
            ->MouseLButtonUp()
            ->ExpectManipulatorNotBeingInteracted()
            ->ExpectTrue(linearManipulator->GetLocalPosition().IsClose(finalPositionWorld, 0.01f));
    }

    template<typename Manipulator>
    void ValidateManipulatorSnappingBehavior(
        AZStd::shared_ptr<Manipulator> manipulator,
        AzManipulatorTestFramework::ImmediateModeActionDispatcher* actionDispatcher,
        const AzFramework::CameraState& cameraState)
    {
        manipulator->SetLocalOrientation(AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(180.0f, 0.0f, 135.0f)));

        // the initial starting position of the manipulator (in front of the camera)
        const auto initialPositionWorld = manipulator->GetLocalPosition() + AZ::Vector3::CreateAxisX(0.15f);
        // where the manipulator should end up (unmoved)
        const auto finalPositionWorld = manipulator->GetLocalPosition();
        // where we should move the mouse to
        const auto attemptPositionWorld = manipulator->GetLocalPosition() + AZ::Vector3::CreateAxisX(0.35f);
        // calculate the position in screen space of the initial position of the manipulator
        const auto initialPositionScreen = AzFramework::WorldToScreen(initialPositionWorld, cameraState);
        // calculate the position in screen space of the final position of the manipulator
        const auto attemptPositionScreen = AzFramework::WorldToScreen(attemptPositionWorld, cameraState);

        // callback to update the manipulator's current position
        manipulator->InstallMouseMoveCallback(
            [manipulator](const typename Manipulator::Action& action)
            {
                manipulator->SetLocalPosition(action.LocalPosition());
            });

        actionDispatcher->SetSnapToGrid(true)
            ->GridSize(1.0f)
            ->CameraState(cameraState)
            ->MousePosition(initialPositionScreen)
            ->MouseLButtonDown()
            ->ExpectManipulatorBeingInteracted()
            ->MousePosition(attemptPositionScreen)
            ->MouseLButtonUp()
            ->ExpectManipulatorNotBeingInteracted()
            ->ExpectThat(manipulator->GetLocalPosition(), IsCloseTolerance(finalPositionWorld, 0.01f));
    }

    TEST_F(GridSnappingFixture, MouseDownAndMoveLinearManipulatorDoesNotSnapWithMovementSmallerThanHalfGridSize)
    {
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> linearManipulator(AzManipulatorTestFramework::CreateLinearManipulator(
            m_viewportManipulatorInteraction->GetManipulatorManager().GetId(),
            /*position=*/AZ::Vector3(0.0f, 10.0f, 0.0f),
            /*radius=*/m_boundsRadius));

        linearManipulator->SetAxis(AZ::Vector3::CreateAxisY());

        ValidateManipulatorSnappingBehavior(linearManipulator, m_actionDispatcher.get(), m_cameraState);
    }

    TEST_F(GridSnappingFixture, MouseDownAndMovePlanarManipulatorDoesNotSnapWithMovementSmallerThanHalfGridSize)
    {
        AZStd::shared_ptr<AzToolsFramework::PlanarManipulator> planarManipulator(AzManipulatorTestFramework::CreatePlanarManipulator(
            m_viewportManipulatorInteraction->GetManipulatorManager().GetId(),
            /*position=*/AZ::Vector3(0.0f, 10.0f, 0.0f),
            /*radius=*/m_boundsRadius));

        planarManipulator->SetAxes(AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());

        ValidateManipulatorSnappingBehavior(planarManipulator, m_actionDispatcher.get(), m_cameraState);
    }
} // namespace UnitTest

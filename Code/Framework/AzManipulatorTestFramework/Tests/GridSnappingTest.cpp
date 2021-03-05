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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include "AzManipulatorTestFrameworkTestFixtures.h"
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace UnitTest
{
    class GridSnappingFixture
        : public ToolsApplicationFixture
    {
    public:
        GridSnappingFixture()
            : m_viewportManipulatorInteraction(AZStd::make_unique<AzManipulatorTestFramework::DirectCallManipulatorViewportInteraction>())
            , m_actionDispatcher(AZStd::make_unique<AzManipulatorTestFramework::ImmediateModeActionDispatcher>(*m_viewportManipulatorInteraction))
            , m_linearManipulator(
                AzManipulatorTestFramework::CreateLinearManipulator(
                    m_viewportManipulatorInteraction->GetManipulatorManager().GetId(),
                    /*position=*/AZ::Vector3(0.0f, 50.0f, 0.0f),
                    /*radius=*/m_boundsRadius))
        {}

    protected:
        void SetUpEditorFixtureImpl() override
        {
            m_cameraState = AzFramework::CreateIdentityDefaultCamera(
                AZ::Vector3::CreateZero(), AzManipulatorTestFramework::DefaultViewportSize);
        }

    public:

        const float m_boundsRadius = 1.0f;
        AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteraction> m_viewportManipulatorInteraction;
        AZStd::unique_ptr<AzManipulatorTestFramework::ImmediateModeActionDispatcher> m_actionDispatcher;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_linearManipulator;
        AzFramework::CameraState m_cameraState;
    };

    TEST_F(GridSnappingFixture, MouseDownWithSnappingEnabledSnapsToClosestGridSize)
    {
        // the initial starting position of the manipulator (in front of the camera)
        const auto initialPositionWorld = m_linearManipulator->GetPosition();
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
        const auto initialPositionScreen =
            AzFramework::WorldToScreen(initialPositionWorld, m_cameraState);
        // calculate the position in screen space of the final position of the manipulator
        const auto finalPositionScreen = AzFramework::WorldToScreen(finalPositionWorldAdjusted, m_cameraState);

        // callback to update the manipulator's current position
        m_linearManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
        {
            auto pos = action.LocalPosition();
            m_linearManipulator->SetLocalPosition(pos);
        });

        m_actionDispatcher
            ->EnableSnapToGrid()
            ->GridSize(5.0f)
            ->CameraState(m_cameraState)
            ->MousePosition(initialPositionScreen)
            ->MouseLButtonDown()
            ->ExpectManipulatorBeingInteracted()
            ->MousePosition(finalPositionScreen)
            ->MouseLButtonUp()
            ->ExpectManipulatorNotBeingInteracted()
            ->ExpectTrue(m_linearManipulator->GetPosition().IsClose(finalPositionWorld, 0.01f))
            ;
    }
} // namespace UnitTest

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Asset/EditorWhiteBoxMeshAsset.h"
#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxPolygonModifierBus.h"
#include "EditorWhiteBoxSystemComponent.h"
#include "Rendering/WhiteBoxNullRenderMesh.h"
#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "Viewport/WhiteBoxManipulatorViews.h"
#include "WhiteBox/EditorWhiteBoxComponentBus.h"
#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <QMessageBox>

namespace UnitTest
{
    static const AzToolsFramework::ManipulatorManagerId TestManipulatorManagerId =
        AzToolsFramework::ManipulatorManagerId(AZ::Crc32("TestManipulatorManagerId"));

    class WhiteBoxManipulatorFixture : public WhiteBoxTestFixture
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<AzToolsFramework::ManipulatorManager> m_manipulatorManager;
    };

    void WhiteBoxManipulatorFixture::SetUpEditorFixtureImpl()
    {
        m_manipulatorManager = AZStd::make_unique<AzToolsFramework::ManipulatorManager>(TestManipulatorManagerId);
    }

    void WhiteBoxManipulatorFixture::TearDownEditorFixtureImpl()
    {
        m_manipulatorManager.reset();
    }

    TEST_F(WhiteBoxManipulatorFixture, ManipulatorBoundsRefreshedAfterBeingMarkedDirty)
    {
        namespace Api = WhiteBox::Api;

        // create the direct call manipulator viewport interaction and an immediate mode dispatcher
        AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteraction> viewportManipulatorInteraction =
            AZStd::make_unique<AzManipulatorTestFramework::DirectCallManipulatorViewportInteraction>(
                AZStd::make_shared<NullDebugDisplayRequests>());
        AZStd::unique_ptr<AzManipulatorTestFramework::ImmediateModeActionDispatcher> actionDispatcher =
            AZStd::make_unique<AzManipulatorTestFramework::ImmediateModeActionDispatcher>(
                *viewportManipulatorInteraction);

        // create and register the manipulator with the test manipulator manager
        auto manipulator = AzToolsFramework::LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());
        manipulator->Register(viewportManipulatorInteraction->GetManipulatorManagerId());

        // create a simple white box mesh
        Api::InitializeAsUnitQuad(*m_whiteBox);

        // create polygon manipulator view from white box
        const Api::PolygonHandle polygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const Api::VertexPositionsCollection outlines = Api::PolygonBorderVertexPositions(*m_whiteBox, polygonHandle);
        const AZStd::vector<AZ::Vector3> triangles = Api::PolygonFacesPositions(*m_whiteBox, polygonHandle);
        auto polygonView = WhiteBox::CreateManipulatorViewPolygon(triangles, outlines);

        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AZStd::move(polygonView));
        manipulator->SetViews(views);

        // position the manipulator offset down the y axis
        const AZ::Vector3& initialPosition = AZ::Vector3::CreateAxisY(10.0f);
        manipulator->SetLocalPosition(initialPosition);

        // simple callback to update the manipulator's current position
        manipulator->InstallMouseMoveCallback(
            [initialPosition, &manipulator](const AzToolsFramework::LinearManipulator::Action& action)
            {
                manipulator->SetLocalPosition(initialPosition + action.LocalPositionOffset());
            });

        // camera state to represent the viewer in world space
        auto cameraState = AzFramework::CreateIdentityDefaultCamera(AZ::Vector3::CreateZero(), DefaultViewportSize);

        // the initial starting position of the mouse (center of viewport)
        const auto initialPositionScreen = AzManipulatorTestFramework::GetCameraStateViewportCenter(cameraState);
        // the final position of the mouse (an arbitrary amount of pixels to the right)
        const auto finalPositionScreen =
            AzFramework::ScreenPoint(initialPositionScreen.m_x + 100, initialPositionScreen.m_y);

        actionDispatcher->CameraState(cameraState)
            ->Trace("center the camera at the origin")
            ->CameraState(AzManipulatorTestFramework::SetCameraStatePosition(AZ::Vector3::CreateZero(), cameraState))
            ->Trace("point the camera down the y axis")
            ->CameraState(AzManipulatorTestFramework::SetCameraStateDirection(AZ::Vector3::CreateAxisY(), cameraState))
            ->Trace("move to a valid position so the mouse pick ray intersects the manipulator bound (view)")
            ->MousePosition(initialPositionScreen)
            ->Trace("verify precondition - the manipulator recognizes is has the mouse over it")
            ->ExpectTrue(manipulator->MouseOver())
            ->Trace("simulate a click and drag motion (click and then move the camera to the right)")
            ->MouseLButtonDown()
            ->MousePosition(finalPositionScreen)
            ->Trace("mouse up after (ending drag)")
            ->MouseLButtonUp()
            ->Trace("simulate event from Qt (immediate mouse move after mouse up)")
            ->MousePosition(finalPositionScreen)
            ->ExpectTrue(manipulator->MouseOver());
        ;
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, EditorWhiteBoxComponentRespectsEntityHiddenVisibility)
    {
        // given (precondition)
        EXPECT_TRUE(m_whiteBoxComponent->HasRenderMesh());

        // when
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);

        // then
        EXPECT_FALSE(m_whiteBoxComponent->HasRenderMesh());
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, EditorWhiteBoxComponentRespectsEntityHiddenVisibilityWhenActivated)
    {
        // given (precondition)
        EXPECT_TRUE(m_whiteBoxComponent->HasRenderMesh());
        m_whiteBoxComponent->Deactivate();

        // when
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);

        // then
        m_whiteBoxComponent->Activate();
        EXPECT_FALSE(m_whiteBoxComponent->HasRenderMesh());
    }

    // simple listener class to listen for changes to polygon handles after a change
    // by a polygon modifier
    class PolygonModifierDetector : private WhiteBox::EditorWhiteBoxPolygonModifierNotificationBus::Handler
    {
        void OnPolygonModifierUpdatedPolygonHandle(
            const WhiteBox::Api::PolygonHandle& previousPolygonHandle,
            const WhiteBox::Api::PolygonHandle& nextPolygonHandle) override
        {
            m_previousPolygonHandle = previousPolygonHandle;
            m_nextPolygonHandle = nextPolygonHandle;
        }

    public:
        PolygonModifierDetector(const AZ::EntityComponentIdPair& entityComponentIdPair)
        {
            WhiteBox::EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusConnect(entityComponentIdPair);
        }

        ~PolygonModifierDetector()
        {
            WhiteBox::EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusDisconnect();
        }

        WhiteBox::Api::PolygonHandle m_previousPolygonHandle;
        WhiteBox::Api::PolygonHandle m_nextPolygonHandle;
    };

    TEST_F(EditorWhiteBoxModifierTestFixture, SelectedPolygonHandleModifierUpdatesAfterExtrusion)
    {
        namespace Api = WhiteBox::Api;
        using AzToolsFramework::ViewportInteraction::KeyboardModifier;
        using ::testing::Ne;

        // the initial starting position of the entity (in front just below the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 8.0f, 23.0f));

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox = nullptr;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // create a 3x3 grid from the starting cube and hide all top edges
        Initialize3x3CubeGrid(*whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*whiteBox);

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        // used to listen for when a polygon handle has been updated (the same as in DefaultMode)
        PolygonModifierDetector polygonModifierDetector(entityComponentIdPair);

        AzFramework::SetCameraTransform(
            m_cameraState, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 25.0f)));

        // the middle of the top, merged polygon (3x3)
        MultiSpacePoint topPolygonMidpoint(
            Api::PolygonMidpoint(*whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(36))),
            initialEntityTransformWorld, m_cameraState);

        // middle of simple square polygon at the bottom right of the screen facing the camera
        MultiSpacePoint forwardPolygonMidpoint(
            Api::PolygonMidpoint(*whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(18))),
            initialEntityTransformWorld, m_cameraState);

        // the position to move to when doing the polygon impression
        MultiSpacePoint movedForwardPolygonMidpoint(
            forwardPolygonMidpoint.GetLocalSpace() + AZ::Vector3::CreateAxisY(), initialEntityTransformWorld,
            m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the top middle of the merged polygons (3x3 square grid)
            ->MousePosition(topPolygonMidpoint.GetScreenSpace())
            // select the polygon - creates a scale manipulator
            ->MouseLButtonDown()
            ->MouseLButtonUp()
            // move the mouse to the front right polygon facing the camera
            ->MousePosition(forwardPolygonMidpoint.GetScreenSpace())
            // appends inwards creating an impression by 1 meter in the y axis
            ->KeyboardModifierDown(KeyboardModifier::Ctrl)
            ->MouseLButtonDown()
            ->MousePosition(movedForwardPolygonMidpoint.GetScreenSpace())
            // release after moving the polygon manipulator
            ->MouseLButtonUp();

        // the size of the polygon handles should be the same (no faces have been merged or split)
        EXPECT_EQ(
            polygonModifierDetector.m_nextPolygonHandle.m_faceHandles.size(),
            polygonModifierDetector.m_previousPolygonHandle.m_faceHandles.size());
        // but the handles will have changed as the mesh will have updates after internally adding/removing new verts
        EXPECT_THAT(polygonModifierDetector.m_nextPolygonHandle, Ne(polygonModifierDetector.m_previousPolygonHandle));
    }

    TEST_F(EditorWhiteBoxModifierTestFixture, SwitchToRestoreModeDestroysModifierWhileInteractingWithFace)
    {
        namespace Api = WhiteBox::Api;
        using AzToolsFramework::ViewportInteraction::KeyboardModifier;

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());

        const auto displayEntityViewport = [whiteBoxEntityId = m_whiteBoxEntityId]()
        {
            AzFramework::EntityDebugDisplayEventBus::Event(
                whiteBoxEntityId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
                AzFramework::ViewportInfo{0}, NullDebugDisplayRequests{});
        };

        // helper to check which submode we're in
        const auto subMode = [entityComponentIdPair]()
        {
            WhiteBox::SubMode subMode;
            WhiteBox::EditorWhiteBoxComponentModeRequestBus::EventResult(
                subMode, entityComponentIdPair,
                &WhiteBox::EditorWhiteBoxComponentModeRequestBus::Events::GetCurrentSubMode);
            return subMode;
        };

        // the initial starting position of the entity (in front of and just below the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 7.0f, 23.0f));

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox = nullptr;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        // override the default modifier key behavior for the white box component mode
        WhiteBox::EditorWhiteBoxComponentModeRequestBus::Event(
            entityComponentIdPair,
            &WhiteBox::EditorWhiteBoxComponentModeRequestBus::Events::OverrideKeyboardModifierQuery,
            [this]()
            {
                return m_actionDispatcher->QueryKeyboardModifiers();
            });

        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-45.0f)), AZ::Vector3(0.0f, 4.0f, 26.0f)));

        const AZ::Vector3 topPolygonMidpointLocal =
            Api::PolygonMidpoint(*whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(1)));

        const AZ::Vector3 topPolygonNextPositionLocal = topPolygonMidpointLocal + AZ::Vector3::CreateAxisZ(0.5f);

        // the middle of the top
        MultiSpacePoint topPolygonMidpoint(topPolygonMidpointLocal, initialEntityTransformWorld, m_cameraState);
        MultiSpacePoint topPolygonNext(topPolygonNextPositionLocal, initialEntityTransformWorld, m_cameraState);

        // begin interacting with a polygon and then use the modifier keys to
        // mimic transitioning to restore mode
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(topPolygonMidpoint.GetScreenSpace())
            ->MouseLButtonDown()
            ->MousePosition(topPolygonNext.GetScreenSpace())
            ->KeyboardModifierDown(KeyboardModifier::Ctrl)
            ->KeyboardModifierDown(KeyboardModifier::Shift)
            // trigger moving to RestoreMode (handled in Display of EditorWhiteBoxComponentMode)
            ->ExecuteBlock(
                [displayEntityViewport]()
                {
                    displayEntityViewport();
                })
            ->ExpectEq(subMode(), WhiteBox::SubMode::EdgeRestore)
            // continue trying to move in RestoreMode
            ->MousePosition(topPolygonMidpoint.GetScreenSpace())
            ->MouseLButtonUp()
            ->KeyboardModifierUp(KeyboardModifier::Ctrl)
            ->KeyboardModifierUp(KeyboardModifier::Shift)
            // run update/draw logic again to change modes
            ->ExecuteBlock(
                [displayEntityViewport]()
                {
                    displayEntityViewport();
                })
            // verify we are back in DefaultMode
            ->ExpectEq(subMode(), WhiteBox::SubMode::Default);
    }

    TEST_F(EditorWhiteBoxModifierTestFixture, SwitchToRestoreModeDestroysModifierWhileInteractingWithVertex)
    {
        namespace Api = WhiteBox::Api;
        using AzToolsFramework::ViewportInteraction::KeyboardModifier;

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());

        const auto displayEntityViewport = [whiteBoxEntityId = m_whiteBoxEntityId]()
        {
            AzFramework::EntityDebugDisplayEventBus::Event(
                whiteBoxEntityId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
                AzFramework::ViewportInfo{0}, NullDebugDisplayRequests{});
        };

        // helper to check which submode we're in
        const auto subMode = [entityComponentIdPair]()
        {
            WhiteBox::SubMode subMode;
            WhiteBox::EditorWhiteBoxComponentModeRequestBus::EventResult(
                subMode, entityComponentIdPair,
                &WhiteBox::EditorWhiteBoxComponentModeRequestBus::Events::GetCurrentSubMode);
            return subMode;
        };

        // the initial starting position of the entity (in front of and just below the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 7.0f, 23.0f));

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox = nullptr;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        // override the default modifier key behavior for the white box component mode
        WhiteBox::EditorWhiteBoxComponentModeRequestBus::Event(
            entityComponentIdPair,
            &WhiteBox::EditorWhiteBoxComponentModeRequestBus::Events::OverrideKeyboardModifierQuery,
            [this]()
            {
                return m_actionDispatcher->QueryKeyboardModifiers();
            });

        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-45.0f)), AZ::Vector3(0.0f, 4.0f, 26.0f)));

        const AZ::Vector3 vertexLocalPosition = Api::VertexPosition(*whiteBox, Api::VertexHandle(1)); 
        const AZ::Vector3 topPolygonNextPositionLocal = vertexLocalPosition + AZ::Vector3::CreateAxisZ(0.5f);

        // the middle of the top
        MultiSpacePoint topPolygonMidpoint(vertexLocalPosition, initialEntityTransformWorld, m_cameraState);
        MultiSpacePoint topPolygonNext(topPolygonNextPositionLocal, initialEntityTransformWorld, m_cameraState);

        // begin interacting with a polygon and then use the modifier keys to
        // mimic transitioning to restore mode
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(topPolygonMidpoint.GetScreenSpace())
            ->MouseLButtonDown()
            ->MousePosition(topPolygonNext.GetScreenSpace())
            ->KeyboardModifierDown(KeyboardModifier::Ctrl)
            ->KeyboardModifierDown(KeyboardModifier::Shift)
            // trigger moving to RestoreMode (handled in Display of EditorWhiteBoxComponentMode)
            ->ExecuteBlock(
                [displayEntityViewport]()
                {
                    displayEntityViewport();
                })
            ->ExpectEq(subMode(), WhiteBox::SubMode::EdgeRestore)
            // continue trying to move in RestoreMode
            ->MousePosition(topPolygonMidpoint.GetScreenSpace())
            ->MouseLButtonUp()
            ->KeyboardModifierUp(KeyboardModifier::Ctrl)
            ->KeyboardModifierUp(KeyboardModifier::Shift)
            // run update/draw logic again to change modes
            ->ExecuteBlock(
                [displayEntityViewport]()
                {
                    displayEntityViewport();
                })
            // verify we are back in DefaultMode
            ->ExpectEq(subMode(), WhiteBox::SubMode::Default);
    }
    TEST_F(EditorWhiteBoxModifierTestFixture, SelectedVertexModifierIsCleanedUpAfterDefaultShapeChange)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::ElementsAre;

        // the initial starting position of the entity (in front of and just below the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 8.0f, 23.0f));

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox = nullptr;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // create a 3x3 grid from the starting cube
        Initialize3x3CubeGrid(*whiteBox);

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        AzFramework::SetCameraTransform(
            m_cameraState, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 25.0f)));

        // arbitrary vertex (top, right corner of the tessellated box)
        MultiSpacePoint vertexHandle14Position(
            Api::VertexPosition(*whiteBox, Api::VertexHandle(14)), initialEntityTransformWorld, m_cameraState);

        // select the vertex
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(vertexHandle14Position.GetScreenSpace())
            ->MouseLButtonDown()
            ->MouseLButtonUp();

        // little wrapper for EBus call
        const auto selectedVertexHandles = [entityComponentIdPair]()
        {
            Api::VertexHandles selectedVertexHandles;
            WhiteBox::EditorWhiteBoxDefaultModeRequestBus::EventResult(
                selectedVertexHandles, entityComponentIdPair,
                &WhiteBox::EditorWhiteBoxDefaultModeRequestBus::Events::SelectedVertexHandles);
            return selectedVertexHandles;
        };

        // given
        // verify the vertex is selected
        EXPECT_THAT(selectedVertexHandles(), ElementsAre(Api::VertexHandle(14)));

        // when
        // change the default shape
        WhiteBox::EditorWhiteBoxComponentRequestBus::Event(
            entityComponentIdPair, &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::SetDefaultShape,
            WhiteBox::DefaultShapeType::Cylinder);

        // selected vertices have been cleared
        EXPECT_THAT(selectedVertexHandles(), ElementsAre());
    }

    TEST_F(EditorWhiteBoxModifierTestFixture, HiddenVertexCannotBeHoveredInDefaultMode)
    {
        namespace Api = WhiteBox::Api;

        // the initial starting position of the entity (in front just below the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 8.0f, 23.0f));

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox = nullptr;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-45.0f)), AZ::Vector3(0.0f, 4.0f, 26.0f)));

        // given
        // create a 3x3 grid from the starting cube and hide all top edges
        Initialize3x3CubeGrid(*whiteBox);

        // calculate screen space positions for vertices
        MultiSpacePoint vertexHandle20Position(
            Api::VertexPosition(*whiteBox, Api::VertexHandle(20)), initialEntityTransformWorld, m_cameraState);
        MultiSpacePoint vertexHandle0Position(
            Api::VertexPosition(*whiteBox, Api::VertexHandle(0)), initialEntityTransformWorld, m_cameraState);
        MultiSpacePoint vertexHandle11Position(
            Api::VertexPosition(*whiteBox, Api::VertexHandle(11)), initialEntityTransformWorld, m_cameraState);
        MultiSpacePoint vertexHandle16Position(
            Api::VertexPosition(*whiteBox, Api::VertexHandle(16)), initialEntityTransformWorld, m_cameraState);

        struct MultiSpacePointVertexHandlePair
        {
            MultiSpacePoint m_multiSpacePoint;
            Api::VertexHandle m_vertexHandle;
        };

        // associate vertex handles and positions
        const MultiSpacePointVertexHandlePair multiSpacePointVertexHandlePairs[] = {
            {vertexHandle20Position, Api::VertexHandle(20)},
            {vertexHandle0Position, Api::VertexHandle(0)},
            {vertexHandle11Position, Api::VertexHandle(11)},
            {vertexHandle16Position, Api::VertexHandle(16)}};

        m_actionDispatcher->CameraState(m_cameraState);

        // wrapper for bus call
        const auto hoveredVertexHandle = [entityComponentIdPair]()
        {
            Api::VertexHandle hoveredVertexHandle;
            WhiteBox::EditorWhiteBoxDefaultModeRequestBus::EventResult(
                hoveredVertexHandle, entityComponentIdPair,
                &WhiteBox::EditorWhiteBoxDefaultModeRequestBus::Events::HoveredVertexHandle);
            return hoveredVertexHandle;
        };

        // check all vertices are tracked as hovered (before hiding edges)
        for (const auto& [multiSpacePoint, vertexHandle] : multiSpacePointVertexHandlePairs)
        {
            m_actionDispatcher->MousePosition(multiSpacePoint.GetScreenSpace());
            EXPECT_EQ(hoveredVertexHandle(), vertexHandle);
        }

        // when
        HideAllTopUserEdgesFor3x3Grid(*whiteBox);

        // then
        // check hovering over a vertex no longer returns the handle (as there are no connecting edges)
        for (const auto& [multiSpacePoint, vertexHandle] : multiSpacePointVertexHandlePairs)
        {
            m_actionDispatcher->MousePosition(multiSpacePoint.GetScreenSpace());
            EXPECT_EQ(hoveredVertexHandle(), Api::VertexHandle());
        }
    }

    AZStd::optional<AZStd::string> RelativePathNullopt([[maybe_unused]] const AZStd::string& absolutePath)
    {
        return AZStd::nullopt;
    }

    int SaveDecisionAccept()
    {
        return QMessageBox::Save;
    }

    int SaveDecisionCancel()
    {
        return QMessageBox::Cancel;
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, TrySaveEmptyWhiteBoxAssetPathCancelsSave)
    {
        using testing::IsFalse;

        const auto absoluteSavePathFn = []([[maybe_unused]] const AZStd::string& initialAbsolutePath)
        {
            return AZStd::string();
        };

        const AZStd::optional<WhiteBox::WhiteBoxSaveResult> saveResult =
            WhiteBox::TrySaveAs("Entity1", absoluteSavePathFn, &RelativePathNullopt, &SaveDecisionCancel);

        EXPECT_FALSE(saveResult.has_value());
    }

    inline ::testing::PolymorphicMatcher<::testing::internal::StrEqualityMatcher<AZStd::string>> StrEq(
        const AZStd::string& str)
    {
        return ::testing::MakePolymorphicMatcher(
            ::testing::internal::StrEqualityMatcher<AZStd::string>(str, true, true));
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, TrySaveWhiteBoxAssetCanBeSavedInsideProjectFolder)
    {
        AZStd::string absolutePath;
        const auto absoluteSavePathFn = [&absolutePath](const AZStd::string& initialAbsolutePath)
        {
            absolutePath = initialAbsolutePath;
            return absolutePath;
        };

        AZStd::string relativePath;
        const auto relativePathSuccessFn =
            [&relativePath]([[maybe_unused]] const AZStd::string& absolutePath) -> AZStd::optional<AZStd::string>
        {
            // return relative path as if the asset was at the root of the project
            AZ::IO::Path path(absolutePath);
            relativePath = AZ::IO::Path(path.Filename()).String();
            return relativePath;
        };

        const AZStd::optional<WhiteBox::WhiteBoxSaveResult> saveResult =
            WhiteBox::TrySaveAs("Entity1", absoluteSavePathFn, relativePathSuccessFn, &SaveDecisionAccept);

        EXPECT_TRUE(saveResult.has_value());
        EXPECT_THAT(saveResult->m_absoluteFilePath, StrEq(absolutePath));
        EXPECT_THAT(saveResult->m_relativeAssetPath.value(), StrEq(relativePath));
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, TrySaveWhiteBoxAssetCanBeSavedOutsideProjectFolder)
    {
        AZStd::string absolutePath;
        const auto absoluteSavePathFn = [&absolutePath](const AZStd::string& initialAbsolutePath)
        {
            absolutePath = initialAbsolutePath;
            return absolutePath;
        };

        const auto relativePathFailureFn = []([[maybe_unused]] const AZStd::string& absolutePath)
        {
            return AZStd::nullopt;
        };

        const AZStd::optional<WhiteBox::WhiteBoxSaveResult> saveResult =
            WhiteBox::TrySaveAs("Entity1", absoluteSavePathFn, relativePathFailureFn, &SaveDecisionAccept);

        EXPECT_TRUE(saveResult.has_value());
        EXPECT_THAT(saveResult->m_absoluteFilePath, StrEq(absolutePath));
        EXPECT_FALSE(saveResult->m_relativeAssetPath.has_value());
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, TrySaveWhiteBoxAssetCancelOutsideProjectFolder)
    {
        const auto absoluteSavePathFn = []([[maybe_unused]] const AZStd::string& initialAbsolutePath)
        {
            return initialAbsolutePath;
        };

        const auto relativePathFailureFn =
            []([[maybe_unused]] const AZStd::string& absolutePath) -> AZStd::optional<AZStd::string>
        {
            return AZStd::nullopt;
        };

        const AZStd::optional<WhiteBox::WhiteBoxSaveResult> saveResult =
            WhiteBox::TrySaveAs("Entity1", absoluteSavePathFn, relativePathFailureFn, &SaveDecisionCancel);

        EXPECT_FALSE(saveResult.has_value());
    }

    class EditorWhiteBoxAssetFixture : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        WhiteBox::Api::WhiteBoxMeshPtr m_whiteBox;
        WhiteBox::EditorWhiteBoxMeshAsset* m_whiteBoxMeshAsset;
        WhiteBox::EditorWhiteBoxComponent* m_editorWhiteBoxSystemComponent = nullptr;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorWhiteBoxComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorWhiteBoxSystemComponentDescriptor;
    };

    void EditorWhiteBoxAssetFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_editorWhiteBoxComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(WhiteBox::EditorWhiteBoxComponent::CreateDescriptor());
        m_editorWhiteBoxSystemComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(WhiteBox::EditorWhiteBoxSystemComponent::CreateDescriptor());

        m_editorWhiteBoxComponentDescriptor->Reflect(serializeContext);
        m_editorWhiteBoxSystemComponentDescriptor->Reflect(serializeContext);

        m_whiteBox = WhiteBox::Api::CreateWhiteBoxMesh();
        WhiteBox::Api::InitializeAsUnitCube(*m_whiteBox);

        m_whiteBoxMeshAsset = aznew WhiteBox::EditorWhiteBoxMeshAsset();
        m_whiteBoxMeshAsset->Associate(AZ::EntityComponentIdPair{});

        AZ::Entity* systemEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            systemEntity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::SystemEntityId);

        systemEntity->Deactivate();
        systemEntity->AddComponent(m_editorWhiteBoxSystemComponentDescriptor->CreateComponent());
        systemEntity->Activate();
    }

    void EditorWhiteBoxAssetFixture::TearDownEditorFixtureImpl()
    {
        delete m_whiteBoxMeshAsset;
        m_whiteBox.reset();
        m_editorWhiteBoxSystemComponentDescriptor.reset();
        m_editorWhiteBoxComponentDescriptor.reset();
    }

    TEST_F(EditorWhiteBoxAssetFixture, WhiteBoxAssetCanBeCreatedFromWhiteBoxMesh)
    {
        // verify preconditions
        EXPECT_FALSE(m_whiteBoxMeshAsset->InUse());

        m_whiteBoxMeshAsset->TakeOwnershipOfWhiteBoxMesh("test-asset", AZStd::move(m_whiteBox));

        // asset is created immediately from the white box mesh
        EXPECT_TRUE(m_whiteBoxMeshAsset->InUse());
        EXPECT_TRUE(m_whiteBoxMeshAsset->Loaded());
        EXPECT_TRUE(m_whiteBoxMeshAsset->GetWhiteBoxMeshAssetId().IsValid());
    }

    TEST_F(EditorWhiteBoxAssetFixture, WhiteBoxAssetCanBeSerialized)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Eq;
        using ::testing::NotNull;

        const auto whiteBoxCopy = Api::CloneMesh(*m_whiteBox);

        m_whiteBoxMeshAsset->TakeOwnershipOfWhiteBoxMesh("test-asset", AZStd::move(m_whiteBox));
        m_whiteBoxMeshAsset->Serialize();

        const auto meshAsset = m_whiteBoxMeshAsset->GetWhiteBoxMeshAsset();

        Api::WhiteBoxMeshPtr deserializedWhiteBox = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*deserializedWhiteBox, meshAsset->GetWhiteBoxData());

        EXPECT_THAT(deserializedWhiteBox, NotNull());
        EXPECT_THAT(Api::MeshVertexCount(*deserializedWhiteBox), Eq(Api::MeshVertexCount(*whiteBoxCopy)));
        EXPECT_THAT(Api::MeshVertexHandles(*deserializedWhiteBox), Eq(Api::MeshVertexHandles(*whiteBoxCopy)));
        EXPECT_THAT(Api::MeshFaceHandles(*deserializedWhiteBox), Eq(Api::MeshFaceHandles(*whiteBoxCopy)));
        EXPECT_THAT(Api::MeshEdgeHandles(*deserializedWhiteBox), Eq(Api::MeshEdgeHandles(*whiteBoxCopy)));
        EXPECT_THAT(Api::MeshHalfedgeCount(*deserializedWhiteBox), Eq(Api::MeshHalfedgeCount(*whiteBoxCopy)));
        EXPECT_THAT(Api::MeshFaces(*deserializedWhiteBox), Eq(Api::MeshFaces(*whiteBoxCopy)));
    }

    TEST_F(EditorWhiteBoxAssetFixture, WhiteBoxAssetCanBeCleared)
    {
        using ::testing::Eq;
        using ::testing::IsFalse;
        using ::testing::IsNull;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // ensure the White Box request bus only returns a null render mesh
        WhiteBox::WhiteBoxRequestBus::Broadcast(
            &WhiteBox::WhiteBoxRequestBus::Events::SetRenderMeshInterfaceBuilder,
            [](AZ::EntityId entityId)
            {
                return AZStd::make_unique<WhiteBox::WhiteBoxNullRenderMesh>(entityId);
            });

        // create an editor entity with a White Box component on it
        auto editorEntityAndWhiteBox = CreateEditorEntityWithEditorWhiteBoxComponent();
        auto whiteBoxComponent = editorEntityAndWhiteBox.m_editorWhiteBoxComponent;
        auto entityId = editorEntityAndWhiteBox.GetEntityId();

        // install our own EditorWhiteBoxMeshAsset
        whiteBoxComponent->OverrideEditorWhiteBoxMeshAsset(m_whiteBoxMeshAsset);
        m_whiteBoxMeshAsset->Associate(AZ::EntityComponentIdPair{entityId, whiteBoxComponent->GetId()});

        // change shape type to asset (equivalent to picking Asset from Entity Inspector)
        whiteBoxComponent->SetDefaultShape(WhiteBox::DefaultShapeType::Asset);

        // initialize the asset with our own White Box mesh
        m_whiteBoxMeshAsset->TakeOwnershipOfWhiteBoxMesh("test-asset", AZStd::move(m_whiteBox));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // change back to a set shape (no longer using Asset)
        whiteBoxComponent->SetDefaultShape(WhiteBox::DefaultShapeType::Cube);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        // ensure the EditorWhiteBoxMeshAsset was correctly cleared and unset
        EXPECT_THAT(m_whiteBoxMeshAsset->GetWhiteBoxMesh(), IsNull());
        EXPECT_THAT(m_whiteBoxMeshAsset->GetWhiteBoxMeshAssetId(), Eq(AZ::Data::AssetId()));
        EXPECT_THAT(m_whiteBoxMeshAsset->InUse(), IsFalse());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // cleanup
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, entityId);

        // now owned by EditorWhiteBoxComponent so ensure we do not attempt to delete it ourselves
        m_whiteBoxMeshAsset = nullptr;
    }

    TEST_F(EditorWhiteBoxAssetFixture, EditorWhiteBoxMeshAssetNotClearedWhenDeactivatingAndActivatingEntity)
    {
        using ::testing::IsTrue;
        using ::testing::NotNull;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // ensure the White Box request bus only returns a null render mesh
        WhiteBox::WhiteBoxRequestBus::Broadcast(
            &WhiteBox::WhiteBoxRequestBus::Events::SetRenderMeshInterfaceBuilder,
            [](AZ::EntityId entityId)
            {
                return AZStd::make_unique<WhiteBox::WhiteBoxNullRenderMesh>(entityId);
            });

        // create an editor entity with a White Box component on it
        auto editorEntityAndWhiteBox = CreateEditorEntityWithEditorWhiteBoxComponent();
        auto* entity = editorEntityAndWhiteBox.m_entity;
        auto* whiteBoxComponent = editorEntityAndWhiteBox.m_editorWhiteBoxComponent;
        auto entityId = editorEntityAndWhiteBox.GetEntityId();

        // install our own EditorWhiteBoxMeshAsset
        whiteBoxComponent->OverrideEditorWhiteBoxMeshAsset(m_whiteBoxMeshAsset);
        m_whiteBoxMeshAsset->Associate(AZ::EntityComponentIdPair{entityId, whiteBoxComponent->GetId()});

        // initialize the asset with our own White Box mesh
        m_whiteBoxMeshAsset->TakeOwnershipOfWhiteBoxMesh("test-asset", AZStd::move(m_whiteBox));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        entity->Deactivate();
        entity->Activate();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(whiteBoxComponent->AssetInUse(), IsTrue());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // cleanup
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, entityId);

        // now owned by EditorWhiteBoxComponent so ensure we do not attempt to delete it ourselves
        m_whiteBoxMeshAsset = nullptr;
    }
} // namespace UnitTest

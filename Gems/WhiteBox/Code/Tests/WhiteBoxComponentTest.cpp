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
#include "SubComponentModes/EditorWhiteBoxTransformMode.h"
#include "Util/WhiteBoxMeshUtil.h"

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

        // Destroying the system entity actually deletes absolutely everything, nearly equivalent to
        // restarting the application.  We have to re-initialize things destroyed by this, most notably
        // the things related to startup in the fixture that it does after starting the app.
        InitializeRootPrefab();
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

namespace UnitTest
{
    TEST_F(EditorWhiteBoxModifierTestFixture, TransformVertexMultiselectTogglesAndScalesAroundSharedCenter)
    {
        namespace Api = WhiteBox::Api;
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        Api::InitializeAsUnitCube(*mesh);
        const auto vertices = Api::MeshVertexHandles(*mesh);
        ASSERT_GE(vertices.size(), 3);
        const auto before = Api::VertexPositions(*mesh, vertices);
        const AZ::EntityComponentIdPair pair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());
        WhiteBox::TransformMode mode(pair);
        WhiteBox::IntersectionAndRenderData data;
        Viewport::MouseInteractionEvent event{};
        event.m_mouseEvent = Viewport::MouseEvent::Down;
        event.m_mouseInteraction.m_mouseButtons.m_mouseButtons = static_cast<AZ::u32>(Viewport::MouseButton::Left);
        const auto click = [&](size_t vertex, bool ctrl)
        {
            event.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers =
                ctrl ? static_cast<AZ::u32>(Viewport::KeyboardModifier::Ctrl) : 0;
            WhiteBox::VertexIntersection hit;
            hit.m_closestVertexWithHandle.m_handle = vertices[vertex];
            hit.m_intersection.m_closestDistance = 1.0f;
            mode.HandleMouseInteraction({event, pair, AZ::Transform::CreateIdentity(), data, {}, {}, hit});
        };
        click(2, false);
        click(0, false); // Replace the previous selection.
        click(1, true);
        click(2, true);
        click(2, true); // Toggle it back off.
        mode.ChangeTransformType(WhiteBox::TransformType::Scale);
        mode.NumericBeginScale();
        mode.NumericAppendDigit('2');
        mode.NumericConfirm();
        const AZ::Vector3 center = (before[0] + before[1]) * 0.5f;
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            const auto expected = i < 2 ? center + (before[i] - center) * 2.0f : before[i];
            EXPECT_TRUE(Api::VertexPosition(*mesh, vertices[i]).IsClose(expected));
        }
        click(0, true);
        click(1, true); // Removing the last vertex clears the selection.
        const auto after = Api::VertexPositions(*mesh, vertices);
        mode.NumericBeginMove();
        mode.NumericSetAxisX();
        mode.NumericAppendDigit('1');
        mode.NumericConfirm();
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            EXPECT_TRUE(Api::VertexPosition(*mesh, vertices[i]).IsClose(after[i]));
        }
    }

    TEST_F(EditorWhiteBoxModifierTestFixture, TransformEdgeMultiselectMovesSharedEndpointOnlyOnce)
    {
        namespace Api = WhiteBox::Api;
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        Api::InitializeAsUnitCube(*mesh);
        const auto vertices = Api::MeshVertexHandles(*mesh);
        const auto edges = Api::VertexEdgeHandles(*mesh, vertices.front());
        ASSERT_GE(edges.size(), 3);
        const auto before = Api::VertexPositions(*mesh, vertices);
        const AZ::EntityComponentIdPair pair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());
        WhiteBox::TransformMode mode(pair);
        WhiteBox::IntersectionAndRenderData data;
        Viewport::MouseInteractionEvent event{};
        event.m_mouseEvent = Viewport::MouseEvent::Down;
        event.m_mouseInteraction.m_mouseButtons.m_mouseButtons = static_cast<AZ::u32>(Viewport::MouseButton::Left);
        const auto click = [&](size_t edge, bool ctrl)
        {
            event.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers =
                ctrl ? static_cast<AZ::u32>(Viewport::KeyboardModifier::Ctrl) : 0;
            WhiteBox::EdgeIntersection hit;
            hit.m_closestEdgeWithHandle.m_handle = edges[edge];
            hit.m_intersection.m_closestDistance = 1.0f;
            mode.HandleMouseInteraction({event, pair, AZ::Transform::CreateIdentity(), data, hit, {}, {}});
        };
        click(2, false);
        click(0, false);
        click(1, true);
        click(2, true);
        click(2, true);
        mode.NumericBeginMove();
        mode.NumericSetAxisX();
        mode.NumericAppendDigit('1');
        mode.NumericConfirm();
        const auto first = Api::EdgeVertexHandles(*mesh, edges[0]);
        const auto second = Api::EdgeVertexHandles(*mesh, edges[1]);
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            const bool selected = AZStd::find(first.begin(), first.end(), vertices[i]) != first.end() ||
                AZStd::find(second.begin(), second.end(), vertices[i]) != second.end();
            const auto expected = before[i] + (selected ? AZ::Vector3::CreateAxisX() : AZ::Vector3::CreateZero());
            EXPECT_TRUE(Api::VertexPosition(*mesh, vertices[i]).IsClose(expected));
        }
    }

    TEST_F(EditorWhiteBoxModifierTestFixture, TransformPolygonSelectionSupportsCtrlToggleAndUniqueVertexMovement)
    {
        namespace Api = WhiteBox::Api;
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        ASSERT_NE(mesh, nullptr);
        Api::InitializeAsUnitCube(*mesh);
        const auto polygons = Api::MeshPolygonHandles(*mesh);
        ASSERT_GE(polygons.size(), 3);
        const AZ::EntityComponentIdPair pair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());
        WhiteBox::TransformMode mode(pair);
        WhiteBox::IntersectionAndRenderData intersectionData;
        Viewport::MouseInteractionEvent event{};
        event.m_mouseEvent = Viewport::MouseEvent::Down;
        event.m_mouseInteraction.m_mouseButtons.m_mouseButtons = static_cast<AZ::u32>(Viewport::MouseButton::Left);
        const auto click = [&](size_t polygon, bool ctrl)
        {
            event.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers =
                ctrl ? static_cast<AZ::u32>(Viewport::KeyboardModifier::Ctrl) : 0;
            WhiteBox::PolygonIntersection hit;
            hit.m_closestPolygonWithHandle.m_handle = polygons[polygon];
            hit.m_intersection.m_closestDistance = 1.0f;
            WhiteBox::ModeMouseInteraction mouse{
                event, pair, AZ::Transform::CreateIdentity(), intersectionData, {}, hit, {}};
            mode.HandleMouseInteraction(mouse);
        };
        click(0, false);
        click(1, true);
        ASSERT_EQ(mode.GetSelectedPolygons().size(), 2);
        click(0, true);
        ASSERT_EQ(mode.GetSelectedPolygons().size(), 1);
        EXPECT_EQ(mode.GetSelectedPolygons().front(), polygons[1]);
        click(2, false);
        ASSERT_EQ(mode.GetSelectedPolygons().size(), 1);
        EXPECT_EQ(mode.GetSelectedPolygons().front(), polygons[2]);
        click(0, true);
        click(1, true);
        const auto selected = mode.GetSelectedPolygons();

        const auto handles = Api::MeshVertexHandles(*mesh);
        const auto before = Api::VertexPositions(*mesh, handles);
        mode.NumericBeginMove();
        mode.NumericSetAxisX();
        mode.NumericAppendDigit('1');
        mode.NumericConfirm();
        for (size_t i = 0; i < handles.size(); ++i)
        {
            bool affected = false;
            for (const auto& polygon : selected)
            {
                const auto vertices = Api::PolygonVertexHandles(*mesh, polygon);
                affected |= AZStd::find(vertices.begin(), vertices.end(), handles[i]) != vertices.end();
            }
            const AZ::Vector3 expected = before[i] + (affected ? AZ::Vector3::CreateAxisX() : AZ::Vector3::CreateZero());
            EXPECT_TRUE(Api::VertexPosition(*mesh, handles[i]).IsClose(expected));
        }
        mode.Refresh();
        EXPECT_TRUE(mode.GetSelectedPolygons().empty());
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, PolygonAndDefaultMaterialsAreBakedForGameModeAndCanBeReset)
    {
        namespace Api = WhiteBox::Api;
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        Api::InitializeAsUnitCube(*mesh);
        m_whiteBoxComponent->SerializeWhiteBox();
        const auto polygons = Api::MeshPolygonHandles(*mesh);
        const auto defaultMaterial = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:6");
        const auto polygonMaterial = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:7");
        m_whiteBoxComponent->SetMaterialOverride(defaultMaterial);
        m_whiteBoxComponent->AssignPolygonMaterial({polygons.front(), polygons.back()}, polygonMaterial);
        AZ::Entity gameEntity;
        static_cast<AzToolsFramework::Components::EditorComponentBase*>(m_whiteBoxComponent)->BuildGameEntity(&gameEntity);
        auto* runtime = gameEntity.FindComponent<WhiteBox::WhiteBoxComponent>();
        ASSERT_NE(runtime, nullptr);
        const auto& renderData = runtime->GetActiveRenderData();
        EXPECT_EQ(renderData.m_material.m_materialAsset.GetId(), defaultMaterial);
        size_t assigned = 0;
        for (const auto& face : renderData.m_faces)
        {
            assigned += face.m_materialAsset.GetId() == polygonMaterial ? 1 : 0;
        }
        EXPECT_EQ(assigned, polygons.front().m_faceHandles.size() + polygons.back().m_faceHandles.size());
        m_whiteBoxComponent->AssignPolygonMaterial({polygons.front()}, {});
        for (const auto face : polygons.front().m_faceHandles)
        {
            EXPECT_FALSE(Api::FaceMaterial(*mesh, face).IsValid());
        }
        for (const auto face : polygons.back().m_faceHandles)
        {
            EXPECT_EQ(Api::FaceMaterial(*mesh, face), polygonMaterial);
        }
        m_whiteBoxComponent->SetMaterialOverride({});
        EXPECT_FALSE(m_whiteBoxComponent->GetMaterialOverride().IsValid());
    }
}

namespace UnitTest
{
    TEST_F(EditorWhiteBoxComponentTestFixture, PolygonMaterialAssignmentSupportsUndoAndRedo)
    {
        namespace Api = WhiteBox::Api;
        {
            AzToolsFramework::ScopedUndoBatch setup("Create material test mesh");
            Api::InitializeAsUnitCube(*m_whiteBoxComponent->GetWhiteBoxMesh());
            m_whiteBoxComponent->SerializeWhiteBox();
            WhiteBox::EditorWhiteBoxComponentNotificationBus::Event(
                AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId()),
                &WhiteBox::EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            setup.MarkEntityDirty(m_whiteBoxEntityId);
        }
        const auto polygon = Api::MeshPolygonHandles(*m_whiteBoxComponent->GetWhiteBoxMesh()).front();
        const auto material = AZ::Data::AssetId::CreateString("{15214A10-CEAC-49D8-AB23-B7129F69B9F1}:8");
        {
            AzToolsFramework::ScopedUndoBatch assign("Assign polygon material");
            m_whiteBoxComponent->AssignPolygonMaterial({polygon}, material);
            assign.MarkEntityDirty(m_whiteBoxEntityId);
        }
        const auto currentMaterial = [&]()
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationRequests::FindEntity, m_whiteBoxEntityId);
            auto* component = entity->FindComponent<WhiteBox::EditorWhiteBoxComponent>();
            return Api::FaceMaterial(*component->GetWhiteBoxMesh(), polygon.m_faceHandles.front());
        };
        EXPECT_EQ(currentMaterial(), material);
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        EXPECT_FALSE(currentMaterial().IsValid());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);
        EXPECT_EQ(currentMaterial(), material);
    }
}

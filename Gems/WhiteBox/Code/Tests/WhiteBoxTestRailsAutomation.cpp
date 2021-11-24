/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace UnitTest
{
    using TestRailsAutomationFixture = EditorWhiteBoxModifierTestFixture;
    constexpr bool LogActions = false;

    TEST_F(TestRailsAutomationFixture, C28798184_PressAndDragOnAPolygonTranslatesItAlongTheSurfaceNormal)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        // the initial starting position of the entity (in front and to the left of the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, 10.0f, 0.0f));
        // world space delta we will be moving the polygon face
        const auto worldTranslationDelta = AZ::Vector3::CreateAxisX(20.0f);
        // the face handle we will use to get the parent polygon from
        const int faceHandle = 7;

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId()),
            &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // the initial position of the white box mesh vertices
        const auto initialVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);

        // the center position of the polygon modifier
        MultiSpacePoint initialModifierMidPoint(
            Api::PolygonMidpoint(*whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(faceHandle))),
            initialEntityTransformWorld, m_cameraState);
        // where we expect the interaction to leave the modifier after the movement
        const auto finalScreenModifierMidPoint =
            AzFramework::WorldToScreen(initialModifierMidPoint.GetWorldSpace() + worldTranslationDelta, m_cameraState);

        // clone the initial vertex positions as our starting point for the expected positions
        AZStd::vector<AZ::Vector3> expectedFinalLocalFaceVertexPositions(initialVertexLocalPositions);
        // where we expect the polygon vertices to end up after the movement
        expectedFinalLocalFaceVertexPositions[1] += worldTranslationDelta;
        expectedFinalLocalFaceVertexPositions[2] += worldTranslationDelta;
        expectedFinalLocalFaceVertexPositions[5] += worldTranslationDelta;
        expectedFinalLocalFaceVertexPositions[6] += worldTranslationDelta;

        m_actionDispatcher->LogActions(LogActions)
            ->CameraState(m_cameraState)
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str())
            ->Trace("Moving mouse to polygon modifier midpoint")
            ->MousePosition(initialModifierMidPoint.GetScreenSpace())
            ->Trace("Dragging polygon to extrude")
            ->MouseLButtonDown()
            ->MousePosition(finalScreenModifierMidPoint)
            ->MouseLButtonUp()
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str());

        // grab the white box mesh vertices
        const auto finalVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);
        // check that the number of vertices hasn't changed (no extrusions)
        EXPECT_EQ(finalVertexLocalPositions.size(), initialVertexLocalPositions.size());
        // check our expected modified vertices and the vertices after the modifier match
        EXPECT_THAT(
            finalVertexLocalPositions,
            Pointwise(ContainerIsCloseTolerance(0.01f), expectedFinalLocalFaceVertexPositions));
    }

    TEST_F(TestRailsAutomationFixture, C28798192_PressAndDragOutwardsFromAPolygonWithCtrlHeldWillExtrudeThePolygon)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        // the initial starting position of the entity (in front and to the left of the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, 10.0f, 0.0f));
        // world space delta we will be moving the polygon face
        const auto worldTranslationDelta = AZ::Vector3::CreateAxisX(20.0f);
        // the face handle we will use to get the parent polygon from
        const int faceHandle = 7;

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId()),
            &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // the initial position of the white box mesh vertices
        const auto initialVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);

        // the center position of the polygon modifier
        MultiSpacePoint initialModifierMidPoint(
            Api::PolygonMidpoint(*whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(faceHandle))),
            initialEntityTransformWorld, m_cameraState);
        // where we expect the interaction to leave the modifier after the movement
        const auto finalScreenModifierMidPoint =
            AzFramework::WorldToScreen(initialModifierMidPoint.GetWorldSpace() + worldTranslationDelta, m_cameraState);

        // clone the initial vertex positions as our starting point for the expected positions
        AZStd::vector<AZ::Vector3> expectedFinalLocalFaceVertexPositions(initialVertexLocalPositions);
        // where we expect the extra polygon vertices added by the extrusion to be after the movement
        expectedFinalLocalFaceVertexPositions.emplace_back(initialVertexLocalPositions[5] + worldTranslationDelta);
        expectedFinalLocalFaceVertexPositions.emplace_back(initialVertexLocalPositions[6] + worldTranslationDelta);
        expectedFinalLocalFaceVertexPositions.emplace_back(initialVertexLocalPositions[2] + worldTranslationDelta);
        expectedFinalLocalFaceVertexPositions.emplace_back(initialVertexLocalPositions[1] + worldTranslationDelta);

        m_actionDispatcher->LogActions(LogActions)
            ->CameraState(m_cameraState)
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str())
            ->Trace("Moving mouse to polygon modifier midpoint")
            ->MousePosition(initialModifierMidPoint.GetScreenSpace())
            ->KeyboardModifierDown(AzToolsFramework::ViewportInteraction::KeyboardModifier::Ctrl)
            ->Trace("Dragging polygon to extrude")
            ->MouseLButtonDown()
            ->MousePosition(finalScreenModifierMidPoint)
            ->MouseLButtonUp()
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str());

        // grab the white box mesh vertices
        const auto finalVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);
        // check that the number of vertices has changed (one extrusion)
        EXPECT_EQ(finalVertexLocalPositions.size(), initialVertexLocalPositions.size() + 4);
        // check our expected modified vertices and the vertices after the modifier match
        EXPECT_THAT(
            finalVertexLocalPositions,
            Pointwise(ContainerIsCloseTolerance(0.01f), expectedFinalLocalFaceVertexPositions));
    }

    TEST_F(
        TestRailsAutomationFixture, C28798192_PressAndDragOnAPolygonScaleModifierScalesTheVerticesAboutThePolygonCenter)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        // the initial starting position of the entity (in front and to the left of the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, 10.0f, 0.0f));
        // the face handle we will use to get the parent polygon from
        const int faceHandle = 7;
        // the polygon vertex handle we will be dragging
        const int vertexHandle = 2;
        // lerp amount to derive mid point between polygon vertex and polygon mid point
        const float vertexLerp = 0.5f;

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId()),
            &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // the initial position of the white box mesh vertices
        const auto initialVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);

        // the center position of the polygon modifier
        MultiSpacePoint initialModifierMidPoint(
            Api::PolygonMidpoint(*whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(faceHandle))),
            initialEntityTransformWorld, m_cameraState);
        // the position of the vertex handle of the polygon we will be resizing
        MultiSpacePoint initialModifierVertex(
            initialVertexLocalPositions[vertexHandle], initialEntityTransformWorld, m_cameraState);
        // mid point between polygon vertex and polygon modifier mid point
        MultiSpacePoint finalModifierVertex(
            initialModifierVertex.GetLocalSpace().Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp),
            initialEntityTransformWorld, m_cameraState);

        // clone the initial vertex positions as our starting point for the expected positions
        AZStd::vector<AZ::Vector3> expectedFinalLocalFaceVertexPositions(initialVertexLocalPositions);
        // where we expect the polygon vertices to end up after the movement (moved half-way towards the face mid point)
        expectedFinalLocalFaceVertexPositions[1] =
            initialVertexLocalPositions[1].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);
        expectedFinalLocalFaceVertexPositions[2] =
            initialVertexLocalPositions[2].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);
        expectedFinalLocalFaceVertexPositions[5] =
            initialVertexLocalPositions[5].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);
        expectedFinalLocalFaceVertexPositions[6] =
            initialVertexLocalPositions[6].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);

        m_actionDispatcher->LogActions(LogActions)
            ->CameraState(m_cameraState)
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str())
            ->Trace("Moving mouse to polygon modifier midpoint")
            ->MousePosition(initialModifierMidPoint.GetScreenSpace())
            ->Trace("Selecting polygon")
            ->MouseLButtonDown()
            ->MouseLButtonUp()
            ->Trace("Moving mouse to polygon vertex % d", vertexHandle)
            ->MousePosition(initialModifierVertex.GetScreenSpace())
            ->Trace("Selecting polygon vertex % d", vertexHandle)
            ->MouseLButtonDown()
            ->Trace("Dragging vertex %d towards centroid of polygon", vertexHandle)
            ->MousePosition(finalModifierVertex.GetScreenSpace())
            ->MouseLButtonUp()
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str());

        // grab the white box mesh vertices
        const auto finalVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);
        // check that the number of vertices has not changed
        EXPECT_EQ(finalVertexLocalPositions.size(), initialVertexLocalPositions.size());
        // check our expected modified vertices and the vertices after the modifier match
        EXPECT_THAT(
            finalVertexLocalPositions,
            Pointwise(ContainerIsCloseTolerance(0.01f), expectedFinalLocalFaceVertexPositions));
    }

    TEST_F(TestRailsAutomationFixture, C28798193_PressAndDragInwardsFromAPolygonWithCtrlHeldWillImpressThePolygon)
    {
        namespace Api = WhiteBox::Api;
        using ::testing::Pointwise;

        // the initial starting position of the entity (in front and to the left of the camera)
        const AZ::Transform initialEntityTransformWorld =
            AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, 10.0f, 0.0f));
        // world space delta we will be moving the polygon face
        const auto worldTranslationDelta = AZ::Vector3::CreateAxisX(20.0f);
        // the face handle we will use to get the parent polygon from
        const int faceHandle = 7;
        // the polygon vertex handle we will be dragging
        const int vertexHandle = 2;
        // lerp amount to derive mid point between polygon vertex and polygon mid point
        const float vertexLerp = 0.5f;

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_whiteBoxEntityId, initialEntityTransformWorld);
        // select the entity with the White Box Component
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        // mimic pressing the 'Edit' button on the Component Card
        EnterComponentMode<WhiteBox::EditorWhiteBoxComponent>();

        // grab the White Box Mesh (for use with the White Box Tool Api)
        WhiteBox::WhiteBoxMesh* whiteBox;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId()),
            &WhiteBox::EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMesh);

        // the initial position of the white box mesh vertices
        const auto initialVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);

        // the center position of the polygon modifier
        MultiSpacePoint initialModifierMidPoint(
            Api::PolygonMidpoint(*whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(faceHandle))),
            initialEntityTransformWorld, m_cameraState);
        // the position of the vertex handle of the polygon we will be resizing
        MultiSpacePoint initialModifierVertex(
            initialVertexLocalPositions[vertexHandle], initialEntityTransformWorld, m_cameraState);
        // mid point between polygon vertex and polygon modifier mid point
        MultiSpacePoint finalModifierVertex(
            initialModifierVertex.GetLocalSpace().Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp),
            initialEntityTransformWorld, m_cameraState);
        // where we expect the interaction to leave the modifier after the movement
        const auto finalScreenModifierMidPoint =
            AzFramework::WorldToScreen(initialModifierMidPoint.GetWorldSpace() + worldTranslationDelta, m_cameraState);

        // clone the initial vertex positions as our starting point for the expected positions
        AZStd::vector<AZ::Vector3> expectedFinalLocalFaceVertexPositions(initialVertexLocalPositions);
        // where we expect the polygon vertices to end up after the movement (moved half-way towards the face mid point)
        expectedFinalLocalFaceVertexPositions[1] =
            initialVertexLocalPositions[1].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);
        expectedFinalLocalFaceVertexPositions[2] =
            initialVertexLocalPositions[2].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);
        expectedFinalLocalFaceVertexPositions[5] =
            initialVertexLocalPositions[5].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);
        expectedFinalLocalFaceVertexPositions[6] =
            initialVertexLocalPositions[6].Lerp(initialModifierMidPoint.GetLocalSpace(), vertexLerp);
        // where we expect the extra polygon vertices added by the impression to be after the movement
        expectedFinalLocalFaceVertexPositions.emplace_back(
            expectedFinalLocalFaceVertexPositions[5] + worldTranslationDelta);
        expectedFinalLocalFaceVertexPositions.emplace_back(
            expectedFinalLocalFaceVertexPositions[6] + worldTranslationDelta);
        expectedFinalLocalFaceVertexPositions.emplace_back(
            expectedFinalLocalFaceVertexPositions[2] + worldTranslationDelta);
        expectedFinalLocalFaceVertexPositions.emplace_back(
            expectedFinalLocalFaceVertexPositions[1] + worldTranslationDelta);

        m_actionDispatcher->LogActions(LogActions)
            ->CameraState(m_cameraState)
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str())
            ->Trace("Moving mouse to polygon modifier midpoint")
            ->MousePosition(initialModifierMidPoint.GetScreenSpace())
            ->Trace("Selecting polygon")
            ->MouseLButtonDown()
            ->MouseLButtonUp()
            ->Trace("Moving mouse to polygon vertex % d", vertexHandle)
            ->MousePosition(initialModifierVertex.GetScreenSpace())
            ->Trace("Selecting polygon vertex % d", vertexHandle)
            ->MouseLButtonDown()
            ->Trace("Dragging vertex %d towards centroid of polygon", vertexHandle)
            ->MousePosition(finalModifierVertex.GetScreenSpace())
            ->MouseLButtonUp()
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str())
            ->Trace("Moving mouse to centroid of polygon")
            ->MousePosition(initialModifierMidPoint.GetScreenSpace())
            ->Trace("Impressing polygon and extruding")
            ->KeyboardModifierDown(AzToolsFramework::ViewportInteraction::KeyboardModifier::Ctrl)
            ->MouseLButtonDown()
            ->MousePosition(finalScreenModifierMidPoint)
            ->MouseLButtonUp()
            ->Trace(WhiteBox::VerticesToString(whiteBox, initialEntityTransformWorld).c_str());

        // grab the white box mesh vertices
        const auto finalVertexLocalPositions = Api::MeshVertexPositions(*whiteBox);
        // check that the number of vertices has changed (one impression)
        EXPECT_EQ(finalVertexLocalPositions.size(), initialVertexLocalPositions.size() + 4);
        // check our expected modified vertices and the vertices after the modifier match
        EXPECT_THAT(
            finalVertexLocalPositions,
            Pointwise(ContainerIsCloseTolerance(0.01f), expectedFinalLocalFaceVertexPositions));
    }
} // namespace UnitTest

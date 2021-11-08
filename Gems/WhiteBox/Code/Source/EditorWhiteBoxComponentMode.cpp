/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentMode.h"
#include "SubComponentModes/EditorWhiteBoxDefaultMode.h"
#include "SubComponentModes/EditorWhiteBoxEdgeRestoreMode.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <QApplication> // required for querying modifier keys
#include <QVBoxLayout>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorWhiteBoxComponentMode, AZ::SystemAllocator, 0)

    // helper function to return what modifier keys move us to restore mode
    static bool RestoreModifier(AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers)
    {
        return modifiers.Shift() && modifiers.Ctrl();
    }

    // helper function to return what type of edge selection mode we're in
    static EdgeSelectionType DecideEdgeSelectionMode(const SubMode subMode)
    {
        return subMode == SubMode::EdgeRestore ? EdgeSelectionType::All : EdgeSelectionType::Polygon;
    }

    EditorWhiteBoxComponentMode::EditorWhiteBoxComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_worldFromLocal(AZ::Transform::Identity())
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorWhiteBoxComponentModeRequestBus::Handler::BusConnect(entityComponentIdPair);
        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorWhiteBoxComponentNotificationBus::Handler::BusConnect(entityComponentIdPair);

        // default behavior for querying modifier keys (ask the QApplication)
        m_keyboardMofifierQueryFn = []()
        {
            return AzToolsFramework::ViewportInteraction::QueryKeyboardModifiers();
        };

        m_worldFromLocal = AzToolsFramework::WorldFromLocalWithUniformScale(entityComponentIdPair.GetEntityId());
        CreateSubModeSelectionCluster();
        // start with DefaultMode
        EnterDefaultMode();
    }

    EditorWhiteBoxComponentMode::~EditorWhiteBoxComponentMode()
    {
        RemoveSubModeSelectionCluster();

        EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentModeRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EditorWhiteBoxComponentMode::Refresh()
    {
        MarkWhiteBoxIntersectionDataDirty();

        AZStd::visit(
            [](auto& mode)
            {
                mode->Refresh();
            },
            m_modes);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);
    }

    static AZStd::optional<VertexIntersection> FindClosestVertexIntersection(
        const GeometryIntersectionData& whiteBoxIntersectionData, const AZ::Vector3& localRayOrigin,
        const AZ::Vector3& localRayDirection, const AZ::Transform& worldFromLocal,
        const AzFramework::CameraState& cameraState)
    {
        VertexIntersection vertexIntersection;

        const float scaleRecip = AzToolsFramework::ScaleReciprocal(worldFromLocal);

        // find the closest vertex bound
        for (const auto& vertexBound : whiteBoxIntersectionData.m_vertexBounds)
        {
            const AZ::Vector3 worldCenter = worldFromLocal.TransformPoint(vertexBound.m_bound.m_center);

            const float screenRadius = vertexBound.m_bound.m_radius *
                AzToolsFramework::CalculateScreenToWorldMultiplier(worldCenter, cameraState) * scaleRecip;

            float vertexDistance = std::numeric_limits<float>::max();
            const bool intersection = IntersectRayVertex(
                vertexBound.m_bound, screenRadius, localRayOrigin, localRayDirection, vertexDistance);

            if (intersection && vertexDistance < vertexIntersection.m_intersection.m_closestDistance)
            {
                vertexIntersection.m_closestVertexWithHandle = vertexBound;
                vertexIntersection.m_intersection.m_closestDistance = vertexDistance;
            }
        }

        if (vertexIntersection.m_intersection.m_closestDistance < std::numeric_limits<float>::max())
        {
            vertexIntersection.m_intersection.m_localIntersectionPoint =
                localRayOrigin + localRayDirection * vertexIntersection.m_intersection.m_closestDistance;

            return vertexIntersection;
        }
        else
        {
            return AZStd::optional<VertexIntersection>{};
        }
    }

    static AZStd::optional<EdgeIntersection> FindClosestEdgeIntersection(
        const GeometryIntersectionData& whiteBoxIntersectionData, const AZ::Vector3& localRayOrigin,
        const AZ::Vector3& localRayDirection, const AZ::Transform& worldFromLocal,
        const AzFramework::CameraState& cameraState)
    {
        EdgeIntersection edgeIntersection;

        const float scaleRecip = AzToolsFramework::ScaleReciprocal(worldFromLocal);

        // find the closest edge bound
        for (const auto& edgeBound : whiteBoxIntersectionData.m_edgeBounds)
        {
            // degenerate edges cause false positives in the intersection test
            if (edgeBound.m_bound.m_start.IsClose(edgeBound.m_bound.m_end))
            {
                continue;
            }

            const AZ::Vector3 localMidpoint = (edgeBound.m_bound.m_end + edgeBound.m_bound.m_start) * 0.5f;
            const AZ::Vector3 worldMidpoint = worldFromLocal.TransformPoint(localMidpoint);

            const float screenRadius = edgeBound.m_bound.m_radius *
                AzToolsFramework::CalculateScreenToWorldMultiplier(worldMidpoint, cameraState) * scaleRecip;

            float edgeDistance = std::numeric_limits<float>::max();
            const bool intersection =
                IntersectRayEdge(edgeBound.m_bound, screenRadius, localRayOrigin, localRayDirection, edgeDistance);

            if (intersection && edgeDistance < edgeIntersection.m_intersection.m_closestDistance)
            {
                edgeIntersection.m_closestEdgeWithHandle = edgeBound;
                edgeIntersection.m_intersection.m_closestDistance = edgeDistance;
            }
        }

        if (edgeIntersection.m_intersection.m_closestDistance < std::numeric_limits<float>::max())
        {
            // calculate closest intersection point
            edgeIntersection.m_intersection.m_localIntersectionPoint =
                localRayOrigin + localRayDirection * edgeIntersection.m_intersection.m_closestDistance;

            return edgeIntersection;
        }
        else
        {
            return AZStd::optional<EdgeIntersection>{};
        }
    }

    static AZStd::optional<PolygonIntersection> FindClosestPolygonIntersection(
        const GeometryIntersectionData& whiteBoxIntersectionData, const AZ::Vector3& localRayOrigin,
        const AZ::Vector3& localRayDirection)
    {
        PolygonIntersection polygonIntersection;

        // find closest polygon bound
        for (const auto& polygonBound : whiteBoxIntersectionData.m_polygonBounds)
        {
            int64_t pickedTriangleIndex;
            float polygonDistance = std::numeric_limits<float>::max();
            const bool intersection = IntersectRayPolygon(
                polygonBound.m_bound, localRayOrigin, localRayDirection, polygonDistance, pickedTriangleIndex);

            if (intersection && polygonDistance < polygonIntersection.m_intersection.m_closestDistance)
            {
                polygonIntersection.m_pickedFaceHandle = polygonBound.m_handle.m_faceHandles[pickedTriangleIndex];
                polygonIntersection.m_closestPolygonWithHandle = polygonBound;
                polygonIntersection.m_intersection.m_closestDistance = polygonDistance;
                polygonIntersection.m_intersection.m_localIntersectionPoint =
                    localRayOrigin + localRayDirection * polygonDistance;
            }
        }

        return polygonIntersection.m_intersection.m_closestDistance < std::numeric_limits<float>::max()
            ? polygonIntersection
            : AZStd::optional<PolygonIntersection>{};
    }

    bool EditorWhiteBoxComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, GetEntityComponentIdPair(), &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // generate mesh to query if it needs to be rebuilt
        if (!m_intersectionAndRenderData.has_value())
        {
            RecalculateWhiteBoxIntersectionData(DecideEdgeSelectionMode(m_currentSubMode));
        }

        const AZ::Transform localFromWorld = m_worldFromLocal.GetInverse();

        const AZ::Vector3 localRayOrigin =
            localFromWorld.TransformPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin);
        const AZ::Vector3 localRayDirection = AzToolsFramework::TransformDirectionNoScaling(
            localFromWorld, mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection);

        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportId);

        const AZStd::optional<EdgeIntersection> edgeIntersection = FindClosestEdgeIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection,
            m_worldFromLocal, cameraState);

        const AZStd::optional<PolygonIntersection> polygonIntersection = FindClosestPolygonIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection);

        const AZStd::optional<VertexIntersection> vertexIntersection = FindClosestVertexIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection,
            m_worldFromLocal, cameraState);

        // interactionHandled will be set to true if the mouse interaction has been handled by this white box component
        // which involves either interacting with a manipulator from this white box or clicking on the white box mesh
        // itself
        bool interactionHandled = AZStd::visit(
            [&mouseInteraction, entityComponentIdPair = GetEntityComponentIdPair(), &edgeIntersection,
             &polygonIntersection, &vertexIntersection](auto& mode)
            {
                return mode->HandleMouseInteraction(
                    mouseInteraction, entityComponentIdPair, edgeIntersection, polygonIntersection, vertexIntersection);
            },
            m_modes);

        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up &&
            (edgeIntersection || polygonIntersection || vertexIntersection))
        {
            interactionHandled = true;
        }

        return interactionHandled;
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorWhiteBoxComponentMode::PopulateActionsImpl()
    {
        return AZStd::visit(
            [entityComponentIdPair = GetEntityComponentIdPair()](auto& mode)
            {
                return mode->PopulateActions(entityComponentIdPair);
            },
            m_modes);
    }

    static void SetViewportUiClusterActiveButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, AzToolsFramework::ViewportUi::ButtonId buttonId)
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, clusterId, buttonId);
    }

    void EditorWhiteBoxComponentMode::EnterDefaultMode()
    {
        m_modes = AZStd::make_unique<DefaultMode>(GetEntityComponentIdPair());
        m_intersectionAndRenderData = {};
        m_currentSubMode = SubMode::Default;
        SetViewportUiClusterActiveButton(m_modeSelectionClusterId, m_defaultModeButtonId);
    }

    void EditorWhiteBoxComponentMode::EnterEdgeRestoreMode()
    {
        m_modes = AZStd::make_unique<EdgeRestoreMode>();
        m_intersectionAndRenderData = {};
        m_currentSubMode = SubMode::EdgeRestore;
        SetViewportUiClusterActiveButton(m_modeSelectionClusterId, m_edgeRestoreModeButtonId);
    }

    void EditorWhiteBoxComponentMode::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const auto modifiers = m_keyboardMofifierQueryFn();

        // handle mode switch
        {
            auto* defaultMode = AZStd::get_if<AZStd::unique_ptr<DefaultMode>>(&m_modes);
            auto* edgeRestoreMode = AZStd::get_if<AZStd::unique_ptr<EdgeRestoreMode>>(&m_modes);

            // enter edge restore mode if inside normal mode and restore modifier is held
            if (RestoreModifier(modifiers))
            {
                if (defaultMode != nullptr)
                {
                    EnterEdgeRestoreMode();
                }

                m_restoreModifierHeld = true;
            }
            // enter default mode if restore modifier is not currently held and
            // was held to enter restore mode (as opposed to viewport ui widget)
            else if (!RestoreModifier(modifiers))
            {
                if (m_restoreModifierHeld && edgeRestoreMode != nullptr)
                {
                    EnterDefaultMode();
                }

                m_restoreModifierHeld = false;
            }
        }

        // generate mesh to query
        if (!m_intersectionAndRenderData.has_value())
        {
            RecalculateWhiteBoxIntersectionData(DecideEdgeSelectionMode(m_currentSubMode));
        }

        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(cl_whiteBoxEdgeUserColor);
        debugDisplay.SetLineWidth(4.0f);

        AZStd::visit(
            [entityComponentIdPair = GetEntityComponentIdPair(),
             &whiteBoxIntersectionAndRenderData = m_intersectionAndRenderData, viewportInfo, &debugDisplay,
             &worldFromLocal = m_worldFromLocal](auto& mode)
            {
                mode->Display(
                    entityComponentIdPair, worldFromLocal, whiteBoxIntersectionAndRenderData.value(), viewportInfo,
                    debugDisplay);
            },
            m_modes);

        debugDisplay.DepthTestOff();
    }

    void EditorWhiteBoxComponentMode::MarkWhiteBoxIntersectionDataDirty()
    {
        m_intersectionAndRenderData = {};
    }

    // combine user and mesh edge handles into a single collection
    static Api::EdgeHandles BuildAllEdgeHandles(const Api::EdgeTypes& edgeHandlesPair)
    {
        Api::EdgeHandles allEdgeHandles;
        allEdgeHandles.reserve(edgeHandlesPair.m_mesh.size() + edgeHandlesPair.m_user.size());
        allEdgeHandles.insert(allEdgeHandles.end(), edgeHandlesPair.m_mesh.cbegin(), edgeHandlesPair.m_mesh.cend());
        allEdgeHandles.insert(allEdgeHandles.end(), edgeHandlesPair.m_user.cbegin(), edgeHandlesPair.m_user.cend());
        return allEdgeHandles;
    }

    void EditorWhiteBoxComponentMode::RecalculateWhiteBoxIntersectionData(const EdgeSelectionType edgeSelectionMode)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, GetEntityComponentIdPair(), &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        m_intersectionAndRenderData = IntersectionAndRenderData{};
        for (const auto& vertexHandle : Api::MeshVertexHandles(*whiteBox))
        {
            const auto vertexPosition = Api::VertexPosition(*whiteBox, vertexHandle);
            m_intersectionAndRenderData->m_whiteBoxIntersectionData.m_vertexBounds.emplace_back(
                VertexBoundWithHandle{{vertexPosition, cl_whiteBoxVertexManipulatorSize}, vertexHandle});
        }

        for (const auto& polygonHandle : Api::MeshPolygonHandles(*whiteBox))
        {
            const auto triangles = Api::FacesPositions(*whiteBox, polygonHandle.m_faceHandles);
            m_intersectionAndRenderData->m_whiteBoxIntersectionData.m_polygonBounds.emplace_back(
                PolygonBoundWithHandle{{triangles}, polygonHandle});
        }

        const auto edgeHandlesPair = Api::MeshUserEdgeHandles(*whiteBox);

        const auto edgeHandles = [edgeSelectionMode, &edgeHandlesPair]()
        {
            switch (edgeSelectionMode)
            {
            case EdgeSelectionType::Polygon:
                return edgeHandlesPair.m_user;
            case EdgeSelectionType::All:
                return BuildAllEdgeHandles(edgeHandlesPair);
            default:
                return Api::EdgeHandles{};
            }
        }();

        // all edges that are valid to interact with at this time
        for (const auto& edgeHandle : edgeHandles)
        {
            const auto edge = Api::EdgeVertexPositions(*whiteBox, edgeHandle);
            m_intersectionAndRenderData->m_whiteBoxIntersectionData.m_edgeBounds.emplace_back(
                EdgeBoundWithHandle{EdgeBound{edge[0], edge[1], cl_whiteBoxEdgeSelectionWidth}, edgeHandle});
        }

        // handle drawing 'user' and 'mesh' edges slightly differently
        for (const auto& edgeHandle : edgeHandlesPair.m_user)
        {
            const auto edge = Api::EdgeVertexPositions(*whiteBox, edgeHandle);
            m_intersectionAndRenderData->m_whiteBoxEdgeRenderData.m_bounds.m_user.emplace_back(
                EdgeBoundWithHandle{EdgeBound{edge[0], edge[1], cl_whiteBoxEdgeSelectionWidth}, edgeHandle});
        }

        for (const auto& edgeHandle : edgeHandlesPair.m_mesh)
        {
            const auto edge = Api::EdgeVertexPositions(*whiteBox, edgeHandle);
            m_intersectionAndRenderData->m_whiteBoxEdgeRenderData.m_bounds.m_mesh.emplace_back(
                EdgeBoundWithHandle{EdgeBound{edge[0], edge[1], cl_whiteBoxEdgeSelectionWidth}, edgeHandle});
        }
    }

    void EditorWhiteBoxComponentMode::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        m_worldFromLocal = AzToolsFramework::TransformUniformScale(world);
    }

    void EditorWhiteBoxComponentMode::OnDefaultShapeTypeChanged([[maybe_unused]] const DefaultShapeType defaultShape)
    {
        // ensure the mode and all modifiers are refreshed
        Refresh();
    }

    void EditorWhiteBoxComponentMode::OverrideKeyboardModifierQuery(
        const KeyboardModifierQueryFn& keyboardModifierQueryFn)
    {
        m_keyboardMofifierQueryFn = keyboardModifierQueryFn;
    }

    static AzToolsFramework::ViewportUi::ButtonId RegisterClusterButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName)
    {
        AzToolsFramework::ViewportUi::ButtonId buttonId;
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId, AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        return buttonId;
    }

    void EditorWhiteBoxComponentMode::RemoveSubModeSelectionCluster()
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_modeSelectionClusterId);
    }

    void EditorWhiteBoxComponentMode::CreateSubModeSelectionCluster()
    {
        // create the cluster for changing transform mode
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            m_modeSelectionClusterId, AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster, AzToolsFramework::ViewportUi::Alignment::TopLeft);

        // create and register the buttons
        m_defaultModeButtonId = RegisterClusterButton(m_modeSelectionClusterId, "SketchMode");
        m_edgeRestoreModeButtonId = RegisterClusterButton(m_modeSelectionClusterId, "RestoreMode");

        auto onButtonClicked = [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
        {
            if (buttonId == m_defaultModeButtonId)
            {
                EnterDefaultMode();
            }
            else if (buttonId == m_edgeRestoreModeButtonId)
            {
                EnterEdgeRestoreMode();
            }
        };

        m_modeSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(onButtonClicked);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_modeSelectionClusterId, m_modeSelectionHandler);
    }
} // namespace WhiteBox

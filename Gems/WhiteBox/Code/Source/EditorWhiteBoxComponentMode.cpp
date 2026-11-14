/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxComponent.h"
#include "SubComponentModes/EditorWhiteBoxDefaultMode.h"
#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "SubComponentModes/EditorWhiteBoxEdgeRestoreMode.h"
#include "SubComponentModes/EditorWhiteBoxTransformMode.h"
#include "SubComponentModes/EditorWhiteBoxPaintMode.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "Util/WhiteBoxModelingOps.h"
#include "Tools/WhiteBoxBevelWindow.h"
#include "Tools/WhiteBoxPaintWindow.h"
#include "Tools/WhiteBoxShapeOptionsWindow.h"
#include "Tools/WhiteBoxWeldWindow.h"
#include "Tools/WhiteBoxExtrudeInsetWindow.h"
#include "Tools/WhiteBoxUvProjectionWindow.h"
#include "Tools/WhiteBoxSmoothingWindow.h"
#include "Tools/WhiteBoxMaterialUiUnlock.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include "Util/WhiteBoxSnapUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <QApplication> // required for querying modifier keys
#include <QCursor>
#include <QMenu>
#include <QTimer>
#include <QToolButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

#include "Viewport/WhiteBoxDrawShapeMode.h"

namespace WhiteBox
{
    constexpr AZStd::string_view WhiteBoxPaintSubModeIdentifier = "o3de.context.mode.whiteBox.vertexPaint";

    constexpr AZStd::string_view WhiteBoxDefaultSubModeIdentifier = "o3de.context.mode.whiteBox.default";
    constexpr AZStd::string_view WhiteBoxEdgeRestoreSubModeIdentifier = "o3de.context.mode.whiteBox.edgeRestore";
    constexpr AZStd::string_view WhiteBoxTransformSubModeIdentifier = "o3de.context.mode.whiteBox.transform";
    constexpr AZStd::string_view WhiteBoxDrawShapeSubModeIdentifier =
    "o3de.context.mode.whiteBox.drawShape";

    AZ_CLASS_ALLOCATOR_IMPL(EditorWhiteBoxComponentMode, AZ::SystemAllocator)

    static void SetViewportUiClusterActiveButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, AzToolsFramework::ViewportUi::ButtonId buttonId)
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, clusterId, buttonId);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);
    }

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
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
        EditorWhiteBoxComponentModeRequestBus::Handler::BusConnect(entityComponentIdPair);
        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorWhiteBoxComponentNotificationBus::Handler::BusConnect(entityComponentIdPair);
        AzToolsFramework::ViewportSnapping::ViewportDragCancelRequestBus::Handler::BusConnect();

        // default behavior for querying modifier keys (ask the QApplication)
        m_keyboardModifierQueryFn = []()
        {
            return AzToolsFramework::ViewportInteraction::QueryKeyboardModifiers();
        };

        m_deferredWorkToken = AZStd::make_shared<bool>(true);
        m_worldFromLocal = EditorSpaceFromLocal(entityComponentIdPair);
        CreateSubModeSelectionCluster();
        // start with DefaultMode
        EnterDefaultMode();
        if (qApp != nullptr)
        {
            m_materialUiUnlock = AZStd::make_unique<WhiteBoxMaterialUiUnlock>();
        }
    }

    EditorWhiteBoxComponentMode::~EditorWhiteBoxComponentMode()
    {
        // Invalidate any queued deferred work (see EnterDefaultMode) before anything else, so a
        // callback that fires after this point cannot touch the editor's action context mode.
        m_deferredWorkToken.reset();

        // Safety net: if a White Box sub-mode is still set after leaving component mode, restore the
        // default, or Play and most actions stay locked out.
        if (!AzToolsFramework::ComponentModeFramework::InComponentMode())
        {
            if (auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
            {
                actionManagerInterface->SetActiveActionContextMode(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    AzToolsFramework::DefaultActionContextModeIdentifier);
            }
        }

        RemoveSubModeSelectionCluster();

        AzToolsFramework::ViewportSnapping::ViewportDragCancelRequestBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentModeRequestBus::Handler::BusDisconnect();
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EditorWhiteBoxComponentMode::Reflect(AZ::ReflectContext* context)
    {
        AzToolsFramework::ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<EditorWhiteBoxComponentMode>(context);
    }

    void EditorWhiteBoxComponentMode::RegisterActionContextModes()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "EditorWhiteBoxComponentMode - could not get ActionManagerInterface on RegisterActionContextModes.");

        actionManagerInterface->RegisterActionContextMode(EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxDefaultSubModeIdentifier);
        actionManagerInterface->RegisterActionContextMode(EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxEdgeRestoreSubModeIdentifier);
        actionManagerInterface->RegisterActionContextMode(EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxTransformSubModeIdentifier);
        actionManagerInterface->RegisterActionContextMode(EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxPaintSubModeIdentifier);
        actionManagerInterface->RegisterActionContextMode(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            WhiteBoxDrawShapeSubModeIdentifier);
    }

    void EditorWhiteBoxComponentMode::RegisterActionUpdaters()
    {
        DefaultMode::RegisterActionUpdaters();
        EdgeRestoreMode::RegisterActionUpdaters();
        TransformMode::RegisterActionUpdaters();
    }

    void EditorWhiteBoxComponentMode::RegisterActions()
    {
        DefaultMode::RegisterActions();
        EdgeRestoreMode::RegisterActions();
        TransformMode::RegisterActions();
        DrawShapeMode::RegisterActions();
    }

    void EditorWhiteBoxComponentMode::BindActionsToModes()
    {
        DefaultMode::BindActionsToModes(WhiteBoxDefaultSubModeIdentifier);
        EdgeRestoreMode::BindActionsToModes(WhiteBoxEdgeRestoreSubModeIdentifier);
        TransformMode::BindActionsToModes(WhiteBoxTransformSubModeIdentifier);
        DrawShapeMode::BindActionsToModes(WhiteBoxDrawShapeSubModeIdentifier);
        if (auto* actionManager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
        {
            actionManager->AssignModeToAction(WhiteBoxPaintSubModeIdentifier, "o3de.action.componentMode.end");
        }
    }

    void EditorWhiteBoxComponentMode::BindActionsToMenus()
    {
        DefaultMode::BindActionsToMenus();
        EdgeRestoreMode::BindActionsToMenus();
        TransformMode::BindActionsToMenus();
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

        // Publish the viewport for the manipulator callbacks, whose Action types (Linear, Planar,
        // Surface) do not carry a viewport id of their own. See WhiteBoxSnapUtil.h.
        SnapUtil::SetActiveViewportId(viewportId);

        const AZStd::optional<EdgeIntersection> edgeIntersection = FindClosestEdgeIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection,
            m_worldFromLocal, cameraState);

        const AZStd::optional<PolygonIntersection> polygonIntersection = FindClosestPolygonIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection);

        const AZStd::optional<VertexIntersection> vertexIntersection = FindClosestVertexIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection,
            m_worldFromLocal, cameraState);

        // Dropping excluded hits here gates hover and picking for every mode in one place.
        const auto allowed = [this](const SelectionFilter kind, auto intersection)
        {
            using Intersection = decltype(intersection);
            return m_selectionFilter == SelectionFilter::None || m_selectionFilter == kind
                ? intersection : Intersection{};
        };
        const auto selectableEdge = allowed(SelectionFilter::Edges, edgeIntersection);
        const auto selectablePolygon = allowed(SelectionFilter::Polygons, polygonIntersection);
        const auto selectableVertex = allowed(SelectionFilter::Vertices, vertexIntersection);

        // interactionHandled will be set to true if the mouse interaction has been handled by this white box component
        // which involves either interacting with a manipulator from this white box or clicking on the white box mesh
        // itself
        // Bundle everything any mode might need so all modes share one HandleMouseInteraction
        // signature (DrawShapeMode uses the transform + intersection cache, the editing modes use
        // the precomputed edge/polygon/vertex hits).
        const ModeMouseInteraction mouseContext{
            mouseInteraction,
            GetEntityComponentIdPair(),
            m_worldFromLocal,
            m_intersectionAndRenderData.value(),
            selectableEdge,
            selectablePolygon,
            selectableVertex,
            m_selectionTool,
            m_selectionFilter};

        bool interactionHandled = AZStd::visit(
            [&mouseContext](auto& mode)
            {
                return mode->HandleMouseInteraction(mouseContext);
            },
            m_modes);

        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up)
        {
            RefreshModelingClusterState(); // the selection is what gates these buttons
        }

        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up &&
            (selectableEdge || selectablePolygon || selectableVertex))
        {
            interactionHandled = true;
        }

        return interactionHandled;
    }

    AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> EditorWhiteBoxComponentMode::PopulateViewportUiImpl()
    {
        auto clusterIds = EditorBaseComponentMode::PopulateViewportUiImpl();

        // The framework adds the border after this returns and on every refresh, so remove it next tick.
        AZ::TickBus::QueueFunction(
            []()
            {
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                    AzToolsFramework::ViewportUi::DefaultViewportId,
                    &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveViewportBorder);
            });

        return clusterIds;
    }

    AZStd::string EditorWhiteBoxComponentMode::GetComponentModeName() const
    {
        return "White Box Edit Mode";
    }

    AZ::Uuid EditorWhiteBoxComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<EditorWhiteBoxComponentMode>();
    }

    bool EditorWhiteBoxComponentMode::CancelActiveDrag()
    {
        return AZStd::visit(
            [](auto& mode)
            {
                return mode->CancelActiveDrag();
            },
            m_modes);
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorWhiteBoxComponentMode::PopulateActionsImpl()
    {
        auto actions = AZStd::visit(
            [entityComponentIdPair = GetEntityComponentIdPair()](auto& mode)
            {
                return mode->PopulateActions(entityComponentIdPair);
            },
            m_modes);

        // Take over Escape by reusing the back-action URI (the later binding wins). Cancel a drag, then
        // numeric input, and only then leave component mode.
        actions.push_back(AzToolsFramework::CreateBackAction(
            "Done",
            "Cancel the current operation, or return to normal viewport editing",
            [this]()
            {
                // A latched drag is the innermost thing Escape can cancel. The window that armed the
                // latch stays open, so the next drag still works.
                bool latchedDrag = false;
                EditorWhiteBoxTransformModeRequestBus::EventResult(
                    latchedDrag, GetEntityComponentIdPair(),
                    &EditorWhiteBoxTransformModeRequests::HasLatchedDrag);
                if (latchedDrag)
                {
                    AZStd::visit(
                        [](auto& mode)
                        {
                            return mode->HandleEscape();
                        },
                        m_modes);
                    return;
                }
                if (m_extrudeInsetWindow && m_extrudeInsetWindow->isVisible())
                {
                    m_extrudeInsetWindow->reject();
                    return;
                }
                if (m_weldWindow && m_weldWindow->isVisible())
                {
                    m_weldWindow->reject();
                    return;
                }
                if (m_bevelWindow && m_bevelWindow->isVisible())
                {
                    m_bevelWindow->reject();
                    return;
                }
                if (m_uvProjectionWindow && m_uvProjectionWindow->isVisible())
                {
                    m_uvProjectionWindow->reject();
                    return;
                }
                if (m_smoothingWindow && m_smoothingWindow->isVisible())
                {
                    m_smoothingWindow->reject();
                    return;
                }
                const bool consumed = AZStd::visit(
                    [](auto& mode)
                    {
                        return mode->HandleEscape();
                    },
                    m_modes);

                if (!consumed)
                {
                    AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                        &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::EndComponentMode);
                }
            }));

        return actions;
    }


    void EditorWhiteBoxComponentMode::SetSubMode(SubMode subMode)
    {
        switch (subMode)
        {
        case SubMode::VertexPaint:
            EnterPaintMode();
            break;
        case SubMode::DrawShape:
            EnterDrawShapeMode();
            break;
        case SubMode::EdgeRestore:
            EnterEdgeRestoreMode();
            break;
        case SubMode::Transform:
            EnterTransformMode();
            break;
        case SubMode::Default:
        default:
            EnterDefaultMode();
            break;
        }
    }

    void EditorWhiteBoxComponentMode::EnterPaintMode()
    {
        m_modes = AZStd::make_unique<PaintMode>(GetEntityComponentIdPair());
        m_intersectionAndRenderData = {};
        m_currentSubMode = SubMode::VertexPaint;
        SetViewportUiClusterActiveButton(m_modeSelectionClusterId, m_paintModeButtonId);
        RemoveModelingCluster();
        RemoveShapeSwitcher();
        RemoveSketchCluster();
        RemoveEdgeRestoreCluster();
        CreatePaintCluster();
        if (auto* actionManager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
        {
            actionManager->SetActiveActionContextMode(
                EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxPaintSubModeIdentifier);
        }
    }

    void EditorWhiteBoxComponentMode::EnterDrawShapeMode()
    {
        m_modes = AZStd::make_unique<DrawShapeMode>(GetEntityComponentIdPair());
        m_intersectionAndRenderData = {};
        m_currentSubMode = SubMode::DrawShape;
        SetViewportUiClusterActiveButton(m_modeSelectionClusterId, m_drawShapeModeButtonId);
        RemoveModelingCluster();
        RemoveEdgeRestoreCluster();
        CreateShapeSwitcher();
        RemovePaintCluster();
        RemoveSketchCluster();
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (actionManagerInterface)
        {
            actionManagerInterface->SetActiveActionContextMode(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                WhiteBoxDrawShapeSubModeIdentifier);
        }
    }

    void EditorWhiteBoxComponentMode::EnterDefaultMode()
    {
        m_modes = AZStd::make_unique<DefaultMode>(GetEntityComponentIdPair());
        m_intersectionAndRenderData = {};
        m_currentSubMode = SubMode::Default;
        SetViewportUiClusterActiveButton(m_modeSelectionClusterId, m_defaultModeButtonId);
        RemoveModelingCluster();
        RemoveShapeSwitcher();
        RemovePaintCluster();
        RemoveEdgeRestoreCluster();
        CreateSketchCluster();
        // Switch to the default sub-mode next frame, after ComponentModeActionHandler's own switch - only
        // if this mode and component mode are both still alive, or the editor is left stranded.
        QTimer::singleShot(
            0,
            [this, aliveToken = AZStd::weak_ptr<bool>(m_deferredWorkToken)]()
            {
                if (aliveToken.expired() || !AzToolsFramework::ComponentModeFramework::InComponentMode())
                {
                    return; // the White Box component mode has gone away - leave the mode alone
                }
                // Safe to touch 'this' now: the token is only released by the destructor.
                if (m_currentSubMode != SubMode::Default)
                {
                    return; // the sub-mode moved on before this ran; whoever moved it set the mode
                }

                // Set the Action Context Mode in the Action Manager, if enabled.
                auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
                if (actionManagerInterface)
                {
                    actionManagerInterface->SetActiveActionContextMode(
                        EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxDefaultSubModeIdentifier);
                }
            }
        );
    }

    void EditorWhiteBoxComponentMode::EnterEdgeRestoreMode()
    {
        m_modes = AZStd::make_unique<EdgeRestoreMode>();
        m_intersectionAndRenderData = {};
        m_currentSubMode = SubMode::EdgeRestore;
        SetViewportUiClusterActiveButton(m_modeSelectionClusterId, m_edgeRestoreModeButtonId);
        RemoveModelingCluster();
        RemoveShapeSwitcher();
        RemovePaintCluster();
        RemoveSketchCluster();
        CreateEdgeRestoreCluster();
        // Set the Action Context Mode in the Action Manager, if enabled.
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (actionManagerInterface)
        {
            actionManagerInterface->SetActiveActionContextMode(EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxEdgeRestoreSubModeIdentifier);
        }
    }

    void EditorWhiteBoxComponentMode::EnterTransformMode()
    {
        m_modes = AZStd::make_unique<TransformMode>(GetEntityComponentIdPair());
        m_intersectionAndRenderData = {};
        m_currentSubMode = SubMode::Transform;
        SetViewportUiClusterActiveButton(m_modeSelectionClusterId, m_transformModeButtonId);
        RemoveShapeSwitcher();
        RemovePaintCluster();
        RemoveSketchCluster();
        RemoveEdgeRestoreCluster();
        CreateModelingCluster();
        // Set the Action Context Mode in the Action Manager, if enabled.
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (actionManagerInterface)
        {
            actionManagerInterface->SetActiveActionContextMode(EditorIdentifiers::MainWindowActionContextIdentifier, WhiteBoxTransformSubModeIdentifier);
        }
    }

    void EditorWhiteBoxComponentMode::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // Draw the active snap target here; most draggable tools are not display handlers.
        const auto previousDisplayState = debugDisplay.GetState();
        debugDisplay.DepthWriteOff();
        SnapUtil::DrawActiveSnapTarget(debugDisplay);


        const auto modifiers = m_keyboardModifierQueryFn();
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
    
        // Recomputed every frame because the active layer's transform is part of it, and the layer
        // gizmo writes a new one on every mouse move of a drag.
        m_worldFromLocal = EditorSpaceFromLocal(GetEntityComponentIdPair());

        // generate mesh to query
        if (!m_intersectionAndRenderData.has_value())
        {
            RecalculateWhiteBoxIntersectionData(DecideEdgeSelectionMode(m_currentSubMode));
        }
    
        // Selection changes arrive via the manipulator manager, so re-evaluate the gated buttons per frame.
        RefreshSketchClusterState();
        RefreshModelingClusterState();

        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(ed_whiteBoxEdgeDefault);
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
    
        debugDisplay.SetState(previousDisplayState);
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
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        // Not the world transform handed in: the editing space also carries the active layer's
        // transform, and manipulators cannot hold a non-uniform scale.
        m_worldFromLocal = EditorSpaceFromLocal(GetEntityComponentIdPair());
    }

    void EditorWhiteBoxComponentMode::OnDefaultShapeTypeChanged([[maybe_unused]] const DefaultShapeType defaultShape)
    {
        // ensure the mode and all modifiers are refreshed
        Refresh();
    }

    void EditorWhiteBoxComponentMode::OverrideKeyboardModifierQuery(
        const KeyboardModifierQueryFn& keyboardModifierQueryFn)
    {
        m_keyboardModifierQueryFn = keyboardModifierQueryFn;
    }

    static AzToolsFramework::ViewportUi::ButtonId RegisterClusterButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName)
    {
        AzToolsFramework::ViewportUi::ButtonId buttonId;
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId, AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
            iconName[0] == ':' ? AZStd::string(iconName)
                              : AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        return buttonId;
    }

    void EditorWhiteBoxComponentMode::CreateEdgeRestoreCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_edgeRestoreClusterId != ViewportUi::InvalidClusterId)
        {
            return;
        }

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_edgeRestoreClusterId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster, ViewportUi::Alignment::TopRight);

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_flipEdgeButtonId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, m_edgeRestoreClusterId,
            AZStd::string(":/WhiteBox/Icons/FlipEdge.svg"));
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_edgeRestoreClusterId, m_flipEdgeButtonId,
            AZStd::string("Flip Edge - split the hovered quad across its other diagonal (or right-click it)"));

        m_edgeRestoreHandler = AZ::Event<ViewportUi::ButtonId>::Handler(
            [this](const ViewportUi::ButtonId buttonId)
            {
                if (buttonId != m_flipEdgeButtonId)
                {
                    return;
                }
                // Edge Restore has no request bus; the mode object lives in m_modes, so reach it there.
                if (auto* mode = AZStd::get_if<AZStd::unique_ptr<EdgeRestoreMode>>(&m_modes))
                {
                    (*mode)->FlipHoveredEdge(GetEntityComponentIdPair());
                }
            });

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_edgeRestoreClusterId, m_edgeRestoreHandler);
    }

    void EditorWhiteBoxComponentMode::RemoveEdgeRestoreCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_edgeRestoreClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        m_edgeRestoreHandler.Disconnect();
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_edgeRestoreClusterId);
        m_edgeRestoreClusterId = ViewportUi::InvalidClusterId;
    }

    void EditorWhiteBoxComponentMode::CreateSketchCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_sketchClusterId != ViewportUi::InvalidClusterId)
        {
            RefreshSketchClusterState();
            return;
        }

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_sketchClusterId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster, ViewportUi::Alignment::TopRight);

        const auto addButton = [this](const char* icon, const char* tooltip)
        {
            namespace ViewportUi = AzToolsFramework::ViewportUi;
            ViewportUi::ButtonId buttonId;
            ViewportUi::ViewportUiRequestBus::EventResult(
                buttonId, ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, m_sketchClusterId,
                AZStd::string::format(":/WhiteBox/Icons/%s.svg", icon));
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
                m_sketchClusterId, buttonId, AZStd::string(tooltip));
            return buttonId;
        };

        // Latched drag modifiers - click to turn on, click again to turn off.
        m_extrudeButtonId = addButton("Extrude", "Extrude - drag a face or edge to pull new geometry from it (or hold Ctrl)");
        m_insetButtonId = addButton("Inset", "Inset - drag a face's scale handle to inset it (or hold Ctrl)");
        // Momentary verbs on whatever is selected.
        m_hideEdgeButtonId = addButton("HideEdge", "Hide Edge - merge the two polygons either side of the selected edge");
        m_hideVertexButtonId = addButton("HideVertex", "Hide Vertex - remove the selected vertex from its polygon");

        m_sketchHandler = AZ::Event<ViewportUi::ButtonId>::Handler(
            [this](const ViewportUi::ButtonId buttonId)
            {
                auto* component = FindWhiteBoxComponent(GetEntityComponentIdPair());
                if (component == nullptr)
                {
                    return;
                }

                if (buttonId == m_extrudeButtonId || buttonId == m_insetButtonId)
                {
                    // Toggle: pressing the latched one turns it off. They stay mutually exclusive, so a
                    // drag always does exactly one thing.
                    const bool extrude = buttonId == m_extrudeButtonId && !component->GetStickyExtrude();
                    const bool inset = buttonId == m_insetButtonId && !component->GetStickyInset();
                    component->SetStickyExtrude(extrude);
                    component->SetStickyInset(inset);
                }
                else if (buttonId == m_hideEdgeButtonId)
                {
                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        GetEntityComponentIdPair(), &EditorWhiteBoxDefaultModeRequests::HideSelectedEdge);
                }
                else if (buttonId == m_hideVertexButtonId)
                {
                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        GetEntityComponentIdPair(), &EditorWhiteBoxDefaultModeRequests::HideSelectedVertex);
                }
                else
                {
                    return;
                }

                RefreshSketchClusterState();
            });

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_sketchClusterId, m_sketchHandler);

        RefreshSketchClusterState();
    }

    void EditorWhiteBoxComponentMode::RemoveSketchCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_sketchClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        m_sketchHandler.Disconnect();
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_sketchClusterId);
        m_sketchClusterId = ViewportUi::InvalidClusterId;
        m_sketchEdgeSelected.reset();
        m_sketchVertexSelected.reset();
    }

    void EditorWhiteBoxComponentMode::RefreshSketchClusterState()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_sketchClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        auto* component = FindWhiteBoxComponent(GetEntityComponentIdPair());
        if (component == nullptr)
        {
            return;
        }

        if (component->GetStickyExtrude() || component->GetStickyInset())
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_sketchClusterId, component->GetStickyExtrude() ? m_extrudeButtonId : m_insetButtonId);
        }
        else
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_sketchClusterId);
        }

        // Ask the same question the Hide Edge / Hide Vertex shortcuts ask. An earlier attempt tested the
        // sub-mode's selected-modifier variant directly and never reported a selection.
        Api::EdgeHandles edges;
        Api::VertexHandles vertices;
        EditorWhiteBoxDefaultModeRequestBus::EventResult(
            edges, GetEntityComponentIdPair(), &EditorWhiteBoxDefaultModeRequests::SelectedEdgeHandles);
        EditorWhiteBoxDefaultModeRequestBus::EventResult(
            vertices, GetEntityComponentIdPair(), &EditorWhiteBoxDefaultModeRequests::SelectedVertexHandles);

        // Per frame: the manipulator manager consumes selection clicks before the mouse handler sees them.
        const bool hasEdge = !edges.empty();
        const bool hasVertex = !vertices.empty();
        const auto enable = [this](const ViewportUi::ButtonId buttonId, const bool usable)
        {
            namespace ViewportUi = AzToolsFramework::ViewportUi;
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterDisableButton,
                m_sketchClusterId, buttonId, !usable);
        };
        if (m_sketchEdgeSelected != hasEdge)
        {
            m_sketchEdgeSelected = hasEdge;
            enable(m_hideEdgeButtonId, hasEdge);
        }
        if (m_sketchVertexSelected != hasVertex)
        {
            m_sketchVertexSelected = hasVertex;
            enable(m_hideVertexButtonId, hasVertex);
        }
    }

    void EditorWhiteBoxComponentMode::CreatePaintCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_paintClusterId != ViewportUi::InvalidClusterId)
        {
            RefreshPaintClusterActive();
            return;
        }

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_paintClusterId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster, ViewportUi::Alignment::TopRight);

        const AZStd::pair<FacePaintOperation, AZStd::pair<const char*, const char*>> operations[] = {
            { FacePaintOperation::Material, { "PaintMaterial", "Paint Material - assign the chosen material to faces" } },
            { FacePaintOperation::Color, { "PaintColor", "Paint Color - assign the chosen colour to faces" } },
            { FacePaintOperation::ResetMaterial, { "ResetMaterial", "Reset Material - drop a face's material override" } },
            { FacePaintOperation::ResetColor, { "ResetColor", "Reset Color - drop a face's painted colour" } },
            { FacePaintOperation::BlendLayer2,
              { "BlendLayer2", "Blend Layer 2 - brush vertices towards layer 2 of a vertex-blend material" } },
            { FacePaintOperation::BlendLayer3,
              { "BlendLayer3", "Blend Layer 3 - brush vertices towards layer 3 of a vertex-blend material" } },
            { FacePaintOperation::BlendBase, { "BlendBase", "Blend Base - brush vertices back to the base layer" } },
        };

        m_paintButtons.clear();
        m_paintButtons.reserve(AZ_ARRAY_SIZE(operations));
        for (const auto& operation : operations)
        {
            ViewportUi::ButtonId buttonId;
            ViewportUi::ViewportUiRequestBus::EventResult(
                buttonId, ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, m_paintClusterId,
                AZStd::string::format(":/WhiteBox/Icons/%s.svg", operation.second.first));

            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_paintClusterId, buttonId,
                AZStd::string(operation.second.second));

            m_paintButtons.push_back({ buttonId, operation.first });
        }

        m_paintHandler = AZ::Event<ViewportUi::ButtonId>::Handler(
            [this](const ViewportUi::ButtonId buttonId)
            {
                const auto found = AZStd::find_if(
                    m_paintButtons.begin(), m_paintButtons.end(),
                    [buttonId](const auto& entry)
                    {
                        return entry.first == buttonId;
                    });
                if (found == m_paintButtons.end())
                {
                    return;
                }
                auto* component = FindWhiteBoxComponent(GetEntityComponentIdPair());
                if (component == nullptr)
                {
                    return;
                }

                // Brush settings are transient tool state, not scene data, so no undo step here - the
                // paint stroke itself is what gets recorded.
                FacePaintSettings settings = component->GetFacePaintSettings();
                settings.m_operation = found->second;
                component->SetFacePaintSettings(settings);

                RefreshPaintClusterActive();
                if (m_paintWindow)
                {
                    m_paintWindow->RefreshValues(); // different verb, different payload
                }
            });

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_paintClusterId, m_paintHandler);

        RefreshPaintClusterActive();

        if (!m_paintWindow)
        {
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
            m_paintWindow = new WhiteBoxPaintWindow(GetEntityComponentIdPair(), mainWindow);
        }
        m_paintWindow->ShowNearCursor();
    }

    void EditorWhiteBoxComponentMode::RemovePaintCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_paintClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        m_paintHandler.Disconnect();
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_paintClusterId);
        m_paintClusterId = ViewportUi::InvalidClusterId;
        m_paintButtons.clear();

        if (m_paintWindow)
        {
            m_paintWindow->Dismiss();
            m_paintWindow.clear();
        }
    }

    void EditorWhiteBoxComponentMode::RefreshPaintClusterActive()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_paintClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        auto* component = FindWhiteBoxComponent(GetEntityComponentIdPair());
        if (component == nullptr)
        {
            return;
        }

        const FacePaintOperation current = component->GetFacePaintSettings().m_operation;
        const auto found = AZStd::find_if(
            m_paintButtons.begin(), m_paintButtons.end(),
            [current](const auto& entry)
            {
                return entry.second == current;
            });
        if (found != m_paintButtons.end())
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_paintClusterId, found->first);
        }
    }

    void EditorWhiteBoxComponentMode::CreateShapeSwitcher()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_shapeSwitcherId != ViewportUi::InvalidClusterId)
        {
            RefreshShapeSwitcherActive();
            return;
        }

        // Top-right column; a cluster (not a switcher) so the active shape is highlighted and it overflows cleanly.
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_shapeSwitcherId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster, ViewportUi::Alignment::TopRight);

        // Room, Door and Circular Stairs are parametric-only, so they are not offered here.
        const AZStd::pair<DrawShapeType, AZStd::pair<const char*, const char*>> shapes[] = {
            { DrawShapeType::Box, { "ShapeBox", "Box" } },
            { DrawShapeType::Cylinder, { "ShapeCylinder", "Cylinder" } },
            { DrawShapeType::Pyramid, { "ShapePyramid", "Pyramid" } },
            { DrawShapeType::Cone, { "ShapeCone", "Cone" } },
            { DrawShapeType::Sphere, { "ShapeSphere", "Sphere" } },
            { DrawShapeType::Plane, { "ShapePlane", "Plane" } },
            { DrawShapeType::Torus, { "ShapeTorus", "Torus" } },
            { DrawShapeType::Pipe, { "ShapePipe", "Pipe" } },
            { DrawShapeType::Staircase, { "ShapeStaircase", "Staircase" } },
            { DrawShapeType::Polygon, { "ShapePolygon", "Freeform Polygon" } },
        };

        m_shapeButtons.clear();
        m_shapeButtons.reserve(AZ_ARRAY_SIZE(shapes));
        for (const auto& shape : shapes)
        {
            ViewportUi::ButtonId buttonId;
            ViewportUi::ViewportUiRequestBus::EventResult(
                buttonId, ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, m_shapeSwitcherId,
                AZStd::string::format(":/WhiteBox/Icons/%s.svg", shape.second.first));

            // The cluster shows no label, so the name only survives in the tooltip and in the Shape
            // Options window's title.
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_shapeSwitcherId, buttonId,
                AZStd::string::format("Draw a %s", shape.second.second));

            m_shapeButtons.push_back({ buttonId, shape.first });
        }

        // The cube stamp: same cluster, because picking it changes what a click produces just as much
        // as picking a primitive does.
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_cubeStampButtonId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, m_shapeSwitcherId,
            AZStd::string(":/WhiteBox/Icons/ShapeCubeStamp.svg"));
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_shapeSwitcherId, m_cubeStampButtonId,
            AZStd::string("Cube Stamp - click to place grid-snapped cubes instead of dragging a footprint"));

        m_shapeSwitcherHandler = AZ::Event<ViewportUi::ButtonId>::Handler(
            [this](const ViewportUi::ButtonId buttonId)
            {
                auto* component = FindWhiteBoxComponent(GetEntityComponentIdPair());
                if (component == nullptr)
                {
                    return;
                }

                const bool cubeStamp = buttonId == m_cubeStampButtonId;
                const auto found = AZStd::find_if(
                    m_shapeButtons.begin(), m_shapeButtons.end(),
                    [buttonId](const auto& entry)
                    {
                        return entry.first == buttonId;
                    });
                if (!cubeStamp && found == m_shapeButtons.end())
                {
                    return;
                }

                // Serialized state, so undoable. Stamp and primitive are mutually exclusive.
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("White Box Draw Shape");
                    component->SetDrawUnitCube(cubeStamp);
                    if (!cubeStamp)
                    {
                        component->SetDrawShapeType(found->second);
                    }
                    undoBatch.MarkEntityDirty(GetEntityComponentIdPair().GetEntityId());
                }
                RefreshShapeSwitcherActive();
                if (m_shapeOptionsWindow)
                {
                    m_shapeOptionsWindow->RefreshValues(); // different primitive, different rows
                }
            });

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_shapeSwitcherId,
            m_shapeSwitcherHandler);

        RefreshShapeSwitcherActive();

        // The cluster picks the primitive; its settings belong beside it, not back in the pane.
        if (!m_shapeOptionsWindow)
        {
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
            m_shapeOptionsWindow = new WhiteBoxShapeOptionsWindow(GetEntityComponentIdPair(), mainWindow);
        }
        m_shapeOptionsWindow->ShowNearCursor();
    }

    void EditorWhiteBoxComponentMode::RemoveShapeSwitcher()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_shapeSwitcherId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        m_shapeSwitcherHandler.Disconnect();
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_shapeSwitcherId);
        m_shapeSwitcherId = ViewportUi::InvalidClusterId;
        m_shapeButtons.clear();

        if (m_shapeOptionsWindow)
        {
            m_shapeOptionsWindow->Dismiss();
            m_shapeOptionsWindow.clear();
        }
    }

    void EditorWhiteBoxComponentMode::RefreshShapeSwitcherActive()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_shapeSwitcherId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        auto* component = FindWhiteBoxComponent(GetEntityComponentIdPair());
        if (component == nullptr)
        {
            return;
        }

        if (component->GetDrawUnitCube())
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_shapeSwitcherId, m_cubeStampButtonId);
            return;
        }

        const DrawShapeType current = component->GetDrawShape();
        const auto found = AZStd::find_if(
            m_shapeButtons.begin(), m_shapeButtons.end(),
            [current](const auto& entry)
            {
                return entry.second == current;
            });
        if (found != m_shapeButtons.end())
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, m_shapeSwitcherId,
                found->first);
        }
    }

    void EditorWhiteBoxComponentMode::CreateModelingCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_modelingClusterId != ViewportUi::InvalidClusterId)
        {
            RefreshModelingClusterState();
            return; // already up, e.g. re-entering Transform from Transform
        }

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_modelingClusterId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster, ViewportUi::Alignment::TopRight);

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_selectionClusterId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster, ViewportUi::Alignment::TopRight);
        m_selectVerticesButtonId = RegisterClusterButton(m_selectionClusterId, ":/WhiteBox/Icons/SelectVertices.svg");
        m_selectEdgesButtonId = RegisterClusterButton(m_selectionClusterId, ":/WhiteBox/Icons/SelectEdges.svg");
        m_selectPolygonsButtonId = RegisterClusterButton(m_selectionClusterId, ":/WhiteBox/Icons/SelectPolygons.svg");
        const auto selectionTooltip = [this](const ViewportUi::ButtonId buttonId, const char* text)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
                m_selectionClusterId, buttonId, AZStd::string(text));
        };
        selectionTooltip(m_selectVerticesButtonId, WhiteboxSelectVerticesTooltip);
        selectionTooltip(m_selectEdgesButtonId, WhiteboxSelectEdgesTooltip);
        selectionTooltip(m_selectPolygonsButtonId, WhiteboxSelectPolygonsTooltip);
        m_selectionHandler = AZ::Event<ViewportUi::ButtonId>::Handler(
            [this](const ViewportUi::ButtonId buttonId)
            {
                const auto wanted = buttonId == m_selectVerticesButtonId ? SelectionFilter::Vertices
                    : buttonId == m_selectEdgesButtonId ? SelectionFilter::Edges
                    : buttonId == m_selectPolygonsButtonId ? SelectionFilter::Polygons : SelectionFilter::None;
                if (wanted != SelectionFilter::None && wanted != m_selectionFilter)
                {
                    // Switching element type carries the selection across; Ctrl takes everything it touches.
                    const auto target = wanted == SelectionFilter::Vertices ? Api::SelectionElement::Vertex
                        : wanted == SelectionFilter::Edges                  ? Api::SelectionElement::Edge
                                                                            : Api::SelectionElement::Polygon;
                    const bool touching = QApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
                    const auto result = ModelingOps::ConvertSelection(GetEntityComponentIdPair(), target, touching);
                    AZ_Warning("White Box", result.m_success, "%s", result.m_message.c_str());
                }
                // Clicking the lit one turns the restriction off, so no fourth button is needed.
                m_selectionFilter = m_selectionFilter == wanted ? SelectionFilter::None : wanted;
                RefreshSelectionClusterState();
            });
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_selectionClusterId, m_selectionHandler);
        RefreshSelectionClusterState();

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_toolClusterId, ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster, ViewportUi::Alignment::TopRight);
        m_stickySelectButtonId = RegisterClusterButton(m_toolClusterId, ":/WhiteBox/Icons/StickySelect.svg");
        m_boxSelectButtonId = RegisterClusterButton(m_toolClusterId, ":/WhiteBox/Icons/BoxSelect.svg");
        const auto toolTooltip = [this](const ViewportUi::ButtonId buttonId, const char* text)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
                m_toolClusterId, buttonId, AZStd::string(text));
        };
        toolTooltip(m_stickySelectButtonId, WhiteboxStickySelectTooltip);
        toolTooltip(m_boxSelectButtonId, WhiteboxBoxSelectTooltip);
        m_toolHandler = AZ::Event<ViewportUi::ButtonId>::Handler(
            [this](const ViewportUi::ButtonId buttonId)
            {
                const auto wanted = buttonId == m_stickySelectButtonId ? SelectionTool::Sticky
                    : buttonId == m_boxSelectButtonId ? SelectionTool::Box : SelectionTool::None;
                m_selectionTool = m_selectionTool == wanted ? SelectionTool::None : wanted;
                RefreshToolClusterState();
            });
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_toolClusterId, m_toolHandler);
        RefreshToolClusterState();

        m_transformExtrudeButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Extrude.svg");
        m_transformInsetButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Inset.svg");
        m_edgeLoopButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/EdgeLoop.svg");
        m_edgeRingButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/EdgeRing.svg");
        m_selectCoplanarButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/SelectCoplanar.svg");
        m_selectLinkedButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/SelectLinked.svg");
        m_growSelectionButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/GrowSelection.svg");
        m_shrinkSelectionButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/ShrinkSelection.svg");
        m_selectSimilarButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/SelectSimilar.svg");
        m_mergePolygonsButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/MergePolygons.svg");
        m_bridgeButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Bridge.svg");
        m_weldButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Weld.svg");
        m_connectVerticesButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/ConnectVertices.svg");
        m_subdivideButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Subdivide.svg");
        m_fillHoleButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/FillHole.svg");
        m_deletePolygonButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/DeletePolygon.svg");
        m_detachButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/DetachToLayer.svg");
        m_loopCutButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/LoopCut.svg");
        m_knifeButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Knife.svg");
        m_insertVertexButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/InsertVertex.svg");
        m_bevelButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Bevel.svg");
        m_uvProjectionButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/UvProjection.svg");
        m_smoothingButtonId = RegisterClusterButton(m_modelingClusterId, ":/WhiteBox/Icons/Smoothing.svg");

        const auto tooltip = [this](const ViewportUi::ButtonId buttonId, const char* text)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
                m_modelingClusterId, buttonId, AZStd::string(text));
        };
        tooltip(m_transformExtrudeButtonId, "Extrude - type a distance, or latch it and drag any polygon or edge");
        tooltip(m_transformInsetButtonId, "Inset - type a percentage, or latch it and drag any region, flat or bent");
        tooltip(m_edgeLoopButtonId, "Select Loop: follow connected edges, or run a strip through selected quads");
        tooltip(m_edgeRingButtonId, "Select Ring: cross opposite quad edges, or the strip the other way");
        tooltip(m_selectCoplanarButtonId, WhiteboxSelectCoplanarTooltip);
        tooltip(m_selectLinkedButtonId, WhiteboxSelectLinkedTooltip);
        tooltip(m_growSelectionButtonId, WhiteboxGrowSelectionTooltip);
        tooltip(m_shrinkSelectionButtonId, WhiteboxShrinkSelectionTooltip);
        tooltip(m_detachButtonId, WhiteboxDetachTooltip);
        tooltip(m_connectVerticesButtonId, WhiteboxConnectVerticesTooltip);
        tooltip(m_insertVertexButtonId, WhiteboxInsertVertexTooltip);
        tooltip(m_uvProjectionButtonId, WhiteboxUvProjectionTooltip);
        tooltip(m_smoothingButtonId, WhiteboxSmoothingTooltip);
        tooltip(m_subdivideButtonId, WhiteboxSubdivideTooltip);
        tooltip(m_selectSimilarButtonId, WhiteboxSelectSimilarTooltip);
        tooltip(m_mergePolygonsButtonId, WhiteboxMergePolygonsTooltip);
        tooltip(m_bridgeButtonId, WhiteboxModelingClusterBridgeTooltip);
        tooltip(m_weldButtonId, WhiteboxModelingClusterWeldTooltip);
        tooltip(m_fillHoleButtonId, WhiteboxModelingClusterFillHoleTooltip);
        tooltip(m_deletePolygonButtonId, WhiteboxModelingClusterDeletePolygonTooltip);
        tooltip(m_loopCutButtonId, WhiteboxModelingClusterLoopCutTooltip);
        tooltip(m_knifeButtonId, "Knife: click surface points to cut; Enter applies, Esc cancels");
        tooltip(m_bevelButtonId, WhiteboxModelingClusterBevelTooltip);

        m_modelingHandler = AZ::Event<ViewportUi::ButtonId>::Handler(
            [this](const ViewportUi::ButtonId buttonId)
            {
                const AZ::EntityComponentIdPair pair = GetEntityComponentIdPair();
                ModelingOps::Result result;
                if (m_extrudeInsetWindow)
                {
                    m_extrudeInsetWindow->Dismiss();
                    m_extrudeInsetWindow.clear();
                }
                if (buttonId == m_transformExtrudeButtonId || buttonId == m_transformInsetButtonId)
                {
                    // Clicking an armed latch button releases the latch instead of reopening its window.
                    const auto wanted = buttonId == m_transformInsetButtonId
                        ? TransformModelingLatch::Inset : TransformModelingLatch::Extrude;
                    TransformModelingLatch latch = TransformModelingLatch::None;
                    EditorWhiteBoxTransformModeRequestBus::EventResult(
                        latch, pair, &EditorWhiteBoxTransformModeRequests::GetModelingLatch);
                    if (latch == wanted)
                    {
                        EditorWhiteBoxTransformModeRequestBus::Event(
                            pair, &EditorWhiteBoxTransformModeRequests::SetModelingLatch,
                            TransformModelingLatch::None);
                        RefreshModelingClusterState();
                        return;
                    }
                    // Opening the other one's window releases whatever was armed, so the highlight,
                    // the checkbox and what a drag actually does never disagree.
                    EditorWhiteBoxTransformModeRequestBus::Event(
                        pair, &EditorWhiteBoxTransformModeRequests::SetModelingLatch,
                        TransformModelingLatch::None);
                    if (m_bevelWindow) { m_bevelWindow->Dismiss(); m_bevelWindow.clear(); }
                    if (m_weldWindow) { m_weldWindow->Dismiss(); m_weldWindow.clear(); }
                    QWidget* mainWindow = nullptr;
                    AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                        mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                    m_extrudeInsetWindow = new WhiteBoxExtrudeInsetWindow(
                        pair, buttonId == m_transformInsetButtonId, mainWindow);
                    m_extrudeInsetWindow->ShowNearCursor();
                    result = {true, {}};
                }
                else if (buttonId == m_edgeLoopButtonId || buttonId == m_edgeRingButtonId)
                {
                    result = ModelingOps::SelectEdgePattern(pair, buttonId == m_edgeRingButtonId);
                }
                else if (buttonId == m_bridgeButtonId)
                {
                    result = ModelingOps::Bridge(pair);
                }
                else if (buttonId == m_fillHoleButtonId)
                {
                    result = ModelingOps::FillHole(pair);
                }
                else if (buttonId == m_selectCoplanarButtonId)
                {
                    result = ModelingOps::SelectCoplanar(pair);
                }
                else if (buttonId == m_selectLinkedButtonId)
                {
                    result = ModelingOps::SelectLinked(pair);
                }
                else if (buttonId == m_growSelectionButtonId)
                {
                    result = ModelingOps::GrowSelection(pair);
                }
                else if (buttonId == m_shrinkSelectionButtonId)
                {
                    result = ModelingOps::ShrinkSelection(pair);
                }
                else if (buttonId == m_selectSimilarButtonId)
                {
                    // Polygons can match on several properties, so they ask which; edges and vertices have one each.
                    Api::SimilarBy similarBy = Api::SimilarBy::Material;
                    if (!ModelingOps::CurrentSelection(pair).m_polygons.empty())
                    {
                        QMenu menu;
                        const AZStd::pair<const char*, Api::SimilarBy> choices[] = {
                            { "Material", Api::SimilarBy::Material },
                            { "Facing (normal)", Api::SimilarBy::Normal },
                            { "Area", Api::SimilarBy::Area },
                            { "Side Count", Api::SimilarBy::Sides },
                            { "Smoothing Groups", Api::SimilarBy::SmoothingGroups },
                        };
                        for (const auto& choice : choices)
                        {
                            menu.addAction(QString::fromUtf8(choice.first))->setData(static_cast<int>(choice.second));
                        }
                        QAction* picked = menu.exec(QCursor::pos());
                        if (picked == nullptr)
                        {
                            return;
                        }
                        similarBy = static_cast<Api::SimilarBy>(picked->data().toInt());
                    }
                    result = ModelingOps::SelectSimilar(pair, similarBy);
                }
                else if (buttonId == m_subdivideButtonId)
                {
                    result = ModelingOps::Subdivide(pair);
                }
                else if (buttonId == m_smoothingButtonId)
                {
                    if (m_smoothingWindow && !m_smoothingWindow->HasCurrentLayer())
                    {
                        m_smoothingWindow->Dismiss();
                        m_smoothingWindow.clear();
                    }
                    if (!m_smoothingWindow || !m_smoothingWindow->isVisible())
                    {
                        QWidget* mainWindow = nullptr;
                        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                            mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                        m_smoothingWindow = new WhiteBoxSmoothingWindow(pair, mainWindow);
                    }
                    m_smoothingWindow->ShowNearCursor();
                    result = {true, {}};
                }
                else if (buttonId == m_detachButtonId)
                {
                    result = ModelingOps::DetachToLayer(pair);
                }
                else if (buttonId == m_connectVerticesButtonId)
                {
                    result = ModelingOps::ConnectVertices(pair);
                }
                else if (buttonId == m_insertVertexButtonId)
                {
                    if (m_bevelWindow) { m_bevelWindow->Dismiss(); m_bevelWindow.clear(); }
                    if (m_weldWindow) { m_weldWindow->Dismiss(); m_weldWindow.clear(); }
                    result = ModelingOps::BeginInsertVertex(pair);
                    if (result.m_success)
                    {
                        result.m_message.clear(); // the in-viewport status line explains the tool
                    }
                }
                else if (buttonId == m_uvProjectionButtonId)
                {
                    if (m_uvProjectionWindow && !m_uvProjectionWindow->HasCurrentLayer())
                    {
                        m_uvProjectionWindow->Dismiss();
                        m_uvProjectionWindow.clear();
                    }
                    if (!m_uvProjectionWindow || !m_uvProjectionWindow->isVisible())
                    {
                        QWidget* mainWindow = nullptr;
                        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                            mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                        m_uvProjectionWindow = new WhiteBoxUvProjectionWindow(pair, mainWindow);
                    }
                    m_uvProjectionWindow->ShowNearCursor();
                    result = {true, {}};
                }
                else if (buttonId == m_mergePolygonsButtonId)
                {
                    result = ModelingOps::MergePolygons(pair);
                }
                else if (buttonId == m_deletePolygonButtonId)
                {
                    result = ModelingOps::DeletePolygon(pair);
                }
                else if (buttonId == m_weldButtonId)
                {
                    if (m_bevelWindow)
                    {
                        m_bevelWindow->Dismiss();
                        m_bevelWindow.clear();
                    }
                    if (m_weldWindow && !m_weldWindow->HasCurrentLayer())
                    {
                        m_weldWindow->Dismiss();
                        m_weldWindow.clear();
                    }
                    if (!m_weldWindow || !m_weldWindow->isVisible())
                    {
                        QWidget* mainWindow = nullptr;
                        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                            mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                        m_weldWindow = new WhiteBoxWeldWindow(pair, mainWindow);
                    }
                    m_weldWindow->ShowNearCursor();
                    result = {true, {}};
                }
                else if (buttonId == m_knifeButtonId)
                {
                    if (m_bevelWindow) { m_bevelWindow->Dismiss(); m_bevelWindow.clear(); }
                    if (m_weldWindow) { m_weldWindow->Dismiss(); m_weldWindow.clear(); }
                    EditorWhiteBoxTransformModeRequestBus::Event(pair, &EditorWhiteBoxTransformModeRequests::BeginKnife);
                    result = {true, {}};
                }
                else if (buttonId == m_loopCutButtonId)
                {
                    result = ModelingOps::BeginLoopCut(pair);
                }
                else if (buttonId == m_bevelButtonId)
                {
                    result = ModelingOps::HasLiveBevel(pair)
                        ? ModelingOps::Result{true, {}} : ModelingOps::BeginBevel(pair);
                    if (result.m_success)
                    {
                        if (m_bevelWindow && !m_bevelWindow->HasCurrentBevel())
                        {
                            m_bevelWindow->Dismiss();
                            m_bevelWindow.clear();
                        }
                        if (!m_bevelWindow || !m_bevelWindow->isVisible())
                        {
                            QWidget* mainWindow = nullptr;
                            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                                mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                            m_bevelWindow = new WhiteBoxBevelWindow(pair, mainWindow);
                        }
                        if (m_weldWindow)
                        {
                            m_weldWindow->Dismiss();
                            m_weldWindow.clear();
                        }
                        m_bevelWindow->ShowNearCursor();
                    }
                }
                else
                {
                    return;
                }

                // A viewport button has nowhere to put a status line, so a refusal goes to the console
                // rather than failing silently; the pane shows the same message inline.
                AZ_Warning("White Box", result.m_success, "%s", result.m_message.c_str());
                RefreshModelingClusterState();
            });

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_modelingClusterId, m_modelingHandler);

        RefreshModelingClusterState();
    }

    void EditorWhiteBoxComponentMode::RefreshSelectionClusterState()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_selectionClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }
        if (m_selectionFilter == SelectionFilter::None)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton,
                m_selectionClusterId);
            return;
        }
        const auto active = m_selectionFilter == SelectionFilter::Vertices ? m_selectVerticesButtonId
            : m_selectionFilter == SelectionFilter::Edges ? m_selectEdgesButtonId : m_selectPolygonsButtonId;
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            m_selectionClusterId, active);
    }

    void EditorWhiteBoxComponentMode::RefreshToolClusterState()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_toolClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }
        if (m_selectionTool == SelectionTool::None)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton,
                m_toolClusterId);
            return;
        }
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            m_toolClusterId,
            m_selectionTool == SelectionTool::Sticky ? m_stickySelectButtonId : m_boxSelectButtonId);
    }

    void EditorWhiteBoxComponentMode::RemoveModelingCluster()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_toolClusterId != ViewportUi::InvalidClusterId)
        {
            m_toolHandler.Disconnect();
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
                m_toolClusterId);
            m_toolClusterId = ViewportUi::InvalidClusterId;
        }

        if (m_selectionClusterId != ViewportUi::InvalidClusterId)
        {
            m_selectionHandler.Disconnect();
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
                m_selectionClusterId);
            m_selectionClusterId = ViewportUi::InvalidClusterId;
        }

        if (m_extrudeInsetWindow)
        {
            m_extrudeInsetWindow->Dismiss();
            m_extrudeInsetWindow.clear();
        }
        if (m_weldWindow)
        {
            m_weldWindow->Dismiss();
            m_weldWindow.clear();
        }
        if (m_bevelWindow)
        {
            m_bevelWindow->Dismiss();
            m_bevelWindow.clear();
        }
        if (m_uvProjectionWindow)
        {
            m_uvProjectionWindow->Dismiss();
            m_uvProjectionWindow.clear();
        }
        if (m_smoothingWindow)
        {
            m_smoothingWindow->Dismiss();
            m_smoothingWindow.clear();
        }
        if (m_modelingClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        m_modelingHandler.Disconnect();
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_modelingClusterId);
        m_modelingClusterId = ViewportUi::InvalidClusterId;
        m_modelingClusterState.reset();
    }

    void EditorWhiteBoxComponentMode::RefreshModelingClusterState()
    {
        namespace ViewportUi = AzToolsFramework::ViewportUi;
        if (m_modelingClusterId == ViewportUi::InvalidClusterId)
        {
            return;
        }

        const AZ::EntityComponentIdPair pair = GetEntityComponentIdPair();
        TransformModelingLatch latch = TransformModelingLatch::None;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            latch, pair, &EditorWhiteBoxTransformModeRequests::GetModelingLatch);
        const ModelingOps::Selection selection = ModelingOps::CurrentSelection(pair);
        const bool edgeSelection = ModelingOps::CanSelectEdgePattern(selection);
        const bool bridge = ModelingOps::CanBridge(selection);
        const bool weld = ModelingOps::CanWeld(selection);
        const bool fillHole = ModelingOps::CanFillHole(selection);
        const bool deletePolygon = ModelingOps::CanDeletePolygon(selection);
        const bool mergePolygons = ModelingOps::CanMergePolygons(selection);
        const bool selectCoplanar = ModelingOps::CanSelectCoplanar(selection);
        const bool selectLinked = ModelingOps::CanSelectLinked(selection);
        const bool growShrink = ModelingOps::CanGrowShrink(selection);
        const bool detach = ModelingOps::CanDetach(selection);
        const bool connectVertices = ModelingOps::CanConnectVertices(selection);
        const bool subdivide = ModelingOps::CanSubdivide(selection);
        const bool selectSimilar = ModelingOps::CanSelectSimilar(selection);
        const bool loopCut = ModelingOps::CanLoopCut(selection);
        const bool bevel = ModelingOps::CanBevel(selection) || selection.m_liveBevel;
        bool knife = false;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            knife, pair, &EditorWhiteBoxTransformModeRequests::IsKnifeActive);
        bool insertVertex = false;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            insertVertex, pair, &EditorWhiteBoxTransformModeRequests::IsInsertVertexActive);

        // Runs every frame; only touches the widget when the state changes.
        const AZ::u32 state = (static_cast<AZ::u32>(latch) << 5) | (edgeSelection ? 1u : 0u) |
            (bridge ? 2u : 0u) | (weld ? 4u : 0u) | (loopCut ? 8u : 0u) | (bevel ? 16u : 0u) | (knife ? 128u : 0u) |
            (fillHole ? 256u : 0u) | (deletePolygon ? 512u : 0u) | (mergePolygons ? 1024u : 0u) |
            (selectCoplanar ? 2048u : 0u) | (selectLinked ? 4096u : 0u) | (growShrink ? 8192u : 0u) |
            (detach ? 16384u : 0u) | (connectVertices ? 32768u : 0u) | (insertVertex ? 65536u : 0u) |
            (subdivide ? 131072u : 0u) | (selectSimilar ? 262144u : 0u);
        if (m_modelingClusterState == state)
        {
            return;
        }
        m_modelingClusterState = state;

        const auto enable = [this](const ViewportUi::ButtonId buttonId, const bool usable)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterDisableButton,
                m_modelingClusterId, buttonId, !usable);
        };
        // These two stay live with nothing selected: the window they open can arm a latch first and
        // take its selection from whatever the next drag lands on.
        enable(m_transformExtrudeButtonId, true);
        enable(m_transformInsetButtonId, true);
        if (knife)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_modelingClusterId, m_knifeButtonId);
        }
        else if (insertVertex)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_modelingClusterId, m_insertVertexButtonId);
        }
        else if (latch != TransformModelingLatch::None)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_modelingClusterId, latch == TransformModelingLatch::Extrude ? m_transformExtrudeButtonId : m_transformInsetButtonId);
        }
        else
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_modelingClusterId);
        }
        enable(m_edgeLoopButtonId, edgeSelection);
        enable(m_edgeRingButtonId, edgeSelection);
        enable(m_bridgeButtonId, bridge);
        enable(m_weldButtonId, weld);
        enable(m_fillHoleButtonId, fillHole);
        enable(m_deletePolygonButtonId, deletePolygon);
        enable(m_mergePolygonsButtonId, mergePolygons);
        enable(m_selectCoplanarButtonId, selectCoplanar);
        enable(m_selectLinkedButtonId, selectLinked);
        enable(m_growSelectionButtonId, growShrink);
        enable(m_shrinkSelectionButtonId, growShrink);
        enable(m_detachButtonId, detach);
        enable(m_connectVerticesButtonId, connectVertices);
        enable(m_insertVertexButtonId, loopCut);
        enable(m_uvProjectionButtonId, true); // the window waits for a polygon selection, like Extrude's
        enable(m_smoothingButtonId, true); // Auto Smooth works on the whole layer with nothing selected
        enable(m_subdivideButtonId, subdivide);
        enable(m_selectSimilarButtonId, selectSimilar);
        enable(m_loopCutButtonId, loopCut);
        enable(m_knifeButtonId, loopCut);
        enable(m_bevelButtonId, bevel);
    }

    void EditorWhiteBoxComponentMode::RemoveSubModeSelectionCluster()
    {
        RemoveModelingCluster();
        RemoveShapeSwitcher();
        RemovePaintCluster();
        RemoveSketchCluster();
        RemoveEdgeRestoreCluster();

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_modeSelectionClusterId);
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
        m_drawShapeModeButtonId = RegisterClusterButton(m_modeSelectionClusterId, ":/WhiteBox/Icons/Draw.svg");
        m_paintModeButtonId = RegisterClusterButton(m_modeSelectionClusterId, ":/WhiteBox/Icons/VertexPaint.svg");

        m_transformModeButtonId = RegisterClusterButton(m_modeSelectionClusterId, "Align_to_Object");
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_modeSelectionClusterId,
            m_transformModeButtonId, WhiteboxModeClusterManipulatorTooltip);

        // set button tooltips
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_modeSelectionClusterId,
            m_defaultModeButtonId, WhiteboxModeClusterDefaultTooltip);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_modeSelectionClusterId,
            m_edgeRestoreModeButtonId, WhiteboxModeClusterEdgeRestoreTooltip);

        m_modeSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == m_defaultModeButtonId)
                {
                    EnterDefaultMode();
                }
                else if (buttonId == m_edgeRestoreModeButtonId)
                {
                    EnterEdgeRestoreMode();
                }
                else if (buttonId == m_transformModeButtonId)
                {
                    EnterTransformMode();
                }
                else if (buttonId == m_drawShapeModeButtonId)
                {
                    EnterDrawShapeMode();
                }
                else if (buttonId == m_paintModeButtonId)
                {
                    EnterPaintMode();
                }
            });
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_modeSelectionClusterId,
            m_modeSelectionHandler);

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_modeSelectionClusterId, m_drawShapeModeButtonId,
            WhiteboxModeClusterDrawShapeTooltip);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_modeSelectionClusterId, m_paintModeButtonId, "Switch to Vertex Paint mode");

        // The toolbar forces its own icon size; resize just these two tools once Qt created their widgets.
        QTimer::singleShot(0, qApp, []()
        {
            for (auto* widget : QApplication::allWidgets())
            {
                auto* button = qobject_cast<QToolButton*>(widget);
                if (!button || !qobject_cast<QToolBar*>(button->parentWidget())) { continue; }
                if (button->toolTip() == QString::fromUtf8(WhiteboxModeClusterDrawShapeTooltip) ||
                    button->toolTip() == QStringLiteral("Switch to Vertex Paint mode"))
                {
                    button->setIconSize(QSize(50, 50));
                    button->setMinimumSize(QSize(40, 40));
                    button->updateGeometry();
                    button->parentWidget()->updateGeometry();
                }
            }
        });
    }
} // namespace WhiteBox

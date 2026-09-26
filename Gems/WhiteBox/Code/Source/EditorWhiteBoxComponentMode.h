/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include "SubComponentModes/WhiteBoxPaintSettings.h"

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utility/pair.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzToolsFramework/ViewportSnapping/ViewportDragCancelBus.h>
#include <QPointer>
#include <EditorWhiteBoxComponentModeBus.h>
#include <EditorWhiteBoxComponentModeTypes.h>
#include "Viewport/WhiteBoxModifierUtil.h" // SelectionFilter, held by value below
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    class DefaultMode;
    class EdgeRestoreMode;
    class TransformMode;
    class DrawShapeMode;
    class PaintMode;
    class WhiteBoxBevelWindow;
    class WhiteBoxWeldWindow;
    class WhiteBoxExtrudeInsetWindow;
    class WhiteBoxUvProjectionWindow;
    class WhiteBoxSmoothingWindow;
    class WhiteBoxShapeOptionsWindow;
    class WhiteBoxPaintWindow;
    class WhiteBoxMaterialUiUnlock;

    //! The type of edge selection the component mode is in (either normal selection of
    //! 'user' edges or selection of all edges ('mesh') in restoration mode).
    enum class EdgeSelectionType
    {
        Polygon,
        All
    };

    //! The Component Mode responsible for handling all interactions with the White Box Tool.
    class EditorWhiteBoxComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private EditorWhiteBoxComponentNotificationBus::Handler
        , public EditorWhiteBoxComponentModeRequestBus::Handler
        , private AzToolsFramework::ViewportSnapping::ViewportDragCancelRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorWhiteBoxComponentMode, "{F05B83A8-6F3A-43C6-A742-11BAB2D8A7C1}", EditorBaseComponentMode)

        constexpr static const char* const WhiteboxModeClusterEdgeRestoreTooltip = "Switch to Edge Restore mode";
        constexpr static const char* const WhiteboxModeClusterDefaultTooltip = "Switch to Sketch mode";
        constexpr static const char* const WhiteboxModeClusterManipulatorTooltip = "Switch to Manipulator mode";
        constexpr static const char* const WhiteboxModeClusterDrawShapeTooltip = "Switch to Draw Shape mode";

        // Modeling cluster. Each says what it needs selected, because a disabled button with no
        // explanation is the same as a broken one.
        constexpr static const char* const WhiteboxModelingClusterBridgeTooltip =
            "Bridge — connect two selected boundary edges, or two facing polygons";
        constexpr static const char* const WhiteboxModelingClusterWeldTooltip =
            "Weld — choose how to merge two or more selected vertices";
        constexpr static const char* const WhiteboxModelingClusterFillHoleTooltip =
            "Fill Hole — close the flat hole bordered by the selected open edges";
        constexpr static const char* const WhiteboxMergePolygonsTooltip =
            "Merge Polygons — hide every border the selected polygons share, making them one";
        constexpr static const char* const WhiteboxSelectLinkedTooltip =
            "Select Linked — grow the selection to every part joined to it";
        constexpr static const char* const WhiteboxSelectCoplanarTooltip =
            "Select Coplanar — grow the selection over neighbours facing the same way";
        constexpr static const char* const WhiteboxGrowSelectionTooltip =
            "Grow Selection — add every neighbour of the selection, one step out";
        constexpr static const char* const WhiteboxShrinkSelectionTooltip =
            "Shrink Selection — drop every selected element that touches something unselected";
        constexpr static const char* const WhiteboxDetachTooltip =
            "Detach to Layer — move the selected polygons into a new layer, keeping materials, colours and UVs";
        constexpr static const char* const WhiteboxConnectVerticesTooltip =
            "Connect Vertices — split polygons with straight edges between the selected vertices";
        constexpr static const char* const WhiteboxInsertVertexTooltip =
            "Insert Vertex — click polygon edges to add vertices; Ctrl snaps to the midpoint, Shift to tenths";
        constexpr static const char* const WhiteboxUvProjectionTooltip =
            "UV Projection — set how textures map onto the selected polygons";
        constexpr static const char* const WhiteboxSmoothingTooltip =
            "Smoothing Groups — shade the selected polygons smooth by group, or Auto Smooth by angle";
        constexpr static const char* const WhiteboxSubdivideTooltip =
            "Subdivide — split each selected polygon into quads meeting at its centre";
        constexpr static const char* const WhiteboxSelectSimilarTooltip =
            "Select Similar — add polygons matching by material, facing, area, sides or smoothing; edges by length; vertices by edge count";
        constexpr static const char* const WhiteboxStickySelectTooltip =
            "Sticky Select — drag across the mesh and everything the cursor crosses joins the selection";
        constexpr static const char* const WhiteboxBoxSelectTooltip =
            "Box Select — drag a rectangle to select everything inside it";
        constexpr static const char* const WhiteboxSelectVerticesTooltip =
            "Vertex Select — only vertices can be picked and the selection converts. Ctrl: touching. Click again for all three";
        constexpr static const char* const WhiteboxSelectEdgesTooltip =
            "Edge Select — only edges can be picked and the selection converts. Ctrl: touching. Click again for all three";
        constexpr static const char* const WhiteboxSelectPolygonsTooltip =
            "Face Select — only polygons can be picked and the selection converts. Ctrl: touching. Click again for all three";
        constexpr static const char* const WhiteboxModelingClusterDeletePolygonTooltip =
            "Delete Polygon — remove the selected polygons and keep their vertices";
        constexpr static const char* const WhiteboxModelingClusterLoopCutTooltip =
            "Loop Cut — hover a face or edge, wheel for count, click to cut";
        constexpr static const char* const WhiteboxModelingClusterBevelTooltip =
            "Bevel — start a live bevel on the selected edges or polygons";
        
        EditorWhiteBoxComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        EditorWhiteBoxComponentMode(EditorWhiteBoxComponentMode&&) = delete;
        EditorWhiteBoxComponentMode& operator=(EditorWhiteBoxComponentMode&&) = delete;
        ~EditorWhiteBoxComponentMode() override;

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActionContextModes();
        static void RegisterActionUpdaters();
        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        // EditorBaseComponentMode ...
        void Refresh() override;
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        AZStd::string GetComponentModeName() const override;
        //! Drops the framework's blue viewport border. White Box already names the active mode in its
        //! own cluster, so the border is redundant chrome across the top of the viewport.
        AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> PopulateViewportUiImpl() override;
        AZ::Uuid GetComponentModeType() const override;

        // EditorWhiteBoxComponentModeRequestBus ...
        void MarkWhiteBoxIntersectionDataDirty() override;
        SubMode GetCurrentSubMode() const override;
        void SetSubMode(SubMode subMode) override;
        void OverrideKeyboardModifierQuery(const KeyboardModifierQueryFn& keyboardModifierQueryFn) override;

    private:
        // AzToolsFramework::ViewportSnapping::ViewportDragCancelRequestBus ...
        //! Right click during a drag (sent by the viewport snapping drag handler).
        bool CancelActiveDrag() override;

        // Active editing overlays must also render when entity helpers are hidden.
        // AzFramework::ViewportDebugDisplayEventBus ...
        void DisplayViewport(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // TransformNotificationBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorWhiteBoxComponentNotificationBus ...
        void OnDefaultShapeTypeChanged(DefaultShapeType defaultShape) override;

        //! Rebuild the intermediate intersection data from the source white box data.
        //! @param edgeSelectionMode Determines whether to include all edges ('mesh' + 'user') or
        //! just 'user' edges when generating the intersection data.
        void RecalculateWhiteBoxIntersectionData(EdgeSelectionType edgeSelectionMode);

        //! Enter the sub-mode for default mode.
        void EnterDefaultMode();
        //! Enter the sub-mode for edge restore.
        void EnterEdgeRestoreMode();
        //! Enter the sub-mode for transforming
        void EnterTransformMode();

        //! Create the Viewport UI cluster for sub mode selection.
        void CreateSubModeSelectionCluster();
        //! Remove the Viewport UI cluster for sub mode selection.
        void RemoveSubModeSelectionCluster();

        void EnterDrawShapeMode();
        void EnterPaintMode();
        //! The current set of 'sub' modes the white box component mode can be in.
        AZStd::variant<AZStd::unique_ptr<DefaultMode>, AZStd::unique_ptr<EdgeRestoreMode>, AZStd::unique_ptr<TransformMode>, AZStd::unique_ptr<DrawShapeMode>, AZStd::unique_ptr<PaintMode>> m_modes;

        //! The most up to date intersection and render data for the white box (edge and polygon bounds).
        AZStd::optional<IntersectionAndRenderData> m_intersectionAndRenderData;
        //! The world transform of the entity this ComponentMode is on.
        AZ::Transform m_worldFromLocal;
        //! The function to use for querying modifier keys (while drawing).
        KeyboardModifierQueryFn m_keyboardModifierQueryFn;

        SubMode m_currentSubMode = SubMode::Default;
        bool m_restoreModifierHeld = false;
        //! Keeps the Material and Collider cards and the Material Instance Editor usable during this mode.
        AZStd::unique_ptr<WhiteBoxMaterialUiUnlock> m_materialUiUnlock;

        //! Lets deferred work (EnterDefaultMode) detect that this mode was destroyed in the meantime.
        AZStd::shared_ptr<bool> m_deferredWorkToken;

        AzToolsFramework::ViewportUi::ClusterId
            m_transformClusterId; 
        AzToolsFramework::ViewportUi::ButtonId
            m_transformTranslateButtonId; 
        AzToolsFramework::ViewportUi::ButtonId
            m_transformRotateButtonId; 
        AzToolsFramework::ViewportUi::ButtonId
            m_transformScaleButtonId; 

        AzToolsFramework::ViewportUi::ButtonId m_drawShapeModeButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_paintModeButtonId;
        //! Viewport UI cluster for changing sub mode.
        AzToolsFramework::ViewportUi::ClusterId
            m_modeSelectionClusterId;
        //! Id of the Viewport UI button for default mode.
        AzToolsFramework::ViewportUi::ButtonId
            m_defaultModeButtonId;
        //! Id of the Viewport UI button for edge restore mode.
        AzToolsFramework::ViewportUi::ButtonId
            m_edgeRestoreModeButtonId;
        //! Id of the Viewport UI button for transform mode.
        AzToolsFramework::ViewportUi::ButtonId
            m_transformModeButtonId;
        //! Event handler for sub mode changes.
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler
            m_modeSelectionHandler;

        //! The modeling cluster (Extrude, Inset, Bridge, Weld, Loop Cut, Bevel...), shown in Transform mode only.
        AzToolsFramework::ViewportUi::ClusterId m_modelingClusterId;
        AzToolsFramework::ViewportUi::ButtonId m_bridgeButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_transformExtrudeButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_transformInsetButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_weldButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_fillHoleButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_deletePolygonButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_mergePolygonsButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_selectCoplanarButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_selectLinkedButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_growSelectionButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_shrinkSelectionButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_detachButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_connectVerticesButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_insertVertexButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_uvProjectionButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_smoothingButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_subdivideButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_selectSimilarButtonId;
        //! Its own cluster: the modelling one's single active-button slot is taken by the latch and knife.
        AzToolsFramework::ViewportUi::ClusterId m_selectionClusterId =
            AzToolsFramework::ViewportUi::InvalidClusterId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_selectionHandler;
        AzToolsFramework::ViewportUi::ButtonId m_selectVerticesButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_selectEdgesButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_selectPolygonsButtonId;
        //! A third cluster, because a filter and a tool have to be able to show as lit at once.
        AzToolsFramework::ViewportUi::ClusterId m_toolClusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_toolHandler;
        AzToolsFramework::ViewportUi::ButtonId m_stickySelectButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_boxSelectButtonId;
        //! At most one selection tool runs at a time, so they share the filter cluster's neighbour.
        SelectionTool m_selectionTool = SelectionTool::None;
        //! Restricts what a click can hit. None lets all three through, as before.
        SelectionFilter m_selectionFilter = SelectionFilter::None;
        AzToolsFramework::ViewportUi::ButtonId m_loopCutButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_knifeButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_bevelButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_edgeLoopButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_edgeRingButtonId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_modelingHandler;
        //! Last pushed enable/latch state packed into a word, so the per-frame refresh only talks to
        //! the widget when one of those answers actually changes.
        AZStd::optional<AZ::u32> m_modelingClusterState;

        QPointer<WhiteBoxBevelWindow> m_bevelWindow;
        QPointer<WhiteBoxWeldWindow> m_weldWindow;
        QPointer<WhiteBoxExtrudeInsetWindow> m_extrudeInsetWindow;
        QPointer<WhiteBoxUvProjectionWindow> m_uvProjectionWindow;
        QPointer<WhiteBoxSmoothingWindow> m_smoothingWindow;

        //! The Draw Shape primitive switcher along the bottom of the viewport.
        AzToolsFramework::ViewportUi::ClusterId m_shapeSwitcherId;
        AZStd::vector<AZStd::pair<AzToolsFramework::ViewportUi::ButtonId, DrawShapeType>> m_shapeButtons;
        //! The cube stamp sits in the same cluster because it answers the same question - what does a
        //! click produce - but it is a mode flag rather than a DrawShapeType, so it is tracked apart.
        AzToolsFramework::ViewportUi::ButtonId m_cubeStampButtonId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_shapeSwitcherHandler;

        QPointer<WhiteBoxShapeOptionsWindow> m_shapeOptionsWindow;

        //! The Vertex Paint cluster; the paint window holds its material and colour.
        AzToolsFramework::ViewportUi::ClusterId m_paintClusterId;
        AZStd::vector<AZStd::pair<AzToolsFramework::ViewportUi::ButtonId, FacePaintOperation>> m_paintButtons;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_paintHandler;
        QPointer<WhiteBoxPaintWindow> m_paintWindow;

        //! The Sketch cluster. Extrude and Inset latch on and off; the rest are momentary verbs. Both
        //! were Ctrl-held drag gestures with nothing in the UI to reveal them, and Ctrl still works.
        AzToolsFramework::ViewportUi::ClusterId m_sketchClusterId;
        AzToolsFramework::ViewportUi::ButtonId m_extrudeButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_insetButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_hideEdgeButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_hideVertexButtonId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_sketchHandler;
        //! Last pushed enable state, so the per-frame refresh only talks to the widget when it changes.
        AZStd::optional<bool> m_sketchEdgeSelected;
        AZStd::optional<bool> m_sketchVertexSelected;

        //! Edge Restore's one extra verb. Flipping is a right-click there and nothing says so, which is
        //! the same discoverability problem Extrude had.
        AzToolsFramework::ViewportUi::ClusterId m_edgeRestoreClusterId;
        AzToolsFramework::ViewportUi::ButtonId m_flipEdgeButtonId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_edgeRestoreHandler;

        void CreateEdgeRestoreCluster();
        void RemoveEdgeRestoreCluster();

        void CreateSketchCluster();
        void RemoveSketchCluster();
        //! Highlight whichever drag modifier is latched, and grey out the verbs that need a selection.
        void RefreshSketchClusterState();

        void CreatePaintCluster();
        void RemovePaintCluster();
        //! Point the cluster's active button at the component's current paint operation.
        void RefreshPaintClusterActive();

        void CreateShapeSwitcher();
        void RemoveShapeSwitcher();
        //! Point the switcher's active entry at whatever the component currently has set, so it agrees
        //! with the pane and with a shape restored from the level.
        void RefreshShapeSwitcherActive();

        void CreateModelingCluster();
        void RefreshSelectionClusterState();
        void RefreshToolClusterState();
        void RemoveModelingCluster();
        //! Enable each modeling button for what the selection allows, and highlight the armed latch.
        void RefreshModelingClusterState();
    };

    inline SubMode EditorWhiteBoxComponentMode::GetCurrentSubMode() const
    {
        return m_currentSubMode;
    }
} // namespace WhiteBox

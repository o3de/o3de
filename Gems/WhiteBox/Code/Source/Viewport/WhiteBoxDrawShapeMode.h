/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzCore/EBus/Event.h>
#include <EditorWhiteBoxComponentModeTypes.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <SubComponentModes/WhiteBoxNumericInput.h>
#include <Viewport/WhiteBoxDrawShapeModeBus.h>

namespace WhiteBox
{
    enum class LayerCombineMode; // EditorWhiteBoxComponent.h

    //! DrawShapeMode implements Lumberyard-style click-drag-release-pull shape drawing
    //! inside the White Box component mode.
    //!
    //! Interaction flow:
    //!   1. Left mouse down  -> anchor first corner (world hit or ground plane)
    //!   2. Left mouse drag  -> size the base rectangle (live preview)
    //!   3. Left mouse up    -> lock base, enter height-pull phase
    //!   4. Mouse move       -> pull height along up-axis (live preview)
    //!   5. Left mouse down  -> commit box to white box mesh + undo batch
    //!   Right-click / Esc  -> cancel at any phase
    //!
    //! During the height-pull phase the depth can also be typed (Blender-style
    //! numeric entry, expressions allowed e.g. "-5+3"). The first digit/minus
    //! locks the mouse pull; Enter commits, Escape cancels the numeric entry.
    //! In Ctrl (boolean) mode the sign decides the operation: + adds, - carves.
    class DrawShapeMode
        : private AzFramework::ViewportDebugDisplayEventBus::Handler
        , public EditorWhiteBoxDrawShapeModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        explicit DrawShapeMode(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~DrawShapeMode();

        //! Register/assign the numeric-entry keyboard actions for the draw sub-mode.
        static void RegisterActions();
        static void BindActionsToModes(const AZStd::string& modeIdentifier);

        // EditorWhiteBoxDrawShapeModeRequestBus overrides - feed typed keys into
        // the numeric input state (only while pulling height).
        void NumericAppendDigit(char digit) override   { if (BeginNumericIfPulling()) { m_numericInput.AppendDigit(digit); SyncPreviewHeight(); } }
        void NumericAppendDecimal() override            { if (BeginNumericIfPulling()) { m_numericInput.AppendDecimal(); SyncPreviewHeight(); } }
        void NumericNegate() override                   { if (BeginNumericIfPulling()) { m_numericInput.AppendOperator('-'); SyncPreviewHeight(); } }
        void NumericAppendOperatorPlus() override       { if (m_numericInput.IsActive()) { m_numericInput.AppendOperator('+'); SyncPreviewHeight(); } }
        void NumericAppendOperatorMult() override       { if (m_numericInput.IsActive()) { m_numericInput.AppendOperator('*'); SyncPreviewHeight(); } }
        void NumericAppendOperatorDiv() override        { if (m_numericInput.IsActive()) { m_numericInput.AppendOperator('/'); SyncPreviewHeight(); } }
        void NumericBackspace() override;
        void NumericConfirm() override;
        void NumericCancel() override;

        //! Forward a raw mouse interaction event.
        //! @return true if the event was consumed (prevents other handlers from seeing it).
        bool HandleMouseInteraction(const ModeMouseInteraction& mouse);

        //! Required by EditorWhiteBoxComponentMode variant dispatch. DrawShapeMode draws its
        //! own ghost preview through DisplayViewport (ViewportDebugDisplayEventBus), so the
        //! shared per-mode Display path is intentionally a no-op here.
        void Display(
            const AZ::EntityComponentIdPair&, const AZ::Transform&, const IntersectionAndRenderData&,
            const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests&)
        {
        }

        //! Required by EditorWhiteBoxComponentMode variant dispatch.
        void Refresh() {}

        //! Escape drops numeric entry first, then the shape. True if consumed (do not leave component mode).
        bool HandleEscape();

        //! Draw mode is not manipulator driven, so right click already reaches its own
        //! HandleMouseInteraction and cancels there. Nothing extra to do for the polled path.
        bool CancelActiveDrag() { return false; }

        //! Required by EditorWhiteBoxComponentMode variant dispatch.
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(
            const AZ::EntityComponentIdPair& entityComponentIdPair);

    private:
        // Override the global Viewport Display method
        void DisplayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;
        //! Three-phase draw state.
        enum class DrawState
        {
            Idle,           //!< Waiting for first click.
            DraggingBase,   //!< User is dragging out the base rectangle.
            PullingHeight,  //!< Base is locked; user moves mouse to set height.
            DrawingPolygon, //!< Click-defined planar outline.
            PullingPolygonHeight, //!< Closed outline; previewing its extrusion.
        };

        //! Raycast the mouse against the mesh, falling back to a plane through the entity origin. World space.
        AZ::Vector3 RaycastToSurface(
            const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction,
            const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& intersectionData,
            AZ::Vector3& outWorldNormal) const;   // NEW out-param

        //! Raycast the mouse ray against a vertical plane that faces the camera
        //! and passes through baseCenter (world space).
        //! Returns the signed height delta above baseCenter.
        float RaycastToHeightPlane(
            const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction,
            const AZ::Transform& worldFromLocal,
            const AZ::Vector3& baseCenterWorld) const;

        //! Ctrl + draw: apply a CSG boolean using a cutter prism (the drawn
        //! footprint pulled to @p height along the surface normal). The sign of
        //! @p height picks the operation: pull in = subtract (carve), pull out =
        //! union (add).
        void BooleanAtPolygon(const AZ::Transform& worldFromLocal, float height, bool forceUnion = false);
        //! Shared boolean commit for primitive and freeform cutters; maps to active-layer storage space.
        bool ApplyDrawBoolean(WhiteBoxMesh& cutter, bool carve);
        
        bool m_carveMode = false;   // set on first click if Ctrl is held
        //! Stamp the current drawn AABB into the white box mesh and record an undo batch.
        void CommitBox(const AZ::Transform& worldFromLocal);
        //! Commit the pulled shape as a parametric layer, a boolean or plain geometry.
        void CommitDrawnShape(const AZ::Transform& worldFromLocal);
        //! Add the drawn shape as a parametric layer. False for a freeform polygon or a degenerate shape.
        bool CommitParametricLayer(const AZ::Transform& worldFromLocal, LayerCombineMode combineMode);

        //! Cancel draw and return to Idle, discarding any in-progress shape.
        void Cancel();

        //! Current shape and side count, read from the component's "Draw Shape" /
        //! "Draw Sides" properties (sides clamped to a safe range).
        DrawShapeType CurrentShape() const;
        int CurrentSides() const;
        float CurrentHoleRatio() const;
        int CurrentTubeSides() const;
        bool FinishPolygonOutline();
        bool CommitPolygon(float height = 0.0f);
        bool PolygonContextValid() const;
        AZ::u64 m_polygonLayerId = 0;
        AZ::Vector3 m_polygonHeightAnchor = AZ::Vector3::CreateZero();
        AZStd::vector<AZ::Vector3> m_polygonPoints;
        bool m_polygonInvalid = false;
        bool m_polygonCloseHovered = false;
        bool m_polygonBooleanFailed = false;

        //! Staircase build parameters (step count, step-division mode, step height and rotation),
        //! read from the component in one request and sanitised to safe ranges.
        DrawStairInfo CurrentStairInfo() const;

        //! Effective staircase step count for the current pull height: the fixed
        //! "Step Count" in count mode, or derived from "Step Height" otherwise.
        int EffectiveStairSteps() const;

        //! Whether the component's "Carve (Boolean)" toggle is active (a persistent
        //! alternative to holding Ctrl).
        bool CurrentCarve() const;
        //! Whether the component's "Merge Draw Shape (Union)" toggle is active.
        bool CurrentMergeUnion() const;
        bool CurrentParametric() const;

        //! Whether the component's "Unit Cube Stamp" mode is active.
        bool UnitCubeMode() const;
        //! Number of cells per stamped cube - now always 1 (a cube is one cell).
        int CurrentUnitCubeSize() const;
        //! World-space size of one cube/cell (grid spacing), read from the component.
        float CurrentCellSize() const;
        //! Whether the ghost preview should show the per-cube grid (true) or a single box.
        bool UnitCubeShowGrid() const;
        //! The snappable vertex under the cursor (nothing excluded: the drawn shape is new geometry).
        AZStd::optional<AZ::Vector3> SnapTargetUnderCursor(int viewportId) const;
        //! Round a point to the viewport grid when grid snapping is on, then return it to the drawing plane through @p planePoint.
        AZ::Vector3 SnapToGrid(const AZ::Vector3& world, const AZ::Vector3& planePoint, int viewportId) const;
        //! Round a pull height to whole grid steps when grid snapping is on.
        float SnapHeightToGrid(float height, int viewportId) const;
        //! Next freeform outline point: with angle snapping the edge from the last point turns in angle steps (and grows in
        //! grid steps when grid snapping is also on); otherwise the point snaps to the grid.
        AZ::Vector3 SnapPolygonPoint(const AZ::Vector3& onPlane, int viewportId) const;

        //! @param snapTargetWorld When set, the cell is derived from this vertex instead of from
        //! @p hitWorld, so the stamp lands on an existing corner rather than wherever the ray hit.
        bool UnitCubeCell(
            const AZ::Transform& worldFromLocal, const AZ::Vector3& hitWorld, const AZ::Vector3& hitNormal, bool carve,
            AZ::Vector3& outMinLocal,
            const AZStd::optional<AZ::Vector3>& snapTargetWorld = AZStd::nullopt) const;
        //! Local dominant axis (0=X,1=Y,2=Z) and sign (+/-1) of a world-space surface
        //! normal - used to pick the region's thickness axis/direction.
        void UnitCubeAxisFromNormal(
            const AZ::Transform& worldFromLocal, const AZ::Vector3& worldNormal, int& outAxis, int& outSign) const;
        //! Recompute the previewed region as the base cube unioned with the cursor cell; it only grows.
        void UpdateUnitCubeRegion(const AZ::Vector3& cursorCell);
        //! Stamp (union) or remove (subtract) every cell in the current region box in
        //! a single undo batch.
        void StampUnitCubeRegion();

        //! Begin a numeric depth session if we're in the height-pull phase.
        //! @return true if numeric input is now active (and the key should apply).
        bool BeginNumericIfPulling();

        //! Mirror the live numeric value into m_height so the ghost preview and
        //! the eventual commit follow the typed expression.
        void SyncPreviewHeight();

        AZ::EntityComponentIdPair m_entityComponentIdPair;

        DrawState m_state = DrawState::Idle;

        //! Blender-style numeric depth entry (active only during height pull).
        NumericInputState m_numericInput;

        //! Latest entity-to-world transform (without the active layer transform), cached
        //! so a keyboard-driven confirm (which carries no transform) can commit.
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity();

        // Add a member to store the anchor's surface frame:
        AZ::Vector3 m_surfaceNormal = AZ::Vector3::CreateAxisZ();

        //! World-space corners of the drawn base rectangle.
        AZ::Vector3 m_worldP0 = AZ::Vector3::CreateZero();
        AZ::Vector3 m_worldP1 = AZ::Vector3::CreateZero();

        //! Height above the base plane pulled during phase 3.
        float m_height = 0.f;

        //! World-space Y (up) value of the ground plane established on first click.
        float m_groundZ = 0.f;

        //! Unit-cube stamp hover/drag preview state.
        bool m_unitCubeHoverValid = false;
        bool m_unitCubeCarve = false;
        //! True between left-press and release while stretching the region footprint.
        bool m_unitCubeDragging = false;
        //! Latches true once the drag moves clearly past the press point (~1 cell), so a
        //! near-stationary click keeps a fixed preview and can't jitter across a cell edge.
        bool m_unitCubeDragMoved = false;
        //! Grow across the surface (true) or extrude along the normal (false); follows the live Ctrl state.
        bool m_unitCubeAcrossGrow = false;
        //! Ctrl state captured at press-time (also what decides carve). The drag compares
        //! the live Ctrl state against this to pick depth vs across.
        bool m_unitCubeCtrlPrev = false;
        //! Cells per cube (always 1) and the world-space cube/cell size, cached each event.
        int m_unitCubeSize = 1;
        float m_unitCubeCellSize = 1.0f;
        //! Local grid axis (0/1/2) and sign the block's thickness runs along (the clicked
        //! surface normal), captured when the drag is anchored.
        int m_unitCubeAxis = 2;
        int m_unitCubeSign = 1;
        //! Anchored corner cell (local min), set on left-press.
        AZ::Vector3 m_unitCubeAnchorMin = AZ::Vector3::CreateZero();
        //! Hysteretic cursor cell, so sub-cell jitter does not flip the preview.
        AZ::Vector3 m_unitCubeCursorCell = AZ::Vector3::CreateZero();
        //! Anchor point and normal from the press; the drag stays on this plane.
        AZ::Vector3 m_unitCubeAnchorWorld = AZ::Vector3::CreateZero();
        AZ::Vector3 m_unitCubeAnchorNormalWorld = AZ::Vector3::CreateAxisZ();
        //! Previewed region box: local min corner and per-axis cell extents.
        AZ::Vector3 m_unitCubeMinLocal = AZ::Vector3::CreateZero();
        AZ::Vector3 m_unitCubeExtent = AZ::Vector3::CreateOne();

    };
} // namespace WhiteBox

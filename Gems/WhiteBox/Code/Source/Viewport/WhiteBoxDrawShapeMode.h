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

    //! DrawShapeMode implements Lumberyard-style click-drag-release-pull shape drawing
    //! inside the White Box component mode.
    //!
    //! Interaction flow:
    //!   1. Left mouse down  → anchor first corner (world hit or ground plane)
    //!   2. Left mouse drag  → size the base rectangle (live preview)
    //!   3. Left mouse up    → lock base, enter height-pull phase
    //!   4. Mouse move       → pull height along up-axis (live preview)
    //!   5. Left mouse down  → commit box to white box mesh + undo batch
    //!   Right-click / Esc  → cancel at any phase
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
        void NumericBackspace() override                { if (m_numericInput.IsActive()) { m_numericInput.Backspace(); SyncPreviewHeight(); } }
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
        };

        //! Raycast the mouse ray against the existing white box mesh polygons,
        //! falling back to an infinite XZ plane through the entity origin.
        //! Returns a position in world space.
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
        void BooleanAtPolygon(const AZ::Transform& worldFromLocal, float height);
        
        bool m_carveMode = false;   // set on first click if Ctrl is held
        //! Stamp the current drawn AABB into the white box mesh and record an undo batch.
        void CommitBox(const AZ::Transform& worldFromLocal);

        //! Cancel draw and return to Idle, discarding any in-progress shape.
        void Cancel();

        //! Current shape and side count, read from the component's "Draw Shape" /
        //! "Draw Sides" properties (sides clamped to a safe range).
        DrawShapeType CurrentShape() const;
        int CurrentSides() const;

        //! Staircase build parameters (step count, step-division mode, step height and rotation),
        //! read from the component in one request and sanitised to safe ranges.
        DrawStairInfo CurrentStairInfo() const;

        //! Effective staircase step count for the current pull height: the fixed
        //! "Step Count" in count mode, or derived from "Step Height" otherwise.
        int EffectiveStairSteps() const;

        //! Whether the component's "Carve (Boolean)" toggle is active (a persistent
        //! alternative to holding Ctrl).
        bool CurrentCarve() const;

        //! Whether the component's "Unit Cube Stamp" mode is active.
        bool UnitCubeMode() const;
        //! Snapped local-space min corner of the unit cube targeted by a hit.
        //! @param carve subtract (true) targets the clicked cell; add (false) the empty cell beyond it.
        bool UnitCubeCell(
            const AZ::Transform& worldFromLocal, const AZ::Vector3& hitWorld, const AZ::Vector3& hitNormal, bool carve,
            AZ::Vector3& outMinLocal) const;
        //! Stamp (union) or remove (subtract) a grid-snapped 1x1x1 cube at a hit.
        void StampUnitCube(
            const AZ::Transform& worldFromLocal, const AZ::Vector3& hitWorld, const AZ::Vector3& hitNormal, bool carve);

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

        //! Latest worldFromLocal seen in HandleMouseInteraction, cached so a
        //! keyboard-driven confirm (which carries no transform) can commit.
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

        //! Unit-cube stamp hover preview (local-space min corner of the target cell).
        bool m_unitCubeHoverValid = false;
        bool m_unitCubeCarve = false;
        AZ::Vector3 m_unitCubeMinLocal = AZ::Vector3::CreateZero();

    };
} // namespace WhiteBox

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxComponentModeTypes.h"
#include "SubComponentModes/WhiteBoxNumericInput.h"
#include "Viewport/WhiteBoxManipulatorViews.h"
#include "Viewport/WhiteBoxModifierUtil.h" // GeometryIntersection, taken by value below

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzFramework/Viewport/ScreenGeometry.h> // ScreenPoint, held by value below
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/unordered_map.h>
#include <QPointer>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SubComponentModes/EditorWhiteBoxTransformModeBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AZ
{
    class Color;
    class Transform;
    class EntityComponentIdPair;
} // namespace AZ

namespace AzFramework
{
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    struct ActionOverride;
} // namespace AzToolsFramework

namespace WhiteBox
{
    struct IntersectionAndRenderData;
    class WhiteBoxToolStatus;

    class TransformMode
        : public EditorWhiteBoxTransformModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        constexpr static const char* const ManipulatorModeClusterTranslateTooltip = "Switch to translate mode";
        constexpr static const char* const ManipulatorModeClusterRotateTooltip = "Switch to rotate mode";
        constexpr static const char* const ManipulatorModeClusterScaleTooltip = "Switch to scale mode";

        using IntersectionSelection = AZStd::variant<PolygonIntersection, EdgeIntersection, VertexIntersection, AZStd::monostate>;

        TransformMode(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~TransformMode();

        static void RegisterActionUpdaters();
        static void RegisterActions();
        static void BindActionsToModes(const AZStd::string& modeIdentifier);
        static void BindActionsToMenus();

        void Refresh();
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData,
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);

        bool HandleMouseInteraction(const ModeMouseInteraction& mouse);

        // EditorWhiteBoxTransformModeRequestBus overrides ...
        void ChangeTransformType(TransformType subModeType) override;
        void SetModelingLatch(TransformModelingLatch latch) override;
        TransformModelingLatch GetModelingLatch() const override { return m_modelingLatch; }
        bool HasLatchedDrag() const override { return m_latchSource != nullptr; }
        Api::PolygonHandles GetSelectedPolygons() const override;
        Api::FaceHandles GetHoveredFaces() const override;
        Api::EdgeHandles GetSelectedEdges() const override;
        Api::VertexHandles GetSelectedVertices() const override;
        void ClearSelection() override;
        void RefreshManipulatorSpace() override { if (m_knifeActive) { Refresh(); } else { RefreshManipulator(); } }
        void SetSelectedPolygons(const Api::PolygonHandles& polygons) override;
        void SetSelectedEdges(const Api::EdgeHandles& edges) override;
        void SetSelectedVertices(const Api::VertexHandles& vertices) override;
        void BeginLoopCut() override;
        void BeginKnife() override;
        bool IsKnifeActive() const override { return m_knifeActive; }
        void BeginInsertVertex() override;
        bool IsInsertVertexActive() const override { return m_insertVertexActive; }
        bool ExpandEdgeSelection(bool ring) override;

        // Numeric input bus overrides
        void NumericBeginMove()         override { if (m_whiteBoxSelection) m_numericInput.Begin(NumericOpMode::Move);   }
        void NumericBeginRotate()       override { if (m_whiteBoxSelection) m_numericInput.Begin(NumericOpMode::Rotate); }
        void NumericBeginScale()        override { if (m_whiteBoxSelection) m_numericInput.Begin(NumericOpMode::Scale);  }
        void NumericSetAxisX()          override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::X); }
        void NumericSetAxisY()          override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::Y); }
        void NumericSetAxisZ()          override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::Z); }
        void NumericConfirm()           override { if (m_knifeActive) { ConfirmKnife(); } else { ApplyNumericTransform(); } }
        //! @note Escape is NOT routed here - see the note on DefaultMode::NumericMoveCancel.
        //! The real handler is HandleEscape, dispatched from the back-action override.
        void NumericCancel() override { m_numericInput.Reset(); }
        void NumericBackspace()         override { if (m_numericInput.IsActive()) m_numericInput.Backspace(); }
        void NumericDecimal()           override { if (m_numericInput.IsActive()) m_numericInput.AppendDecimal(); }
        void NumericNegate()             override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('-'); }
        void NumericAppendDigit(char d)  override { if (m_numericInput.IsActive()) m_numericInput.AppendDigit(d); }
        void NumericAppendOperatorPlus() override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('+'); }
        void NumericAppendOperatorMult() override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('*'); }
        void NumericAppendOperatorDiv()  override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('/'); }

        //! Abandon the manipulator drag in progress, restoring the selection to its pre-drag
        //! positions. @return True if there was a drag to cancel.
        bool CancelActiveDrag();

        //! Handle Escape. Cancels a drag in progress, otherwise clears any numeric input.
        //! @return True if the Escape was consumed (so it must not also leave component mode).
        bool HandleEscape();

    private:
        //! shared data that is used between the different transformation modes Translation/Rotation/Scale.
        struct VertexTransformSelection
        {
            AZ::Vector3 m_localPosition = AZ::Vector3::CreateZero();
            AZ::Quaternion m_localRotation = AZ::Quaternion::CreateIdentity();
            AZStd::vector<AZ::Vector3> m_vertexPositions;
            Api::VertexHandles m_vertexHandles;
            IntersectionSelection m_selection;
            Api::PolygonHandles m_polygons;
            Api::EdgeHandles m_edges;
            Api::VertexHandles m_vertices;

            //! Snap anchor (translate only), resolved on the first move of a drag and held until mouse up.
            AZStd::optional<size_t> m_snapAnchorIndex;
            bool m_snapAnchorResolved = false;
            //! The snap correction applied on the most recent move, so mouse up can fold it into
            //! m_localPosition and keep the gizmo on the geometry.
            AZ::Vector3 m_snapOffset = AZ::Vector3::CreateZero();

            //! Set when Escape abandons the drag; blocks movement until the mouse is released.
            bool m_dragCancelled = false;
        };


        void CreateTranslationManipulators();
        void CreateRotationManipulators();
        void CreateScaleManipulators();
        void UpdateTransformHandles(WhiteBoxMesh* mesh);
        void RefreshManipulator();
        void DestroyManipulators();
        void UpdateToolStatus(int viewportId);
        void HideToolStatus();
        AZStd::unordered_map<int, QPointer<WhiteBoxToolStatus>> m_toolStatus;

        //! Apply the current numeric input state to the selected geometry, then reset the state.
        void ApplyNumericTransform();

        AZ::EntityComponentIdPair m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.

        AZStd::shared_ptr<AzToolsFramework::Manipulators> m_manipulator = nullptr;
        AZStd::shared_ptr<VertexTransformSelection> m_whiteBoxSelection = nullptr;

        AZStd::optional<PolygonIntersection> m_polygonIntersection = AZStd::nullopt;
        AZStd::optional<EdgeIntersection> m_edgeIntersection = AZStd::nullopt;
        AZStd::optional<VertexIntersection> m_vertexIntersection = AZStd::nullopt;

        //! Run the finished marquee against the mesh and take what falls inside it.
        void ApplyBoxSelection(const ModeMouseInteraction& mouse);
        //! Armed on mouse down; only becomes a real marquee once the cursor actually travels.
        bool m_boxSelectPending = false;
        bool m_boxSelectActive = false;
        AzFramework::ScreenPoint m_boxSelectAnchor;
        AzFramework::ScreenPoint m_boxSelectCursor;
        //! Add whatever the cursor is over to the selection, never removing. True if it grew.
        bool AddHitToSelection(const ModeMouseInteraction& mouse, GeometryIntersection hit);
        bool BeginLatchedDrag(const ModeMouseInteraction& mouse, WhiteBoxMesh& mesh, GeometryIntersection hit);
        bool HandleLatchedDrag(const ModeMouseInteraction& mouse);
        //! Run one amount against the live mesh, always from the pristine source so the drag is not
        //! cumulative. False means the operation refused it and left the mesh at the source.
        bool ApplyLatch(float amount, const AZ::Vector3& edgeOffset, AZStd::string& error);
        //! Put the mesh back the way the drag found it.
        void RestoreLatchSource();
        //! Redraw the edited mesh and drop the cached hit data - what Sketch mode does per drag step.
        //! Deliberately no serialize and no undo step: those belong to mouse up.
        void PublishLatchMesh();
        void ClearLatchedDrag();
        TransformModelingLatch m_modelingLatch = TransformModelingLatch::None;
        //! The mesh as the drag found it. Held for the whole drag, because every step re-runs the
        //! operation from it rather than on top of the previous step's result.
        Api::WhiteBoxMeshPtr m_latchSource;
        Api::PolygonHandles m_latchPolygons;
        Api::EdgeHandles m_latchEdges;
        Api::PolygonHandles m_latchResultPolygons;
        Api::EdgeHandles m_latchResultEdges;
        //! How big the dragged selection is, so "barely moved" means the same on a 10cm face and a
        //! 100m one. Anything under a thousandth of this counts as back where the drag started.
        float m_latchExtent = 1.0f;
        //! The last amount the operation accepted, so a refused one holds instead of snapping flat.
        float m_latchAmount = 0.0f;
        AZ::Vector3 m_latchEdgeOffset = AZ::Vector3::CreateZero();
        bool m_latchApplied = false;
        AZ::u64 m_latchLayerId = 0;
        AZ::Vector2 m_latchStartScreen = AZ::Vector2::CreateZero();
        AZ::Vector3 m_latchAnchor = AZ::Vector3::CreateZero();
        AZ::Vector3 m_latchPlaneNormal = AZ::Vector3::CreateAxisZ();
        AZ::Vector2 m_latchScreenNormal = AZ::Vector2::CreateZero();
        AZ::Transform m_latchWorldFromLocal = AZ::Transform::CreateIdentity();
        float m_latchFallbackScale = 0.01f;
        AZStd::string m_latchError;
        void ConfirmKnife();
        bool HandleKnife(const ModeMouseInteraction& mouse);
        bool m_knifeActive = false;
        AZ::u64 m_knifeLayerId = 0;
        Api::WhiteBoxMeshPtr m_knifeMesh;
        Api::WhiteBoxMeshPtr m_knifeHoverMesh;
        Api::WhiteBoxMeshStream m_knifeSourceBytes;
        AZStd::optional<Api::KnifePoint> m_knifeAnchor;
        AZStd::optional<Api::KnifePoint> m_knifeHover;
        AZStd::vector<AZ::Vector3> m_knifeLines;
        AZStd::vector<AZ::Vector3> m_knifeHoverLines;
        AZStd::string m_knifeError;

        bool HandleLoopCut(const ModeMouseInteraction& mouse, WhiteBoxMesh& mesh);
        bool HandleInsertVertex(const ModeMouseInteraction& mouse, WhiteBoxMesh& mesh);
        //! Leave Insert Vertex with everything it added selected, ready for Connect.
        void FinishInsertVertex();
        bool m_insertVertexActive = false;
        Api::EdgeHandle m_insertVertexEdge; //!< Hovered border edge; invalid when the cursor is over nothing usable.
        float m_insertVertexFraction = 0.5f;
        AZ::Vector3 m_insertVertexPoint = AZ::Vector3::CreateZero();
        AZStd::array<AZ::Vector3, 2> m_insertVertexLine = { AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero() };
        Api::VertexHandles m_insertedVertices;
        AZStd::string m_insertVertexError;
        bool m_loopCutActive = false;
        bool m_loopCutSliding = false;
        float m_loopCutSlide = 0.0f;
        AZ::Vector2 m_loopCutMouseAnchor = AZ::Vector2::CreateZero();
        AZ::Vector2 m_loopCutScreenAxis = AZ::Vector2::CreateZero();
        int m_loopCutCount = 1;
        Api::EdgeHandle m_loopCutSeed;
        AZStd::vector<AZ::Vector3> m_loopCutLines;
        AZStd::string m_loopCutError;

        TransformType m_transformType = TransformType::Translation;

        //! Blender-style numeric input state (G/R/S + optional X/Y/Z + digits + Enter/Escape).
        NumericInputState m_numericInput;

        AzToolsFramework::ViewportUi::ClusterId m_transformClusterId;
        AzToolsFramework::ViewportUi::ButtonId m_transformTranslateButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_transformRotateButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_transformScaleButtonId;

        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_transformSelectionHandler;
    };

} // namespace WhiteBox

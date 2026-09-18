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

#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/variant.h>
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
        Api::PolygonHandles GetSelectedPolygons() const override;

        // Numeric input bus overrides
        void NumericBeginMove()         override { if (m_whiteBoxSelection) m_numericInput.Begin(NumericOpMode::Move);   }
        void NumericBeginRotate()       override { if (m_whiteBoxSelection) m_numericInput.Begin(NumericOpMode::Rotate); }
        void NumericBeginScale()        override { if (m_whiteBoxSelection) m_numericInput.Begin(NumericOpMode::Scale);  }
        void NumericSetAxisX()          override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::X); }
        void NumericSetAxisY()          override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::Y); }
        void NumericSetAxisZ()          override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::Z); }
        void NumericConfirm()           override { ApplyNumericTransform(); }
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

        //! Apply the current numeric input state to the selected geometry, then reset the state.
        void ApplyNumericTransform();

        AZ::EntityComponentIdPair m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.

        AZStd::shared_ptr<AzToolsFramework::Manipulators> m_manipulator = nullptr;
        AZStd::shared_ptr<VertexTransformSelection> m_whiteBoxSelection = nullptr;

        AZStd::optional<PolygonIntersection> m_polygonIntersection = AZStd::nullopt;
        AZStd::optional<EdgeIntersection> m_edgeIntersection = AZStd::nullopt;
        AZStd::optional<VertexIntersection> m_vertexIntersection = AZStd::nullopt;

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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorWhiteBoxComponentModeTypes.h"
#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "SubComponentModes/WhiteBoxNumericInput.h"

#include <EditorWhiteBoxEdgeModifierBus.h>
#include <EditorWhiteBoxPolygonModifierBus.h>
#include <QScopedPointer>

namespace AZ
{
    class EntityComponentIdPair;
}

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
    class EdgeTranslationModifier;
    class PolygonTranslationModifier;
    class PolygonScaleModifier;
    class EdgeScaleModifier;
    class VertexTranslationModifier;

    //! The default mode of the EditorWhiteBoxComponentMode - this state allows immediate
    //! interaction of polygons and edges.
    class DefaultMode
        : private EditorWhiteBoxDefaultModeRequestBus::Handler
        , private EditorWhiteBoxPolygonModifierNotificationBus::Handler
        , private EditorWhiteBoxEdgeModifierNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        //! A variant to hold either a selected polygon translation, edge translation,
        //! or a vertex selection modifier - default is empty (monostate).
        using SelectedTranslationModifier = AZStd::variant<
            AZStd::monostate, AZStd::unique_ptr<PolygonTranslationModifier>, AZStd::unique_ptr<EdgeTranslationModifier>,
            AZStd::unique_ptr<VertexTranslationModifier>>;

        explicit DefaultMode(const AZ::EntityComponentIdPair& entityComponentIdPair);
        DefaultMode(DefaultMode&&) = default;
        DefaultMode& operator=(DefaultMode&&) = default;
        ~DefaultMode();

        static void RegisterActionUpdaters();
        static void RegisterActions();
        static void BindActionsToModes(const AZStd::string& modeIdentifier);
        static void BindActionsToMenus();

        // Interface
        void Refresh();
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(
            const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData, const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZStd::optional<EdgeIntersection>& edgeIntersection,
            const AZStd::optional<PolygonIntersection>& polygonIntersection,
            const AZStd::optional<VertexIntersection>& vertexIntersection);

    private:
        // EditorWhiteBoxDefaultModeRequestBus ...
        void CreatePolygonScaleModifier(const Api::PolygonHandle& polygonHandle) override;
        void CreateEdgeScaleModifier(Api::EdgeHandle edgeHandle) override;
        void AssignSelectedPolygonTranslationModifier() override;
        void AssignSelectedEdgeTranslationModifier() override;
        void AssignSelectedVertexSelectionModifier() override;
        void RefreshPolygonScaleModifier() override;
        void RefreshEdgeScaleModifier() override;
        void RefreshPolygonTranslationModifier() override;
        void RefreshEdgeTranslationModifier() override;
        void RefreshVertexSelectionModifier() override;
        Api::VertexHandles SelectedVertexHandles() const override;
        Api::EdgeHandles SelectedEdgeHandles() const override;
        Api::PolygonHandles SelectedPolygonHandles() const override;
        Api::VertexHandle HoveredVertexHandle() const override;
        Api::EdgeHandle HoveredEdgeHandle() const override;
        Api::PolygonHandle HoveredPolygonHandle() const override;
        void HideSelectedEdge() override;
        void HideSelectedVertex() override;

        // EditorWhiteBoxPolygonModifierNotificationBus ...
        void OnPolygonModifierUpdatedPolygonHandle(
            const Api::PolygonHandle& previousPolygonHandle, const Api::PolygonHandle& nextPolygonHandle) override;

        // EditorWhiteBoxEdgeModifierNotificationBus ...
        void OnEdgeModifierUpdatedEdgeHandle(
            Api::EdgeHandle previousEdgeHandle, Api::EdgeHandle nextEdgeHandle) override;

        //! Find all potentially interactive edge handles the user can select and manipulate.
        Api::EdgeHandles FindInteractiveEdgeHandles(const WhiteBoxMesh& whiteBox);

        //! Apply the pending numeric operation (move / scale / rotate) to the hovered geometry, then reset state.
        void ApplyNumeric();

        // EditorWhiteBoxDefaultModeRequestBus numeric input overrides
        void NumericMoveBegin()    override;
        void NumericScaleBegin()   override;
        void NumericRotateBegin()  override;
        void NumericExtrudeBegin() override;
        void NumericInsetBegin()   override;
        void NumericMoveSetAxisX()  override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::X); }
        void NumericMoveSetAxisY()  override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::Y); }
        void NumericMoveSetAxisZ()  override { if (m_numericInput.IsActive()) m_numericInput.SetAxis(NumericAxisConstraint::Z); }
        void NumericMoveConfirm()   override { ApplyNumeric(); }
        void NumericMoveCancel()    override { m_numericInput.Reset(); }
        void NumericMoveBackspace() override { if (m_numericInput.IsActive()) m_numericInput.Backspace(); }
        void NumericMoveDecimal()   override { if (m_numericInput.IsActive()) m_numericInput.AppendDecimal(); }
        void NumericMoveNegate()    override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('-'); }
        void NumericMoveAppendDigit(char d) override { if (m_numericInput.IsActive()) m_numericInput.AppendDigit(d); }
        void NumericAppendOperatorPlus() override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('+'); }
        void NumericAppendOperatorMult() override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('*'); }
        void NumericAppendOperatorDiv()  override { if (m_numericInput.IsActive()) m_numericInput.AppendOperator('/'); }

        //! The entity component id of the component mode this sub-mode is associated with.
        AZ::EntityComponentIdPair m_entityComponentIdPair;

        //! Blender-style numeric move input state (M + optional X/Y/Z + digits + Enter/Escape).
        NumericInputState m_numericInput;

        //! Local-space anchor captured when numeric input begins; used for the viewport overlay label.
        AZ::Vector3 m_numericAnchorLocal{ AZ::Vector3::CreateZero() };

        AZStd::unique_ptr<PolygonTranslationModifier>
            m_polygonTranslationModifier; //!< The hovered polygon translation modifier.
        AZStd::unique_ptr<EdgeTranslationModifier>
            m_edgeTranslationModifier; //!< The currently instantiated edge translation modifier.
        AZStd::unique_ptr<PolygonScaleModifier>
            m_polygonScaleModifier; //!< The currently instantiated polygon scale modifier.
        AZStd::unique_ptr<EdgeScaleModifier> m_edgeScaleModifier; //!< The currently instantiated edge scale modifier.
        AZStd::unique_ptr<VertexTranslationModifier>
            m_vertexTranslationModifier; //!< The currently instantiated vertex selection modifier.
        SelectedTranslationModifier m_selectedModifier; //!< The type of selected translation modifier.
    };
} // namespace WhiteBox

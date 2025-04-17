/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/variant.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <EditorWhiteBoxComponentModeBus.h>
#include <EditorWhiteBoxComponentModeTypes.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    class DefaultMode;
    class EdgeRestoreMode;
    class TransformMode;

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
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private EditorWhiteBoxComponentNotificationBus::Handler
        , public EditorWhiteBoxComponentModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorWhiteBoxComponentMode, "{F05B83A8-6F3A-43C6-A742-11BAB2D8A7C1}", EditorBaseComponentMode)

        constexpr static const char* const WhiteboxModeClusterEdgeRestoreTooltip = "Switch to Edge Restore mode";
        constexpr static const char* const WhiteboxModeClusterDefaultTooltip = "Switch to Sketch mode";
        constexpr static const char* const WhiteboxModeClusterManipulatorTooltip = "Switch to Manipulator mode";

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
        AZ::Uuid GetComponentModeType() const override;

        // EditorWhiteBoxComponentModeRequestBus ...
        void MarkWhiteBoxIntersectionDataDirty() override;
        SubMode GetCurrentSubMode() const override;
        void OverrideKeyboardModifierQuery(const KeyboardModifierQueryFn& keyboardModifierQueryFn) override;

    private:
        // AzFramework::EntityDebugDisplayEventBus ...
        void DisplayEntityViewport(
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

        //! The current set of 'sub' modes the white box component mode can be in.
        AZStd::variant<AZStd::unique_ptr<DefaultMode>, AZStd::unique_ptr<EdgeRestoreMode>, AZStd::unique_ptr<TransformMode>> m_modes;

        //! The most up to date intersection and render data for the white box (edge and polygon bounds).
        AZStd::optional<IntersectionAndRenderData> m_intersectionAndRenderData;
        //! The world transform of the entity this ComponentMode is on.
        AZ::Transform m_worldFromLocal;
        //! The function to use for querying modifier keys (while drawing).
        KeyboardModifierQueryFn m_keyboardModifierQueryFn;

        SubMode m_currentSubMode = SubMode::Default;
        bool m_restoreModifierHeld = false;

        AzToolsFramework::ViewportUi::ClusterId
            m_transformClusterId; 
        AzToolsFramework::ViewportUi::ButtonId
            m_transformTranslateButtonId; 
        AzToolsFramework::ViewportUi::ButtonId
            m_transformRotateButtonId; 
        AzToolsFramework::ViewportUi::ButtonId
            m_transformScaleButtonId; 

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
    };

    inline SubMode EditorWhiteBoxComponentMode::GetCurrentSubMode() const
    {
        return m_currentSubMode;
    }
} // namespace WhiteBox

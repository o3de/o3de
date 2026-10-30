/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// moc's preprocessor mis-parses the O3DE EBus macros (it ends up believing this class lives in
// namespace AZ::WhiteBox), so hide the AZ headers from moc - it only needs the Qt base class.
#if !defined(Q_MOC_RUN)
#include "Tools/WhiteBoxEntityGizmo.h"
#include "Tools/WhiteBoxLayerGizmo.h"

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/FocusMode/FocusModeNotificationBus.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#endif

#include <QScrollArea>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;
class QTimer;

namespace AzToolsFramework { class PropertyAssetCtrl; }

namespace WhiteBox
{
    class EditorWhiteBoxComponent;

    //! The dockable White Box pane: pick the White Box to edit, enter edit mode and manage its layers.
    class WhiteBoxPaneWidget
        : public QScrollArea
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
        , private AzToolsFramework::EntityCompositionNotificationBus::Handler
        , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
        , private AzToolsFramework::FocusModeNotificationBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private EditorWhiteBoxComponentNotificationBus::Handler
    {
        Q_OBJECT

    public:
        static constexpr const char* PaneName = "White Box";

        explicit WhiteBoxPaneWidget(QWidget* parent = nullptr);
        ~WhiteBoxPaneWidget() override;

    protected:
        // Stay enabled in White Box component mode, when the editor disables every dock pane.
        void changeEvent(QEvent* event) override;

    private:
        //! Re-enable this widget and any disabled ancestors (the dock wrapper) while in
        //! component mode. Deferred so it runs after whoever is doing the disabling.
        void EnsureEnabledInComponentMode();
        // EditorEntityContextNotificationBus overrides ...
        void OnEditorEntityCreated(const AZ::EntityId& entityId) override;
        void OnEditorEntityDeleted(const AZ::EntityId& entityId) override;

        // ToolsApplicationNotificationBus overrides ...
        void AfterEntitySelectionChanged(
            const AzToolsFramework::EntityIdList& newlySelectedEntities,
            const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;
        void AfterUndoRedo() override;

        // EntityCompositionNotificationBus overrides ...
        void OnEntityComponentAdded(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        void OnEntityComponentRemoved(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;

        // ViewportEditorModeNotificationsBus overrides ... (re-enable the pane in component mode)
        void OnEditorModeActivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode) override;

        // FocusModeNotificationBus overrides ...
        // Entering or leaving prefab edit mode changes which White Box entities may be edited.
        void OnEditorFocusChanged(AZ::EntityId previousFocusEntityId, AZ::EntityId newFocusEntityId) override;

        // AZ::TransformNotificationBus overrides ... (live-update the Entity Transform section)
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorWhiteBoxComponentNotificationBus overrides ... (viewport edits change state behind the pane)
        void OnWhiteBoxMeshModified() override;
        void OnDefaultShapeTypeChanged(DefaultShapeType defaultShape) override;
        void OnLayerStructureChanged() override;

        // UI construction (one function per section, called from the constructor).
        QWidget* BuildEntitySection();
        QWidget* BuildEntityTransformSection();
        QWidget* BuildModeSection();
        QWidget* BuildLayersSection();
        QWidget* BuildViewportHandlesSection();

        //! The White Box component on the entity currently picked in the dropdown (null if none).
        EditorWhiteBoxComponent* CurrentComponent() const;
        //! Run @p fn on the current component inside a single undo batch, then refresh the pane.
        void ModifyComponent(const char* undoLabel, const AZStd::function<void(EditorWhiteBoxComponent*)>& fn);

        //! Repopulate the entity dropdown with every editable entity that has a White Box component.
        void RefreshEntityList();
        //! Reload every control from the current component's state.
        void RefreshFromComponent();
        //! RefreshEntityList + RefreshFromComponent once, next event-loop pass, so a prefab reload does
        //! not rebuild the pane per entity or switch away from the current White Box.
        void QueueRefresh();
        //! Reload the layer list / active-layer combo / per-layer meta editors.
        void RefreshLayerControls(EditorWhiteBoxComponent* component);
        //! Move layer row @p from to row @p to (drag-and-drop in the list, or Move Up/Down),
        //! keeping the moved layer selected and the edit target. No-op if either row is invalid.
        void MoveLayerRow(int from, int to);
        //! Select @p entityId in the dropdown (adds it first if the list is stale).
        void SetCurrentEntity(AZ::EntityId entityId);
        //! Load the Entity Transform spin boxes from the entity's local transform.
        void UpdateEntityTransformUi();

        // Entity / mode actions.
        void OnEntityComboChanged(int index);
        void CreateWhiteBoxEntity(bool asChildLayer);

        AZ::EntityId m_currentEntityId; //!< The entity being edited (picked in the dropdown).
        AZ::EntityId m_transformBusEntityId; //!< The entity the TransformNotificationBus handler follows.
        AZ::EntityComponentIdPair m_meshBusPair; //!< The pair the white box notification handler follows.
        bool m_updating = false; //!< Guard: true while the pane writes to its own controls.
        bool m_refreshQueued = false; //!< A QueueRefresh is pending.

        //! Viewport gizmo for the selected layer's Position/Rotation/Scale.
        AZStd::unique_ptr<WhiteBoxLayerGizmo> m_layerGizmo;
        //! Viewport gizmo for the ENTITY transform (works even during component mode).
        AZStd::unique_ptr<WhiteBoxEntityGizmo> m_entityGizmo;

        //! In-viewport Move/Rotate/Scale button cluster driving the entity gizmo, shown only
        //! while a White Box component mode is active (when the editor's own gizmo is gone).
        void CreateEntityGizmoCluster();
        void RemoveEntityGizmoCluster();
        void UpdateEntityGizmoClusterHighlight();
        AzToolsFramework::ViewportUi::ClusterId m_entityGizmoClusterId; //!< Invalid when not shown.
        AzToolsFramework::ViewportUi::ButtonId m_entityGizmoMoveButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_entityGizmoRotateButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_entityGizmoScaleButtonId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_entityGizmoClusterHandler;

        // Entity section.
        QComboBox* m_entityCombo = nullptr;
        QPushButton* m_newEntityButton = nullptr;
        QPushButton* m_convertMeshButton = nullptr;
        QPushButton* m_newChildButton = nullptr;
        QPushButton* m_editButton = nullptr;
        QPushButton* m_doneButton = nullptr;

        // Mode section.
        QPushButton* m_modeSketch = nullptr;
        QPushButton* m_modeEdgeRestore = nullptr;
        QPushButton* m_modeTransform = nullptr;
        QPushButton* m_modeDrawShape = nullptr;
        QPushButton* m_modePaint = nullptr;

        // Shape section.

        // Entity Transform section.
        QComboBox* m_entitySpaceCombo = nullptr; //!< Parent (local) or World display/edit space.
        QDoubleSpinBox* m_entityPos[3] = {};
        QDoubleSpinBox* m_entityRot[3] = {};
        QDoubleSpinBox* m_entityScale = nullptr;
        QDoubleSpinBox* m_entityNonUniformScale[3] = {}; //!< Per-axis scale (via the Non-Uniform Scale component).
        QPushButton* m_entityGizmoOff = nullptr;
        QPushButton* m_entityGizmoMove = nullptr;
        QPushButton* m_entityGizmoRotate = nullptr;
        QPushButton* m_entityGizmoScale = nullptr;

        // Layers section.
        QComboBox* m_shapeParamShape = nullptr;
        QDoubleSpinBox* m_shapeParamWidth = nullptr;
        QDoubleSpinBox* m_shapeParamDepth = nullptr;
        QDoubleSpinBox* m_shapeParamHeight = nullptr;
        QSpinBox* m_shapeParamSides = nullptr;
        QSpinBox* m_shapeParamSteps = nullptr;
        QCheckBox* m_shapeParamStepsByHeight = nullptr;
        QDoubleSpinBox* m_shapeParamStepHeight = nullptr;
        QLabel* m_shapeParamStepHeightLabel = nullptr;
        QLabel* m_shapeParamSidesLabel = nullptr;
        QLabel* m_shapeParamStepsLabel = nullptr;
        // Room-only shape parameters.
        QDoubleSpinBox* m_shapeParamWallThickness = nullptr;
        QDoubleSpinBox* m_shapeParamCavityGap = nullptr;
        QCheckBox* m_shapeParamFloor = nullptr;
        QCheckBox* m_shapeParamCeiling = nullptr;
        QCheckBox* m_shapeParamDoorFrame = nullptr;
        QDoubleSpinBox* m_shapeParamArchHeight = nullptr;
        QDoubleSpinBox* m_shapeParamInnerRadius = nullptr;
        QDoubleSpinBox* m_shapeParamSweepAngle = nullptr;
        QDoubleSpinBox* m_shapeParamHoleRatio = nullptr;
        QSpinBox* m_shapeParamTubeSides = nullptr;
        QLabel* m_shapeParamTubeSidesLabel = nullptr;
        QLabel* m_shapeParamWallThicknessLabel = nullptr;
        QLabel* m_shapeParamCavityGapLabel = nullptr;
        QPushButton* m_bakeShapeButton = nullptr;
        QGroupBox* m_shapeParamsGroup = nullptr;
        //! Restarted by every shape spin-box change; the commit (cook, bake, undo step) waits for it.
        QTimer* m_shapeParamCommitTimer = nullptr;
        //! A preview has been applied to the component but not yet committed inside an undo batch, and
        //! the layer row it was applied to (the commit uses it, in case the selection moved meanwhile).
        bool m_shapeParamsPendingCommit = false;
        int m_shapeParamsPendingRow = -1;
        QPushButton* m_gizmoOff = nullptr;
        QPushButton* m_gizmoMove = nullptr;
        QPushButton* m_gizmoRotate = nullptr;
        QPushButton* m_gizmoScale = nullptr;
        QComboBox* m_activeLayerCombo = nullptr;
        QListWidget* m_layerList = nullptr;
        QLabel* m_layerCountLabel = nullptr;
        QPushButton* m_layerTintButton = nullptr;
        QComboBox* m_layerCombineCombo = nullptr;
        QCheckBox* m_layerInvertNormals = nullptr;
        QCheckBox* m_layerEdgesOnly = nullptr;
        QCheckBox* m_layerCollision;
        QDoubleSpinBox* m_layerPos[3] = {};
        QDoubleSpinBox* m_layerRot[3] = {};
        QDoubleSpinBox* m_layerScale[3] = {};
        QCheckBox* m_layerMirror[3] = {};         //!< Mirror across the layer's X / Y / Z plane.
        QSpinBox* m_layerArrayCount = nullptr;     //!< Array copies (1 = off).
        QDoubleSpinBox* m_layerArrayOffset[3] = {}; //!< Step between array copies.
        QGroupBox* m_layerMetaGroup = nullptr;

        // Draw Shape section.

        // Unit Cube Stamp section.

        // Boolean section.
        // Global (scene-wide) boolean controls.

        // Material section.
        QCheckBox* m_edgesOnly = nullptr;

        // Mesh ops section.
    };
} // namespace WhiteBox

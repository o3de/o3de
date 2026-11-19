/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Tools/WhiteBoxUvCanvas.h"
#include "Tools/WhiteBoxUvTexture.h"

#include <QStringList>

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <QWidget>

class QAction;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;
class QLabel;

namespace WhiteBox
{
    //! Dockable UV editor (Tools > White Box UV Editor) for the polygons selected in White Box Transform mode.
    //! Edited faces become Manual, so their UVs are kept through later modelling; a whole drag is one undo step.
    class WhiteBoxUvEditorPane
        : public QWidget
        , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        static constexpr const char* PaneName = "White Box UV Editor";

        explicit WhiteBoxUvEditorPane(QWidget* parent = nullptr);
        ~WhiteBoxUvEditorPane() override;

    protected:
        //! The editor disables every dock pane when component mode begins; this one is part of White Box editing.
        void changeEvent(QEvent* event) override;

    private:
        // ViewportEditorModeNotificationsBus ...
        void OnEditorModeActivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

        // ViewportDebugDisplayEventBus ...
        //! The UV selection drawn on the mesh, so it is clear which faces, edges and corners are being edited.
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        //! Put the viewed faces' base colour texture behind the UVs, or the checker when there is none.
        void RefreshTexture(const WhiteBoxMesh& mesh);
        //! Show the viewed material's trim bands in the panel and on the canvas.
        void RefreshTrim();
        //! Store new band edges for the viewed material.
        void SetTrimEdges(const AZStd::vector<float>& edges);

        //! Undo the component-mode disable (and its grey dimming), deferred until the editor has finished applying it.
        void EnsureEnabledInComponentMode();
        //! Follow the selected White Box entity and its polygon selection; skipped mid-drag.
        void Refresh();
        void ApplyEdit(const AZStd::vector<UvChange>& changes, bool final);
        //! Run a layout operation (Unwrap or Pack) on the target faces as one undoable edit, then frame the result.
        void ApplyLayout(const AZStd::vector<UvChange>& changes, const QString& done);

        AZ::EntityComponentIdPair m_pair;
        bool m_editing = false; //!< Between the first change of a gesture and its final one.
        WhiteBoxUvCanvas* m_canvas = nullptr;
        QLabel* m_status = nullptr;
        QLabel* m_message = nullptr; //!< What the last operation did or why it could not.
        QAction* m_showTexture = nullptr;
        //! Trim sheet: bands of the viewed material's texture to fit UV islands into.
        QAction* m_trimToggle = nullptr;
        QWidget* m_trimPanel = nullptr;
        QLineEdit* m_trimEdges = nullptr;    //!< The band edges as text, "0, 0.25, 0.5, 1".
        QSpinBox* m_trimEvenCount = nullptr;
        QComboBox* m_trimBand = nullptr;
        QDoubleSpinBox* m_trimInset = nullptr;
        AZ::Data::AssetId m_viewMaterial;    //!< The material of the viewed faces, which the trim sheet belongs to.
        bool m_viewMaterialKnown = false;
        QComboBox* m_textureLayer = nullptr;       //!< Which layer's map to show, for a multilayer material.
        QAction* m_textureLayerAction = nullptr;   //!< The combo's toolbar slot, hidden when there is one map or none.
        QStringList m_textureLayerNames;           //!< What the combo lists, so it is only refilled when that changes.
        UvTextureCache m_textures;
    };
} // namespace WhiteBox

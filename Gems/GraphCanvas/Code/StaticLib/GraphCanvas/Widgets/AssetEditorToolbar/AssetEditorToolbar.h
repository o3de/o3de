/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#endif

class QToolButton;

namespace Ui
{
    class AssetEditorToolbar;
}

namespace GraphCanvas
{    
    class CreateCommentPresetMenuActionGroup;
    class CreateNodeGroupPresetMenuActionGroup;
    class EditorContextMenu;

    class AssetEditorToolbar
        : public QWidget
        , public AssetEditorNotificationBus::Handler
        , public SceneNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AssetEditorToolbar, AZ::SystemAllocator);

        AssetEditorToolbar(EditorId editorId);
        ~AssetEditorToolbar() = default;

        void AddCustomAction(QToolButton* toolButton);

        void AddCreationAction(QToolButton* toolButton);
        
        // AssetEditorNotificationBus
        void OnActiveGraphChanged(const GraphId& graphId) override;
        ////
        
        // SceneNotificationBus
        void OnSelectionChanged() override;
        ////        

        void OnViewDisabled();
        void OnViewEnabled();

    public Q_SLOTS:

        void AddComment(bool checked);
        void GroupSelection(bool checked);
        void UngroupSelection(bool checked);
    
        // Alignment Tooling
        void AlignSelectedTop(bool checked);
        void AlignSelectedBottom(bool checked);
        
        void AlignSelectedLeft(bool checked);
        void AlignSelectedRight(bool checked);        

        // Organization Tooling
        void OrganizeTopLeft(bool checked);
        void OrganizeCentered(bool checked);
        void OrganizeBottomRight(bool checked);

        // Context Menus
        void OnCommentPresetsContextMenu(const QPoint& pos);
        void OnNodeGroupPresetsContextMenu(const QPoint& pos);
        
    private:

        void AlignSelected(const AlignConfig& alignConfig);
        void OrganizeSelected(const AlignConfig& alignConfig);
        
        void UpdateButtonStates();

        void OnCommentMenuAboutToShow();
        void OnNodeGroupMenuAboutToShow();
        void OnPresetActionTriggered(QAction* action);

        EditorContextMenu*              m_commentPresetsMenu;
        CreateCommentPresetMenuActionGroup*  m_commentPresetActionGroup;

        EditorContextMenu*                  m_nodeGroupPresetsMenu;
        CreateNodeGroupPresetMenuActionGroup*    m_nodeGroupPresetActionGroup;
        
        EditorId m_editorId;
        GraphId m_activeGraphId;

        bool m_viewDisabled = false;
    
        AZStd::unique_ptr<Ui::AssetEditorToolbar> m_ui;
    };
}

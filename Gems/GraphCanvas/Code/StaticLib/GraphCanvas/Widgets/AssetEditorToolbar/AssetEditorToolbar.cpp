/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QToolButton>

#include <GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <StaticLib/GraphCanvas/Widgets/AssetEditorToolbar/ui_AssetEditorToolbar.h>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Editor/Automation/AutomationIds.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

namespace GraphCanvas
{
    ///////////////////////
    // AssetEditorToolbar
    ///////////////////////

    AssetEditorToolbar::AssetEditorToolbar(EditorId editorId)
        : m_editorId(editorId)
        , m_ui(new Ui::AssetEditorToolbar())
        , m_commentPresetActionGroup(nullptr)
        , m_nodeGroupPresetActionGroup(nullptr)
        , m_viewDisabled(false)
    {
        m_ui->setupUi(this);
        
        AssetEditorNotificationBus::Handler::BusConnect(editorId);

        QObject::connect(m_ui->addComment, &QToolButton::clicked, this, &AssetEditorToolbar::AddComment);
        QObject::connect(m_ui->groupNodes, &QToolButton::clicked, this, &AssetEditorToolbar::GroupSelection);
        QObject::connect(m_ui->ungroupNodes, &QToolButton::clicked, this, &AssetEditorToolbar::UngroupSelection);

        m_commentPresetsMenu = aznew EditorContextMenu(editorId, this);
        m_commentPresetsMenu->SetIsToolBarMenu(true);

        QObject::connect(m_commentPresetsMenu, &QMenu::aboutToShow, this, &AssetEditorToolbar::OnCommentMenuAboutToShow);
        QObject::connect(m_commentPresetsMenu, &QMenu::triggered, this, &AssetEditorToolbar::OnPresetActionTriggered);

        m_ui->addComment->setMenu(m_commentPresetsMenu);
        m_ui->addComment->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        QObject::connect(m_ui->addComment, &QWidget::customContextMenuRequested, this, &AssetEditorToolbar::OnCommentPresetsContextMenu);

        m_nodeGroupPresetsMenu = aznew EditorContextMenu(editorId, this);
        m_nodeGroupPresetsMenu->SetIsToolBarMenu(true);

        QObject::connect(m_nodeGroupPresetsMenu, &QMenu::aboutToShow, this, &AssetEditorToolbar::OnNodeGroupMenuAboutToShow);
        QObject::connect(m_nodeGroupPresetsMenu, &QMenu::triggered, this, &AssetEditorToolbar::OnPresetActionTriggered);

        m_ui->groupNodes->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
        m_ui->groupNodes->setMenu(m_nodeGroupPresetsMenu);
        m_ui->groupNodes->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        QObject::connect(m_ui->groupNodes, &QWidget::customContextMenuRequested, this, &AssetEditorToolbar::OnNodeGroupPresetsContextMenu);

        QObject::connect(m_ui->topAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedTop);
        QObject::connect(m_ui->bottomAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedBottom);

        QObject::connect(m_ui->leftAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedLeft);
        QObject::connect(m_ui->rightAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedRight);

        // Disabling these while they are a bit dodgey
        m_ui->organizationLine->setVisible(false);
        m_ui->organizeTopLeft->setVisible(false);
        m_ui->organizeCentered->setVisible(false);
        m_ui->organizeBottomRight->setVisible(false);
        
        // Disable customization panel until they are used
        m_ui->customizationPanel->setVisible(false);
        m_ui->creationPanel->setVisible(false);

        QObject::connect(m_ui->organizeTopLeft, &QToolButton::clicked, this, &AssetEditorToolbar::OrganizeTopLeft);
        QObject::connect(m_ui->organizeCentered, &QToolButton::clicked, this, &AssetEditorToolbar::OrganizeCentered);
        QObject::connect(m_ui->organizeBottomRight, &QToolButton::clicked, this, &AssetEditorToolbar::OrganizeBottomRight);

        UpdateButtonStates();

        AssetEditorAutomationRequests* editorAutomationRequests = AssetEditorAutomationRequestBus::FindFirstHandler(m_editorId);

        if (editorAutomationRequests)
        {
            editorAutomationRequests->RegisterObject(AutomationIds::CreateCommentButton, m_ui->addComment);
            editorAutomationRequests->RegisterObject(AutomationIds::GroupButton, m_ui->groupNodes);
            editorAutomationRequests->RegisterObject(AutomationIds::UngroupButton, m_ui->ungroupNodes);
        }
    }

    void AssetEditorToolbar::AddCustomAction(QToolButton* action)
    {
        m_ui->customizationPanel->setVisible(true);
        m_ui->customizationPanel->layout()->addWidget(action);
    }

    void AssetEditorToolbar::AddCreationAction(QToolButton* action)
    {
        m_ui->creationPanel->setVisible(true);
        m_ui->creationPanel->layout()->addWidget(action);
    }

    void AssetEditorToolbar::OnActiveGraphChanged(const GraphId& graphId)
    {
        m_activeGraphId = graphId;

        SceneNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusConnect(m_activeGraphId);

        UpdateButtonStates();
    }

    void AssetEditorToolbar::OnSelectionChanged()
    {
        UpdateButtonStates();
    }

    void AssetEditorToolbar::OnViewDisabled()
    {
        m_viewDisabled = true;
        UpdateButtonStates();
    }

    void AssetEditorToolbar::OnViewEnabled()
    {
        m_viewDisabled = false;
        UpdateButtonStates();
    }

    void AssetEditorToolbar::AddComment(bool)
    {
        if (m_editorId != EditorId() && m_activeGraphId.IsValid())
        {
            const ConstructTypePresetBucket* presetBucket = nullptr;
            AssetEditorSettingsRequestBus::EventResult(presetBucket, m_editorId, &AssetEditorSettingsRequests::GetConstructTypePresetBucket, ConstructType::CommentNode);

            if (presetBucket)
            {
                EditorContextMenu fakeMenu(m_editorId);
                fakeMenu.SetIsToolBarMenu(true);

                AddCommentPresetMenuAction menuAction(&fakeMenu, presetBucket->GetDefaultPreset());
                menuAction.SetTarget(m_activeGraphId, AZ::EntityId());

                OnPresetActionTriggered(&menuAction);
            }
        }
    }

    void AssetEditorToolbar::GroupSelection(bool)
    {
        if (m_editorId != EditorId() && m_activeGraphId.IsValid())
        {
            const ConstructTypePresetBucket* presetBucket = nullptr;
            AssetEditorSettingsRequestBus::EventResult(presetBucket, m_editorId, &AssetEditorSettingsRequests::GetConstructTypePresetBucket, ConstructType::NodeGroup);

            if (presetBucket)
            {
                EditorContextMenu fakeMenu(m_editorId);
                fakeMenu.SetIsToolBarMenu(true);

                AddNodeGroupPresetMenuAction menuAction(&fakeMenu, presetBucket->GetDefaultPreset());
                menuAction.SetTarget(m_activeGraphId, AZ::EntityId());

                OnPresetActionTriggered(&menuAction);
            }
        }
    }

    void AssetEditorToolbar::UngroupSelection(bool)
    {
        if (m_editorId != EditorId() && m_activeGraphId.IsValid())
        {
            AZStd::vector< AZ::EntityId >  selectedElements;

            SceneRequestBus::EventResult(selectedElements, m_activeGraphId, &SceneRequests::GetSelectedNodes);

            for (const auto& selectedElement : selectedElements)
            {
                AZStd::vector< NodeId > selectedNodes;
                SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &SceneRequests::GetSelectedNodes);
                for (const auto& nodeId : selectedNodes)
                {
                    if (GraphUtils::IsNodeGroup(nodeId))
                    {
                        GraphCanvas::GraphUtils::UngroupGroup(m_activeGraphId, selectedElement);
                    }
                }
            }
        }
    }

    void AssetEditorToolbar::AlignSelectedTop(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::None;
        config.m_verAlign = GraphUtils::VerticalAlignment::Top;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::AlignSelectedBottom(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::None;
        config.m_verAlign = GraphUtils::VerticalAlignment::Bottom;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::AlignSelectedLeft(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Left;
        config.m_verAlign = GraphUtils::VerticalAlignment::None;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::AlignSelectedRight(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Right;
        config.m_verAlign = GraphUtils::VerticalAlignment::None;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::OrganizeTopLeft(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Left;
        config.m_verAlign = GraphUtils::VerticalAlignment::Top;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        OrganizeSelected(config);
    }

    void AssetEditorToolbar::OrganizeCentered(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Center;
        config.m_verAlign = GraphUtils::VerticalAlignment::Middle;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        OrganizeSelected(config);
    }

    void AssetEditorToolbar::OrganizeBottomRight(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Right;
        config.m_verAlign = GraphUtils::VerticalAlignment::Bottom;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        OrganizeSelected(config);
    }

    void AssetEditorToolbar::OnCommentPresetsContextMenu(const QPoint& pos)
    {
        QMenu contextMenu;

        QAction* editPresetsAction = contextMenu.addAction("Edit Presets");
        
        QAction* action = contextMenu.exec(m_ui->addComment->mapToGlobal(pos));

        if (action == editPresetsAction)
        {
            AssetEditorRequestBus::Event(m_editorId, &AssetEditorRequests::ShowAssetPresetsMenu, ConstructType::CommentNode);
        }
    }

    void AssetEditorToolbar::OnNodeGroupPresetsContextMenu(const QPoint& pos)
    {
        QMenu contextMenu;

        QAction* editPresetsAction = contextMenu.addAction("Edit Presets");

        QAction* action = contextMenu.exec(m_ui->addComment->mapToGlobal(pos));

        if (action == editPresetsAction)
        {
            AssetEditorRequestBus::Event(m_editorId, &AssetEditorRequests::ShowAssetPresetsMenu, ConstructType::NodeGroup);
        }
    }

    void AssetEditorToolbar::AlignSelected(const AlignConfig& alignConfig)
    {
        AZStd::vector< NodeId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &SceneRequests::GetSelectedNodes);

        GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void AssetEditorToolbar::OrganizeSelected(const AlignConfig& alignConfig)
    {
        AZStd::vector< NodeId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &SceneRequests::GetSelectedNodes);

        GraphUtils::OrganizeNodes(selectedNodes, alignConfig);
    }

    void AssetEditorToolbar::UpdateButtonStates()
    {
        bool hasScene = m_activeGraphId.IsValid();

        bool hasSelection = false;
        SceneRequestBus::EventResult(hasSelection, m_activeGraphId, &SceneRequests::HasSelectedItems);

        m_ui->addComment->setEnabled(hasScene && !m_viewDisabled);
        m_ui->groupNodes->setEnabled(hasScene && !m_viewDisabled);

        bool isGroup = false;

        AZStd::vector< NodeId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &SceneRequests::GetSelectedNodes);
        for (const auto& nodeId : selectedNodes)
        {
            if (GraphUtils::IsNodeGroup(nodeId))
            {
                isGroup = true;
            }
        }

        m_ui->ungroupNodes->setEnabled(hasSelection && hasScene && !m_viewDisabled && isGroup);

        bool multipleNodesSelected = selectedNodes.size() > 1;

        bool enableAlignment = multipleNodesSelected && hasSelection && hasScene && !m_viewDisabled;
        m_ui->topAlign->setEnabled(enableAlignment);
        m_ui->bottomAlign->setEnabled(enableAlignment);
        m_ui->leftAlign->setEnabled(enableAlignment);
        m_ui->rightAlign->setEnabled(enableAlignment);

    }

    void AssetEditorToolbar::OnCommentMenuAboutToShow()
    {
        if (m_commentPresetActionGroup == nullptr)
        {
            m_commentPresetActionGroup = aznew CreateCommentPresetMenuActionGroup();
            m_commentPresetActionGroup->PopulateMenu(m_commentPresetsMenu);
        }

        m_commentPresetActionGroup->RefreshPresets();
    }

    void AssetEditorToolbar::OnNodeGroupMenuAboutToShow()
    {
        if (m_nodeGroupPresetActionGroup == nullptr)
        {
            m_nodeGroupPresetActionGroup = aznew CreateNodeGroupPresetMenuActionGroup();
            m_nodeGroupPresetActionGroup->PopulateMenu(m_nodeGroupPresetsMenu);
        }

        m_nodeGroupPresetActionGroup->RefreshPresets();
    }

    void AssetEditorToolbar::OnPresetActionTriggered(QAction* action)
    {
        GraphCanvas::ContextMenuAction* contextMenuAction = qobject_cast<GraphCanvas::ContextMenuAction*>(action);

        if (contextMenuAction)
        {
            ContextMenuAction::SceneReaction reaction = ContextMenuAction::SceneReaction::Unknown;

            {
                ScopedGraphUndoBlocker undoBlocker(m_activeGraphId);

                QPointF scenePosition;
                SceneRequestBus::EventResult(scenePosition, m_activeGraphId, &SceneRequests::SignalGenericAddPositionUseBegin);

                reaction = contextMenuAction->TriggerAction(m_activeGraphId, ConversionUtils::QPointToVector(scenePosition));

                SceneRequestBus::Event(m_activeGraphId, &SceneRequests::SignalGenericAddPositionUseEnd);
            }

            if (reaction == ContextMenuAction::SceneReaction::PostUndo)
            {
                GraphModelRequestBus::Event(m_activeGraphId, &GraphModelRequests::RequestUndoPoint);
            }

        }
    }
}

#include <StaticLib/GraphCanvas/Widgets/AssetEditorToolbar/moc_AssetEditorToolbar.cpp>

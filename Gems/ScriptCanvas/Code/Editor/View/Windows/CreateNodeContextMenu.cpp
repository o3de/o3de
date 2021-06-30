/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/Nodes/NodeUtils.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>

#include "ScriptCanvasContextMenus.h"

namespace ScriptCanvasEditor
{
    //////////////////////////////
    // AddSelectedEntitiesAction
    //////////////////////////////

    AddSelectedEntitiesAction::AddSelectedEntitiesAction(QObject* parent)
        : GraphCanvas::ContextMenuAction("", parent)
    {
    }

    GraphCanvas::ActionGroupId AddSelectedEntitiesAction::GetActionGroupId() const
    {
        return AZ_CRC("EntityActionGroup", 0x17e16dfe);
    }

    void AddSelectedEntitiesAction::RefreshAction(const GraphCanvas::GraphId&, const AZ::EntityId&)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        setEnabled(!selectedEntities.empty());

        if (selectedEntities.size() <= 1)
        {
            setText("Reference selected entity");
        }
        else
        {
            setText("Reference selected entities");
        }
    }

    GraphCanvas::ContextMenuAction::SceneReaction AddSelectedEntitiesAction::TriggerAction(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePos)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        AZ::EntityId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::Vector2 addPosition = scenePos;

        for (const AZ::EntityId& id : selectedEntities)
        {
            NodeIdPair nodePair = Nodes::CreateEntityNode(id, scriptCanvasGraphId);
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, addPosition);
            addPosition += AZ::Vector2(20, 20);
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
    }

    ////////////////////////////
    // EndpointSelectionAction
    ////////////////////////////

    EndpointSelectionAction::EndpointSelectionAction(const GraphCanvas::Endpoint& proposedEndpoint)
        : QAction(nullptr)
        , m_endpoint(proposedEndpoint)
    {
        AZStd::string name;
        GraphCanvas::SlotRequestBus::EventResult(name, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetName);

        AZStd::string tooltip;
        GraphCanvas::SlotRequestBus::EventResult(tooltip, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetTooltip);

        setText(name.c_str());
        setToolTip(tooltip.c_str());
    }

    const GraphCanvas::Endpoint& EndpointSelectionAction::GetEndpoint() const
    {
        return m_endpoint;
    }

    ////////////////////////////////////
    // RemoveUnusedVariablesMenuAction
    ////////////////////////////////////

    RemoveUnusedVariablesMenuAction::RemoveUnusedVariablesMenuAction(QObject* parent)
        : SceneContextMenuAction("Variables", parent)
    {
        setToolTip("Removes all of the unused variables from the active graph");
    }

    void RemoveUnusedVariablesMenuAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        setEnabled(true);
    }

    bool RemoveUnusedVariablesMenuAction::IsInSubMenu() const
    {
        return true;
    }

    AZStd::string RemoveUnusedVariablesMenuAction::GetSubMenuPath() const
    {
        return "Remove Unused";
    }

    GraphCanvas::ContextMenuAction::SceneReaction RemoveUnusedVariablesMenuAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
        return SceneReaction::PostUndo;
    }
    
    /////////////////////
    // SceneContextMenu
    /////////////////////

    SceneContextMenu::SceneContextMenu(const NodePaletteModel& paletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
    {
        QWidgetAction* actionWidget = new QWidgetAction(this);

        const bool inContextMenu = true;
        m_palette = aznew Widget::NodePaletteDockWidget(paletteModel, tr("Node Palette"), this, assetModel, inContextMenu);

        actionWidget->setDefaultWidget(m_palette);

        GraphCanvas::ContextMenuAction* menuAction = aznew AddSelectedEntitiesAction(this);
        
        AddActionGroup(menuAction->GetActionGroupId());
        AddMenuAction(menuAction);

        AddMenuAction(actionWidget);

        connect(this, &QMenu::aboutToShow, this, &SceneContextMenu::SetupDisplay);
        connect(m_palette, &Widget::NodePaletteDockWidget::OnContextMenuSelection, this, &SceneContextMenu::HandleContextMenuSelection);
    }

    void SceneContextMenu::ResetSourceSlotFilter()
    {
        m_palette->ResetSourceSlotFilter();
    }

    void SceneContextMenu::FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId)
    {
        m_palette->FilterForSourceSlot(scriptCanvasGraphId, sourceSlotId);
    }

    const Widget::NodePaletteDockWidget* SceneContextMenu::GetNodePalette() const
    {
        return m_palette;
    }

    void SceneContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        // Don't want to overly manipulate the state. So we only modify this when we know we want to turn it on.
        if (GraphVariablesTableView::HasCopyVariableData())
        {
            m_editorActionsGroup.SetPasteEnabled(true);
        }
    }

    void SceneContextMenu::HandleContextMenuSelection()
    {
        close();
    }

    void SceneContextMenu::SetupDisplay()
    {
        m_palette->ResetDisplay();
        m_palette->FocusOnSearchFilter();
    }

    void SceneContextMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        if (!m_palette->hasFocus())
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }

    //////////////////////////
    // ConnectionContextMenu
    //////////////////////////

    ConnectionContextMenu::ConnectionContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
    {
        QWidgetAction* actionWidget = new QWidgetAction(this);

        const bool inContextMenu = true;
        m_palette = aznew Widget::NodePaletteDockWidget(nodePaletteModel, tr("Node Palette"), this, assetModel, inContextMenu);

        actionWidget->setDefaultWidget(m_palette);

        AddMenuAction(actionWidget);

        connect(this, &QMenu::aboutToShow, this, &ConnectionContextMenu::SetupDisplay);
        connect(m_palette, &Widget::NodePaletteDockWidget::OnContextMenuSelection, this, &ConnectionContextMenu::HandleContextMenuSelection);
    }

    const Widget::NodePaletteDockWidget* ConnectionContextMenu::GetNodePalette() const
    {
        return m_palette;
    }

    void ConnectionContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        GraphCanvas::ConnectionContextMenu::OnRefreshActions(graphId, targetMemberId);

        m_palette->ResetSourceSlotFilter();

        m_connectionId = targetMemberId;
        
        // TODO: Filter nodes.
    }

    void ConnectionContextMenu::HandleContextMenuSelection()
    {
        close();
    }

    void ConnectionContextMenu::SetupDisplay()
    {
        m_palette->ResetDisplay();
        m_palette->FocusOnSearchFilter();
    }

    void ConnectionContextMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        if (!m_palette->hasFocus())
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }

    #include "Editor/View/Windows/moc_ScriptCanvasContextMenus.cpp"
}


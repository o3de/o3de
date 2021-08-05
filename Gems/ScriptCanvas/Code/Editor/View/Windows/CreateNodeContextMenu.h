/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector2.h>

#include <QMenu>
#include <QWidgetAction>

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/ConnectionContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuAction.h>
#endif

namespace ScriptCanvasEditor
{
    class NodePaletteModel;

    namespace Widget
    {
        class NodePaletteDockWidget;
    }

    class AddSelectedEntitiesAction
        : public GraphCanvas::ContextMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AddSelectedEntitiesAction, AZ::SystemAllocator, 0);

        AddSelectedEntitiesAction(QObject* parent);
        virtual ~AddSelectedEntitiesAction() = default;

        GraphCanvas::ActionGroupId GetActionGroupId() const override;

        void RefreshAction(const GraphCanvas::GraphId& graphCanvasGraphId, const AZ::EntityId& targetId) override;
        SceneReaction TriggerAction(const GraphCanvas::GraphId& graphCanvasGraphId, const AZ::Vector2& scenePos) override;
    };

    class EndpointSelectionAction
        : public QAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EndpointSelectionAction, AZ::SystemAllocator, 0);

        EndpointSelectionAction(const GraphCanvas::Endpoint& endpoint);
        ~EndpointSelectionAction() = default;

        const GraphCanvas::Endpoint& GetEndpoint() const;

    private:
        GraphCanvas::Endpoint m_endpoint;
    };

    class RemoveUnusedVariablesMenuAction
        : public GraphCanvas::SceneContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoveUnusedVariablesMenuAction, AZ::SystemAllocator, 0);

        RemoveUnusedVariablesMenuAction(QObject* parent);
        virtual ~RemoveUnusedVariablesMenuAction() = default;

        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;
    };

    class SceneContextMenu
        : public GraphCanvas::SceneContextMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SceneContextMenu, AZ::SystemAllocator, 0);

        SceneContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
        ~SceneContextMenu() = default;

        void ResetSourceSlotFilter();
        void FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId);
        const Widget::NodePaletteDockWidget* GetNodePalette() const;

        // EditConstructContextMenu
        void OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId) override;
        ////

    public slots:

        void HandleContextMenuSelection();
        void SetupDisplay();

    protected:
        void keyPressEvent(QKeyEvent* keyEvent) override;
        
        AZ::EntityId                      m_sourceSlotId;
        Widget::NodePaletteDockWidget*    m_palette;
    };

    class ConnectionContextMenu
        : public GraphCanvas::ConnectionContextMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConnectionContextMenu, AZ::SystemAllocator, 0);

        ConnectionContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
        ~ConnectionContextMenu() = default;

        const Widget::NodePaletteDockWidget* GetNodePalette() const;

    protected:

        void OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId);

    public slots:

        void HandleContextMenuSelection();
        void SetupDisplay();

    protected:
        void keyPressEvent(QKeyEvent* keyEvent) override;

    private:

        AZ::EntityId                      m_connectionId;
        Widget::NodePaletteDockWidget*    m_palette;
    };
}

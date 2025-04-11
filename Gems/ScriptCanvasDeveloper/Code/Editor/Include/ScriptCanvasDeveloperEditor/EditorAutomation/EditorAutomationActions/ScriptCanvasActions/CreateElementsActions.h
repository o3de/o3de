/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationModelIds.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationAction that will create the specified node from the NodePalette at the specified scene point
    */
    class CreateNodeFromPaletteAction
        : public CompoundAction
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNodeFromPaletteAction, AZ::SystemAllocator);
        AZ_RTTI(CreateNodeFromPaletteAction, "{5711220C-48D6-4853-87B9-A9866B82DFCD}", CompoundAction);

        CreateNodeFromPaletteAction(GraphCanvas::NodePaletteWidget* paletteWidget, GraphCanvas::GraphId graphId, QString nodeName, QPointF scenePoint);
        CreateNodeFromPaletteAction(GraphCanvas::NodePaletteWidget* paletteWidget, GraphCanvas::GraphId graphId, QString nodeName, GraphCanvas::ConnectionId connectionId);

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

        GraphCanvas::NodeId GetCreatedNodeId() const;

        // GraphCanvas::SceneNotificationBus::Handler
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        ////

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

    private:

        GraphCanvas::ConnectionId m_spliceTarget;
        GraphCanvas::Endpoint     m_sourceEndpoint;
        GraphCanvas::Endpoint     m_targetEndpoint;

        GraphCanvas::GraphId m_graphId;
        QPointF m_scenePoint;
        QString m_nodeName;

        GraphCanvas::NodePaletteWidget* m_paletteWidget;
        
        GraphCanvas::NodeId m_createdNodeId;

        bool m_centerOnScene = false;
        bool m_writeToSearchFilter =  false;
    };

    /**
        EditorAutomationAction that will create all nodes under the specified category from the NodePalette at the specified scene point
    */
    class CreateCategoryFromNodePaletteAction
        : public CompoundAction
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateCategoryFromNodePaletteAction, AZ::SystemAllocator);
        AZ_RTTI(CreateCategoryFromNodePaletteAction, "{FA073F04-5DE2-4124-9BAB-7507FFD046CD}", CompoundAction);

        CreateCategoryFromNodePaletteAction(GraphCanvas::NodePaletteWidget* nodePalette, GraphCanvas::GraphId graphId, QString category, QPointF scenePoint);
        ~CreateCategoryFromNodePaletteAction() override = default;

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

        // GraphCanvas::SceneNotificationBus::Handler
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        ////

        AZStd::vector< GraphCanvas::NodeId > GetCreatedNodes() const;

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

    private:

        GraphCanvas::GraphId m_graphId;
        QPointF m_scenePoint;

        QString m_categoryName;

        GraphCanvas::NodePaletteWidget* m_paletteWidget;

        int m_expectedCreations = 0;
        AZStd::vector< GraphCanvas::NodeId > m_createdNodeIds;
    };

    /**
        EditorAutomationAction that will create the specified node at the specified point using the context menu.
        Can specify a ConnectionId instead of a scene point to use the context menu to splice a node onto the given connection as well
    */
    class CreateNodeFromContextMenuAction
        : public CompoundAction
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNodeFromContextMenuAction, AZ::SystemAllocator);
        AZ_RTTI(CreateNodeFromContextMenuAction, "{0318F9D3-6433-400F-9E29-41B57A8ADB32}", CompoundAction);

        CreateNodeFromContextMenuAction(GraphCanvas::GraphId graphId, QString nodeName, QPointF scenePoint);
        CreateNodeFromContextMenuAction(GraphCanvas::GraphId graphId, QString nodeName, AZ::EntityId connectionId);

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

        // GraphCanvas::SceneNotificationBus::Handler
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        ////

        AZ::EntityId GetCreatedNodeId() const;

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

    private:

        AZ::EntityId m_spliceTarget;
        GraphCanvas::Endpoint m_sourceEndpoint;
        GraphCanvas::Endpoint m_targetEndpoint;

        GraphCanvas::GraphId m_graphId;
        QPointF m_scenePoint;
        QString m_nodeName;

        GraphCanvas::NodeId m_createdNodeId;

        bool m_centerOnScene = false;
    };

    /**
        EditorAutomationAction that will create the specified node at the specified point by dragging a connection from the specified endpoint.
        Will place the new node at a 'reasonable' distance from the specified endpoint, or at the specified point.
    */
    class CreateNodeFromProposalAction
        : public CompoundAction
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNodeFromProposalAction, AZ::SystemAllocator);
        AZ_RTTI(CreateNodeFromProposalAction, "{236A56E5-73A8-42B0-9239-77670DD4EA44}", CompoundAction);

        CreateNodeFromProposalAction(GraphCanvas::GraphId graphId, GraphCanvas::Endpoint endpoint, QString nodeName);
        CreateNodeFromProposalAction(GraphCanvas::GraphId graphId, GraphCanvas::Endpoint endpoint, QString nodeName, QPointF scenePoint);

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

        // GraphCanvas::SceneNotificationBus::Handler
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        ////

        AZ::EntityId GetCreatedNodeId() const;
        AZ::EntityId GetConnectionId() const;

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

    private:

        GraphCanvas::GraphId m_graphId;

        GraphCanvas::Endpoint m_endpoint;

        QPointF m_scenePoint;
        QString m_nodeName;

        GraphCanvas::NodeId m_createdNodeId;
    };

    /**
        EditorAutomationAction that will create a group using the specified creation type[Hotkey action or toolbar click]
    */
    class CreateGroupAction
        : public CompoundAction
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:

        enum class CreationType
        {
            Toolbar,
            Hotkey
        };

        AZ_CLASS_ALLOCATOR(CreateGroupAction, AZ::SystemAllocator);
        AZ_RTTI(CreateGroupAction, "{853F34EB-F8AB-47E3-A601-35091DC23E11}", CompoundAction);

        CreateGroupAction(GraphCanvas::EditorId editorGraph, GraphCanvas::GraphId graphId, CreationType creationType = CreationType::Hotkey);
        ~CreateGroupAction() override = default;

        void SetupAction() override;

        // GraphCanvas::SceneNotificationBus::Handler
        void OnNodeAdded(const AZ::EntityId& groupId, bool isPaste = false) override;
        ////

        AZ::EntityId GetCreatedGroupId() const;
        ActionReport GenerateReport() const override;

    protected:

        void SetupToolbarAction();
        void SetupHotkeyAction();

        void OnActionsComplete() override;

    private:

        GraphCanvas::EditorId m_editorId;
        GraphCanvas::GraphId  m_graphId;

        CreationType m_creationType = CreationType::Hotkey;

        AZ::EntityId m_createdGroup;
    };
}

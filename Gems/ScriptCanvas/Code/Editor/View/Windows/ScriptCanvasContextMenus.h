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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SlotContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeMenuActions/NodeContextMenuAction.h>
#endif

namespace ScriptCanvasEditor
{
    class NodePaletteModel;

    //////////////////
    // CustomActions
    //////////////////

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

        using GraphCanvas::SceneContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::SceneContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;
    };

    class ConvertVariableNodeToReferenceAction
        : public GraphCanvas::ContextMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConvertVariableNodeToReferenceAction, AZ::SystemAllocator, 0);

        ConvertVariableNodeToReferenceAction(QObject* parent);
        virtual ~ConvertVariableNodeToReferenceAction() = default;

        GraphCanvas::ActionGroupId GetActionGroupId() const override;

        using GraphCanvas::ContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::ContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:

        AZ::EntityId m_targetId;
    };

    class SlotManipulationMenuAction
        : public GraphCanvas::ContextMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SlotManipulationMenuAction, AZ::SystemAllocator, 0);

        SlotManipulationMenuAction(AZStd::string_view actionName, QObject* parent);

        static ScriptCanvas::Slot* GetScriptCanvasSlot(const GraphCanvas::Endpoint& endpoint) ;
    
    };

    class ConvertReferenceToVariableNodeAction
        : public SlotManipulationMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConvertReferenceToVariableNodeAction, AZ::SystemAllocator, 0);

        ConvertReferenceToVariableNodeAction(QObject* parent);
        virtual ~ConvertReferenceToVariableNodeAction() = default;

        GraphCanvas::ActionGroupId GetActionGroupId() const override;

        using SlotManipulationMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using SlotManipulationMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:

        AZ::EntityId m_targetId;
    };

    class ExposeSlotMenuAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ExposeSlotMenuAction, AZ::SystemAllocator, 0);

        ExposeSlotMenuAction(QObject* parent);
        virtual ~ExposeSlotMenuAction() = default;

        using GraphCanvas::SlotContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::SlotContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    protected:

        void CreateNodeling(const GraphCanvas::GraphId& graphId, AZ::EntityId scriptCanvasGraphId, GraphCanvas::GraphId slotId, const AZ::Vector2& scenePos, GraphCanvas::ConnectionType connectionType);
    };

    class SetDataSlotTypeMenuAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SetDataSlotTypeMenuAction, AZ::SystemAllocator, 0);

        explicit SetDataSlotTypeMenuAction(QObject* parent);
        virtual ~SetDataSlotTypeMenuAction() = default;
        static bool IsSupportedSlotType(const AZ::EntityId& slotId);

        using GraphCanvas::SlotContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::SlotContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:

        ScriptCanvas::Slot* GetSlot(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId);

    };

    //! Context Menu Action for Creating an AzEventHandler node from a data slot of a Behavior Method node
    //! which returns an AZ::Event<Params...> type
    class CreateAzEventHandlerSlotMenuAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateAzEventHandlerSlotMenuAction, AZ::SystemAllocator, 0);

        CreateAzEventHandlerSlotMenuAction(QObject* parent);

        using GraphCanvas::SlotContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::SlotContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

        static const AZ::BehaviorMethod* FindBehaviorMethodWithAzEventReturn(const GraphCanvas::GraphId& graphId, AZ::EntityId targetId);

    protected:

        const AZ::BehaviorMethod* m_methodWithAzEventReturn{};
        GraphCanvas::Endpoint m_methodNodeAzEventEndpoint;

    };

    /////////////////
    // ContextMenus
    /////////////////

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

        // EditConstructContextMenu
        void OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId) override;
        ////

        void SetupDisplayForProposal();

    protected:

        AZ::EntityId                      m_sourceSlotId;
    };

    class ConnectionContextMenu
        : public GraphCanvas::ConnectionContextMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConnectionContextMenu, AZ::SystemAllocator, 0);

        ConnectionContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
        ~ConnectionContextMenu() = default;

    protected:

        void OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId);

    private:

        AZ::EntityId                      m_connectionId;
    };

    //////////////////////////
    // RenameFunctionDefinitionNode
    //////////////////////////

    class NodeDescriptorComponent;

    //! Context menu to rename a node
    class RenameFunctionDefinitionNodeAction
        : public GraphCanvas::NodeContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RenameFunctionDefinitionNodeAction, AZ::SystemAllocator, 0);

        RenameFunctionDefinitionNodeAction(NodeDescriptorComponent* descriptor, QObject* parent);
        virtual ~RenameFunctionDefinitionNodeAction() = default;

        using GraphCanvas::NodeContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::NodeContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

        NodeDescriptorComponent* m_descriptor;
    };

}

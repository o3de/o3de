/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuAction.h>
#endif

namespace ScriptCanvasEditor
{
    class NodePaletteModel;

    //////////////////
    // CustomActions
    //////////////////

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

    class ConvertVariableNodeToReferenceAction
        : public GraphCanvas::ContextMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConvertVariableNodeToReferenceAction, AZ::SystemAllocator, 0);

        ConvertVariableNodeToReferenceAction(QObject* parent);
        virtual ~ConvertVariableNodeToReferenceAction() = default;

        GraphCanvas::ActionGroupId GetActionGroupId() const override;

        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;
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

    protected:
        ScriptCanvas::Slot* GetScriptCanvasSlot(const GraphCanvas::Endpoint& endpoint) const;
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

        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;
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

        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    protected:

        void CreateNodeling(const GraphCanvas::GraphId& graphId, AZ::EntityId scriptCanvasGraphId, GraphCanvas::GraphId slotId, const AZ::Vector2& scenePos, GraphCanvas::ConnectionType connectionType);
    };

    //! Context Menu Action for Creating an AzEventHandler node from a data slot of a Behavior Method node
    //! which returns an AZ::Event<Params...> type
    class CreateAzEventHandlerSlotMenuAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateAzEventHandlerSlotMenuAction, AZ::SystemAllocator, 0);

        CreateAzEventHandlerSlotMenuAction(QObject* parent);

        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;
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

        AddSelectedEntitiesAction* m_addSelectedEntitiesAction;
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
}

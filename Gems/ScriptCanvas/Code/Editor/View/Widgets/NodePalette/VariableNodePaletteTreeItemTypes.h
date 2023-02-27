/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include "CreateNodeMimeEvent.h"

#include "Editor/Undo/ScriptCanvasGraphCommand.h"

#include <ScriptCanvas/Variable/VariableBus.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>

namespace ScriptCanvasEditor
{
    // <GetVariableNode>
    class CreateGetVariableNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateGetVariableNodeMimeEvent, "{A9784FF3-E749-4EB4-B5DB-DF510F7CD151}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateGetVariableNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateGetVariableNodeMimeEvent() = default;
        explicit CreateGetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId);
        ~CreateGetVariableNodeMimeEvent() = default;

        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

    private:
        ScriptCanvas::VariableId m_variableId;
    };

    class GetVariableNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_RTTI(GetVariableNodePaletteTreeItem, "{0589E084-2E57-4650-96BF-E42DA17D7731}", GraphCanvas::DraggableNodePaletteTreeItem)
        AZ_CLASS_ALLOCATOR(GetVariableNodePaletteTreeItem, AZ::SystemAllocator);
        static const QString& GetDefaultIcon();

        GetVariableNodePaletteTreeItem();
        GetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        ~GetVariableNodePaletteTreeItem();

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

        // VariableNotificationBus::Handler
        void OnVariableRenamed(AZStd::string_view variableName) override;
        ////

        const ScriptCanvas::VariableId& GetVariableId() const;

    private:
        ScriptCanvas::VariableId m_variableId;
    };
    // </GetVariableNode>

    // <SetVariableNode>
    class CreateSetVariableNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateSetVariableNodeMimeEvent, "{D855EE9C-74E0-4760-AA0F-239ADF7507B6}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateSetVariableNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateSetVariableNodeMimeEvent() = default;
        explicit CreateSetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId);
        ~CreateSetVariableNodeMimeEvent() = default;

        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

    private:
        ScriptCanvas::VariableId m_variableId;
    };

    class SetVariableNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_RTTI(SetVariableNodePaletteTreeItem, "{BCFD5653-6621-4BAC-BD8E-71EC6190062F}", GraphCanvas::DraggableNodePaletteTreeItem)
        AZ_CLASS_ALLOCATOR(SetVariableNodePaletteTreeItem, AZ::SystemAllocator);
        static const QString& GetDefaultIcon();

        SetVariableNodePaletteTreeItem();
        SetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        ~SetVariableNodePaletteTreeItem();

        // VariableNotificationBus::Handler
        void OnVariableRenamed(AZStd::string_view variableName) override;
        ////

        const ScriptCanvas::VariableId& GetVariableId() const;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
    private:
        ScriptCanvas::VariableId m_variableId;
    };
    // </SetVariableNode>

    // <VariableChanged>
    class CreateVariableChangedNodeMimeEvent
        : public CreateEBusHandlerEventMimeEvent
    {
    public:
        AZ_RTTI(CreateVariableChangedNodeMimeEvent, "{C117AC91-FBB5-410D-BA7F-B4C15140EA6F}", CreateEBusHandlerEventMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateVariableChangedNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateVariableChangedNodeMimeEvent() = default;
        explicit CreateVariableChangedNodeMimeEvent(const ScriptCanvas::VariableId& variableId);
        ~CreateVariableChangedNodeMimeEvent() = default;

        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId) override;
        NodeIdPair ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) override;

    private:
        void ConfigureEBusEvent();

        ScriptCanvas::VariableId m_variableId;
    };

    class VariableChangedNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_RTTI(VariableChangedNodePaletteTreeItem, "{209D877C-9D15-4B4F-ADF0-2D1A127A4A0D}", GraphCanvas::DraggableNodePaletteTreeItem);
        AZ_CLASS_ALLOCATOR(VariableChangedNodePaletteTreeItem, AZ::SystemAllocator);
        static const QString& GetDefaultIcon();

        VariableChangedNodePaletteTreeItem();
        VariableChangedNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        ~VariableChangedNodePaletteTreeItem();

        // VariableNotificationBus::Handler
        void OnVariableRenamed(AZStd::string_view variableName) override;
        ////

        const ScriptCanvas::VariableId& GetVariableId() const;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
    private:
        ScriptCanvas::VariableId m_variableId;
    };
    // </VariableChanged>

    // <CreateVariableSpecificNodeMimeEvent>
    class CreateVariableSpecificNodeMimeEvent
        : public MultiCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateVariableSpecificNodeMimeEvent, "{924C1192-C32A-4A35-B146-2739AB4383DB}", MultiCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateVariableSpecificNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateVariableSpecificNodeMimeEvent() = default;
        explicit CreateVariableSpecificNodeMimeEvent(const ScriptCanvas::VariableId& variableId);
        ~CreateVariableSpecificNodeMimeEvent() = default;

        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId) override;
        ScriptCanvasEditor::NodeIdPair ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) override;

        AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > CreateMimeEvents() const override;

    private:

        ScriptCanvas::VariableId m_variableId;
    };
    // </CreateVariableSpecificNodeMimeEvent>

    // <VariableCategoryNodePaletteTreeItem>
    class VariableCategoryNodePaletteTreeItem
        : public GraphCanvas::NodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(VariableCategoryNodePaletteTreeItem, AZ::SystemAllocator);
        VariableCategoryNodePaletteTreeItem(AZStd::string_view displayName);
        ~VariableCategoryNodePaletteTreeItem() = default;

    private:

        void PreOnChildAdded(GraphCanvasTreeItem* item) override;
    };
    // </VariableNodePaeltteTreeItem>

    // <LocalVariablesListNodePaletteTreeItem>
    class LocalVariablesListNodePaletteTreeItem
        : public GraphCanvas::NodePaletteTreeItem
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
        , public GraphItemCommandNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalVariablesListNodePaletteTreeItem, AZ::SystemAllocator);
        LocalVariablesListNodePaletteTreeItem(AZStd::string_view displayName);
        ~LocalVariablesListNodePaletteTreeItem() = default;

        // GraphCanvas::AssetEditorNotificationBus
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        ////

        // GraphItemCommandNotificationBus
        void PostRestore(const UndoData& undoData) override;
        ////

        // GraphVariableManagerNotificationBus
        void OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        void OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        ////

    protected:

        void OnChildAdded(GraphCanvas::GraphCanvasTreeItem* treeItem) override;

    private:

        void RefreshVariableList();

        ScriptCanvas::ScriptCanvasId m_scriptCanvasId;

        bool m_ignoreTreeSignals = false;
        AZStd::unordered_set<GraphCanvas::GraphCanvasTreeItem*> m_nonVariableTreeItems;
    };
    // </LocalVariablesNodePaletteTreeItem>

    // <LocalVariableNodePaletteTreeItem>
    class LocalVariableNodePaletteTreeItem
        : public GraphCanvas::NodePaletteTreeItem
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalVariableNodePaletteTreeItem, AZ::SystemAllocator);
        LocalVariableNodePaletteTreeItem(ScriptCanvas::VariableId variableTreeItem, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        ~LocalVariableNodePaletteTreeItem();

        void PopulateChildren();
        const ScriptCanvas::VariableId& GetVariableId() const;

        // VariableNotificationBus
        void OnVariableRenamed(AZStd::string_view) override;
        ////

    private:
        ScriptCanvas::ScriptCanvasId    m_scriptCanvasId;
        ScriptCanvas::VariableId        m_variableId;
    };

    // </VariableNodePaletteTreeItem>
}

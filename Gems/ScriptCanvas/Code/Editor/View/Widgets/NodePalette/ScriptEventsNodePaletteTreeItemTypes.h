/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QIcon>

#include <AzCore/Asset/AssetCommon.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>

#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptEvents/ScriptEventsAsset.h>

#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    // <ScriptEventsHandlerMimeEvent>
    // Handles the EBus wrapper 
    class CreateScriptEventsHandlerMimeEvent
        : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(CreateScriptEventsHandlerMimeEvent, "{4734F4B6-5915-4AEF-92A3-25FE3DBB6700}", GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateScriptEventsHandlerMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateScriptEventsHandlerMimeEvent() = default;
        CreateScriptEventsHandlerMimeEvent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition);
        ~CreateScriptEventsHandlerMimeEvent() = default;

        bool ExecuteEvent(const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasGraphId) override final;
        const ScriptCanvasEditor::NodeIdPair& GetCreatedPair() const { return m_nodeIdPair; }

    protected:

        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const;

    private:
        
        AZ::Data::AssetId m_assetId;
        ScriptEvents::Method m_methodDefinition;

        NodeIdPair m_nodeIdPair;
    };

    class ScriptEventsPaletteTreeItem
        : public GraphCanvas::NodePaletteTreeItem
        , public AZ::Data::AssetBus::Handler
    {
    public:
        AZ_RTTI(ScriptEventsPaletteTreeItem, "{50839A0D-5FD4-4964-BEA2-CB9A74A50477}", GraphCanvas::NodePaletteTreeItem);
        AZ_CLASS_ALLOCATOR(ScriptEventsPaletteTreeItem, AZ::SystemAllocator, 0);

        ScriptEventsPaletteTreeItem(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset);
        ~ScriptEventsPaletteTreeItem() override;

        const ScriptEvents::ScriptEvent& GetBusDefinition() const;
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> GetAsset() const { return m_asset; }

        // AZ::Data::AssetBus::Handler
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        QVariant OnData(const QModelIndex& index, int role) const override;

    protected:

        void OnHoverStateChanged() override;

        void OnClicked(int row) override;
        bool OnDoubleClicked(int row) override;

    private:

        void PopulateEvents(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>);
        
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;

        QIcon m_editIcon;
    };
    // </ScriptEventsHandlerMimeEvent>

    //<CreateScriptEventsReceiverEventMimeEvent>
    // This one is for handling the events
    class CreateScriptEventsReceiverMimeEvent
        : public SpecializedCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateScriptEventsReceiverMimeEvent, "{F957AF1F-55D9-4D85-AC92-EBFABCDF9D96}", SpecializedCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateScriptEventsReceiverMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateScriptEventsReceiverMimeEvent() = default;
        CreateScriptEventsReceiverMimeEvent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition);
        ~CreateScriptEventsReceiverMimeEvent() = default;

        NodeIdPair ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) override;
        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId) override;

        NodeIdPair CreateEventNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) const;

    private:

        AZ::Data::AssetId m_assetId;
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;

        ScriptEvents::Method m_methodDefinition;
    };

    class ScriptEventsHandlerEventPaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_RTTI(ScriptEventsHandlerEventPaletteTreeItem, "{0E96CD24-C5DB-491C-9A3E-6EE82F73ADBA}", GraphCanvas::DraggableNodePaletteTreeItem);
        AZ_CLASS_ALLOCATOR(ScriptEventsHandlerEventPaletteTreeItem, AZ::SystemAllocator, 0);

        ScriptEventsHandlerEventPaletteTreeItem(const AZ::Data::AssetId assetId, const ScriptEvents::Method& m_methodDefinition);
        ~ScriptEventsHandlerEventPaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const;

    private:

        AZ::Data::AssetId m_assetId;

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;

        ScriptEvents::ScriptEvent m_definition;
        ScriptEvents::Method m_methodDefinition;
    };
    //</CreateScriptEventsReceiverEventMimeEvent>

    //<CreateScriptEventsSenderEventMimeEvent>
    // This one is for sending the events
    class CreateScriptEventsSenderMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateScriptEventsSenderMimeEvent, "{9D9146EB-5FA9-4C07-BFC7-399F4F3964E4}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateScriptEventsSenderMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateScriptEventsSenderMimeEvent() = default;
        CreateScriptEventsSenderMimeEvent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition);
        ~CreateScriptEventsSenderMimeEvent() = default;

        const AZStd::string_view GetEventName() { return m_methodDefinition.GetName().c_str(); }

        const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> GetAsset(AZ::Data::AssetLoadBehavior loadBehavior);

        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

    private:

        AZ::Data::AssetId m_assetId;

        ScriptEvents::Method m_methodDefinition;
    };

    class ScriptEventsSenderPaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_RTTI(ScriptEventsSenderPaletteTreeItem, "{0E27EB7A-9F52-4A4E-9D63-28FFAD82310B}", GraphCanvas::DraggableNodePaletteTreeItem);
        AZ_CLASS_ALLOCATOR(ScriptEventsSenderPaletteTreeItem, AZ::SystemAllocator, 0);

        ScriptEventsSenderPaletteTreeItem(const AZ::Data::AssetId assetId, const ScriptEvents::Method& eventDefinition);
        ~ScriptEventsSenderPaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const;

    private:

        AZ::Data::AssetId m_assetId;
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;

        ScriptEvents::Method m_methodDefinition;
    };
    //</CreateScriptEventsSenderEventMimeEvent>

    // <CreateSendOrReceiveScriptEventsEventMimeEvent>
    class CreateSendOrReceiveScriptEventsMimeEvent
        : public MultiCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateSendOrReceiveScriptEventsMimeEvent, "{355FC877-358E-41AF-A78C-16A7DCE0550D}", MultiCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateSendOrReceiveScriptEventsMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateSendOrReceiveScriptEventsMimeEvent() = default;
        CreateSendOrReceiveScriptEventsMimeEvent(const AZ::Data::AssetId asset, const ScriptEvents::Method& methodDefinition, const ScriptCanvas::EBusEventId& eventId);
        ~CreateSendOrReceiveScriptEventsMimeEvent() = default;

        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId) override;
        ScriptCanvasEditor::NodeIdPair ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) override;

        AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > CreateMimeEvents() const override;

    private:

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;
        const AZ::Data::AssetId m_assetId;

        ScriptEvents::Method m_methodDefinition;
        ScriptCanvas::EBusEventId m_eventId;
    };

    class ScriptEventsEventNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_RTTI(ScriptEventsEventNodePaletteTreeItem, "{C6528466-C1FF-43BE-B292-21D8F8AA7C24}", GraphCanvas::DraggableNodePaletteTreeItem);
        AZ_CLASS_ALLOCATOR(ScriptEventsEventNodePaletteTreeItem, AZ::SystemAllocator, 0);
        ScriptEventsEventNodePaletteTreeItem(const AZ::Data::AssetId& m_assetId, const ScriptEvents::Method& methodDefinition, const ScriptCanvas::EBusEventId& eventId);
        ~ScriptEventsEventNodePaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
        QVariant OnData(const QModelIndex& index, int role) const override;

        ScriptCanvas::EBusBusId GetBusIdentifier() const;
        ScriptCanvas::EBusEventId GetEventIdentifier() const;

    protected:

        void OnHoverStateChanged() override;

        void OnClicked(int row) override;
        bool OnDoubleClicked(int row) override;

    private:

        QIcon m_editIcon;

        AZ::Data::AssetId m_assetId;
        ScriptCanvas::EBusEventId m_eventId;

        mutable AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;

        ScriptEvents::Method m_methodDefinition;
    };
    //
}

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
#include <ScriptCanvas/Bus/RequestBus.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    class CreateFunctionMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateFunctionMimeEvent, "{BCB4226C-4863-4646-838C-45ABD662C9BB}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateFunctionMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateFunctionMimeEvent() = default;
        CreateFunctionMimeEvent(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const ScriptCanvas::Grammar::FunctionSourceId& sourceId);
        ~CreateFunctionMimeEvent() = default;

        bool CanGraphHandleEvent(const GraphCanvas::GraphId& graphId) const override;
        ScriptCanvasEditor::NodeIdPair CreateNode(const AZ::EntityId& graphId) const override;

    private:
        ScriptCanvas::Grammar::FunctionSourceId m_sourceId;
        AZ::Data::AssetId m_assetId;
        AZ::Data::AssetType m_assetType;
    };

    class FunctionPaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_RTTI(FunctionPaletteTreeItem, "{AF75BBAD-BC8A-46D2-81B6-54C0E6CB3E41}", GraphCanvas::DraggableNodePaletteTreeItem);
        AZ_CLASS_ALLOCATOR(FunctionPaletteTreeItem, AZ::SystemAllocator, 0);

        FunctionPaletteTreeItem(const char* name, const ScriptCanvas::Grammar::FunctionSourceId& sourceId, AZ::Data::Asset<AZ::Data::AssetData> asset);
        ~FunctionPaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
        QVariant OnData(const QModelIndex& index, int role) const override;

        ScriptCanvas::Grammar::FunctionSourceId GetFunctionSourceId() const;
        AZ::Data::AssetId GetSourceAssetId() const;
        AZ::Data::AssetId GetAssetId() const;
        AZ::Data::AssetType GetAssetType() const;

    protected:
        void OnHoverStateChanged() override;
        void OnClicked(int row) override;
        bool OnDoubleClicked(int row) override;

    private:
        QIcon m_editIcon;
        ScriptCanvas::Grammar::FunctionSourceId m_sourceId;
        AZ::Data::Asset<AZ::Data::AssetData> m_asset;
    };

}

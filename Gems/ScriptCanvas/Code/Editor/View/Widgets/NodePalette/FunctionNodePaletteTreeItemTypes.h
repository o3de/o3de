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

#include <QIcon>

#include <AzCore/Asset/AssetCommon.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>

#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvas/Asset/Functions/ScriptCanvasFunctionAsset.h>

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
        CreateFunctionMimeEvent(const AZ::Data::AssetId& assetId);
        ~CreateFunctionMimeEvent() = default;

        bool CanGraphHandleEvent(const GraphCanvas::GraphId& graphId) const override;
        ScriptCanvasEditor::NodeIdPair CreateNode(const AZ::EntityId& graphId) const override;

    private:

        AZ::Data::AssetId m_assetId;
    };

    class FunctionPaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_RTTI(FunctionPaletteTreeItem, "{AF75BBAD-BC8A-46D2-81B6-54C0E6CB3E41}", GraphCanvas::DraggableNodePaletteTreeItem);
        AZ_CLASS_ALLOCATOR(FunctionPaletteTreeItem, AZ::SystemAllocator, 0);

        FunctionPaletteTreeItem(const char* name, const AZ::Data::AssetId& sourceAssetId, const AZ::Data::AssetId& runtimeAssetId);
        ~FunctionPaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const;
        QVariant OnData(const QModelIndex& index, int role) const;

        const AZ::Data::AssetId& GetSourceAssetId() const;

    protected:

        void OnHoverStateChanged() override;

        void OnClicked(int row) override;
        bool OnDoubleClicked(int row) override;

    private:

        QIcon m_editIcon;

        AZ::Data::AssetId m_sourceAssetId;
        AZ::Data::AssetId m_runtimeAssetId;
    };

}

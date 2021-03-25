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
#include "precompiled.h"

#include <qmenu.h>
#include <QPixmap>

#include <AzToolsFramework/AssetEditor/AssetEditorUtils.h>

#include <Editor/Nodes/NodeCreateUtils.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/NodePalette/FunctionNodePaletteTreeItemTypes.h>

namespace ScriptCanvasEditor
{

    //////////////////////////////////////////
    // CreateFunctionMimeEvent
    //////////////////////////////////////////

    void CreateFunctionMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<CreateFunctionMimeEvent, CreateNodeMimeEvent>()
                ->Version(4)
                ->Field("AssetId", &CreateFunctionMimeEvent::m_assetId)
                ;
        }

    }

    CreateFunctionMimeEvent::CreateFunctionMimeEvent(const AZ::Data::AssetId& assetId)
        : m_assetId(assetId)
    {
    }

    bool CreateFunctionMimeEvent::CanGraphHandleEvent(const GraphCanvas::GraphId& graphId) const
    {
        AZ::EntityId scGraphId;
        GeneralRequestBus::BroadcastResult(scGraphId, &GeneralRequests::GetScriptCanvasId, graphId);

        bool isFunctionGraph = false;
        EditorGraphRequestBus::EventResult(isFunctionGraph, scGraphId, &EditorGraphRequests::IsFunctionGraph);

        return !isFunctionGraph;
    }

    ScriptCanvasEditor::NodeIdPair CreateFunctionMimeEvent::CreateNode(const AZ::EntityId& scriptCanvasGraphId) const
    {
        return Nodes::CreateFunctionNode(scriptCanvasGraphId, m_assetId);
    }

    /////////////////////////////////////
    // FunctionPaletteTreeItem
    /////////////////////////////////////

    FunctionPaletteTreeItem::FunctionPaletteTreeItem(const char* name, const AZ::Data::AssetId& sourceAssetId, const AZ::Data::AssetId& runtimeAssetId)
        : GraphCanvas::DraggableNodePaletteTreeItem(name, ScriptCanvasEditor::AssetEditorId)
        , m_editIcon(":/ScriptCanvasEditorResources/Resources/edit_icon.png")
        , m_sourceAssetId(sourceAssetId)
        , m_runtimeAssetId(runtimeAssetId)
    {
        //TODO
        //SetToolTip(m_methodDefinition.GetTooltip().c_str());
        SetTitlePalette("FunctionNodeTitlePalette");
    }

    void FunctionPaletteTreeItem::SetRuntimeAssetId(const AZ::Data::AssetId& assetId)
    {
        m_runtimeAssetId = assetId;
    }

    GraphCanvas::GraphCanvasMimeEvent* FunctionPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateFunctionMimeEvent(m_runtimeAssetId);
    }

    QVariant FunctionPaletteTreeItem::OnData(const QModelIndex& index, int role) const
    {
        if (index.column() == NodePaletteTreeItem::Column::Customization)
        {
            if (IsHovered())
            {
                if (role == Qt::DecorationRole)
                {
                    return m_editIcon;
                }
                else if (role == Qt::ToolTipRole)
                {
                    return QString("Opens the Script Event Editor to edit the Script Function - %1.").arg(GetName().toStdString().c_str());
                }
            }
        }

        return GraphCanvas::DraggableNodePaletteTreeItem::OnData(index, role);
    }

    const AZ::Data::AssetId& FunctionPaletteTreeItem::GetSourceAssetId() const
    {
        return m_sourceAssetId;
    }

    const AZ::Data::AssetId& FunctionPaletteTreeItem::GetRuntimeAssetId() const
    {
        return m_runtimeAssetId;
    }

    void FunctionPaletteTreeItem::OnHoverStateChanged()
    {
        SignalDataChanged();
    }

    void FunctionPaletteTreeItem::OnClicked(int row)
    {
        if (row == NodePaletteTreeItem::Column::Customization)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::OpenScriptCanvasAsset, m_sourceAssetId, -1);
        }
    }

    bool FunctionPaletteTreeItem::OnDoubleClicked(int row)
    {
        if (row != NodePaletteTreeItem::Column::Customization)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::OpenScriptCanvasAsset, m_sourceAssetId, -1);
            return true;
        }

        return false;
    }
}

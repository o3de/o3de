/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qmenu.h>
#include <QPixmap>

#include <AzToolsFramework/AssetEditor/AssetEditorUtils.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/View/Widgets/NodePalette/FunctionNodePaletteTreeItemTypes.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

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
                ->Version(5)
                ->Field("AssetId", &CreateFunctionMimeEvent::m_assetId)
                ->Field("sourceId", &CreateFunctionMimeEvent::m_sourceId)
                ;
        }
    }

    CreateFunctionMimeEvent::CreateFunctionMimeEvent(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const ScriptCanvas::Grammar::FunctionSourceId& sourceId)
        : m_assetId(assetId)
        , m_assetType(assetType)
        , m_sourceId(sourceId)
    {
    }

    bool CreateFunctionMimeEvent::CanGraphHandleEvent([[maybe_unused]] const GraphCanvas::GraphId& graphId) const
    {
        return true;
    }

    ScriptCanvasEditor::NodeIdPair CreateFunctionMimeEvent::CreateNode(const AZ::EntityId& scriptCanvasGraphId) const
    {
        return Nodes::CreateFunctionNode(scriptCanvasGraphId, m_assetId, m_sourceId);
    }

    /////////////////////////////////////
    // FunctionPaletteTreeItem
    /////////////////////////////////////

    FunctionPaletteTreeItem::FunctionPaletteTreeItem(const char* name, const ScriptCanvas::Grammar::FunctionSourceId& sourceId, AZ::Data::Asset<AZ::Data::AssetData> asset)
        : GraphCanvas::DraggableNodePaletteTreeItem(name, ScriptCanvasEditor::AssetEditorId)
        , m_editIcon(":/ScriptCanvasEditorResources/Resources/edit_icon.png")
        , m_sourceId(sourceId)
        , m_asset(asset)
    {
        // TODO
        //SetToolTip(m_methodDefinition.GetTooltip().c_str());
        SetTitlePalette("FunctionNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* FunctionPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateFunctionMimeEvent(m_asset->GetId(), m_asset->GetType(), m_sourceId);
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

    ScriptCanvas::Grammar::FunctionSourceId FunctionPaletteTreeItem::GetFunctionSourceId() const
    {
        return m_sourceId;
    }

    AZ::Data::AssetId FunctionPaletteTreeItem::GetSourceAssetId() const
    {
        return AZ::Data::AssetId(GetAssetId().m_guid, 0);
    }

    AZ::Data::AssetId FunctionPaletteTreeItem::GetAssetId() const
    {
        return m_asset->GetId();
    }

    AZ::Data::AssetType FunctionPaletteTreeItem::GetAssetType() const
    {
        return m_asset->GetType();
    }

    void FunctionPaletteTreeItem::OnHoverStateChanged()
    {
        SignalDataChanged();
    }

    void FunctionPaletteTreeItem::OnClicked(int row)
    {
        if (row == NodePaletteTreeItem::Column::Customization)
        {
            GeneralRequestBus::Broadcast
                ( &GeneralRequests::OpenScriptCanvasAssetId
                , SourceHandle(nullptr, GetSourceAssetId().m_guid, "")
                , Tracker::ScriptCanvasFileState::UNMODIFIED);
        }
    }

    bool FunctionPaletteTreeItem::OnDoubleClicked(int row)
    {
        if (row != NodePaletteTreeItem::Column::Customization)
        {
            GeneralRequestBus::Broadcast
                ( &GeneralRequests::OpenScriptCanvasAssetId
                , SourceHandle(nullptr, GetSourceAssetId().m_guid, "")
                , Tracker::ScriptCanvasFileState::UNMODIFIED);
            return true;
        }

        return false;
    }
}

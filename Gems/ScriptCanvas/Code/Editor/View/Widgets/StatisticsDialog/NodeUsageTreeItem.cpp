/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/StatisticsDialog/NodeUsageTreeItem.h>


#include <ScriptCanvas/Components/EditorGraph.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////
    // NodePaletteNodeUsageRootItem
    /////////////////////////////////

    NodePaletteNodeUsageRootItem::NodePaletteNodeUsageRootItem(const NodePaletteModel& nodePaletteModel)
        : GraphCanvas::NodePaletteTreeItem("root", ScriptCanvasEditor::AssetEditorId)
        , m_categorizer(nodePaletteModel)
    {
    }

    NodePaletteNodeUsageRootItem::~NodePaletteNodeUsageRootItem()
    {
    }

    GraphCanvas::NodePaletteTreeItem* NodePaletteNodeUsageRootItem::GetCategoryNode(const char* categoryPath, GraphCanvas::NodePaletteTreeItem* parentRoot)
    {
        if (parentRoot)
        {
            return static_cast<GraphCanvas::NodePaletteTreeItem*>(m_categorizer.GetCategoryNode(categoryPath, parentRoot));
        }
        else
        {
            return static_cast<GraphCanvas::NodePaletteTreeItem*>(m_categorizer.GetCategoryNode(categoryPath, this));
        }
    }

    void NodePaletteNodeUsageRootItem::PruneEmptyNodes()
    {
        m_categorizer.PruneEmptyNodes();
    }

    ////////////////////////////////////
    // NodePaletteNodeUsagePaletteItem
    ////////////////////////////////////

    NodePaletteNodeUsagePaletteItem::NodePaletteNodeUsagePaletteItem(const ScriptCanvas::NodeTypeIdentifier& nodeIdentifier, AZStd::string_view displayName)
        : GraphCanvas::IconDecoratedNodePaletteTreeItem(displayName, ScriptCanvasEditor::AssetEditorId)
        , m_nodeIdentifier(nodeIdentifier)
    {
    }

    NodePaletteNodeUsagePaletteItem::~NodePaletteNodeUsagePaletteItem()
    {
    }

    const ScriptCanvas::NodeTypeIdentifier& NodePaletteNodeUsagePaletteItem::GetNodeTypeIdentifier() const
    {
        return m_nodeIdentifier;
    }

    //////////////////////////////
    // ScriptCanvasAssetTreeItem
    //////////////////////////////

    ScriptCanvasAssetNodeUsageTreeItem::ScriptCanvasAssetNodeUsageTreeItem(AZStd::string_view assetName)
        : m_name(QString::fromUtf8(assetName.data(), static_cast<int>(assetName.size())))
        , m_icon(":/ScriptCanvasEditorResources/Resources/edit_icon.png")
        , m_activeIdentifier(0)
    {
    }

    int ScriptCanvasAssetNodeUsageTreeItem::GetColumnCount() const
    {
        return Column::Count;
    }

    QVariant ScriptCanvasAssetNodeUsageTreeItem::Data(const QModelIndex& index, int role) const
    {
        if (index.column() == Column::Name)
        {
            switch (role)
            {
            case Qt::DisplayRole:
                return GetName();
            case Qt::DecorationRole:
                break;
            default:
                break;
            }
        }
        else if (index.column() == Column::UsageCount && m_assetId.IsValid())
        {
            switch (role)
            {
            case Qt::DisplayRole:
                return GetNodeCount();
            default:
                break;
            }
        }
        else if (index.column() == Column::OpenIcon)
        {
            if (m_assetId.IsValid())
            {
                switch (role)
                {
                case Qt::DecorationRole:
                    return m_icon;
                default:
                    break;
                }
            }
        }

        return QVariant();
    }

    Qt::ItemFlags ScriptCanvasAssetNodeUsageTreeItem::Flags(const QModelIndex&) const
    {
        Qt::ItemFlags baseFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        return baseFlags;
    }

    void ScriptCanvasAssetNodeUsageTreeItem::SetAssetId(const AZ::Data::AssetId& assetId, AZ::Data::AssetType assetType)
    {
        if (m_assetId != assetId)
        {
            if (AZ::Data::AssetBus::Handler::BusIsConnected())
            {
                AZ::Data::AssetBus::Handler::BusDisconnect();
            }

            m_assetId = assetId;

            AZ::Data::AssetBus::Handler::BusConnect(assetId);
        }

        m_assetType = assetType;
    }

    const AZ::Data::AssetId& ScriptCanvasAssetNodeUsageTreeItem::GetAssetId() const
    {
        return m_assetId;
    }

    const QString& ScriptCanvasAssetNodeUsageTreeItem::GetName() const
    {
        return m_name;
    }

    void ScriptCanvasAssetNodeUsageTreeItem::SetActiveNodeType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier)
    {
        if (m_activeIdentifier != nodeTypeIdentifier)
        {
            m_activeIdentifier = nodeTypeIdentifier;

            SignalDataChanged();
        }
    }

    int ScriptCanvasAssetNodeUsageTreeItem::GetNodeCount() const
    {
        auto nodeIter = m_statisticsHelper.m_nodeIdentifierCount.find(m_activeIdentifier);

        if (nodeIter == m_statisticsHelper.m_nodeIdentifierCount.end())
        {
            return 0;
        }

        return nodeIter->second;
    }

    /*
    void ScriptCanvasAssetNodeUsageTreeItem::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        ProcessAsset(asset);
    }

    void ScriptCanvasAssetNodeUsageTreeItem::OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool isSuccessful)
    {
        if (isSuccessful)
        {
            ProcessAsset(asset);
        }
    }

    void ScriptCanvasAssetNodeUsageTreeItem::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        ProcessAsset(asset);
    }

    // #sc_editor_asset_redux fix graph use statistics
    void ScriptCanvasAssetNodeUsageTreeItem::ProcessAsset(const AZ::Data::Asset<LegacyScriptCanvasBaseAsset>& scriptCanvasAsset)
    {
        if (scriptCanvasAsset.IsReady())
        {
            AZ::Entity* scriptCanvasEntity = scriptCanvasAsset.Get()->GetScriptCanvasEntity();

            Graph* editorGraph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(scriptCanvasEntity);

            if (editorGraph)
            {
                m_statisticsHelper = editorGraph->GetNodeUsageStatistics();

                // Temporary measure to deal potentially unfilled out data.
                if (m_statisticsHelper.m_nodeIdentifierCount.empty())
                {
                    m_statisticsHelper.PopulateStatisticData(editorGraph);
                }

                SignalDataChanged();
            }
        }
    }
    */

    ///////////////////////////////////////////
    // ScriptCanvasAssetNodeUsageTreeItemRoot
    ///////////////////////////////////////////

    ScriptCanvasAssetNodeUsageTreeItemRoot::ScriptCanvasAssetNodeUsageTreeItemRoot()
        : ScriptCanvasAssetNodeUsageTreeItem("root")
        , m_categorizer((*this))
    {
    }

    void ScriptCanvasAssetNodeUsageTreeItemRoot::RegisterAsset(const AZ::Data::AssetId& assetId, AZ::Data::AssetType assetType)
    {
        auto asset = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();
        if (!asset.IsReady())
        {
            // The asset must be loaded before it an be registered. We will connect to the AssetBus and wait for it to be ready.
            AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
            return;
        }

        auto treeItem = GetAssetItem(assetId);

        if (treeItem == nullptr)
        {
            AZ::Data::AssetInfo assetInfo;
            const AZStd::string platformName = ""; // Empty for default
            AZStd::string rootFilePath;
            bool foundPath = false;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundPath, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById, assetId, assetType, platformName, assetInfo, rootFilePath);

            if (foundPath)
            {
                AZStd::string relativePath = assetInfo.m_relativePath;
                AZStd::string fileName;
                    
                if (AzFramework::StringFunc::Path::GetFileName(relativePath.c_str(), fileName))
                {
                    AzFramework::StringFunc::Path::Normalize(relativePath);

                    const bool stripLastComponent = true;
                    if (AzFramework::StringFunc::Path::StripComponent(relativePath, stripLastComponent))
                    {
                        AzFramework::StringFunc::Replace(relativePath, AZ_CORRECT_FILESYSTEM_SEPARATOR, '/');
                        GraphCanvas::GraphCanvasTreeItem* treeItem2 = m_categorizer.GetCategoryNode(relativePath.c_str(), this);

                        if (treeItem2)
                        {
                            ScriptCanvasAssetNodeUsageTreeItem* usageTreeItem = treeItem2->CreateChildNode<ScriptCanvasAssetNodeUsageTreeItem>(fileName);

                            usageTreeItem->SetAssetId(assetId, assetType);

                            m_scriptCanvasAssetItems[assetId] = usageTreeItem;
                        }
                    }
                }
            }
        }
        else
        {
            treeItem->SetAssetId(assetId, assetType);
        }
    }

    void ScriptCanvasAssetNodeUsageTreeItemRoot::RemoveAsset(const AZ::Data::AssetId& assetId)
    {
        auto assetIter = m_scriptCanvasAssetItems.find(assetId);

        if (assetIter != m_scriptCanvasAssetItems.end())
        {
            assetIter->second->DetachItem();
            delete assetIter->second;

            m_categorizer.PruneEmptyNodes();
        }
    }

    ScriptCanvasAssetNodeUsageTreeItem* ScriptCanvasAssetNodeUsageTreeItemRoot::GetAssetItem(const AZ::Data::AssetId& assetId)
    {
        auto assetIter = m_scriptCanvasAssetItems.find(assetId);

        if (assetIter != m_scriptCanvasAssetItems.end())
        {
            return assetIter->second;
        }

        return nullptr;
    }

    GraphCanvas::GraphCanvasTreeItem* ScriptCanvasAssetNodeUsageTreeItemRoot::CreateCategoryNode(AZStd::string_view, AZStd::string_view categoryName, GraphCanvasTreeItem* parent) const
    {
        return parent->CreateChildNode<ScriptCanvasAssetNodeUsageTreeItem>(categoryName);
    }

    const ScriptCanvasAssetNodeUsageTreeItemRoot::ScriptCanvasAssetMap& ScriptCanvasAssetNodeUsageTreeItemRoot::GetAssetTreeItems() const
    {
        return m_scriptCanvasAssetItems;
    }

    void ScriptCanvasAssetNodeUsageTreeItemRoot::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(*AZ::Data::AssetBus::GetCurrentBusId());
        RegisterAsset(asset.GetId(), asset.GetType());
    }
}

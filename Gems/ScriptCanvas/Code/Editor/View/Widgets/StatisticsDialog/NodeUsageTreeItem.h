/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QIcon>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeCategorizer.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>

#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>

#include <ScriptCanvas/Components/EditorUtils.h>

namespace ScriptCanvasEditor
{
    // NodePaletteItems
    class NodePaletteNodeUsageRootItem
        : public GraphCanvas::NodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteNodeUsageRootItem, AZ::SystemAllocator);
        AZ_RTTI(NodePaletteNodeUsageRootItem, "{ED21874C-6955-40F0-B451-F5FF5A16CF71}", GraphCanvas::NodePaletteTreeItem);

        NodePaletteNodeUsageRootItem(const NodePaletteModel& nodePaletteModel);
        ~NodePaletteNodeUsageRootItem();

        GraphCanvas::NodePaletteTreeItem* GetCategoryNode(const char* categoryPath, GraphCanvas::NodePaletteTreeItem* parentRoot = nullptr);
        void PruneEmptyNodes();

    private:

        GraphCanvas::GraphCanvasTreeCategorizer m_categorizer;
    };

    class NodePaletteNodeUsagePaletteItem
        : public GraphCanvas::IconDecoratedNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteNodeUsagePaletteItem, AZ::SystemAllocator);
        AZ_RTTI(NodePaletteNodeUsagePaletteItem, "{CA8E31A8-56CA-49A2-80F2-68A1E3A9EDF6}", GraphCanvas::IconDecoratedNodePaletteTreeItem);

        NodePaletteNodeUsagePaletteItem(const ScriptCanvas::NodeTypeIdentifier& nodeIdentifier, AZStd::string_view displayName);
        ~NodePaletteNodeUsagePaletteItem();

        const ScriptCanvas::NodeTypeIdentifier& GetNodeTypeIdentifier() const;

    private:

        ScriptCanvas::NodeTypeIdentifier m_nodeIdentifier;
    };
    ////

    // General TreeItems
    class ScriptCanvasAssetNodeUsageTreeItem
        : public GraphCanvas::GraphCanvasTreeItem
        , public AZ::Data::AssetBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetNodeUsageTreeItem, AZ::SystemAllocator);
        AZ_RTTI(ScriptCanvasAssetNodeUsageTreeItem, "{1FF437D9-5159-49CD-8D80-8AC3334886E8}", GraphCanvas::GraphCanvasTreeItem);

        enum Column
        {
            IndexForce = -1,

            Name,
            UsageCount,
            OpenIcon,

            Count
        };

        ScriptCanvasAssetNodeUsageTreeItem(AZStd::string_view assetName);
        ~ScriptCanvasAssetNodeUsageTreeItem() = default;

        int GetColumnCount() const override final;

        QVariant Data(const QModelIndex& index, int role) const override final;
        Qt::ItemFlags Flags(const QModelIndex& index) const override final;

        void SetAssetId(const AZ::Data::AssetId& assetId, AZ::Data::AssetType assetType);
        const AZ::Data::AssetId& GetAssetId() const;

        const QString& GetName() const;

        void SetActiveNodeType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier);
        int GetNodeCount() const;

    private:

        // AZ::Data::AssetBus::Handler
        /*
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool isSuccessful) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        */
        ////
        // void ProcessAsset(const AZ::Data::Asset<LegacyScriptCanvasBaseAsset>& scriptCanvasAsset);

        QString m_name;
        QIcon   m_icon;

        ScriptCanvas::NodeTypeIdentifier m_activeIdentifier;
        AZ::Data::AssetId m_assetId;
        AZ::Data::AssetType m_assetType;

        GraphStatisticsHelper m_statisticsHelper;
    };

    class ScriptCanvasAssetNodeUsageTreeItemRoot
        : public ScriptCanvasAssetNodeUsageTreeItem
        , public GraphCanvas::CategorizerInterface
        , public AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetNodeUsageTreeItemRoot, AZ::SystemAllocator);
        AZ_RTTI(ScriptCanvasAssetNodeUsageTreeItemRoot, "{EDCBFE97-0BF9-4AE5-8C6E-C4805E08CBFC}", ScriptCanvasAssetNodeUsageTreeItem);

        typedef AZStd::unordered_map< AZ::Data::AssetId, ScriptCanvasAssetNodeUsageTreeItem* > ScriptCanvasAssetMap;

        ScriptCanvasAssetNodeUsageTreeItemRoot();
        ~ScriptCanvasAssetNodeUsageTreeItemRoot() = default;

        void RegisterAsset(const AZ::Data::AssetId& assetId, AZ::Data::AssetType assetType);
        void RemoveAsset(const AZ::Data::AssetId& assetId);

        ScriptCanvasAssetNodeUsageTreeItem* GetAssetItem(const AZ::Data::AssetId& assetId);

        // CategorizerInterface
        GraphCanvas::GraphCanvasTreeItem* CreateCategoryNode(AZStd::string_view categoryPath, AZStd::string_view categoryName, GraphCanvasTreeItem* parent) const override;
        ////

        const ScriptCanvasAssetMap& GetAssetTreeItems() const;

    private:

        // AZ::Data::AssetBus::MultiHandler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        GraphCanvas::GraphCanvasTreeCategorizer m_categorizer;

        ScriptCanvasAssetMap m_scriptCanvasAssetItems;
    };    
    ////
}

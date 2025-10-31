/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeCategorizer.h>

#include <Editor/View/Widgets/LoggingPanel/PivotTree/PivotTreeWidget.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>
#endif

namespace ScriptCanvasEditor
{
    class GraphPivotTreeEntityItem
        : public PivotTreeEntityItem
    {
        friend class EntityPivotTreeEntityItem;
    public:
        AZ_CLASS_ALLOCATOR(GraphPivotTreeEntityItem, AZ::SystemAllocator);
        AZ_RTTI(GraphPivotTreeEntityItem, "{17B2C45B-D63B-458E-9A2F-ED0A8218A77B}", PivotTreeEntityItem);

        GraphPivotTreeEntityItem(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);

        Qt::CheckState GetCheckState() const override final;
        void SetCheckState(Qt::CheckState checkState) override final;

        const ScriptCanvas::GraphIdentifier& GetGraphIdentifier() const;

    private:

        Qt::CheckState m_checkState;

        ScriptCanvas::GraphIdentifier m_graphIdentifier;
    };

    class GraphPivotTreeGraphItem
        : public PivotTreeGraphItem
        , public LoggingDataNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphPivotTreeGraphItem, AZ::SystemAllocator);
        AZ_RTTI(GraphPivotTreeGraphItem, "{9B449D02-109E-4D9E-BA99-35C52106432C}", PivotTreeGraphItem);

        GraphPivotTreeGraphItem(const AZ::Data::AssetId& assetId);

        void OnDataSwitch();

        void RegisterEntity(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);
        void UnregisterEntity(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);

        void OnChildDataChanged(GraphCanvasTreeItem* treeItem) override;

        Qt::CheckState GetCheckState() const override final;
        void SetCheckState(Qt::CheckState debugging) override final;

        GraphPivotTreeEntityItem* FindDynamicallySpawnedTreeItem() const;
        GraphPivotTreeEntityItem* FindEntityTreeItem(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) const;

        // LoggingDataNotificationBus
        void OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        ////

    protected:

        void OnLoggingDataIdSet() override;

    private:

        void SetupDynamicallySpawnedElementItem(bool isChecked);

        void CalculateCheckState();

        Qt::CheckState m_checkState;
        AZStd::unordered_multimap< AZ::EntityId, GraphPivotTreeEntityItem*> m_pivotItems;
    };

    class GraphPivotTreeFolder
        : public PivotTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphPivotTreeFolder, AZ::SystemAllocator);
        AZ_RTTI(GraphPivotTreeFolder, "{E67BBC27-6E0D-4D56-A7D4-9389FE30E909}", PivotTreeItem);

        GraphPivotTreeFolder(AZStd::string_view folder);
        ~GraphPivotTreeFolder() override = default;

        Qt::CheckState GetCheckState() const override final;

    protected:

        void SetCheckState(Qt::CheckState debugging) override final;
        AZStd::string GetDisplayName() const override final;

        void OnChildDataChanged(GraphCanvasTreeItem* treeItem) override;

    private:

        void CalculateCheckState();

        AZStd::string m_folderName;
        Qt::CheckState m_checkState;
    };

    class GraphPivotTreeRoot
        : public PivotTreeRoot
        , public LoggingDataNotificationBus::Handler
        , public GraphCanvas::CategorizerInterface
        , public QObject
    {
    public:
        friend class GraphPivotTreeWidget;

        AZ_CLASS_ALLOCATOR(GraphPivotTreeRoot, AZ::SystemAllocator);
        AZ_RTTI(GraphPivotTreeRoot, "{B4CD0FCF-F8C7-44D5-BF4D-12A52BB088CB}", PivotTreeRoot);
        
        GraphPivotTreeRoot();
        ~GraphPivotTreeRoot() override = default;

        // PivotTreeRoot
        void OnDataSourceChanged(const LoggingDataId& aggregateDataSource) override;
        ////

        // LoggedDataNotifications
        void OnDataCaptureBegin() override;
        void OnDataCaptureEnd() override;

        void OnEntityGraphRegistered(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        void OnEntityGraphUnregistered(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& assetId) override;

        void OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        ////

        // Category Interface
        GraphCanvas::GraphCanvasTreeItem* CreateCategoryNode(AZStd::string_view categoryPath, AZStd::string_view categoryName, GraphCanvasTreeItem* parent) const override;
        ////

    protected:
        
        // Slots to hook up into the asset model
        void OnScriptCanvasGraphAssetAdded(const QModelIndex& parentIndex, int first, int last);
        void OnScriptCanvasGraphAssetRemoved(const QModelIndex& parentIndex, int first, int last);
        ////

    private:

        void ProcessEntry(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
        void TraverseTree(QModelIndex index = QModelIndex());

        LoggingDataId m_dataSource;
        AZStd::unordered_map< AZ::Data::AssetId, GraphPivotTreeGraphItem* > m_graphTreeItemMapping;

        GraphCanvas::GraphCanvasTreeCategorizer m_categorizer;

        LoggingDataId m_loggedDataId;

        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_assetModel;
    };

    class GraphPivotTreeWidget
        : public PivotTreeWidget
    {
        Q_OBJECT
    public:
        GraphPivotTreeWidget(QWidget* parent);
    };
}

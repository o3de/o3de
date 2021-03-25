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
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeCategorizer.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Core/Core.h>
#endif

class QToolButton;
namespace ScriptCanvasEditor { class FunctionPaletteTreeItem; }

namespace Ui
{
    class ScriptCanvasNodePaletteToolbar;
}

namespace ScriptCanvasEditor
{
    class ScriptEventsPaletteTreeItem;
    class NodePaletteModel;

    namespace Widget
    {
        struct NodePaletteWidget
        {
            static GraphCanvas::NodePaletteTreeItem* ExternalCreateNodePaletteRoot(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
        };

        class ScriptCanvasRootPaletteTreeItem
            : public GraphCanvas::NodePaletteTreeItem
            , AzFramework::AssetCatalogEventBus::Handler
            , AZ::Data::AssetBus::MultiHandler
            , UpgradeNotifications::Bus::Handler
            , AZ::SystemTickBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(ScriptCanvasRootPaletteTreeItem, AZ::SystemAllocator, 0);
            ScriptCanvasRootPaletteTreeItem(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
            ~ScriptCanvasRootPaletteTreeItem();

            void RegisterCategoryNode(GraphCanvas::GraphCanvasTreeItem* treeItem, const char* subCategory, GraphCanvas::NodePaletteTreeItem* parentRoot = nullptr);
            GraphCanvas::NodePaletteTreeItem* GetCategoryNode(const char* categoryPath, GraphCanvas::NodePaletteTreeItem* parentRoot = nullptr);
            void PruneEmptyNodes();

            void SetActiveScriptCanvasId(const ScriptCanvas::ScriptCanvasId& assetId);

        private:

            void OnRowsInserted(const QModelIndex& parentIndex, int first, int last);
            void OnRowsAboutToBeRemoved(const QModelIndex& parentIndex, int first, int last);

            void TraverseTree(QModelIndex index = QModelIndex());

            void ProcessAsset(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);

            // SystemTickBus
            void OnSystemTick() override;
            ////

            // AssetCatalogEventBus
            void OnCatalogAssetChanged(const AZ::Data::AssetId& /*assetId*/) override;
            ////

            // UpgradeNotifications::Bus
            void OnUpgradeStart() override;
            void OnUpgradeComplete() override;
            void OnUpgradeCancelled() override;
            ////

            void ConnectLambdas();
            void DisconnectLambdas();

            // Requests an async load of a given asset of a type
            void RequestAssetLoad(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType);

            // When the asset loading is ready, this allows to use the asset in some meaningful way
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            bool HasAssetTreeItem(AZ::Data::AssetId assetId) const;

            void CreateFunctionPaletteItem(const AZStd::string& displayName, const AZ::Data::AssetInfo& assetInfo, const AZ::Data::AssetId& sourceId, const AZ::Data::AssetId& runtimeId = {});

            const NodePaletteModel& m_nodePaletteModel;
            AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_assetModel;

            GraphCanvas::GraphCanvasTreeCategorizer m_categorizer;

            bool m_isFunctionGraphActive;
            AZ::Data::AssetId m_previousAssetId;

            AZStd::unordered_map< AZ::Data::AssetId, ScriptEventsPaletteTreeItem* > m_scriptEventElementTreeItems;

            AZStd::unordered_set< AZ::Data::AssetId > m_runtimeAssets;
            AZStd::unordered_map< AZ::Data::AssetId, FunctionPaletteTreeItem* > m_globalFunctionTreeItems;

            AZStd::vector< AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetType>> m_queuedLoads;

            // RequestAssetLoad uses this set to track assets being asynchronously loaded
            AZStd::unordered_set<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetType>> m_pendingAssets;

            AZStd::vector< QMetaObject::Connection > m_lambdaConnections;
        };

        class ScriptCanvasNodePaletteToolbar
            : public QWidget
        {
            Q_OBJECT
        public:

            enum class FilterType
            {
                AllNodes
            };


            ScriptCanvasNodePaletteToolbar(QWidget* parent);

        signals:

            void OnFilterChanged(FilterType newFilter);
            void CreateDynamicEBus();

        private:

            AZStd::unique_ptr< Ui::ScriptCanvasNodePaletteToolbar > m_ui;
        };

        class ScriptCanvasNodePaletteConfig
            : public GraphCanvas::NodePaletteConfig
        {
        public:
            ScriptCanvasNodePaletteConfig(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel, bool isInContextMenu);
            ~ScriptCanvasNodePaletteConfig();

            const NodePaletteModel& m_nodePaletteModel;
            AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_assetModel;
        };

        class NodePaletteDockWidget
            : public GraphCanvas::NodePaletteDockWidget
            , public GraphCanvas::AssetEditorNotificationBus::Handler
            , public GraphCanvas::SceneNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(NodePaletteDockWidget, AZ::SystemAllocator, 0);

            static const char* GetMimeType() { return "scriptcanvas/node-palette-mime-event"; }

            NodePaletteDockWidget(const QString& windowLabel, QWidget* parent, const ScriptCanvasNodePaletteConfig& paletteConfig);
            ~NodePaletteDockWidget();

            void OnNewCustomEvent();
            void OnNewFunctionEvent();

            // GraphCanvas::AssetEditorNotificationBus::Handler
            void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
            ////

            // GraphCanvas::SceneNotificationBus
            void OnSelectionChanged() override;
            ////

            

        protected:

            GraphCanvas::GraphCanvasTreeItem* CreatePaletteRoot() const override;

            void OnTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

            void AddCycleTarget(ScriptCanvas::NodeTypeIdentifier cyclingIdentifier);
            void ClearCycleTarget();
            void CycleToNextNode();
            void CycleToPreviousNode();

        private:

            void HandleTreeItemDoubleClicked(GraphCanvas::GraphCanvasTreeItem* treeItem);

            void ConfigureHelper();
            void ParseCycleTargets(GraphCanvas::GraphCanvasTreeItem* treeItem);

            AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_assetModel;
            const NodePaletteModel& m_nodePaletteModel;

            QToolButton* m_newCustomEvent;

            AZStd::unordered_set< ScriptCanvas::NodeTypeIdentifier > m_cyclingIdentifiers;
            GraphCanvas::NodeFocusCyclingHelper m_cyclingHelper;

            QAction* m_nextCycleAction;
            QAction* m_previousCycleAction;

            bool     m_ignoreSelectionChanged;
        };
    }    
}

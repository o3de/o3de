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

#include <QLineEdit>
#include <QMenu>
#include <QSignalBlocker>
#include <QScrollBar>
#include <QBoxLayout>
#include <QPainter>
#include <QEvent>
#include <QCoreApplication>
#include <QCompleter>
#include <QHeaderView>
#include <QScopedValueRollback>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetEditor/AssetEditorUtils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <ScriptEvents/ScriptEventsAsset.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>

#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/ui_ScriptCanvasNodePaletteToolbar.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/FunctionNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Components/IconComponent.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>

#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Libraries/Entity/EntityRef.h>
#include <ScriptCanvas/Libraries/Libraries.h>

#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>

#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        //////////////////////
        // NodePaletteWidget
        //////////////////////

        GraphCanvas::NodePaletteTreeItem* NodePaletteWidget::ExternalCreateNodePaletteRoot(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        {
            ScriptCanvasRootPaletteTreeItem* root = aznew ScriptCanvasRootPaletteTreeItem(nodePaletteModel, assetModel);

            {
                GraphCanvas::NodePaletteTreeItem* utilitiesRoot = root->GetCategoryNode("Utilities");

                GraphCanvas::NodePaletteTreeItem* variablesRoot = root->CreateChildNode<LocalVariablesListNodePaletteTreeItem>("Variables");
                root->RegisterCategoryNode(variablesRoot, "Variables");

                // We always want to keep thede around as place holders
                GraphCanvas::NodePaletteTreeItem* customEventRoot = root->GetCategoryNode("Script Events");
                customEventRoot->SetAllowPruneOnEmpty(false);

                GraphCanvas::NodePaletteTreeItem* globalFunctionRoot = root->GetCategoryNode("Global Functions");
                globalFunctionRoot->SetAllowPruneOnEmpty(false);
            }

            const NodePaletteModel::NodePaletteRegistry& nodeRegistry = nodePaletteModel.GetNodeRegistry();

            for (const auto& registryPair : nodeRegistry)
            {
                const NodePaletteModelInformation* modelInformation = registryPair.second;

                GraphCanvas::GraphCanvasTreeItem* parentItem = root->GetCategoryNode(modelInformation->m_categoryPath.c_str());
                GraphCanvas::NodePaletteTreeItem* createdItem = nullptr;

                if (auto customModelInformation = azrtti_cast<const CustomNodeModelInformation*>(modelInformation))
                {
                    createdItem = parentItem->CreateChildNode<CustomNodePaletteTreeItem>(customModelInformation->m_typeId, customModelInformation->m_displayName);
                    createdItem->SetToolTip(QString(customModelInformation->m_toolTip.c_str()));
                }
                else if (auto methodNodeModelInformation = azrtti_cast<const MethodNodeModelInformation*>(modelInformation))
                {
                    createdItem = parentItem->CreateChildNode<ClassMethodEventPaletteTreeItem>(methodNodeModelInformation->m_classMethod, methodNodeModelInformation->m_metehodName);
                }
                else if (auto ebusHandlerNodeModelInformation = azrtti_cast<const EBusHandlerNodeModelInformation*>(modelInformation))
                {
                    if (!azrtti_istypeof<const ScriptEventHandlerNodeModelInformation*>(ebusHandlerNodeModelInformation))
                    {
                        createdItem = parentItem->CreateChildNode<EBusHandleEventPaletteTreeItem>(ebusHandlerNodeModelInformation->m_busName, ebusHandlerNodeModelInformation->m_eventName, ebusHandlerNodeModelInformation->m_busId, ebusHandlerNodeModelInformation->m_eventId);
                    }
                }
                else if (auto ebusSenderNodeModelInformation = azrtti_cast<const EBusSenderNodeModelInformation*>(modelInformation))
                {
                    if (!azrtti_istypeof<const ScriptEventSenderNodeModelInformation*>(ebusSenderNodeModelInformation))
                    {
                        createdItem = parentItem->CreateChildNode<EBusSendEventPaletteTreeItem>(ebusSenderNodeModelInformation->m_busName, ebusSenderNodeModelInformation->m_eventName, ebusSenderNodeModelInformation->m_busId, ebusSenderNodeModelInformation->m_eventId);
                    }
                }

                if (createdItem)
                {
                    modelInformation->PopulateTreeItem((*createdItem));
                }
            }

            root->PruneEmptyNodes();

            return root;
        }

        ////////////////////////////////////
        // ScriptCanvasRootPaletteTreeItem
        ////////////////////////////////////

        ScriptCanvasRootPaletteTreeItem::ScriptCanvasRootPaletteTreeItem(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
            : GraphCanvas::NodePaletteTreeItem("root", ScriptCanvasEditor::AssetEditorId)
            , m_nodePaletteModel(nodePaletteModel)
            , m_assetModel(assetModel)
            , m_categorizer(nodePaletteModel)
            , m_isFunctionGraphActive(false)
        {
            if (m_assetModel)
            {
                TraverseTree();

                {
                    auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsInserted, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsInserted(parentIndex, first, last); });
                    m_lambdaConnections.emplace_back(connection);
                }

                {
                    auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsAboutToBeRemoved, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsAboutToBeRemoved(parentIndex, first, last); });
                    m_lambdaConnections.emplace_back(connection);
                }

                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            }
        }

        ScriptCanvasRootPaletteTreeItem::~ScriptCanvasRootPaletteTreeItem()
        {
            for (auto connection : m_lambdaConnections)
            {
                QObject::disconnect(connection);
            }

            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        }

        void ScriptCanvasRootPaletteTreeItem::RegisterCategoryNode(GraphCanvas::GraphCanvasTreeItem* treeItem, const char* subCategory, GraphCanvas::NodePaletteTreeItem* parentRoot)
        {
            if (parentRoot == nullptr)
            {
                parentRoot = this;
            }

            m_categorizer.RegisterCategoryNode(treeItem, subCategory, parentRoot);
        }

        // Given a category path (e.g. "My/Category") and a parent node, creates the necessary intermediate
        // nodes under the given parent and returns the leaf tree item under the given category path.
        GraphCanvas::NodePaletteTreeItem* ScriptCanvasRootPaletteTreeItem::GetCategoryNode(const char* categoryPath, GraphCanvas::NodePaletteTreeItem* parentRoot)
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

        void ScriptCanvasRootPaletteTreeItem::PruneEmptyNodes()
        {
            m_categorizer.PruneEmptyNodes();
        }

        void ScriptCanvasRootPaletteTreeItem::SetActiveScriptCanvasId(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        {
            ScriptCanvas::RuntimeRequestBus::EventResult(m_previousAssetId, scriptCanvasId, &ScriptCanvas::RuntimeRequests::GetAssetId);

            m_isFunctionGraphActive = false;
            EditorGraphRequestBus::EventResult(m_isFunctionGraphActive, scriptCanvasId, &EditorGraphRequests::IsFunctionGraph);

            for (auto functionTreePair : m_globalFunctionTreeItems)
            {
                functionTreePair.second->SetEnabled(!m_isFunctionGraphActive);
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnRowsInserted(const QModelIndex& parentIndex, int first, int last)
        {
            for (int i = first; i <= last; ++i)
            {
                QModelIndex modelIndex = m_assetModel->index(i, 0, parentIndex);
                QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

                AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());
                ProcessAsset(entry);
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnRowsAboutToBeRemoved(const QModelIndex& parentIndex, int first, int last)
        {
            for (int i = first; i <= last; ++i)
            {
                QModelIndex modelIndex = m_assetModel->index(first, 0, parentIndex);
                QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

                const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

                if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
                {
                    const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

                    const AZ::Data::AssetId& assetId = productEntry->GetAssetId();

                    auto scriptEventElementIter = m_scriptEventElementTreeItems.find(assetId);
                    if (scriptEventElementIter != m_scriptEventElementTreeItems.end())
                    {
                        scriptEventElementIter->second->DetachItem();
                        delete scriptEventElementIter->second;
                        m_scriptEventElementTreeItems.erase(scriptEventElementIter);
                    }

                    auto globalFunctionElementIter = m_globalFunctionTreeItems.find(assetId.m_guid);
                    if (globalFunctionElementIter != m_globalFunctionTreeItems.end())
                    {
                        globalFunctionElementIter->second->DetachItem();
                        delete globalFunctionElementIter->second;
                        m_globalFunctionTreeItems.erase(globalFunctionElementIter);
                    }
                }
            }

            PruneEmptyNodes();
        }

        void ScriptCanvasRootPaletteTreeItem::TraverseTree(QModelIndex index)
        {
            QModelIndex sourceIndex = m_assetModel->mapToSource(index);
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            ProcessAsset(entry);

            int rowCount = m_assetModel->rowCount(index);

            for (int i = 0; i < rowCount; ++i)
            {
                QModelIndex nextIndex = m_assetModel->index(i, 0, index);
                TraverseTree(nextIndex);
            }
        }

        void ScriptCanvasRootPaletteTreeItem::ProcessAsset(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
        {
            if (entry)
            {
                if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
                {
                    const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = static_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

                    if (productEntry->GetAssetType() == azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>() ||
                        productEntry->GetAssetType() == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
                    {
                        const AZ::Data::AssetId& assetId = productEntry->GetAssetId();

                        auto elementIter = m_globalFunctionTreeItems.find(assetId.m_guid);
                        if (elementIter == m_globalFunctionTreeItems.end())
                        {
                            RequestAssetLoad(assetId, productEntry->GetAssetType());
                        }
                    }
                }
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnCatalogAssetChanged(const AZ::Data::AssetId& /*assetId*/)
        {
            TraverseTree();
        }

        void ScriptCanvasRootPaletteTreeItem::OnCatalogAssetAdded(const AZ::Data::AssetId& /*assetId*/)
                        {
            TraverseTree();
        }

        void ScriptCanvasRootPaletteTreeItem::OnCatalogAssetRemoved(const AZ::Data::AssetId& /*assetId*/, const AZ::Data::AssetInfo& /*assetInfo*/)
        {
            TraverseTree();
        }
        
        void ScriptCanvasRootPaletteTreeItem::RequestAssetLoad(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType)
        {
            auto entry = AZStd::make_pair(assetId, assetType);
            if (m_pendingAssets.find(entry) == m_pendingAssets.end())
            {
                m_pendingAssets.insert(entry);

                AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
                AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default);
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            AZ::Data::AssetId assetId = asset.GetId();

            if (m_scriptEventElementTreeItems.find(assetId) != m_scriptEventElementTreeItems.end())
                            {
                                return;
                            }

            m_pendingAssets.erase(AZStd::make_pair(assetId, asset.GetType()));

            AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);

            if (asset.GetType() == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
                            {
                ScriptEvents::ScriptEventsAsset* data = asset.GetAs<ScriptEvents::ScriptEventsAsset>();

                                if (data)
                                {
                                    GraphCanvas::NodePaletteTreeItem* categoryRoot = GetCategoryNode(data->m_definition.GetCategory().c_str());
                    ScriptEventsPaletteTreeItem* treeItem = categoryRoot->CreateChildNode<ScriptEventsPaletteTreeItem>(asset);

                                    if (treeItem)
                                    {
                                        m_scriptEventElementTreeItems[assetId] = treeItem;
                                    }
                                }
                            }
            else if (asset.GetType() == azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>())
            {
                ScriptCanvas::RuntimeFunctionAsset* data = asset.GetAs<ScriptCanvas::RuntimeFunctionAsset>();

                AZStd::string rootPath, absolutePath;
                AZ::Data::AssetInfo assetInfo = AssetHelpers::GetAssetInfo(assetId, rootPath);
                AzFramework::StringFunc::Path::Join(rootPath.c_str(), assetInfo.m_relativePath.c_str(), absolutePath);

                AZ::Data::AssetId sourceAssetId;

                AZStd::string normPath = absolutePath;
                AzFramework::StringFunc::Path::Normalize(normPath);

                AZStd::string watchFolder;
                bool sourceInfoFound{};
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, normPath.c_str(), assetInfo, watchFolder);
                if (!sourceInfoFound)
                            {
                    // TODO-LS: report the problem
                    return;
                            }

                sourceAssetId = assetInfo.m_assetId;

                const char* name = assetInfo.m_relativePath.c_str();
                AZStd::string prettyName = data->m_runtimeData.m_name;

                AZStd::string category = "Global Functions";
                AZStd::string relativePath;
                if (AzFramework::StringFunc::Path::GetFolderPath(assetInfo.m_relativePath.c_str(), relativePath))
                {
                    AZStd::to_lower(relativePath.begin(), relativePath.end());

                    const AZStd::string root = "scriptcanvas/functions/";
                    if (relativePath.starts_with(root))
                    {
                        relativePath = relativePath.substr(root.size(), relativePath.size() - root.size());
                        }

                    category.append("/");
                    category.append(relativePath);
                    }

                AZ::Data::AssetId runtimeAssetId = asset.GetId();

                GraphCanvas::NodePaletteTreeItem* categoryRoot = GetCategoryNode(category.c_str());
                FunctionPaletteTreeItem* treeItem = categoryRoot->CreateChildNode<FunctionPaletteTreeItem>(prettyName.empty() ? name : prettyName.c_str(), sourceAssetId, runtimeAssetId);

                if (treeItem)
                {
                    m_globalFunctionTreeItems[assetId.m_guid] = treeItem;
                    treeItem->SetEnabled(!m_isFunctionGraphActive);
                }
            }
        }

        //////////////////////////////////
        // ScriptCanvasNodePaletteConfig
        //////////////////////////////////

        ScriptCanvasNodePaletteConfig::ScriptCanvasNodePaletteConfig(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel, bool isInContextMenu)
            : m_nodePaletteModel(nodePaletteModel)
            , m_assetModel(assetModel)
        {
            m_editorId = ScriptCanvasEditor::AssetEditorId;
            m_mimeType = NodePaletteDockWidget::GetMimeType();
            m_isInContextMenu = isInContextMenu;
            m_allowArrowKeyNavigation = isInContextMenu;
            m_saveIdentifier = m_isInContextMenu ? "ScriptCanvas" : "ScriptCanvas_ContextMenu";

            m_rootTreeItem = Widget::NodePaletteWidget::ExternalCreateNodePaletteRoot(nodePaletteModel, assetModel);
        }

        ScriptCanvasNodePaletteConfig::~ScriptCanvasNodePaletteConfig()
        {

        }

        //////////////////////////
        // NodePaletteDockWidget
        //////////////////////////

        NodePaletteDockWidget::NodePaletteDockWidget(const QString& windowLabel, QWidget* parent, const ScriptCanvasNodePaletteConfig& paletteConfig)
            : GraphCanvas::NodePaletteDockWidget(parent, windowLabel, paletteConfig)
            , m_assetModel(paletteConfig.m_assetModel)
            , m_nodePaletteModel(paletteConfig.m_nodePaletteModel)
            , m_nextCycleAction(nullptr)
            , m_previousCycleAction(nullptr)
            , m_ignoreSelectionChanged(false)
        {
            QMenu* creationMenu = new QMenu();

            auto scriptEventAction = creationMenu->addAction("New Script Event");
            QObject::connect(scriptEventAction, &QAction::triggered, this, &NodePaletteDockWidget::OnNewCustomEvent);

            auto functionAction = creationMenu->addAction("New Function");
            QObject::connect(functionAction, &QAction::triggered, this, &NodePaletteDockWidget::OnNewFunctionEvent);

            m_newCustomEvent = new QToolButton(this);
            m_newCustomEvent->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/add.png"));
            m_newCustomEvent->setToolTip("Click to create a new Script Event or Function");
            m_newCustomEvent->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
            m_newCustomEvent->setMenu(creationMenu);

            //

            AddSearchCustomizationWidget(m_newCustomEvent);

            GraphCanvas::NodePaletteTreeView* treeView = GetTreeView();

            {
                m_nextCycleAction = new QAction(treeView);

                m_nextCycleAction->setShortcut(QKeySequence(Qt::Key_F8));
                treeView->addAction(m_nextCycleAction);

                QObject::connect(m_nextCycleAction, &QAction::triggered, this, &NodePaletteDockWidget::CycleToNextNode);
            }

            {
                m_previousCycleAction = new QAction(treeView);

                m_previousCycleAction->setShortcut(QKeySequence(Qt::Key_F7));
                treeView->addAction(m_previousCycleAction);

                QObject::connect(m_previousCycleAction, &QAction::triggered, this, &NodePaletteDockWidget::CycleToPreviousNode);
            }

            QObject::connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodePaletteDockWidget::OnTreeSelectionChanged);
            QObject::connect(treeView, &GraphCanvas::NodePaletteTreeView::OnTreeItemDoubleClicked, this, &NodePaletteDockWidget::HandleTreeItemDoubleClicked);

            ConfigureSearchCustomizationMargins(QMargins(0, 0, 0, 0), 0);

            GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        }

        NodePaletteDockWidget::~NodePaletteDockWidget()
        {
            GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        }

        void NodePaletteDockWidget::OnNewCustomEvent()
        {
            AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Broadcast(&AzToolsFramework::AssetEditor::AssetEditorRequests::CreateNewAsset, azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
        }

        void NodePaletteDockWidget::OnNewFunctionEvent()
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::CreateNewFunctionAsset);
        }

        void NodePaletteDockWidget::OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId)
        {
            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphCanvasGraphId);

            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);
            static_cast<ScriptCanvasRootPaletteTreeItem*>(ModTreeRoot())->SetActiveScriptCanvasId(scriptCanvasId);
        }

        void NodePaletteDockWidget::OnSelectionChanged()
        {
            if (m_ignoreSelectionChanged)
            {
                return;
            }

            m_cyclingHelper.Clear();

            GetTreeView()->selectionModel()->clearSelection();
        }

        GraphCanvas::GraphCanvasTreeItem* NodePaletteDockWidget::CreatePaletteRoot() const
        {
            return NodePaletteWidget::ExternalCreateNodePaletteRoot(m_nodePaletteModel, m_assetModel);
        }

        void NodePaletteDockWidget::OnTreeSelectionChanged([[maybe_unused]] const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
        {
            ClearCycleTarget();

            AZStd::unordered_set< ScriptCanvas::VariableId > variableSet;

            GraphCanvas::NodePaletteWidget* paletteWidget = GetNodePaletteWidget();

            QModelIndexList indexList = GetTreeView()->selectionModel()->selectedRows();

            if (indexList.size() == 1)
            {
                QSortFilterProxyModel* filterModel = static_cast<QSortFilterProxyModel*>(GetTreeView()->model());

                for (const QModelIndex& index : indexList)
                {
                    QModelIndex sourceIndex = filterModel->mapToSource(index);

                    GraphCanvas::NodePaletteTreeItem* nodePaletteItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(sourceIndex.internalPointer());

                    ParseCycleTargets(nodePaletteItem);
                }
            }
        }

        void NodePaletteDockWidget::AddCycleTarget(ScriptCanvas::NodeTypeIdentifier cyclingIdentifier)
        {
            if (cyclingIdentifier == ScriptCanvas::NodeTypeIdentifier(0))
            {
                return;
            }

            m_cyclingIdentifiers.insert(cyclingIdentifier);
            m_cyclingHelper.Clear();

            if (m_nextCycleAction)
            {
                m_nextCycleAction->setEnabled(true);
                m_previousCycleAction->setEnabled(true);
            }
        }

        void NodePaletteDockWidget::ClearCycleTarget()
        {
            m_cyclingIdentifiers.clear();
            m_cyclingHelper.Clear();

            if (m_nextCycleAction)
            {
                m_nextCycleAction->setEnabled(false);
                m_previousCycleAction->setEnabled(false);
            }
        }

        void NodePaletteDockWidget::CycleToNextNode()
        {
            ConfigureHelper();

            m_cyclingHelper.CycleToNextNode();
        }

        void NodePaletteDockWidget::CycleToPreviousNode()
        {
            ConfigureHelper();

            m_cyclingHelper.CycleToPreviousNode();
        }

        void NodePaletteDockWidget::HandleTreeItemDoubleClicked(GraphCanvas::GraphCanvasTreeItem* treeItem)
        {
            ParseCycleTargets(treeItem);
            CycleToNextNode();
        }

        void NodePaletteDockWidget::ConfigureHelper()
        {
            if (!m_cyclingHelper.IsConfigured() && !m_cyclingIdentifiers.empty())
            {
                ScriptCanvas::ScriptCanvasId scriptCanvasId;
                GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetActiveScriptCanvasId);

                AZ::EntityId graphCanvasGraphId;
                GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetActiveGraphCanvasGraphId);

                m_cyclingHelper.SetActiveGraph(graphCanvasGraphId);

                AZStd::vector<GraphCanvas::NodeId> cyclingNodes;
                AZStd::vector<NodeIdPair> completeNodePairs;

                for (ScriptCanvas::NodeTypeIdentifier nodeTypeIdentifier : m_cyclingIdentifiers)
                {
                    AZStd::vector<NodeIdPair> nodePairs;
                    EditorGraphRequestBus::EventResult(nodePairs, scriptCanvasId, &EditorGraphRequests::GetNodesOfType, nodeTypeIdentifier);

                    cyclingNodes.reserve(cyclingNodes.size() + nodePairs.size());
                    completeNodePairs.reserve(completeNodePairs.size() + nodePairs.size());

                    for (const auto& nodeIdPair : nodePairs)
                    {
                        cyclingNodes.emplace_back(nodeIdPair.m_graphCanvasId);
                        completeNodePairs.emplace_back(nodeIdPair);
                    }
                }

                m_cyclingHelper.SetNodes(cyclingNodes);

                {
                    // Clean-up Selection to maintain the 'single' selection state throughout the editor
                    QScopedValueRollback<bool> ignoreSelection(m_ignoreSelectionChanged, true);
                    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
                }

                EditorGraphRequestBus::Event(scriptCanvasId, &EditorGraphRequests::HighlightNodes, completeNodePairs);
            }
        }

        void NodePaletteDockWidget::ParseCycleTargets(GraphCanvas::GraphCanvasTreeItem* treeItem)
        {
            AZStd::vector< ScriptCanvas::NodeTypeIdentifier > nodeTypeIdentifiers = NodeIdentifierFactory::ConstructNodeIdentifiers(treeItem);

            for (auto nodeTypeIdentifier : nodeTypeIdentifiers)
            {
                AddCycleTarget(nodeTypeIdentifier);
            }
        }
    }
}

#include <Editor/View/Widgets/moc_ScriptCanvasNodePaletteDockWidget.cpp>

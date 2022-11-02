/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/containers/map.h>

#include <AzFramework/Gem/GemInfo.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetEditor/AssetEditorUtils.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Components/IconComponent.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Nodes/NodeUtils.h>
#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/FunctionNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/ui_ScriptCanvasNodePaletteToolbar.h>

#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Utils/NodeUtils.h>
#include <ScriptEvents/ScriptEventsAsset.h>

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
                GraphCanvas::NodePaletteTreeItem* variablesRoot = root->CreateChildNode<LocalVariablesListNodePaletteTreeItem>("Variables");
                root->RegisterCategoryNode(variablesRoot, "Variables");

                GraphCanvas::NodePaletteTreeItem* customEventRoot = root->GetCategoryNode("Script Events");
                customEventRoot->SetAllowPruneOnEmpty(true);

                GraphCanvas::NodePaletteTreeItem* globalFunctionRoot = root->GetCategoryNode("User Functions");
                globalFunctionRoot->SetAllowPruneOnEmpty(true);

            }

            const NodePaletteModel::NodePaletteRegistry& nodeRegistry = nodePaletteModel.GetNodeRegistry();

            for (const auto& registryPair : nodeRegistry)
            {
                
                const NodePaletteModelInformation* modelInformation = registryPair.second;

                GraphCanvas::GraphCanvasTreeItem* parentItem = root->GetCategoryNode(modelInformation->m_categoryPath.c_str());
                GraphCanvas::NodePaletteTreeItem* createdItem = nullptr;

                if (auto customModelInformation = azrtti_cast<const CustomNodeModelInformation*>(modelInformation))
                {
                    createdItem = parentItem->CreateChildNode<CustomNodePaletteTreeItem>(*customModelInformation);
                    createdItem->SetToolTip(QString(customModelInformation->m_toolTip.c_str()));
                }
                else if (auto methodNodeModelInformation = azrtti_cast<const MethodNodeModelInformation*>(modelInformation))
                {
                    createdItem = parentItem->CreateChildNode<ScriptCanvasEditor::ClassMethodEventPaletteTreeItem>(methodNodeModelInformation->m_classMethod, methodNodeModelInformation->m_methodName, methodNodeModelInformation->m_isOverload, methodNodeModelInformation->m_propertyStatus);
                }
                else if (auto globalMethodNodeModelInformation = azrtti_cast<const GlobalMethodNodeModelInformation*>(modelInformation);
                    globalMethodNodeModelInformation != nullptr)
                {
                    createdItem = parentItem->CreateChildNode<ScriptCanvasEditor::GlobalMethodEventPaletteTreeItem>(
                        *globalMethodNodeModelInformation);
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
                        createdItem = parentItem->CreateChildNode<ScriptCanvasEditor::EBusSendEventPaletteTreeItem>(ebusSenderNodeModelInformation->m_busName, ebusSenderNodeModelInformation->m_eventName, ebusSenderNodeModelInformation->m_busId, ebusSenderNodeModelInformation->m_eventId, ebusSenderNodeModelInformation->m_isOverload, ebusSenderNodeModelInformation->m_propertyStatus);
                    }
                }
                else if (auto dataDrivenModelInformation = azrtti_cast<const DataDrivenNodeModelInformation*>(modelInformation))
                {
                    createdItem = parentItem->CreateChildNode<DataDrivenNodePaletteTreeItem>(*dataDrivenModelInformation);
                    createdItem->SetToolTip(QString(dataDrivenModelInformation->m_toolTip.c_str()));
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
        {
            UpgradeNotificationsBus::Handler::BusConnect();

            if (m_assetModel)
            {
                TraverseTree();

                ConnectLambdas();

                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            }
        }

        ScriptCanvasRootPaletteTreeItem::~ScriptCanvasRootPaletteTreeItem()
        {
            DisconnectLambdas();

            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            UpgradeNotificationsBus::Handler::BusDisconnect();

        }

        void ScriptCanvasRootPaletteTreeItem::ConnectLambdas()
        {
            {
                auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsInserted, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsInserted(parentIndex, first, last); });
                m_lambdaConnections.emplace_back(connection);
            }

            {
                auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsAboutToBeRemoved, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsAboutToBeRemoved(parentIndex, first, last); });
                m_lambdaConnections.emplace_back(connection);
            }
        }

        void ScriptCanvasRootPaletteTreeItem::DisconnectLambdas()
        {
            for (auto connection : m_lambdaConnections)
            {
                QObject::disconnect(connection);
            }
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
            ScriptCanvas::GraphRequestBus::EventResult(m_previousAssetId, scriptCanvasId, &ScriptCanvas::GraphRequests::GetAssetId);

            for (auto functionTreePair : m_globalFunctionTreeItems)
            {
                functionTreePair.second->SetEnabled(true);
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
                    else
                    {
                        auto globalFunctionElementIter = m_globalFunctionTreeItems.find(assetId);
                        if (globalFunctionElementIter != m_globalFunctionTreeItems.end())
                        {
                            globalFunctionElementIter->second->SetError("Graph has errors or has been deleted");
                        }
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

                    const auto entryType = productEntry->GetAssetType();

                    if (entryType == azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>())
                    {
                        const AZ::Data::AssetId& assetId = productEntry->GetAssetId();

                        auto elementIter = m_globalFunctionTreeItems.find(assetId);
                        if (elementIter == m_globalFunctionTreeItems.end())
                        {
                            RequestAssetLoad(assetId, productEntry->GetAssetType());
                        }
                        else
                        {
                            if (elementIter->second->HasError())
                            {
                                m_pendingAssets.erase(assetId);
                                RequestAssetLoad(assetId, productEntry->GetAssetType());
                            }
                        }
                    }
                    else if (entryType == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
                    {
                        const AZ::Data::AssetId& assetId = productEntry->GetAssetId();
 
                        auto elementIter = m_scriptEventElementTreeItems.find(assetId.m_guid);
                        if (elementIter == m_scriptEventElementTreeItems.end())
                        {
                            RequestAssetLoad(assetId, productEntry->GetAssetType());
                        }
                    }
                }
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnSystemTick()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();

            while (!m_requestQueue.empty())
            {
                const auto entry = m_requestQueue.front();
                m_requestQueue.pop();

                if (m_pendingAssets.find(entry.first) == m_pendingAssets.end())
                {
                    AZ::Data::AssetBus::MultiHandler::BusConnect(entry.first);
                    m_pendingAssets[entry.first] = AZ::Data::AssetManager::Instance().GetAsset(entry.first, entry.second, AZ::Data::AssetLoadBehavior::Default);
                }
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            auto scriptEventIter = m_scriptEventElementTreeItems.find(assetId.m_guid);
            if (scriptEventIter != m_scriptEventElementTreeItems.end())
            {
                RequestAssetLoad(assetId, azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
            }
            else
            {
                auto functionIter = m_globalFunctionTreeItems.find(assetId);
                if (functionIter != m_globalFunctionTreeItems.end())
                {
                    RequestAssetLoad(assetId, azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>());
                }
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnCatalogAssetAdded(const AZ::Data::AssetId&)
        {
        }

        void ScriptCanvasRootPaletteTreeItem::OnCatalogAssetRemoved(const AZ::Data::AssetId&, const AZ::Data::AssetInfo&)
        {
        }

        void ScriptCanvasRootPaletteTreeItem::OnUpgradeStart()
        {
            DisconnectLambdas();

            // Disconnect from the AssetCatalogEventBus during the upgrade to avoid overlap
            // in asset processing
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        }

        void ScriptCanvasRootPaletteTreeItem::OnUpgradeCancelled()
        {
            if (!AzFramework::AssetCatalogEventBus::Handler::BusIsConnected())
            {
                ConnectLambdas();

                AzFramework::AssetCatalogEventBus::Handler::BusConnect();

                TraverseTree();
            }
        }

        void ScriptCanvasRootPaletteTreeItem::RequestAssetLoad(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType)
        {
            // Delay handling loads until the top of the next tick in case we are handling this on an asset callback thread to avoid potential deadlocks.
            AZ::SystemTickBus::Handler::BusConnect();
            m_requestQueue.emplace(AZStd::make_pair(assetId, assetType));
        }

        void ScriptCanvasRootPaletteTreeItem::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // mark the function on error... if possible
            AZ::Data::AssetId assetId = asset.GetId();
            m_pendingAssets.erase(assetId);
        }

        void ScriptCanvasRootPaletteTreeItem::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            using namespace AzFramework;

            AZ::Data::AssetId assetId = asset.GetId();
            m_pendingAssets.erase(assetId);

            if (asset.GetType() == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
            {
                if (m_scriptEventElementTreeItems.find(assetId) == m_scriptEventElementTreeItems.end())
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
            }
            else if (asset.GetType() == azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>())
            {
                // We only need to add
                auto treePaletteIter = m_globalFunctionTreeItems.find(asset->GetId());

                if (treePaletteIter == m_globalFunctionTreeItems.end())
                {
                    ScriptCanvas::SubgraphInterfaceAsset* data = asset.GetAs<ScriptCanvas::SubgraphInterfaceAsset>();
                    if (!data)
                    {
                        return;
                    }

                    if (!data->m_interfaceData.m_interface.HasAnyFunctionality())
                    {
                        // check for deleting the old entry
                        return;
                    }

                    auto assetInfo = AssetHelpers::GetSourceInfoByProductId(assetId, asset.GetType());

                    if (!assetInfo.m_assetId.IsValid())
                    {
                        return;
                    }
 
                    CreateFunctionPaletteItem(asset, assetInfo);
 
                    treePaletteIter = m_globalFunctionTreeItems.find(asset->GetId());
 
                    if (treePaletteIter != m_globalFunctionTreeItems.end())
                    {
                        treePaletteIter->second->ClearError();
                    }
 
                    m_monitoredAssets.emplace(asset->GetId(), asset);
                }
                else 
                {
                    RequestBuildChildrenFromSubgraphInterface(treePaletteIter->second, asset);
                    treePaletteIter->second->ClearError();
                }
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        bool ScriptCanvasRootPaletteTreeItem::HasAssetTreeItem(AZ::Data::AssetId assetId) const
        {
            return m_scriptEventElementTreeItems.find(assetId) != m_scriptEventElementTreeItems.end()
                || m_globalFunctionTreeItems.find(assetId) != m_globalFunctionTreeItems.end();
        }

        void ScriptCanvasRootPaletteTreeItem::CreateFunctionPaletteItem(AZ::Data::Asset<AZ::Data::AssetData> asset, const AZ::Data::AssetInfo& assetInfo)
        {
            ScriptCanvas::SubgraphInterfaceAsset* data = asset.GetAs<ScriptCanvas::SubgraphInterfaceAsset>();
            if (!data)
            {
                return;
            }

            const ScriptCanvas::Grammar::SubgraphInterface& graphInterface = data->m_interfaceData.m_interface;
            if (!graphInterface.HasAnyFunctionality())
            {
                return;
            }

            AZStd::string name;
            AzFramework::StringFunc::Path::GetFileName(assetInfo.m_relativePath.c_str(), name);

            AZStd::string category = "User Functions";

            AZStd::string relativePath;
            if (AzFramework::StringFunc::Path::GetFolderPath(assetInfo.m_relativePath.c_str(), relativePath))
            {
                AZStd::to_lower(relativePath.begin(), relativePath.end());

                auto stripPathStart = [&](const AZStd::string root)
                {
                    if (relativePath.starts_with(root))
                    {
                        relativePath = relativePath.substr(root.size(), relativePath.size() - root.size());
                    }
                };

                stripPathStart("scriptcanvas/functions");
                stripPathStart("scriptcanvas");
                stripPathStart("/");

                category.append("/");
                category.append(relativePath);
            }

            NodePaletteTreeItem* categoryRoot = GetCategoryNode(category.c_str());
            auto functionCategory = categoryRoot->CreateChildNode<NodePaletteTreeItem>(name.c_str(), ScriptCanvasEditor::AssetEditorId);
            RequestBuildChildrenFromSubgraphInterface(functionCategory, asset);            
            m_globalFunctionTreeItems[asset->GetId()] = functionCategory;
            categoryRoot->SetEnabled(true);
        }

        void ScriptCanvasRootPaletteTreeItem::RequestBuildChildrenFromSubgraphInterface(NodePaletteTreeItem* functionCategory, AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            functionCategory->ClearChildren();

            ScriptCanvas::SubgraphInterfaceAsset* data = asset.GetAs<ScriptCanvas::SubgraphInterfaceAsset>();
            if (!data)
            {
                return;
            }

            const ScriptCanvas::Grammar::SubgraphInterface& graphInterface = data->m_interfaceData.m_interface;
            if (!graphInterface.HasAnyFunctionality())
            {
                return;
            }

            auto parent = functionCategory;
            parent->SetEnabled(true);

            if (graphInterface.IsUserNodeable())
            {
                auto name = functionCategory->GetName().toUtf8().constData();
                parent = parent->CreateChildNode<FunctionPaletteTreeItem>(AZStd::string::format("%s Node", name).c_str(), ScriptCanvas::Grammar::MakeFunctionSourceIdNodeable(), asset);
                parent->SetEnabled(true);
            }

            if (graphInterface.IsMarkedPure())
            {
                for (auto& in : graphInterface.GetIns())
                {
                    auto childNode = parent->CreateChildNode<FunctionPaletteTreeItem>(in.displayName.c_str(), in.sourceID, asset);
                    childNode->SetEnabled(true);
                }
            }
            else
            {
                auto& ins = graphInterface.GetIns();
                AZStd::vector<const ScriptCanvas::Grammar::In*> onNodeIns;
                AZStd::vector<const ScriptCanvas::Grammar::In*> pureIns;

                auto pureParent = functionCategory;

                for (auto& in : ins)
                {
                    if (in.isPure)
                    {
                        pureIns.push_back(&in);
                    }
                    else
                    {
                        onNodeIns.push_back(&in);
                    }
                }

                if (!onNodeIns.empty())
                {
                    parent->SetEnabled(true);

                    for (auto in : onNodeIns)
                    {
                        auto childNode = parent->CreateChildNode<NodePaletteTreeItem>(in->displayName.c_str(), ScriptCanvasEditor::AssetEditorId);
                        childNode->SetEnabled(true);
                    }
                }

                if (!pureIns.empty())
                {
                    parent = pureParent->CreateChildNode<NodePaletteTreeItem>("Pure Functions", ScriptCanvasEditor::AssetEditorId);
                    parent->SetEnabled(true);

                    for (auto in : pureIns)
                    {
                        auto childNode = parent->CreateChildNode<FunctionPaletteTreeItem>(in->displayName.c_str(), in->sourceID, asset);
                        childNode->SetEnabled(true);
                    }
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
            
            GraphCanvas::NodePaletteTreeView* treeView = GetTreeView();

            treeView->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);

            if (!paletteConfig.m_isInContextMenu)
            {
                QMenu* creationMenu = new QMenu();

                auto scriptEventAction = creationMenu->addAction("New Script Event");
                QObject::connect(scriptEventAction, &QAction::triggered, this, &NodePaletteDockWidget::OnNewCustomEvent);

                m_newCustomEvent = new QToolButton(this);
                m_newCustomEvent->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/add.png"));
                m_newCustomEvent->setToolTip("Click to create a new Script Event or Function");
                m_newCustomEvent->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
                m_newCustomEvent->setMenu(creationMenu);

                //

                AddSearchCustomizationWidget(m_newCustomEvent);

                

                {
                    m_nextCycleAction = new QAction(treeView);
                    m_nextCycleAction->setText(tr("Next Instance in Graph"));

                    m_nextCycleAction->setShortcut(QKeySequence(Qt::Key_F8));
                    treeView->addAction(m_nextCycleAction);

                    QObject::connect(m_nextCycleAction, &QAction::triggered, this, &NodePaletteDockWidget::CycleToNextNode);
                }

                {
                    m_previousCycleAction = new QAction(treeView);
                    m_previousCycleAction->setText(tr("Previous Instance in Graph"));

                    m_previousCycleAction->setShortcut(QKeySequence(Qt::Key_F7));
                    treeView->addAction(m_previousCycleAction);

                    QObject::connect(m_previousCycleAction, &QAction::triggered, this, &NodePaletteDockWidget::CycleToPreviousNode);
                }

                QObject::connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodePaletteDockWidget::OnTreeSelectionChanged);
                QObject::connect(treeView, &GraphCanvas::NodePaletteTreeView::OnTreeItemDoubleClicked, this, &NodePaletteDockWidget::HandleTreeItemDoubleClicked);

                {
                    m_openTranslationData = new QAction(treeView);
                    m_openTranslationData->setText("Explore Translation Data");
                    treeView->addAction(m_openTranslationData);

                    QObject::connect(m_openTranslationData, &QAction::triggered, this, &NodePaletteDockWidget::OpenTranslationData);
                }

                {
                    m_generateTranslation = new QAction(treeView);
                    m_generateTranslation->setText("Generate Translation");
                    treeView->addAction(m_generateTranslation);

                    QObject::connect(m_generateTranslation, &QAction::triggered, this, &NodePaletteDockWidget::GenerateTranslation);
                }

            }

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

        void NodePaletteDockWidget::OnTreeSelectionChanged(const QItemSelection&, const QItemSelection&)
        {
            ClearCycleTarget();

            AZStd::unordered_set< ScriptCanvas::VariableId > variableSet;

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
                m_openTranslationData->setEnabled(true);
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
                m_openTranslationData->setEnabled(false);
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

        void NodePaletteDockWidget::NavigateToTranslationFile(GraphCanvas::NodePaletteTreeItem* nodePaletteItem)
        {
            if (nodePaletteItem)
            {
                AZ::IO::Path filePath = nodePaletteItem->GetTranslationDataPath();
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                if (fileIO && !filePath.empty() && fileIO->Exists(filePath.c_str()))
                {
                    AzQtComponents::ShowFileOnDesktop(filePath.c_str());
                }
            }
        }

        void NodePaletteDockWidget::GenerateTranslation()
        {
            QModelIndexList indexList = GetTreeView()->selectionModel()->selectedRows();

            QSortFilterProxyModel* filterModel = static_cast<QSortFilterProxyModel*>(GetTreeView()->model());

            for (const QModelIndex& index : indexList)
            {
                QModelIndex sourceIndex = filterModel->mapToSource(index);

                GraphCanvas::NodePaletteTreeItem* nodePaletteItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(sourceIndex.internalPointer());
                nodePaletteItem->GenerateTranslationData();
            }

            if (indexList.size() == 1)
            {
                QModelIndex sourceIndex = filterModel->mapToSource(indexList[0]);
                if (sourceIndex.isValid())
                {
                    GraphCanvas::NodePaletteTreeItem* nodePaletteItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(sourceIndex.internalPointer());
                    NavigateToTranslationFile(nodePaletteItem);
                }
            }
        }

        void NodePaletteDockWidget::OpenTranslationData()
        {
            QModelIndexList indexList = GetTreeView()->selectionModel()->selectedRows();

            if (indexList.size() == 1)
            {
                QSortFilterProxyModel* filterModel = static_cast<QSortFilterProxyModel*>(GetTreeView()->model());

                for (const QModelIndex& index : indexList)
                {
                    QModelIndex sourceIndex = filterModel->mapToSource(index);

                    GraphCanvas::NodePaletteTreeItem* nodePaletteItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(sourceIndex.internalPointer());
                    NavigateToTranslationFile(nodePaletteItem);
                }
            }
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

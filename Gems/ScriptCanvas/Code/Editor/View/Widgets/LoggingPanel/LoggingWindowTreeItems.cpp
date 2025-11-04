/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingWindowTreeItems.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>
#include <Editor/View/Widgets/AssetGraphSceneDataBus.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasEditor
{
    /////////////////////
    // DebugLogTreeItem
    /////////////////////

    bool DebugLogTreeItem::MatchesFilter(const DebugLogFilter& treeFilter)
    {
        DebugLogTreeItem* parent = static_cast<DebugLogTreeItem*>(GetParent());

        while (parent)
        {
            // We don't want to match against the root, since it always matches.
            // So only check things that have a valid parent.
            DebugLogTreeItem* nextParent = static_cast<DebugLogTreeItem*>(parent->GetParent());

            if (nextParent != nullptr && parent->OnMatchesFilter(treeFilter))
            {
                return true;
            }

            parent = static_cast<DebugLogTreeItem*>(parent->GetParent());
        }

        DebugLogTreeItem* currentItem = this;

        AZStd::unordered_set< DebugLogTreeItem* > children;

        while (currentItem)
        {
            if (currentItem->OnMatchesFilter(treeFilter))
            {
                return true;
            }

            for (int i = 0; i < currentItem->GetChildCount(); ++i)
            {
                children.insert(static_cast<DebugLogTreeItem*>(currentItem->FindChildByRow(i)));
            }

            if (!children.empty())
            {
                currentItem = (*children.begin());
                children.erase(children.begin());
            }
            else
            {
                currentItem = nullptr;
            }
        }

        return false;
    }

    const ScriptCanvas::Endpoint& DebugLogTreeItem::GetIncitingEndpoint() const
    {
        return m_incitingEndpoint;
    }

    bool DebugLogTreeItem::IsTriggeredBy(const ScriptCanvas::Endpoint& endpoint) const
    {
        return m_incitingEndpoint == endpoint;
    }

    Qt::ItemFlags DebugLogTreeItem::Flags([[maybe_unused]] const QModelIndex& index) const
    {
        return Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;
    }

    int DebugLogTreeItem::GetColumnCount() const
    {
        return Column::Count;
    }

    void DebugLogTreeItem::SetIncitingEndpoint(const ScriptCanvas::Endpoint& endpoint)
    {
        m_incitingEndpoint = endpoint;
    }

    /////////////////////
    // DebugLogRootItem
    /////////////////////

    DebugLogRootItem::DebugLogRootItem()
        : m_updatePolicy(UpdatePolicy::Batched)
    {
        m_additionTimer.setSingleShot(true);
        m_additionTimer.setInterval(1000);
        
        QObject::connect(&m_additionTimer, &QTimer::timeout, [this]() { this->RedoLayout(); });
    }

    DebugLogRootItem::~DebugLogRootItem()
    {
    }

    ExecutionLogTreeItem* DebugLogRootItem::CreateExecutionItem(
        const LoggingDataId& loggingDataId,
        const ScriptCanvas::NodeTypeIdentifier& nodeType,
        const ScriptCanvas::GraphInfo& graphInfo,
        const ScriptCanvas::NamedNodeId& nodeId)
    {
        ExecutionLogTreeItem* treeItem = nullptr;

        if (m_updatePolicy == UpdatePolicy::Batched)
        {
            if (!m_additionTimer.isActive())
            {
                m_additionTimer.start();
            }
        }

        if (m_updatePolicy == UpdatePolicy::SingleTime)
        {
            treeItem = CreateChildNodeWithoutAddSignal<ExecutionLogTreeItem>(loggingDataId, nodeType, graphInfo, nodeId);
        }
        else
        {
            treeItem = CreateChildNode<ExecutionLogTreeItem>(loggingDataId, nodeType, graphInfo, nodeId);
        }

        return treeItem;
    }

    QVariant DebugLogRootItem::Data([[maybe_unused]] const QModelIndex& index, [[maybe_unused]] int role) const
    {
        return QVariant();
    }

    void DebugLogRootItem::ResetData()
    {
        SignalLayoutAboutToBeChanged();
        ClearChildren();
        SignalLayoutChanged();
    }

    void DebugLogRootItem::SetUpdatePolicy(UpdatePolicy updatePolicy)
    {
        if (m_updatePolicy != updatePolicy)
        {
            m_updatePolicy = updatePolicy;

            m_additionTimer.stop();
        }
    }

    DebugLogRootItem::UpdatePolicy DebugLogRootItem::GetUpdatePolicy() const
    {
        return m_updatePolicy;
    }

    void DebugLogRootItem::RedoLayout()
    {
        m_additionTimer.stop();

        SignalLayoutAboutToBeChanged();
        SignalLayoutChanged();
    }

    /////////////////////////
    // ExecutionLogTreeItem
    /////////////////////////

    ExecutionLogTreeItem::ExecutionLogTreeItem
        ( const LoggingDataId& loggingDataId
        , const ScriptCanvas::NodeTypeIdentifier& nodeType
        , const ScriptCanvas::GraphInfo& graphInfo
        , const ScriptCanvas::NamedNodeId& nodeId)
        : m_loggingDataId(loggingDataId)
        , m_nodeType(nodeType)
        , m_graphInfo(graphInfo)
        , m_scriptCanvasAssetNodeId(nodeId)
        , m_iconPixmap(nullptr)
    {
        m_paletteConfiguration.m_iconPalette = "NodePaletteTypeIcon";
        m_paletteConfiguration.SetColorPalette("MethodNodeTitlePalette");

        AZ::NamedEntityId entityName;
        LoggingDataRequestBus::EventResult(entityName, m_loggingDataId, &LoggingDataRequests::FindNamedEntityId, m_graphInfo.m_runtimeEntity);

        m_sourceEntityName = entityName.ToString().c_str();
        m_displayName = nodeId.m_name.c_str();

        ScrapeBehaviorContextData();
        ScrapeGraphCanvasData();

        m_inputName = "---";
        m_outputName = "---";

        // GeneralAssetNotificationBus::Handler::BusConnect(GetAssetId());
    }

    QVariant ExecutionLogTreeItem::Data(const QModelIndex& index, int role) const
    {
        switch (index.column())
        {
        case Column::NodeName:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTipRole)
            {
                return m_displayName;
            }
            else if (role == Qt::DecorationRole)
            {
                if (m_iconPixmap != nullptr)
                {
                    return (*m_iconPixmap);
                }
            }
        }
        break;
        case Column::Input:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTipRole)
            {
                return m_inputName;
            }
        }
        break;
        case Column::Output:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTipRole)
            {
                return m_outputName;
            }
        }
        break;
        case Column::TimeStep:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTipRole)
            {
                return m_timeString;
            }
        }
        break;
        case Column::ScriptName:
        {
            if (role == Qt::DisplayRole)
            {
                return m_graphName;
            }
            else if (role == Qt::ToolTipRole)
            {
                return m_relativeGraphPath;
            }
            else if (role == Qt::ForegroundRole)
            {
                return QColor(42,132,252);
            }
            else if (role == Qt::FontRole)
            {
                QFont font;
                font.setUnderline(true);

                return font;
            }
        }
        break;
        case Column::SourceEntity:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTipRole)
            {
                return m_sourceEntityName;
            }
        }
        break;
        default:
            break;
        }

        return QVariant();
    }

    AZ::EntityId ExecutionLogTreeItem::GetNodeId() const
    {
        return m_scriptCanvasAssetNodeId;
    }

    void ExecutionLogTreeItem::RegisterAnnotation(const ScriptCanvas::AnnotateNodeSignal& annotationSignal, bool allowAddSignal)
    {
        // The QTreeView does have a setFirstColumnSpanned, but it doesn't seem dynamic, nor model driven.
        // So I don't want to use it.
        if (allowAddSignal)
        {
            CreateChildNode<NodeAnnotationTreeItem>(annotationSignal.m_annotationLevel, annotationSignal.m_annotation);
        }
        else
        {
            CreateChildNodeWithoutAddSignal<NodeAnnotationTreeItem>(annotationSignal.m_annotationLevel, annotationSignal.m_annotation);
        }
    }

    void ExecutionLogTreeItem::RegisterDataInput(const ScriptCanvas::Endpoint& incitingEndpoint, const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::string_view dataString, bool allowAddSignal)
    {
        if (!HasExecutionInput() && !HasExecutionOutput())
        {
            ResolveWrapperNode();
        }

        DataLogTreeItem* dataTreeItem = nullptr;

        for (int i = 0; i < GetChildCount(); ++i)
        {
            DataLogTreeItem* testLogItem = azrtti_cast<DataLogTreeItem*>(FindChildByRow(i));

            if (testLogItem && !testLogItem->HasInput())
            {
                dataTreeItem = testLogItem;
                break;
            }
        }

        if (dataTreeItem == nullptr)
        {
            if (allowAddSignal)
            {
                dataTreeItem = CreateChildNode<DataLogTreeItem>(GetGraphIdentifier());
            }
            else
            {
                dataTreeItem = CreateChildNodeWithoutAddSignal<DataLogTreeItem>(GetGraphIdentifier());
            }
        }

        ScriptCanvas::Endpoint endpoint(m_scriptCanvasAssetNodeId, slotId);
        dataTreeItem->RegisterDataInput(incitingEndpoint, endpoint, slotName, dataString);
    }

    void ExecutionLogTreeItem::RegisterDataOutput(const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::string_view dataString, bool allowAddSignal)
    {
        if (!HasExecutionInput() && !HasExecutionOutput())
        {
            ResolveWrapperNode();
        }

        DataLogTreeItem* dataTreeItem = nullptr;

        for (int i = 0; i < GetChildCount(); ++i)
        {
            DataLogTreeItem* testLogItem = azrtti_cast<DataLogTreeItem*>(FindChildByRow(i));

            if (testLogItem && !testLogItem->HasOutput())
            {
                dataTreeItem = testLogItem;
                break;
            }
        }

        if (dataTreeItem == nullptr)
        {
            if (allowAddSignal)
            {
                dataTreeItem = CreateChildNode<DataLogTreeItem>(GetGraphIdentifier());
            }
            else
            {
                dataTreeItem = CreateChildNodeWithoutAddSignal<DataLogTreeItem>(GetGraphIdentifier());
            }
        }

        ScriptCanvas::Endpoint endpoint(m_scriptCanvasAssetNodeId, slotId);
        dataTreeItem->RegisterDataOutput(endpoint, slotName, dataString);
    }

    void ExecutionLogTreeItem::RegisterExecutionInput(const ScriptCanvas::Endpoint& incitingEndpoint, const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::chrono::milliseconds relativeExecution)
    {
        m_timeString = QTime::fromMSecsSinceStartOfDay(aznumeric_cast<int>(relativeExecution.count())).toString("mm:ss.zzz");

        m_inputSlot = slotId;
        m_inputName = slotName.data();

        SetIncitingEndpoint(incitingEndpoint);

        if (!HasExecutionOutput())
        {
            ResolveWrapperNode();
        }

        PopulateInputSlotData();

        SignalDataChanged();
    }

    bool ExecutionLogTreeItem::HasExecutionInput() const
    {
        return m_inputSlot.IsValid();
    }

    void ExecutionLogTreeItem::RegisterExecutionOutput(const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::chrono::milliseconds relativeExecution)
    {
        if (!HasExecutionInput())
        {
            m_timeString = QTime::fromMSecsSinceStartOfDay(aznumeric_cast<int>(relativeExecution.count())).toString("mm:ss.zzz");
        }

        m_outputSlot = slotId;
        m_outputName = slotName.data();

        if (!HasExecutionInput())
        {
            ResolveWrapperNode();
        }

        PopulateOutputSlotData();

        SignalDataChanged();
    }

    bool ExecutionLogTreeItem::HasExecutionOutput() const
    {
        return m_outputSlot.IsValid();
    }

    void ExecutionLogTreeItem::OnStylesUnloaded()
    {
        m_iconPixmap = nullptr;
    }

    void ExecutionLogTreeItem::OnStylesLoaded()
    {
        GraphCanvas::StyleManagerRequestBus::EventResult(m_iconPixmap, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetConfiguredPaletteIcon, m_paletteConfiguration);
        SignalDataChanged();
    }

    void ExecutionLogTreeItem::OnAssetVisualized()
    {
        ScrapeGraphCanvasData();

        for (int i = 0; i < GetChildCount(); ++i)
        {
            DataLogTreeItem* dataLogTreeItem = azrtti_cast<DataLogTreeItem*>(FindChildByRow(i));

            if (dataLogTreeItem)
            {
                dataLogTreeItem->ScrapeData();
            }
        }
    }

    void ExecutionLogTreeItem::OnAssetUnloaded()
    {
        EditorGraphNotificationBus::Handler::BusDisconnect();

        m_scriptCanvasNodeId.SetInvalid();

        m_graphCanvasGraphId.SetInvalid();
        m_graphCanvasNodeId.SetInvalid();

        for (int i = 0; i < GetChildCount(); ++i)
        {
            DataLogTreeItem* dataLogTreeItem = azrtti_cast<DataLogTreeItem*>(FindChildByRow(i));

            if (dataLogTreeItem)
            {
                dataLogTreeItem->InvalidateEditorIds();
            }
        }
    }

    void ExecutionLogTreeItem::OnGraphCanvasSceneDisplayed()
    {
        m_graphCanvasGraphId.SetInvalid();
        m_graphCanvasNodeId.SetInvalid();

        for (int i = 0; i < GetChildCount(); ++i)
        {
            DataLogTreeItem* dataLogTreeItem = azrtti_cast<DataLogTreeItem*>(FindChildByRow(i));

            if (dataLogTreeItem)
            {
                dataLogTreeItem->InvalidateGraphCanvasIds();
            }
        }

        ScrapeGraphCanvasData();
    }

    const ScriptCanvas::GraphIdentifier& ExecutionLogTreeItem::GetGraphIdentifier() const
    {
        return m_graphInfo.m_graphIdentifier;
    }

    AZ::Data::AssetId ExecutionLogTreeItem::GetAssetId() const
    {
        return m_graphInfo.m_graphIdentifier.m_assetId;
    }

    AZ::EntityId ExecutionLogTreeItem::GetScriptCanvasAssetNodeId() const
    {
        return m_scriptCanvasAssetNodeId;
    }

    GraphCanvas::NodeId ExecutionLogTreeItem::GetGraphCanvasNodeId() const
    {
        return m_graphCanvasNodeId;
    }

    bool ExecutionLogTreeItem::OnMatchesFilter(const DebugLogFilter& treeFilter)
    {
        bool matches = false;

        matches = matches || (m_displayName.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_inputName.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_outputName.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_graphName.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_sourceEntityName.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_timeString.lastIndexOf(treeFilter.m_filter) >= 0);

        return matches;
    }

    void ExecutionLogTreeItem::ResolveWrapperNode(bool refreshData)
    {
        if (m_graphCanvasNodeId.IsValid())
        {
            if (GraphCanvas::GraphUtils::IsWrapperNode(m_graphCanvasNodeId))
            {
                AZ::EntityId originalNodeId = m_graphCanvasNodeId;

                ScriptCanvas::SlotId slotId;

                if (HasExecutionInput())
                {
                    slotId = m_inputSlot;
                }

                if (HasExecutionOutput())
                {
                    slotId = m_outputSlot;
                }

                GraphCanvas::Endpoint endpoint;
                EBusHandlerNodeDescriptorRequestBus::EventResult(endpoint, m_graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, slotId);

                if (endpoint.IsValid())
                {
                    m_graphCanvasNodeId = endpoint.GetNodeId();
                }

                if (originalNodeId != m_graphCanvasNodeId && refreshData)
                {
                    ScrapeGraphCanvasData();
                }
            }
        }
    }

    void ExecutionLogTreeItem::ScrapeBehaviorContextData()
    {        
        if (m_graphName.isEmpty())
        {
            AZ::Data::AssetInfo assetInfo;

            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, GetAssetId());

            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(assetInfo.m_relativePath.c_str(), fileName);

            m_relativeGraphPath = assetInfo.m_relativePath.c_str();
            m_graphName = fileName.c_str();

            if (m_graphName.isEmpty())
            {
                m_graphName = "Unknown Canvas";
                m_relativeGraphPath = GetAssetId().ToString<AZStd::string>().c_str();
            }
        }

        const NodePaletteModelInformation* modelInformation = nullptr;
        GeneralRequestBus::BroadcastResult(modelInformation, &GeneralRequests::FindNodePaletteModelInformation, m_nodeType);

        if (modelInformation)
        {
            const CategoryInformation* categoryInformation = nullptr;
            GeneralRequestBus::BroadcastResult(categoryInformation, &GeneralRequests::FindNodePaletteCategoryInformation, modelInformation->m_categoryPath);

            m_displayName = QString(modelInformation->m_displayName.c_str());
            
            if (categoryInformation && categoryInformation->m_paletteOverride.compare(GraphCanvas::NodePaletteTreeItem::DefaultNodeTitlePalette) != 0)
            {
                m_paletteConfiguration.SetColorPalette(categoryInformation->m_paletteOverride);
            }
            else if (!modelInformation->m_titlePaletteOverride.empty())
            {
                m_paletteConfiguration.SetColorPalette(modelInformation->m_titlePaletteOverride);
            }
            else
            {
                m_paletteConfiguration.SetColorPalette(GraphCanvas::NodePaletteTreeItem::DefaultNodeTitlePalette);
            }
        }

        OnStylesLoaded();
        SignalDataChanged();
    }

    void ExecutionLogTreeItem::ScrapeGraphCanvasData()
    {
        if (!m_graphCanvasGraphId.IsValid())
        {
            GeneralRequestBus::BroadcastResult(m_graphCanvasGraphId
                , &GeneralRequests::FindGraphCanvasGraphIdByAssetId, SourceHandle(nullptr, GetAssetId().m_guid));

            if (!EditorGraphNotificationBus::Handler::BusIsConnected())
            {
                ScriptCanvas::ScriptCanvasId scriptCanvasId;
                GeneralRequestBus::BroadcastResult(scriptCanvasId
                    , &GeneralRequests::FindScriptCanvasIdByAssetId, SourceHandle(nullptr, GetAssetId().m_guid));

                EditorGraphNotificationBus::Handler::BusConnect(scriptCanvasId);
            }
        }

        if (m_graphCanvasGraphId.IsValid())
        {
            if (!m_graphCanvasNodeId.IsValid())
            {
                AssetGraphSceneBus::BroadcastResult(m_scriptCanvasNodeId
                    , &AssetGraphScene::FindEditorNodeIdByAssetNodeId
                    , SourceHandle(nullptr, GetAssetId().m_guid), m_scriptCanvasAssetNodeId);
                SceneMemberMappingRequestBus::EventResult(m_graphCanvasNodeId, m_scriptCanvasNodeId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);
            }

            if (m_graphCanvasNodeId.IsValid())
            {
                const bool refreshDisplayData = false;
                ResolveWrapperNode(refreshDisplayData);

                AZStd::string displayName;
                GraphCanvas::NodeTitleRequestBus::EventResult(displayName, m_graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::GetTitle);

                if (!displayName.empty())
                {
                    m_displayName = displayName.c_str();
                }

                GraphCanvas::NodeTitleRequestBus::Event(m_graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::ConfigureIconConfiguration, m_paletteConfiguration);

                OnStylesLoaded();

                PopulateInputSlotData();
                PopulateOutputSlotData();

                SignalDataChanged();
            }
        }
    }

    void ExecutionLogTreeItem::PopulateInputSlotData()
    {
        if (m_graphCanvasNodeId.IsValid() && HasExecutionInput())
        {
            GraphCanvas::SlotId slotId;
            SlotMappingRequestBus::EventResult(slotId, m_graphCanvasNodeId, &SlotMappingRequests::MapToGraphCanvasId, m_inputSlot);

            AZStd::string inputName;
            GraphCanvas::SlotRequestBus::EventResult(inputName, slotId, &GraphCanvas::SlotRequests::GetName);

            if (!inputName.empty())
            {
                m_inputName = inputName.c_str();
            }
        }
    }

    void ExecutionLogTreeItem::PopulateOutputSlotData()
    {
        if (m_graphCanvasNodeId.IsValid() && HasExecutionOutput())
        {
            GraphCanvas::SlotId slotId;
            SlotMappingRequestBus::EventResult(slotId, m_graphCanvasNodeId, &SlotMappingRequests::MapToGraphCanvasId, m_outputSlot);

            AZStd::string outputName;
            GraphCanvas::SlotRequestBus::EventResult(outputName, slotId, &GraphCanvas::SlotRequests::GetName);

            if (!outputName.empty())
            {
                m_outputName = outputName.c_str();
            }
        }
    }

    ////////////////////
    // DataLogTreeItem
    ////////////////////

    DataLogTreeItem::DataLogTreeItem(const ScriptCanvas::GraphIdentifier& graphIdentifier)
        : m_graphIdentifier(graphIdentifier)
    {
        m_inputName = "---";
        m_outputName = "---";

        ScrapeData();
    }

    QVariant DataLogTreeItem::Data(const QModelIndex& index, int role) const
    {
        switch (index.column())
        {
        case Column::Input:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTipRole)
            {
                if (m_inputData.isEmpty())
                {
                    return m_inputName;
                }

                return QString("%1 - (%2)").arg(m_inputName, m_inputData);
            }
        }
        break;
        case Column::Output:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTipRole)
            {
                if (m_outputData.isEmpty())
                {
                    return m_outputName;
                }

                return QString("%1 - (%2)").arg(m_outputName, m_outputData);
            }
        }
        break;
        default:
            break;
        }

        return QVariant();
    }

    void DataLogTreeItem::RegisterDataInput(const ScriptCanvas::Endpoint& incitingEndpoint, const ScriptCanvas::Endpoint& endpoint, AZStd::string_view slotName, AZStd::string_view dataString)
    {
        SetIncitingEndpoint(incitingEndpoint);

        m_assetInputEndpoint = endpoint;
        m_inputName = slotName.data();

        m_inputData = dataString.data();

        ScrapeInputName();
    }

    bool DataLogTreeItem::HasInput() const
    {
        return m_assetInputEndpoint.IsValid();
    }

    void DataLogTreeItem::RegisterDataOutput(const ScriptCanvas::Endpoint& endpoint, AZStd::string_view slotName, AZStd::string_view dataString)
    {
        m_assetOutputEndpoint = endpoint;
        m_outputName = slotName.data();

        m_outputData = dataString.data();

        ScrapeOutputName();
    }

    bool DataLogTreeItem::HasOutput() const
    {
        return m_assetOutputEndpoint.IsValid();
    }

    bool DataLogTreeItem::OnMatchesFilter(const DebugLogFilter& treeFilter)
    {
        bool matches = false;

        matches = matches || (m_inputName.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_inputData.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_outputName.lastIndexOf(treeFilter.m_filter) >= 0);
        matches = matches || (m_outputData.lastIndexOf(treeFilter.m_filter) >= 0);

        return matches;
    }

    AZ::Data::AssetId DataLogTreeItem::GetAssetId() const
    {
        return m_graphIdentifier.m_assetId;
    }

    void DataLogTreeItem::ScrapeData()
    {
        if (!m_graphCanvasGraphId.IsValid())
        {
            GeneralRequestBus::BroadcastResult(m_graphCanvasGraphId
                , &GeneralRequests::FindGraphCanvasGraphIdByAssetId, SourceHandle(nullptr, m_graphIdentifier.m_assetId.m_guid));
        }

        ScrapeInputName();
        ScrapeOutputName();
    }

    void DataLogTreeItem::InvalidateEditorIds()
    {
        InvalidateGraphCanvasIds();
    }

    void DataLogTreeItem::InvalidateGraphCanvasIds()
    {
        m_graphCanvasGraphId.SetInvalid();
    }

    void DataLogTreeItem::ScrapeInputName()
    {
        if (m_graphCanvasGraphId.IsValid() && m_assetInputEndpoint.IsValid())
        {
            AZ::EntityId scriptCanvasNodeId;
            AssetGraphSceneBus::BroadcastResult(scriptCanvasNodeId, &AssetGraphScene::FindEditorNodeIdByAssetNodeId, SourceHandle(nullptr, GetAssetId().m_guid), m_assetInputEndpoint.GetNodeId());

            GraphCanvas::NodeId graphCanvasNodeId;
            SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, scriptCanvasNodeId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

            GraphCanvas::SlotId slotId;
            SlotMappingRequestBus::EventResult(slotId, graphCanvasNodeId, &SlotMappingRequests::MapToGraphCanvasId, m_assetInputEndpoint.GetSlotId());

            AZStd::string name;
            GraphCanvas::SlotRequestBus::EventResult(name, slotId, &GraphCanvas::SlotRequests::GetName);

            if (!name.empty())
            {
                m_inputName = name.c_str();
            }
        }
    }

    void DataLogTreeItem::ScrapeOutputName()
    {
        if (m_graphCanvasGraphId.IsValid() && m_assetOutputEndpoint.IsValid())
        {
            AZ::EntityId scriptCanvasNodeId;
            AssetGraphSceneBus::BroadcastResult(scriptCanvasNodeId, &AssetGraphScene::FindEditorNodeIdByAssetNodeId
                , SourceHandle(nullptr, GetAssetId().m_guid), m_assetOutputEndpoint.GetNodeId());
            
            GraphCanvas::NodeId graphCanvasNodeId;
            SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, scriptCanvasNodeId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

            GraphCanvas::SlotId slotId;
            SlotMappingRequestBus::EventResult(slotId, graphCanvasNodeId, &SlotMappingRequests::MapToGraphCanvasId, m_assetOutputEndpoint.GetSlotId());

            AZStd::string name;
            GraphCanvas::SlotRequestBus::EventResult(name, slotId, &GraphCanvas::SlotRequests::GetName);

            if (!name.empty())
            {
                m_outputName = name.c_str();
            }
        }
    }

    bool DataLogTreeItem::LessThan(const GraphCanvas::GraphCanvasTreeItem* graphItem) const
    {
        return !azrtti_istypeof<const NodeAnnotationTreeItem*>(graphItem);
    }

    ///////////////////////////
    // NodeAnnotationTreeItem
    ///////////////////////////

    NodeAnnotationTreeItem::NodeAnnotationTreeItem()
        : m_annotationLevel(ScriptCanvas::AnnotateNodeSignal::AnnotationLevel::Info)
    {
    }

    NodeAnnotationTreeItem::NodeAnnotationTreeItem(ScriptCanvas::AnnotateNodeSignal::AnnotationLevel annotationLevel, const AZStd::string& annotation)
        : m_annotationLevel(annotationLevel)
        , m_annotation(annotation.c_str())
    {
        switch (m_annotationLevel)
        {
        case ScriptCanvas::AnnotateNodeSignal::AnnotationLevel::Info:
            m_annotationIcon = QIcon(":/ScriptCanvasEditorResources/Resources/message_icon.png");
            break;
        case ScriptCanvas::AnnotateNodeSignal::AnnotationLevel::Warning:
            m_annotationIcon = QIcon(":/ScriptCanvasEditorResources/Resources/warning_symbol.png");
            break;
        case ScriptCanvas::AnnotateNodeSignal::AnnotationLevel::Error:
            m_annotationIcon = QIcon(":/ScriptCanvasEditorResources/Resources/error_icon.png");
            break;
        default:
            break;
        }
    }

    QVariant NodeAnnotationTreeItem::Data(const QModelIndex& index, int role) const
    {
        // We are spanned, we we only have a single column
        if (index.column() == DebugLogTreeItem::Column::NodeName)
        {
            switch (role)
            {
            case Qt::DecorationRole:
            {
                return m_annotationIcon;
            }
            case Qt::DisplayRole:
            {
                return m_annotation;
            }
            case Qt::ToolTipRole:
            {
                return m_annotation;
            }
            default:
                break;
            }
        }

        return QVariant();
    }

    bool NodeAnnotationTreeItem::OnMatchesFilter(const DebugLogFilter& treeFilter)
    {
        bool matches = false;

        matches = matches || (m_annotation.lastIndexOf(treeFilter.m_filter) >= 0);

        return matches;
    }
}

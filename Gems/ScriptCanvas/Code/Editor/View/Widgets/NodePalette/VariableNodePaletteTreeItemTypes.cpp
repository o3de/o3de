/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <QCoreApplication>
#include <qmenu.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

#include "VariableNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeCreateUtils.h"
#include "Editor/Translation/TranslationHelper.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"

#include <Core/Attributes.h>
#include <Libraries/Core/Method.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////////
    // CreateGetVariableNodeMimeEvent
    ///////////////////////////////////

    void CreateGetVariableNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateGetVariableNodeMimeEvent, CreateNodeMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateGetVariableNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateGetVariableNodeMimeEvent::CreateGetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId)
        : m_variableId(variableId)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateGetVariableNodeMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        return Nodes::CreateGetVariableNode(m_variableId, scriptCanvasId);
    }

    ///////////////////////////////////
    // GetVariableNodePaletteTreeItem
    ///////////////////////////////////

    const QString& GetVariableNodePaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(AZ::Uuid()).c_str();
        }

        return defaultIcon;
    }

    GetVariableNodePaletteTreeItem::GetVariableNodePaletteTreeItem()
        : DraggableNodePaletteTreeItem("Get Variable", ScriptCanvasEditor::AssetEditorId)
    {
        SetToolTip("After specifying a variable name, this node will expose output slots that return the specified variable's values.\nVariable names must begin with # (for example, #MyVar).");
    }

    GetVariableNodePaletteTreeItem::GetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        : DraggableNodePaletteTreeItem("", ScriptCanvasEditor::AssetEditorId)
        , m_variableId(variableId)
    {
        AZStd::string_view variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
        OnVariableRenamed(variableName);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(ScriptCanvas::GraphScopedVariableId(scriptCanvasId, m_variableId));

        ScriptCanvas::Data::Type scriptCanvasType = ScriptCanvas::Data::Type::Invalid();        
        ScriptCanvas::VariableRequestBus::EventResult(scriptCanvasType, ScriptCanvas::GraphScopedVariableId(scriptCanvasId, m_variableId), &ScriptCanvas::VariableRequests::GetType);

        if (scriptCanvasType.IsValid())
        {
            AZ::Uuid azType = ScriptCanvas::Data::ToAZType(scriptCanvasType);

            AZStd::string colorPalette;
            GraphCanvas::StyleManagerRequestBus::EventResult(colorPalette, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataPaletteStyle, azType);

            SetTitlePalette(colorPalette);
        }
    }

    GetVariableNodePaletteTreeItem::~GetVariableNodePaletteTreeItem()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void GetVariableNodePaletteTreeItem::OnVariableRenamed(AZStd::string_view variableName)
    {
        AZStd::string fullName = AZStd::string::format("Get %s", variableName.data());
        SetName(fullName.c_str());

        AZStd::string tooltip = AZStd::string::format("This node returns %s's values", variableName.data());
        SetToolTip(tooltip.c_str());
    }

    const ScriptCanvas::VariableId& GetVariableNodePaletteTreeItem::GetVariableId() const
    {
        return m_variableId;
    }

    GraphCanvas::GraphCanvasMimeEvent* GetVariableNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateGetVariableNodeMimeEvent(m_variableId);
    }

    ///////////////////////////////////
    // CreateSetVariableNodeMimeEvent
    ///////////////////////////////////

    void CreateSetVariableNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateSetVariableNodeMimeEvent, CreateNodeMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateSetVariableNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateSetVariableNodeMimeEvent::CreateSetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId)
        : m_variableId(variableId)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateSetVariableNodeMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        return Nodes::CreateSetVariableNode(m_variableId, scriptCanvasId);
    }

    ///////////////////////////////////
    // SetVariableNodePaletteTreeItem
    ///////////////////////////////////

    const QString& SetVariableNodePaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(AZ::Uuid()).c_str();
        }

        return defaultIcon;
    }

    SetVariableNodePaletteTreeItem::SetVariableNodePaletteTreeItem()
        : GraphCanvas::DraggableNodePaletteTreeItem("Set Variable", ScriptCanvasEditor::AssetEditorId)
    {
        SetToolTip("This node changes a variable's values according to the data connected to the input slots");
    }

    SetVariableNodePaletteTreeItem::SetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        : GraphCanvas::DraggableNodePaletteTreeItem("", ScriptCanvasEditor::AssetEditorId)
        , m_variableId(variableId)
    {
        AZStd::string_view variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
        OnVariableRenamed(variableName);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(ScriptCanvas::GraphScopedVariableId(scriptCanvasId, m_variableId));

        ScriptCanvas::Data::Type scriptCanvasType = ScriptCanvas::Data::Type::Invalid();
        ScriptCanvas::VariableRequestBus::EventResult(scriptCanvasType, ScriptCanvas::GraphScopedVariableId(scriptCanvasId, m_variableId), &ScriptCanvas::VariableRequests::GetType);

        if (scriptCanvasType.IsValid())
        {
            AZ::Uuid azType = ScriptCanvas::Data::ToAZType(scriptCanvasType);

            AZStd::string colorPalette;
            GraphCanvas::StyleManagerRequestBus::EventResult(colorPalette, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataPaletteStyle, azType);

            SetTitlePalette(colorPalette);
        }
    }

    SetVariableNodePaletteTreeItem::~SetVariableNodePaletteTreeItem()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void SetVariableNodePaletteTreeItem::OnVariableRenamed(AZStd::string_view variableName)
    {
        AZStd::string fullName = AZStd::string::format("Set %s", variableName.data());
        SetName(fullName.c_str());

        AZStd::string tooltip = AZStd::string::format("This node changes %s's values according to the data connected to the input slots", variableName.data());
        SetToolTip(tooltip.c_str());
    }

    const ScriptCanvas::VariableId& SetVariableNodePaletteTreeItem::GetVariableId() const
    {
        return m_variableId;
    }

    GraphCanvas::GraphCanvasMimeEvent* SetVariableNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateSetVariableNodeMimeEvent(m_variableId);
    }

    ///////////////////////////////////////
    // CreateVariableChangedNodeMimeEvent
    ///////////////////////////////////////

    void CreateVariableChangedNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateVariableChangedNodeMimeEvent, CreateEBusHandlerEventMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateVariableChangedNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateVariableChangedNodeMimeEvent::CreateVariableChangedNodeMimeEvent(const ScriptCanvas::VariableId& variableId)
        : m_variableId(variableId)
    {        
    }

    bool CreateVariableChangedNodeMimeEvent::ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ConfigureEBusEvent();

        NodeIdPair nodeIdPair = CreateEBusHandlerEventMimeEvent::ExecuteEventImpl(mousePosition, sceneDropPosition, graphCanvasGraphId);

        ScriptCanvas::GraphScopedVariableId scopedVariableId(ScriptCanvas::ScriptCanvasId(), m_variableId);

        ScriptCanvas::Datum idDatum(ScriptCanvas::Data::FromAZType(azrtti_typeid<ScriptCanvas::GraphScopedVariableId>()), ScriptCanvas::Datum::eOriginality::Original);
        idDatum.Set(AZStd::move(scopedVariableId));

        EBusHandlerEventNodeDescriptorRequestBus::Event(nodeIdPair.m_graphCanvasId, &EBusHandlerEventNodeDescriptorRequests::SetHandlerAddress, idDatum);

        return nodeIdPair.m_graphCanvasId.IsValid();
    }

    NodeIdPair CreateVariableChangedNodeMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        ConfigureEBusEvent();

        NodeIdPair nodeIdPair = CreateEBusHandlerEventMimeEvent::ConstructNode(graphCanvasGraphId, scenePosition);

        ScriptCanvas::GraphScopedVariableId scopedVariableId(ScriptCanvas::ScriptCanvasId(), m_variableId);        

        ScriptCanvas::Datum idDatum(ScriptCanvas::Data::FromAZType(azrtti_typeid<ScriptCanvas::GraphScopedVariableId>()), ScriptCanvas::Datum::eOriginality::Original);
        idDatum.Set(AZStd::move(scopedVariableId));

        EBusHandlerEventNodeDescriptorRequestBus::Event(nodeIdPair.m_graphCanvasId, &EBusHandlerEventNodeDescriptorRequests::SetHandlerAddress, idDatum);

        return nodeIdPair;
    }

    void CreateVariableChangedNodeMimeEvent::ConfigureEBusEvent()
    {
        if (GetBusName().empty())
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

            if (behaviorContext)
            {
                auto busIter = behaviorContext->m_ebuses.find(ScriptCanvas::GraphVariable::GetVariableNotificationBusName());

                if (busIter != behaviorContext->m_ebuses.end())
                {
                    if (busIter->second->m_createHandler)
                    {
                        AZ::BehaviorEBusHandler* handler(nullptr);
                        if (busIter->second->m_createHandler->InvokeResult(handler) && handler)
                        {
                            const AZStd::vector<AZ::BehaviorEBusHandler::BusForwarderEvent>& events = handler->GetEvents();

                            for (auto forwarderEvent : events)
                            {
                                if (strcmp(forwarderEvent.m_name, ScriptCanvas::k_OnVariableWriteEventName) == 0)
                                {
                                    ConfigureEvent(busIter->second->m_name, forwarderEvent.m_name, forwarderEvent.m_eventId);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ///////////////////////////////////////
    // VariableChangedNodePaletteTreeItem
    ///////////////////////////////////////

    const QString& VariableChangedNodePaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(AZ::Uuid()).c_str();
        }

        return defaultIcon;
    }

    VariableChangedNodePaletteTreeItem::VariableChangedNodePaletteTreeItem()
        : GraphCanvas::DraggableNodePaletteTreeItem("On Variable Changed", ScriptCanvasEditor::AssetEditorId)
    {
        SetToolTip("This node changes a variable's values according to the data connected to the input slots");
    }

    VariableChangedNodePaletteTreeItem::VariableChangedNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        : GraphCanvas::DraggableNodePaletteTreeItem("", ScriptCanvasEditor::AssetEditorId)
        , m_variableId(variableId)
    {
        AZStd::string_view variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
        OnVariableRenamed(variableName);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(ScriptCanvas::GraphScopedVariableId(scriptCanvasId, m_variableId));

        ScriptCanvas::Data::Type scriptCanvasType = ScriptCanvas::Data::Type::Invalid();
        ScriptCanvas::VariableRequestBus::EventResult(scriptCanvasType, ScriptCanvas::GraphScopedVariableId(scriptCanvasId, m_variableId), &ScriptCanvas::VariableRequests::GetType);

        if (scriptCanvasType.IsValid())
        {
            AZ::Uuid azType = ScriptCanvas::Data::ToAZType(scriptCanvasType);

            AZStd::string colorPalette;
            GraphCanvas::StyleManagerRequestBus::EventResult(colorPalette, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataPaletteStyle, azType);

            SetTitlePalette(colorPalette);
        }
    }

    VariableChangedNodePaletteTreeItem::~VariableChangedNodePaletteTreeItem()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void VariableChangedNodePaletteTreeItem::OnVariableRenamed(AZStd::string_view variableName)
    {
        AZStd::string fullName = AZStd::string::format("On %s Changed", variableName.data());
        SetName(fullName.c_str());

        AZStd::string tooltip = AZStd::string::format("Signals when %s's values changes.", variableName.data());
        SetToolTip(tooltip.c_str());
    }

    const ScriptCanvas::VariableId& VariableChangedNodePaletteTreeItem::GetVariableId() const
    {
        return m_variableId;
    }

    GraphCanvas::GraphCanvasMimeEvent* VariableChangedNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateVariableChangedNodeMimeEvent(m_variableId);
    }

    ////////////////////////////////////////
    // CreateVariableSpecificNodeMimeEvent
    ////////////////////////////////////////

    void CreateVariableSpecificNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateVariableSpecificNodeMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateVariableSpecificNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateVariableSpecificNodeMimeEvent::CreateVariableSpecificNodeMimeEvent(const ScriptCanvas::VariableId& variableId)
        : m_variableId(variableId)
    {
    }

    bool CreateVariableSpecificNodeMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        NodeIdPair nodeId = ConstructNode(graphCanvasGraphId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ScriptCanvasEditor::NodeIdPair CreateVariableSpecificNodeMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        ScriptCanvasEditor::NodeIdPair nodeIdPair;

        AZ::EntityId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
        GraphCanvas::ViewRequestBus::EventResult(graphicsView, viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

        if (graphicsView)
        {
            AZStd::string variableName;
            ScriptCanvas::VariableRequestBus::EventResult(variableName, ScriptCanvas::GraphScopedVariableId(scriptCanvasId, m_variableId), &ScriptCanvas::VariableRequests::GetName);

            QMenu menu(graphicsView);

            QAction* createGet = new QAction(QString("Get %1").arg(variableName.c_str()), &menu);
            menu.addAction(createGet);

            QAction* createChanged = new QAction(QString("On %1 Changed").arg(variableName.c_str()), &menu);
            menu.addAction(createChanged);

            QAction* createSet = new QAction(QString("Set %1").arg(variableName.c_str()), &menu);
            menu.addAction(createSet);            

            QAction* result = menu.exec(QCursor::pos());

            if (result == createGet)
            {
                CreateGetVariableNodeMimeEvent createGetVariableNode(m_variableId);
                nodeIdPair = createGetVariableNode.CreateNode(scriptCanvasId);
            }
            else if (result == createSet)
            {
                CreateSetVariableNodeMimeEvent createSetVariableNode(m_variableId);
                nodeIdPair = createSetVariableNode.CreateNode(scriptCanvasId);
            }
            else if (result == createChanged)
            {
                CreateVariableChangedNodeMimeEvent createChangedVariableNode(m_variableId);
                nodeIdPair = createChangedVariableNode.ConstructNode(graphCanvasGraphId, scenePosition);
            }

            if (nodeIdPair.m_graphCanvasId.IsValid() && nodeIdPair.m_scriptCanvasId.IsValid())
            {
                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodeIdPair.m_graphCanvasId, scenePosition, false);
                GraphCanvas::SceneMemberUIRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
            }
        }

        return nodeIdPair;
    }

    AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > CreateVariableSpecificNodeMimeEvent::CreateMimeEvents() const
    {
        AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > mimeEvents;

        mimeEvents.push_back(aznew CreateGetVariableNodeMimeEvent(m_variableId));
        mimeEvents.push_back(aznew CreateSetVariableNodeMimeEvent(m_variableId));
        mimeEvents.push_back(aznew CreateVariableChangedNodeMimeEvent(m_variableId));

        return mimeEvents;
    }

    ////////////////////////////////////////
    // VariableCategoryNodePaletteTreeItem
    ////////////////////////////////////////

    VariableCategoryNodePaletteTreeItem::VariableCategoryNodePaletteTreeItem(AZStd::string_view displayName)
        : NodePaletteTreeItem(displayName, ScriptCanvasEditor::AssetEditorId)
    {
    }

    void VariableCategoryNodePaletteTreeItem::PreOnChildAdded(GraphCanvasTreeItem* item)
    {
        // Force elements to display in the order they were added rather then alphabetical.
        static_cast<NodePaletteTreeItem*>(item)->SetItemOrdering(GetChildCount());
    }

    //////////////////////////////////////////
    // LocalVariablesListNodePaletteTreeItem
    //////////////////////////////////////////

    LocalVariablesListNodePaletteTreeItem::LocalVariablesListNodePaletteTreeItem(AZStd::string_view displayName)
        : NodePaletteTreeItem(displayName, ScriptCanvasEditor::AssetEditorId)
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        SetAllowPruneOnEmpty(false);
    }

    void LocalVariablesListNodePaletteTreeItem::OnActiveGraphChanged(const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        if (m_scriptCanvasId != scriptCanvasId)
        {
            if (m_scriptCanvasId.IsValid())
            {
                GraphItemCommandNotificationBus::Handler::BusDisconnect(m_scriptCanvasId);
                ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect(m_scriptCanvasId);
             }

            m_scriptCanvasId = scriptCanvasId;

            if (m_scriptCanvasId.IsValid())
            {
                ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusConnect(m_scriptCanvasId);
                GraphItemCommandNotificationBus::Handler::BusConnect(m_scriptCanvasId);
            }

            RefreshVariableList();
        }
    }

    void LocalVariablesListNodePaletteTreeItem::PostRestore(const UndoData&)
    {
        RefreshVariableList();
    }

    void LocalVariablesListNodePaletteTreeItem::OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        QScopedValueRollback<bool> valueRollback(m_ignoreTreeSignals, true);

        LocalVariableNodePaletteTreeItem* localVariableTreeItem = CreateChildNode<LocalVariableNodePaletteTreeItem>(variableId, m_scriptCanvasId);
        localVariableTreeItem->PopulateChildren();
    }

    void LocalVariablesListNodePaletteTreeItem::OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int rows = GetChildCount();

        for (int i = 0; i < rows; ++i)
        {
            LocalVariableNodePaletteTreeItem* treeItem = static_cast<LocalVariableNodePaletteTreeItem*>(FindChildByRow(i));

            if (treeItem->GetVariableId() == variableId)
            {
                QScopedValueRollback<bool> valueRollback(m_ignoreTreeSignals, true);
                RemoveChild(treeItem);
                
                break;
            }
        }
    }

    void LocalVariablesListNodePaletteTreeItem::OnChildAdded(GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        if (!m_ignoreTreeSignals)
        {
            m_nonVariableTreeItems.insert(treeItem);
        }
    }

    void LocalVariablesListNodePaletteTreeItem::RefreshVariableList()
    {
        QScopedValueRollback<bool> valueRollback(m_ignoreTreeSignals, true);

        for (GraphCanvas::GraphCanvasTreeItem* item : m_nonVariableTreeItems)
        {
            item->DetachItem();
        }

        // Need to let the child clear signal out
        ClearChildren();

        const ScriptCanvas::GraphVariableMapping* variableMapping = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableMapping, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariables);

        if (variableMapping != nullptr)
        {
            for (const auto& mapPair : (*variableMapping))
            {
                LocalVariableNodePaletteTreeItem* rootItem = this->CreateChildNode<LocalVariableNodePaletteTreeItem>(mapPair.first, m_scriptCanvasId);
                rootItem->PopulateChildren();
            }
        }

        for (GraphCanvas::GraphCanvasTreeItem* item : m_nonVariableTreeItems)
        {
            AddChild(item);
        }
    }

    /////////////////////////////////////
    // LocalVariableNodePaletteTreeItem
    /////////////////////////////////////

    LocalVariableNodePaletteTreeItem::LocalVariableNodePaletteTreeItem(ScriptCanvas::VariableId variableId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        : NodePaletteTreeItem("", ScriptCanvasEditor::AssetEditorId)
        , m_scriptCanvasId(scriptCanvasId)
        , m_variableId(variableId)
    {
        AZStd::string_view variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
        OnVariableRenamed(variableName);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(ScriptCanvas::GraphScopedVariableId(scriptCanvasId, variableId));
    }

    LocalVariableNodePaletteTreeItem::~LocalVariableNodePaletteTreeItem()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void LocalVariableNodePaletteTreeItem::PopulateChildren()
    {
        if (GetChildCount() == 0)
        {
            CreateChildNode<GetVariableNodePaletteTreeItem>(GetVariableId(), m_scriptCanvasId);
            CreateChildNode<SetVariableNodePaletteTreeItem>(GetVariableId(), m_scriptCanvasId);
            CreateChildNode<VariableChangedNodePaletteTreeItem>(GetVariableId(), m_scriptCanvasId);
        }
    }

    const ScriptCanvas::VariableId& LocalVariableNodePaletteTreeItem::GetVariableId() const
    {
        return m_variableId;
    }

    void LocalVariableNodePaletteTreeItem::OnVariableRenamed(AZStd::string_view variableName)
    {
        AZStd::string localName(variableName);        
        SetName(localName.c_str());
    }
}

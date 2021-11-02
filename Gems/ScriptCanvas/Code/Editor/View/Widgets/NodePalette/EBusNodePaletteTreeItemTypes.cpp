/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <QCoreApplication>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include "EBusNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeCreateUtils.h"
#include "Editor/Nodes/NodeDisplayUtils.h"
#include "Editor/Translation/TranslationHelper.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"
#include "Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h"

#include <Core/Attributes.h>
#include <Libraries/Core/EBusEventHandler.h>
#include <Libraries/Core/MethodOverloaded.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////
    // CreateEBusSenderMimeEvent
    //////////////////////////////

    void CreateEBusSenderMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateEBusSenderMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("BusName", &CreateEBusSenderMimeEvent::m_busName)
                ->Field("EventName", &CreateEBusSenderMimeEvent::m_eventName)
                ->Field("IsOverload", &CreateEBusSenderMimeEvent::m_isOverload)
                ->Field("propertyStatus", &CreateEBusSenderMimeEvent::m_propertyStatus)
                ;
        }
    }

    CreateEBusSenderMimeEvent::CreateEBusSenderMimeEvent(AZStd::string_view busName, AZStd::string_view eventName, bool isOverload, ScriptCanvas::PropertyStatus propertyStatus)
        : m_busName(busName.data())
        , m_eventName(eventName.data())
        , m_isOverload(isOverload)
        , m_propertyStatus(propertyStatus)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateEBusSenderMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        if (m_isOverload)
        {
            return Nodes::CreateObjectMethodOverloadNode(m_busName, m_eventName, scriptCanvasId);
        }
        else
        {
            return Nodes::CreateObjectMethodNode(m_busName, m_eventName, scriptCanvasId, m_propertyStatus);
        }
    }

    /////////////////////////////////
    // EBusSendEventPaletteTreeItem
    /////////////////////////////////

    const QString& EBusSendEventPaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type()).c_str();
        }

        return defaultIcon;
    }

    EBusSendEventPaletteTreeItem::EBusSendEventPaletteTreeItem(AZStd::string_view busName, AZStd::string_view eventName, const ScriptCanvas::EBusBusId& busIdentifier, const ScriptCanvas::EBusEventId& eventIdentifier, bool isOverload, ScriptCanvas::PropertyStatus propertyStatus)
        : DraggableNodePaletteTreeItem(eventName, ScriptCanvasEditor::AssetEditorId)
        , m_busName(busName.data())
        , m_eventName(eventName.data())
        , m_busId(busIdentifier)
        , m_eventId(eventIdentifier)
        , m_isOverload(isOverload)
        , m_propertyStatus(propertyStatus)
    {
        GraphCanvas::TranslationKey key;
        key << "EBusSender" << busName << "methods" << eventName << "details";

        GraphCanvas::TranslationRequests::Details details;
        details.Name = eventName;
        details.Subtitle = busName;

        GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

        SetName(details.Name.c_str());
        SetToolTip(details.Tooltip.c_str());

        SetTitlePalette("MethodNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* EBusSendEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateEBusSenderMimeEvent(m_busName.toUtf8().data(), m_eventName.toUtf8().data(), m_isOverload, ScriptCanvas::PropertyStatus::None);
    }

    AZStd::string EBusSendEventPaletteTreeItem::GetBusName() const
    {
        return m_busName.toUtf8().data();
    }

    AZStd::string EBusSendEventPaletteTreeItem::GetEventName() const
    {
        return m_eventName.toUtf8().data();
    }

    ScriptCanvas::EBusBusId EBusSendEventPaletteTreeItem::GetBusId() const
    {
        return m_busId;
    }

    ScriptCanvas::EBusEventId EBusSendEventPaletteTreeItem::GetEventId() const
    {
        return m_eventId;
    }

    ScriptCanvas::PropertyStatus EBusSendEventPaletteTreeItem::GetPropertyStatus() const
    {
        return m_propertyStatus;
    }

    bool EBusSendEventPaletteTreeItem::IsOverload() const 
    {
        return m_isOverload;
    }

    ///////////////////////////////
    // CreateEBusHandlerMimeEvent
    ///////////////////////////////

    void CreateEBusHandlerMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateEBusHandlerMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("BusName", &CreateEBusHandlerMimeEvent::m_busName)
                ;
        }
    }

    CreateEBusHandlerMimeEvent::CreateEBusHandlerMimeEvent(AZStd::string_view busName)
        : m_busName(busName.data())
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateEBusHandlerMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        return Nodes::CreateEbusWrapperNode(m_busName, scriptCanvasId);
    }

    ////////////////////////////////////
    // CreateEBusHandlerEventMimeEvent
    ////////////////////////////////////

    void CreateEBusHandlerEventMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateEBusHandlerEventMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(0)
                ->Field("BusName", &CreateEBusHandlerEventMimeEvent::m_busName)
                ->Field("EventName", &CreateEBusHandlerEventMimeEvent::m_eventName)
                ->Field("EventId", &CreateEBusHandlerEventMimeEvent::m_eventId)
                ;
        }
    }

    CreateEBusHandlerEventMimeEvent::CreateEBusHandlerEventMimeEvent(AZStd::string_view busName, AZStd::string_view eventName, const ScriptCanvas::EBusEventId& eventId)
        : m_busName(busName)
        , m_eventName(eventName)
        , m_eventId(eventId)
    {
    }

    NodeIdPair CreateEBusHandlerEventMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair eventNode = CreateEventNode(graphCanvasGraphId, scenePosition);

        CreateEBusHandlerMimeEvent ebusMimeEvent(m_busName);

        AZ::Vector2 position = scenePosition;
        if (ebusMimeEvent.ExecuteEvent(position, position, graphCanvasGraphId))
        {
            NodeIdPair handlerNode = ebusMimeEvent.GetCreatedPair();

            GraphCanvas::WrappedNodeConfiguration configuration;
            EBusHandlerNodeDescriptorRequestBus::EventResult(configuration, handlerNode.m_graphCanvasId, &EBusHandlerNodeDescriptorRequests::GetEventConfiguration, m_eventId);

            GraphCanvas::WrapperNodeRequestBus::Event(handlerNode.m_graphCanvasId, &GraphCanvas::WrapperNodeRequests::WrapNode, eventNode.m_graphCanvasId, configuration);
        }

        return eventNode;
    }

    bool CreateEBusHandlerEventMimeEvent::ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        return ExecuteEventImpl(mousePosition, sceneDropPosition, graphCanvasGraphId).m_graphCanvasId.IsValid();
    }

    NodeIdPair CreateEBusHandlerEventMimeEvent::CreateEventNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) const
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        NodeIdPair nodeIdPair;
        nodeIdPair.m_graphCanvasId = Nodes::DisplayEbusEventNode(graphCanvasGraphId, m_busName, m_eventName, m_eventId);

        if (nodeIdPair.m_graphCanvasId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodeIdPair.m_graphCanvasId, scenePosition, false);
        }

        return nodeIdPair;
    }

    NodeIdPair CreateEBusHandlerEventMimeEvent::ExecuteEventImpl([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        NodeIdPair eventNode = CreateEventNode(graphCanvasGraphId, sceneDropPosition);

        if (eventNode.m_graphCanvasId.IsValid())
        {
            GraphCanvas::SceneMemberUIRequestBus::Event(eventNode.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

            ScriptCanvasEditor::NodeCreationNotificationBus::Event(scriptCanvasId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, eventNode.m_graphCanvasId);

            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return eventNode;
    }
    
    void CreateEBusHandlerEventMimeEvent::ConfigureEvent(AZStd::string_view busName, AZStd::string_view eventName, const ScriptCanvas::EBusEventId& eventId)
    {
        m_busName = busName;
        m_eventName = eventName;
        m_eventId = eventId;
    }

    ///////////////////////////////////
    // EBusHandleEventPaletteTreeItem
    ///////////////////////////////////

    const QString& EBusHandleEventPaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(ScriptCanvas::Nodes::Core::Method::RTTI_Type()).c_str();
        }

        return defaultIcon;
    }

    EBusHandleEventPaletteTreeItem::EBusHandleEventPaletteTreeItem(AZStd::string_view busName, AZStd::string_view eventName, const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId)
        : DraggableNodePaletteTreeItem(eventName, ScriptCanvasEditor::AssetEditorId)
        , m_busName(busName)
        , m_eventName(eventName)
        , m_busId(busId)
        , m_eventId(eventId)
    {
        GraphCanvas::TranslationKey key;
        key << "EBusHandler" << busName << "methods" << eventName << "details";

        GraphCanvas::TranslationRequests::Details details;
        details.Name = m_eventName;
        GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);
        if (details.Name.empty())
        {
            details.Name = m_eventName;
        }

        SetName(details.Name.c_str());
        SetToolTip(details.Tooltip.c_str());

        SetTitlePalette("HandlerNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* EBusHandleEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateEBusHandlerEventMimeEvent(m_busName, m_eventName, m_eventId);
    }

    AZStd::string EBusHandleEventPaletteTreeItem::GetBusName() const
    {
        return m_busName;
    }

    AZStd::string EBusHandleEventPaletteTreeItem::GetEventName() const
    {
        return m_eventName;
    }

    ScriptCanvas::EBusBusId EBusHandleEventPaletteTreeItem::GetBusId() const
    {
        return m_busId;
    }

    ScriptCanvas::EBusEventId EBusHandleEventPaletteTreeItem::GetEventId() const
    {
        return m_eventId;
    }
}

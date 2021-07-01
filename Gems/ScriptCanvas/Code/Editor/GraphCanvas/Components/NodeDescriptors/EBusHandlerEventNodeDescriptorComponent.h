/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include "Editor/Include/ScriptCanvas/Bus/NodeIdPair.h"

#include "Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    class EBusHandlerEventNodeDescriptorComponent
        : public NodeDescriptorComponent        
        , public EBusHandlerEventNodeDescriptorRequestBus::Handler
        , public GraphCanvas::ForcedWrappedNodeRequestBus::Handler
        , public GraphCanvas::SceneMemberNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EBusHandlerEventNodeDescriptorComponent, "{F08F673C-0815-4CCA-AB9D-21965E9A14F2}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        EBusHandlerEventNodeDescriptorComponent();
        EBusHandlerEventNodeDescriptorComponent(const AZStd::string& busName, const AZStd::string& methodName, ScriptCanvas::EBusEventId eventId);
        ~EBusHandlerEventNodeDescriptorComponent() = default;

        void Activate() override;
        void Deactivate() override;

        // EBusHandlerEventNodeDescriptorRequestBus
        AZStd::string_view GetBusName() const override;
        AZStd::string_view GetEventName() const override;

        ScriptCanvas::EBusEventId GetEventId() const override;

        void SetHandlerAddress(const ScriptCanvas::Datum& idDatum) override;
        ////

        // NodeNotificationBus::Handler          
        void OnNodeWrapped(const AZ::EntityId& wrappingNode) override;
        ////

        // SceneMemberNotifications
        void OnSceneMemberAboutToSerialize(GraphCanvas::GraphSerialization& graphSerialization) override;
        ////

        // ForcedWrappedNodeRequestBus
        AZ::Crc32 GetWrapperType() const override;
        AZ::Crc32 GetIdentifier() const override;

        AZ::EntityId CreateWrapperNode(const AZ::EntityId& sceneId, const AZ::Vector2& nodePosition) override;
        ////

    protected:

        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& sceneId, const AZ::EntityId& scriptCanvasNodeId) override;

    private:        

        NodeIdPair    m_ebusWrapper;
    
        AZStd::string m_busName;
        AZStd::string m_eventName;
        ScriptCanvas::EBusEventId m_eventId;

        ScriptCanvas::Datum m_queuedId;
    };
}

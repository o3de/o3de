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

#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Components/Nodes/NodeConfiguration.h>
#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

namespace GraphCanvas
{
    // Going to restrict this to 1:1 mappings for now
    // because the editing flow to make 1:N mappings will require
    // some customization of the Reflected Property Editor
    struct SlotRedirectionConfiguration
    {
        AZ_TYPE_INFO(SlotRedirectionConfiguration, "{E2EAB6D5-BF6B-4D42-8291-B69E59080916}");

        AZStd::string m_name;
        Endpoint m_targetEndpoint;
    };

    class CollapsedNodeGroupConfiguration
        : public NodeConfiguration
    {
    public:
        NodeId m_nodeGroupId;
        AZStd::vector< SlotRedirectionConfiguration > m_redirectionConfigurations;
    };

    class NodeGroupRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! State controller signals whether or not the Node Group is externally moved or not.
        //! Basically a signal of whether or not it should update the elements inside of it or not.
        virtual StateController< bool >* GetExternallyControlledStateController() = 0;

        virtual void SetGroupSize(QRectF blockRectangle) = 0;
        virtual QRectF GetGroupBoundingBox() const = 0;
        virtual AZ::Color GetGroupColor() const = 0;

        virtual void CollapseGroup() = 0;
        virtual void ExpandGroup() = 0;
        virtual void UngroupGroup() = 0;

        virtual bool IsCollapsed() const = 0;
        virtual AZ::EntityId GetCollapsedNodeId() const = 0;

        virtual void FindGroupedElements(AZStd::vector< NodeId >& interiorElements) = 0;

        virtual bool IsInTitle(const QPointF& scenePoint) const = 0;
    };
    
    using NodeGroupRequestBus = AZ::EBus<NodeGroupRequests>;

    class NodeGroupNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual void OnCollapsed([[maybe_unused]] const NodeId& collapsedNodeId) {}
    };

    using NodeGroupNotificationBus = AZ::EBus<NodeGroupNotifications>;
    
    class CollapsedNodeGroupRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void ExpandGroup() = 0;

        virtual AZ::EntityId GetSourceGroup() const = 0;
    };
    
    using CollapsedNodeGroupRequestBus = AZ::EBus<CollapsedNodeGroupRequests>;
}
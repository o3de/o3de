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

#include <QRect>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>

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

        virtual void SetGroupSize(QRectF blockRectangle) = 0;
        virtual QRectF GetGroupBoundingBox() const = 0;
        virtual AZ::Color GetGroupColor() const = 0;

        virtual void CollapseGroup() = 0;
        virtual void ExpandGroup() = 0;
        virtual void UngroupGroup() = 0;

        virtual bool IsCollapsed() const = 0;
        virtual AZ::EntityId GetCollapsedNodeId() const = 0;

        virtual void AddElementToGroup(const AZ::EntityId& groupableElement) = 0;
        virtual void AddElementsToGroup(const AZStd::unordered_set<AZ::EntityId>& groupableElements) = 0;
        virtual void AddElementsVectorToGroup(const AZStd::vector<AZ::EntityId>& groupableElements) = 0;

        virtual void RemoveElementFromGroup(const AZ::EntityId& groupableElement) = 0;
        virtual void RemoveElementsFromGroup(const AZStd::unordered_set<AZ::EntityId>& groupableElements) = 0;
        virtual void RemoveElementsVectorFromGroup(const AZStd::vector<AZ::EntityId>& groupableElements) = 0;

        virtual void FindGroupedElements(AZStd::vector< NodeId >& interiorElements) = 0;

        virtual void ResizeGroupToElements(bool growGroupOnly) = 0;

        virtual bool IsInTitle(const QPointF& scenePoint) const = 0;
        
        virtual void AdjustTitleSize() = 0;
    };
    
    using NodeGroupRequestBus = AZ::EBus<NodeGroupRequests>;

    class NodeGroupNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual void OnCollapsed([[maybe_unused]] const NodeId& collapsedNodeId) {}
        virtual void OnExpanded() {}
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

        virtual AZStd::vector< Endpoint > GetRedirectedEndpoints() const = 0;
        virtual void ForceEndpointRedirection(const AZStd::vector< Endpoint >& redirections) = 0;
    };
    
    using CollapsedNodeGroupRequestBus = AZ::EBus<CollapsedNodeGroupRequests>;

    class CollapsedNodeGroupNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnExpansionComplete() {}
    };

    using CollapsedNodeGroupNotificationBus = AZ::EBus<CollapsedNodeGroupNotifications>;

    class GroupableSceneMemberRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual bool IsGrouped() const = 0;
        virtual const AZ::EntityId& GetGroupId() const = 0;

        virtual void RegisterToGroup(const AZ::EntityId& groupId) = 0;
        virtual void UnregisterFromGroup(const AZ::EntityId& groupId) = 0;
        virtual void RemoveFromGroup() = 0;
    };

    using GroupableSceneMemberRequestBus = AZ::EBus<GroupableSceneMemberRequests>;

    class GroupableSceneMemberNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnGroupChanged() = 0;
    };

    using GroupableSceneMemberNotificationBus = AZ::EBus<GroupableSceneMemberNotifications>;
}

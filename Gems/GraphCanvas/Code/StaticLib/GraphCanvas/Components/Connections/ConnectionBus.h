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

#include <QPointF>

#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>

class QGraphicsItem;

namespace AZ
{
    class any;
}

namespace GraphCanvas
{
    struct ConnectionEndpoints
    {
        ConnectionEndpoints() = default;

        ConnectionEndpoints(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
            : m_sourceEndpoint(sourceEndpoint)
            , m_targetEndpoint(targetEndpoint)
        {
        }        

        Endpoint m_sourceEndpoint;
        Endpoint m_targetEndpoint;
    };

    //! ConnectionRequests
    //! Requests which are serviced by the \ref GraphCanvas::Connection component.
    class ConnectionRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Get this connection's source slot ID.
        virtual AZ::EntityId GetSourceSlotId() const = 0;
        //! Resolve the node the source slot belongs to.
        virtual AZ::EntityId GetSourceNodeId() const = 0;

        //! Retrieves the source endpoint for this connection
        virtual Endpoint GetSourceEndpoint() const = 0;
        //! Retrieves the source position for this connection
        virtual QPointF GetSourcePosition() const = 0;
        //! Begins moving the source of this connection.
        virtual void StartSourceMove() = 0;

        //! Changes the visual Source of the connection to the specified endpoint. Will not
        //  modify the underlying model connection.
        virtual void SnapSourceDisplayTo(const Endpoint& endpoint) = 0;
        virtual void AnimateSourceDisplayTo(const Endpoint& endpoint, float duration) = 0;

        //! Get this connection's target slot ID.
        virtual AZ::EntityId GetTargetSlotId() const = 0;
        //! Resolve the Node the target slot belongs to.
        virtual AZ::EntityId GetTargetNodeId() const = 0;

        //! Retrieves the target endpoint for this connection
        virtual Endpoint GetTargetEndpoint() const = 0;
        //! Retrieves the target position for this connection
        virtual QPointF GetTargetPosition() const = 0;
        //! Begins moving the target of this connection.
        virtual void StartTargetMove() = 0;

        //! Retrieves both the endpoitns for the specified connection
        virtual ConnectionEndpoints GetEndpoints() const
        {
            return ConnectionEndpoints(GetSourceEndpoint(), GetTargetEndpoint());
        }

        Endpoint FindOtherEndpoint(const Endpoint& endpoint)
        {
            ConnectionEndpoints endpoints = GetEndpoints();

            if (endpoints.m_sourceEndpoint == endpoint)
            {
                return endpoints.m_targetEndpoint;
            }
            else if (endpoints.m_targetEndpoint == endpoint)
            {
                return endpoints.m_sourceEndpoint;
            }

            AZ_Assert(false, "Unknown endpoint passed into connection");
            return Endpoint();
        }

        //! Changes the visual target of the connection to the specified endpoint. Will not
        //  modify the underlying model connection.
        virtual void SnapTargetDisplayTo(const Endpoint& endpoint) = 0;
        virtual void AnimateTargetDisplayTo(const Endpoint& endpoint, float duration) = 0;

        virtual bool ContainsEndpoint(const Endpoint& endpoint) const = 0;

        //! Get this connection's tooltip.
        virtual AZStd::string GetTooltip() const = 0;
        //! Set this connection's tooltip.
        virtual void SetTooltip(const AZStd::string&) {}

        //! Get user data from this connection
        virtual AZStd::any* GetUserData() = 0;

        virtual void ChainProposalCreation(const QPointF& scenePos, const QPoint& screenPos) = 0;
    };

    using ConnectionRequestBus = AZ::EBus<ConnectionRequests>;

    //! ConnectionNotifications
    //! Notifications about changes to a connection's state.
    class ConnectionNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! The source slot for the connection changed
        //! # Parameters
        //! 1. The previous source slot entity ID.
        //! 2. The new source slot entity ID.
        virtual void OnSourceSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&) {}
        //! The target slot for the connection changed.
        //! # Parameters
        //! 1. The previous target slot entity ID.
        //! 2. The new target slot entity ID.
        virtual void OnTargetSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&) {}

        //! The connection's tooltip changed.
        virtual void OnTooltipChanged(const AZStd::string&) {}

        virtual void OnMoveBegin() {}

        virtual void OnMoveFinalized(bool isValidConnection)
        {
            if (isValidConnection)
            {
                OnMoveComplete();
            }
        }

        // ConnectionNotification OnMoveComplete renamed to OnMoveFinalized to allow for additional parameter
        // Will be removed in a future release. Cannot deprecate method because it generates tons of warnings
        // for default NotificationBus handler connection.
        virtual void OnMoveComplete() {}
        
    };

    using ConnectionNotificationBus = AZ::EBus<ConnectionNotifications>;

    // Various requests that can be made of the Connection visuals
    class ConnectionUIRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void UpdateConnectionPath() = 0;

        virtual void SetAltDeletionEnabled(bool enabled) = 0;
    };

    using ConnectionUIRequestBus = AZ::EBus<ConnectionUIRequests>;

    class ConnectionVisualNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ConnectionId;

        virtual void OnConnectionPathUpdated() {}
    };

    using ConnectionVisualNotificationBus = AZ::EBus<ConnectionVisualNotifications>;
}

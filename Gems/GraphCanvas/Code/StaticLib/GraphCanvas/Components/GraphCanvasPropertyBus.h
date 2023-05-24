/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphCanvas
{
    // Used by the PropertyGrid to find all Components that have Properties they want to expose to the editor.
    class GraphCanvasPropertyInterface
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZ::Component* GetPropertyComponent() = 0;

        virtual void AddBusId(const AZ::EntityId& busId) = 0;
        virtual void RemoveBusId(const AZ::EntityId& busId) = 0;
    };

    using GraphCanvasPropertyBus = AZ::EBus<GraphCanvasPropertyInterface>;

    class GraphCanvasPropertyInterfaceNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnPropertyComponentChanged() = 0;
    };

    using GraphCanvasPropertyInterfaceNotificationBus = AZ::EBus<GraphCanvasPropertyInterfaceNotifications>;

    class GraphCanvasPropertyBusHandler
        : public GraphCanvasPropertyBus::MultiHandler
    {
    public:

        void OnActivate(const AZ::EntityId& entityId)
        {
            GraphCanvasPropertyBus::MultiHandler::BusConnect(entityId);
        }

        void OnDeactivate()
        {
            GraphCanvasPropertyBus::MultiHandler::BusDisconnect();
        }

        void AddBusId(const AZ::EntityId& busId) final
        {
            GraphCanvasPropertyBus::MultiHandler::BusConnect(busId);
        }

        void RemoveBusId(const AZ::EntityId& busId) final
        {
            GraphCanvasPropertyBus::MultiHandler::BusDisconnect(busId);
        }
    };

    // Stub Component to implement the bus to simplify the usage.
    class GraphCanvasPropertyComponent
        : public AZ::Component
        , public GraphCanvasPropertyBusHandler
    {
    public:
        AZ_COMPONENT(GraphCanvasPropertyComponent, "{12408A55-4742-45B2-8694-EE1C80430FB4}");
        static void Reflect(AZ::ReflectContext* context);

        void Init() override {};

        void Activate() override
        {
            GraphCanvasPropertyBusHandler::OnActivate(GetEntityId());
        }

        void Deactivate() override
        {
            GraphCanvasPropertyBusHandler::OnDeactivate();
        }

        // GraphCanvasPropertyBus
        AZ::Component* GetPropertyComponent() override
        {
            return this;
        }
    };
}

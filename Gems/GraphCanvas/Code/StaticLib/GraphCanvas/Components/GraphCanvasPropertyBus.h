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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

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

        void AddBusId(const AZ::EntityId& busId) override final
        {
            GraphCanvasPropertyBus::MultiHandler::BusConnect(busId);
        }

        void RemoveBusId(const AZ::EntityId& busId) override final
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
        static void Reflect(AZ::ReflectContext* context) 
        { 
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GraphCanvasPropertyComponent, AZ::Component>()
                    ->Version(1);
            }
        }

        void Init() override {};

        void Activate()
        {
            GraphCanvasPropertyBusHandler::OnActivate(GetEntityId());
        }

        void Deactivate()
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
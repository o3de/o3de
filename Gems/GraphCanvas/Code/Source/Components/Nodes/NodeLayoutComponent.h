/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/SceneBus.h>

class QGraphicsLayout;

namespace GraphCanvas
{
    //! Base class for internal Node Layouts to help deal with some book keeping
    class NodeLayoutComponent
        : public AZ::Component
        , public NodeLayoutRequestBus::Handler
    {
    public:
        AZ_COMPONENT(NodeLayoutComponent, "{D3152CCC-1C6D-4E95-829D-0441002440AB}");

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<NodeLayoutComponent, AZ::Component>()
                    ->Version(1)
                ;
            }
        }

        NodeLayoutComponent()
            : m_layout(nullptr)
        {
        }

        virtual ~NodeLayoutComponent()
        {
        }

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(NodeLayoutServiceCrc);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(NodeLayoutServiceCrc);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(NodeLayoutServiceCrc);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_NodeService"));
            required.push_back(AZ_CRC_CE("GraphCanvas_StyledGraphicItemService"));
        }
        
        void Init() override
        {
        }

        void Activate() override
        {
            NodeLayoutRequestBus::Handler::BusConnect(GetEntityId());
        }

        void Deactivate() override
        {
            NodeLayoutRequestBus::Handler::BusDisconnect();
        }
        ////
        
        // NodeLayoutRequestBus
        QGraphicsLayout* GetLayout() override
        {
            return m_layout;
        }
        ////

    protected:

        // Methods to simplify casting in other components
        template<class T>
        T* GetLayoutAs()
        {
            return static_cast<T*>(m_layout);
        }

        template<class T>
        const T* GetLayoutAs() const
        {
            return static_cast<const T*>(m_layout);
        }

        QGraphicsLayout* m_layout;
    };
}

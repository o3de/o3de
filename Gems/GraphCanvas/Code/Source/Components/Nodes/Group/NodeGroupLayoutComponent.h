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

#include <AzCore/Component/Component.h>

#include <QGraphicsLinearLayout>

#include <Components/Nodes/NodeLayoutComponent.h>

#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <GraphCanvas/Styling/StyleHelper.h>

class QGraphicsGridLayout;

namespace GraphCanvas
{
    //! Lays out the parts of the Node Group node
    class NodeGroupLayoutComponent
        : public NodeLayoutComponent
        , protected NodeNotificationBus::Handler
        , protected StyleNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(NodeGroupLayoutComponent, "{0DD4204A-8A75-48C1-AA91-9878BCB0C4D0}", NodeLayoutComponent);

        static void Reflect(AZ::ReflectContext*);

        static AZ::Entity* CreateNodeGroupEntity();

        NodeGroupLayoutComponent();
        ~NodeGroupLayoutComponent() override;

        // AZ::Component
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("GraphCanvas_NodeLayoutSupportService", 0xa8b639be));
            dependent.push_back(AZ_CRC("GraphCanvas_CommentTextService", 0xb650db99));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_NodeService", 0xcc0f32cc));
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
        }
        ////

    protected:

        // StyleNotificationBus::Handler
        void OnStyleChanged() override;
        ////

        // AZ::Component
        void Init();
        void Activate();
        void Deactivate();
        ////

        // NodeNotification
        void OnNodeActivated();
        ////

        void UpdateLayoutParameters();

        Styling::StyleHelper m_style;

        QGraphicsLinearLayout* m_comment;
    };
}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            dependent.push_back(AZ_CRC_CE("GraphCanvas_NodeLayoutSupportService"));
            dependent.push_back(AZ_CRC_CE("GraphCanvas_CommentTextService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_NodeService"));
            required.push_back(AZ_CRC_CE("GraphCanvas_StyledGraphicItemService"));
        }
        ////

    protected:

        // StyleNotificationBus::Handler
        void OnStyleChanged() override;
        ////

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeNotification
        void OnNodeActivated() override;
        ////

        void UpdateLayoutParameters();

        Styling::StyleHelper m_style;

        QGraphicsLinearLayout* m_comment;
    };
}

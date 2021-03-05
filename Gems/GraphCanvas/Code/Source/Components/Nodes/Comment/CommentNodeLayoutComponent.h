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

#include <QGraphicsLinearLayout>

#include <AzCore/Component/Component.h>

#include <Components/Nodes/NodeLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

class QGraphicsGridLayout;

namespace GraphCanvas
{
    //! Lays out the parts of the comment node
    class CommentNodeLayoutComponent
        : public NodeLayoutComponent
        , public NodeNotificationBus::Handler
        , public StyleNotificationBus::Handler
        , public AZ::EntityBus::Handler
    {
    public:
        AZ_COMPONENT(CommentNodeLayoutComponent, "{6926658C-372A-43D5-8758-FB67DDE3D857}", NodeLayoutComponent);
        static void Reflect(AZ::ReflectContext*);

        static AZ::Entity* CreateCommentNodeEntity();

        CommentNodeLayoutComponent();
        ~CommentNodeLayoutComponent();

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

        void Init();
        void Activate();
        void Deactivate();
        ////

        // EntityBus
        void OnEntityExists(const AZ::EntityId& entityId) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // NodeNotification
        void OnNodeActivated();
        ////

    protected:

        void UpdateLayoutParameters();

        Styling::StyleHelper m_style;

        QGraphicsLinearLayout* m_comment;
    };
}
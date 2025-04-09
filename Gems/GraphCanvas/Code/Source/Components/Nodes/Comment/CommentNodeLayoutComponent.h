/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            dependent.push_back(AZ_CRC_CE("GraphCanvas_NodeLayoutSupportService"));
            dependent.push_back(AZ_CRC_CE("GraphCanvas_CommentTextService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_NodeService"));
            required.push_back(AZ_CRC_CE("GraphCanvas_StyledGraphicItemService"));
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // EntityBus
        void OnEntityExists(const AZ::EntityId& entityId) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // NodeNotification
        void OnNodeActivated() override;
        ////

    protected:

        void UpdateLayoutParameters();

        Styling::StyleHelper m_style;

        QGraphicsLinearLayout* m_comment;
    };
}

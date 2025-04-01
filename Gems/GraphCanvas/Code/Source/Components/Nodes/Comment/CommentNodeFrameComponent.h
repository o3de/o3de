/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsWidget>

#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/NodeFrameGraphicsWidget.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    class CommentNodeFrameGraphicsWidget;

    class CommentNodeFrameComponent
        : public AZ::Component
        , public NodeNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(CommentNodeFrameComponent, "{207F2AC3-40C6-49EC-9B73-E691A9ED73E7}", AZ::Component);
        static void Reflect(AZ::ReflectContext*);

        CommentNodeFrameComponent();
        ~CommentNodeFrameComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GraphCanvas_NodeVisualService"));
            provided.push_back(AZ_CRC_CE("GraphCanvas_RootVisualService"));
            provided.push_back(AZ_CRC_CE("GraphCanvas_VisualService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_NodeVisualService"));
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_RootVisualService"));
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_VisualService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
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

        // NodeNotifications
        void OnNodeActivated() override;
        ////

    private:

        CommentNodeFrameComponent(const CommentNodeFrameComponent&) = delete;
        const CommentNodeFrameComponent& operator=(const CommentNodeFrameComponent&) = delete;
        AZStd::unique_ptr<CommentNodeFrameGraphicsWidget> m_frameWidget;
    };

    //! The QGraphicsItem for the generic frame.
    class CommentNodeFrameGraphicsWidget
        : public GeneralNodeFrameGraphicsWidget
        , public CommentNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(CommentNodeFrameGraphicsWidget, "{99343103-C8EF-44D0-BD6C-EF44ACDBD69B}");
        AZ_CLASS_ALLOCATOR(CommentNodeFrameGraphicsWidget, AZ::SystemAllocator);

        // Do not allow Serialization of Graphics Ui classes
        static void Reflect(AZ::ReflectContext*) = delete;

        CommentNodeFrameGraphicsWidget(const AZ::EntityId& nodeVisual);
        ~CommentNodeFrameGraphicsWidget() override = default;

        // CommentNotificationBus
        void OnBackgroundColorChanged(const AZ::Color& color) override;
        ////

        // QGraphicsWidget
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
        ////
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QGraphicsWidget>
AZ_POP_DISABLE_WARNING

#include <Components/Nodes/NodeFrameGraphicsWidget.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    class GeneralNodeFrameGraphicsWidget;

    class GeneralNodeFrameComponent
        : public AZ::Component
        , public NodeNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(GeneralNodeFrameComponent, "{3AD0423E-F3D5-45F7-8656-C66BCD1EC691}", AZ::Component);
        static void Reflect(AZ::ReflectContext*);

        GeneralNodeFrameComponent();
        ~GeneralNodeFrameComponent() override;

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

        void OnNodeWrapped(const AZ::EntityId& wrappingNode) override;
        void OnNodeUnwrapped(const AZ::EntityId& wrappingNode) override;
        ////

    private:

        GeneralNodeFrameComponent(const GeneralNodeFrameComponent&) = delete;
        const GeneralNodeFrameComponent& operator=(const GeneralNodeFrameComponent&) = delete;
        bool                            m_shouldDeleteFrame;
        GeneralNodeFrameGraphicsWidget* m_frameWidget;
    };

    //! The QGraphicsItem for the generic frame.
    class GeneralNodeFrameGraphicsWidget
        : public NodeFrameGraphicsWidget
    {
    public:
        AZ_RTTI(GeneralNodeFrameGraphicsWidget, "{15200183-8316-4A7D-985E-5C3257CD2463}", NodeFrameGraphicsWidget);
        AZ_CLASS_ALLOCATOR(GeneralNodeFrameGraphicsWidget, AZ::SystemAllocator);

        // Do not allow Serialization of Graphics Ui classes
        static void Reflect(AZ::ReflectContext*) = delete;

        explicit GeneralNodeFrameGraphicsWidget(const AZ::EntityId& nodeVisual);
        ~GeneralNodeFrameGraphicsWidget() override = default;

        // SceneMemberUIRequestBus
        QPainterPath GetOutline() const override;
        ////

        // QGraphicsWidget
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        ////
    };
}

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
            provided.push_back(AZ_CRC("GraphCanvas_NodeVisualService", 0x39c4e7f3));
            provided.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
            provided.push_back(AZ_CRC("GraphCanvas_VisualService", 0xfbb2c871));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GraphCanvas_NodeVisualService", 0x39c4e7f3));
            incompatible.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
            incompatible.push_back(AZ_CRC("GraphCanvas_VisualService", 0xfbb2c871));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_NodeService", 0xcc0f32cc));
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
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
        AZ_CLASS_ALLOCATOR(GeneralNodeFrameGraphicsWidget, AZ::SystemAllocator, 0);

        // Do not allow Serialization of Graphics Ui classes
        static void Reflect(AZ::ReflectContext*) = delete;

        GeneralNodeFrameGraphicsWidget(const AZ::EntityId& nodeVisual);
        ~GeneralNodeFrameGraphicsWidget() override = default;

        // SceneMemberUIRequestBus
        QPainterPath GetOutline() const override;
        ////

        // QGraphicsWidget
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        ////
    };
}

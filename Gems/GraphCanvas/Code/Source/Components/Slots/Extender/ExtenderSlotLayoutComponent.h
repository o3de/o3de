/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsLinearLayout>
#include <QTimer>

#include <Components/Slots/SlotLayoutComponent.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <Widgets/GraphCanvasLabel.h>
#include <Widgets/NodePropertyDisplayWidget.h>

namespace GraphCanvas
{
    class ExtenderSlotLayoutComponent;
    class ExtenderSlotConnectionPin;
    class GraphCanvasLabel;

    class ExtenderSlotLayout
        : public QGraphicsLinearLayout
        , public SlotNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public StyleNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ExtenderSlotLayout, AZ::SystemAllocator);

        ExtenderSlotLayout(ExtenderSlotLayoutComponent& owner);
        ~ExtenderSlotLayout();

        void Activate();
        void Deactivate();

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId&) override;
        void OnSceneReady() override;
        ////

        // SlotNotificationBus
        void OnRegisteredToNode(const AZ::EntityId& nodeId) override;

        void OnNameChanged(const AZStd::string& name) override;
        void OnTooltipChanged(const AZStd::string& tooltip) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

    private:

        void UpdateLayout();
        void UpdateGeometry();

        ConnectionType m_connectionType;

        Styling::StyleHelper m_style;
        ExtenderSlotLayoutComponent& m_owner;

        ExtenderSlotConnectionPin* m_slotConnectionPin;
        GraphCanvasLabel*           m_slotText;
        QGraphicsItem*              m_slotLabelFilter;
    };

    //! Lays out the parts of a Extender Slot
    class ExtenderSlotLayoutComponent
        : public SlotLayoutComponent
    {
    public:
        AZ_COMPONENT(ExtenderSlotLayoutComponent, "{596E1A76-6F84-4C1A-B32D-0B6B069FC9AB}", SlotLayoutComponent);
        static void Reflect(AZ::ReflectContext* context);

        ExtenderSlotLayoutComponent();
        ~ExtenderSlotLayoutComponent() override = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        
        ExtenderSlotLayout* m_layout;
    };
}

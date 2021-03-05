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

#include <Components/Slots/SlotLayoutComponent.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    class DefaultSlotLayoutComponent;
    class SlotConnectionPin;

    class DefaultSlotLayout
        : public QGraphicsLinearLayout
        , public StyleNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(DefaultSlotLayout, AZ::SystemAllocator, 0);

        DefaultSlotLayout(DefaultSlotLayoutComponent& owner);
        ~DefaultSlotLayout();

        void Activate();
        void Deactivate();

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId&) override;
        void OnSceneReady() override;
        ////

        // StyleNotificationBus
        void OnStyleChanged();
        ////        

    private:

        void UpdateLayout();
        void UpdateGeometry();

        ConnectionType m_connectionType;

        Styling::StyleHelper m_style;
        DefaultSlotLayoutComponent& m_owner;

        SlotConnectionPin* m_slotConnectionPin;
        GraphCanvasLabel*  m_slotText;
    };

    //! Lays out the parts of a basic Node
    class DefaultSlotLayoutComponent
        : public SlotLayoutComponent
    {
    public:
        AZ_COMPONENT(DefaultSlotLayoutComponent, "{40F5DD3B-3B63-488F-AEBC-F6AE34A00415}", SlotLayoutComponent);
        static void Reflect(AZ::ReflectContext* context);

        DefaultSlotLayoutComponent();
        virtual ~DefaultSlotLayoutComponent() = default;

        void Init();
        void Activate();
        void Deactivate();

    private:
        DefaultSlotLayout* m_defaultSlotLayout;
    };
}
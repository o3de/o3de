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
    class ExecutionSlotLayoutComponent;
    class ExecutionSlotConnectionPin;
    class GraphCanvasLabel;

    class ExecutionSlotLayout
        : public QGraphicsLinearLayout
        , public SlotNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public StyleNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ExecutionSlotLayout, AZ::SystemAllocator, 0);

        ExecutionSlotLayout(ExecutionSlotLayoutComponent& owner);
        ~ExecutionSlotLayout();

        void Activate();
        void Deactivate();

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId&) override;
        void OnSceneReady() override;
        ////

        // SlotNotificationBus
        void OnRegisteredToNode(const AZ::EntityId& nodeId) override;

        void OnNameChanged(const TranslationKeyedString& name) override;
        void OnTooltipChanged(const TranslationKeyedString& tooltip) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged();
        ////

    private:

        void UpdateLayout();
        void UpdateGeometry();

        ConnectionType m_connectionType;

        Styling::StyleHelper m_style;
        ExecutionSlotLayoutComponent& m_owner;

        ExecutionSlotConnectionPin* m_slotConnectionPin;
        GraphCanvasLabel*           m_slotText;
    };

    //! Lays out the parts of a basic Node
    class ExecutionSlotLayoutComponent
        : public SlotLayoutComponent
    {
    public:
        AZ_COMPONENT(ExecutionSlotLayoutComponent, "{9742DEFD-6EC9-4F06-850B-8F5FE2647E34}", SlotLayoutComponent);
        static void Reflect(AZ::ReflectContext* context);

        ExecutionSlotLayoutComponent();
        ~ExecutionSlotLayoutComponent() override = default;

        void Init();
        void Activate();
        void Deactivate();

    private:
        ExecutionSlotLayout* m_layout;
    };
}
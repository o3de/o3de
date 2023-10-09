/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(ExecutionSlotLayout, AZ::SystemAllocator);

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

        void OnNameChanged(const AZStd::string& name) override;
        void OnTooltipChanged(const AZStd::string& tooltip) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

    private:

        // ExecutionSlotLayoutBus
        void SetTextDecoration(const AZStd::string& textDecoration, const AZStd::string& toolTip);
        void ClearTextDecoration();
        ////

        void ApplyTextStyle(GraphCanvasLabel* graphCanvasLabel);

        void UpdateLayout();
        void UpdateGeometry();

        ConnectionType m_connectionType;

        Styling::StyleHelper m_style;
        ExecutionSlotLayoutComponent& m_owner;

        ExecutionSlotConnectionPin* m_slotConnectionPin;
        GraphCanvasLabel*           m_slotText;

        GraphCanvasLabel*           m_textDecoration;

        bool m_isNameHidden = false;
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

        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        ExecutionSlotLayout* m_layout;
    };
}

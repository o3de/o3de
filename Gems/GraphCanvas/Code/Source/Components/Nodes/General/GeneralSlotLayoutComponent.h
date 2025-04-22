/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    class GeneralSlotLayoutGraphicsWidget;

    //! Lays out the slots for the General Node
    class GeneralSlotLayoutComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(GeneralSlotLayoutComponent, "{F6554B50-A42A-4C79-8B1D-547EEA1EA52D}");
        static void Reflect(AZ::ReflectContext*);

        GeneralSlotLayoutComponent();
        ~GeneralSlotLayoutComponent();

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GraphCanvas_SlotsContainerService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incombatible)
        {
            incombatible.push_back(AZ_CRC_CE("GraphCanvas_SlotsContainerService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_StyledGraphicItemService"));
            required.push_back(AZ_CRC_CE("GraphCanvas_SceneMemberService"));
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeSlotsRequestBus
        QGraphicsWidget* GetGraphicsWidget();
        ////

    private:
        GeneralSlotLayoutComponent(const GeneralSlotLayoutComponent&) = delete;

        bool                      m_enableDividers;
        SlotGroupConfigurationMap m_slotGroupConfigurations;

        friend class GeneralSlotLayoutGraphicsWidget;
        GeneralSlotLayoutGraphicsWidget* m_nodeSlotsUi;
    };

    //! The slots QGraphicsWidget for displaying a the node slots
    //! QtWidgets cannot be serialized out, so the component wrapper
    //! stores the actual configuration map for serialization.
    class GeneralSlotLayoutGraphicsWidget
        : public QGraphicsWidget
        , public SlotLayoutRequestBus::Handler
        , public NodeSlotsRequestBus::Handler
        , public NodeNotificationBus::Handler
        , public StyleNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:
        class LayoutDividerWidget
            : public QGraphicsWidget
        {
        public:
            AZ_CLASS_ALLOCATOR(LayoutDividerWidget, AZ::SystemAllocator);
            LayoutDividerWidget(QGraphicsItem* parent);

            void UpdateStyle(const Styling::StyleHelper& styleHelper);
        };

        class LinearSlotGroupWidget
            : public QGraphicsWidget
            , public SlotUINotificationBus::MultiHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(LinearSlotGroupWidget, AZ::SystemAllocator);
            LinearSlotGroupWidget(QGraphicsItem* parent);

            void DisplaySlot(const AZ::EntityId& slotId);
            void RemoveSlot(const AZ::EntityId& slotId);

            QGraphicsLinearLayout* GetLayout();

            QGraphicsWidget* GetSpacer();

            const AZStd::vector< SlotLayoutInfo >& GetInputSlots() const;
            const AZStd::vector< SlotLayoutInfo >& GetOutputSlots() const;

            bool IsEmpty() const;
            void UpdateStyle(const Styling::StyleHelper& styleHelper);

            // SlotUINotificationBus
            void OnSlotLayoutPriorityChanged(int layoutPriority) override;
            ////

        private:

            int LayoutSlot(QGraphicsLinearLayout* layout, AZStd::vector<SlotLayoutInfo>& slotList, const SlotLayoutInfo& slotInfo);

            QGraphicsLayoutItem* GetLayoutItem(const AZ::EntityId& slotId) const;

            QGraphicsLinearLayout* m_layout;

            QGraphicsWidget* m_horizontalSpacer;

            QGraphicsLinearLayout* m_inputs;
            AZStd::vector< SlotLayoutInfo > m_inputSlots;
            AZStd::unordered_set< SlotId >  m_inputSlotSet;

            QGraphicsLinearLayout* m_outputs;
            AZStd::vector< SlotLayoutInfo > m_outputSlots;
            AZStd::unordered_set< SlotId >  m_outputSlotSet;
        };

    public:
        AZ_TYPE_INFO(GeneralSlotLayoutGraphicsWidget, "{9DE7D3C0-D88C-47D8-85D4-5E0F619E60CB}");
        AZ_CLASS_ALLOCATOR(GeneralSlotLayoutGraphicsWidget, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context) = delete;

        GeneralSlotLayoutGraphicsWidget(GeneralSlotLayoutComponent& nodeSlots);
        ~GeneralSlotLayoutGraphicsWidget() override;

        void Activate();
        void Deactivate();

        // NodeNotificationBus
        void OnNodeActivated() override;

        void OnSlotAddedToNode(const AZ::EntityId& slot) override;        
        void OnSlotRemovedFromNode(const AZ::EntityId& slot) override;
        ////

        // NodeSlotsRequestBus
        QGraphicsLayoutItem* GetGraphicsLayoutItem() override;
        QGraphicsLinearLayout* GetLinearLayout(const SlotGroup& slotGroup) override;
        QGraphicsWidget* GetSpacer(const SlotGroup& slotGroup) override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        ////

        // SlotLayoutRequestBus
        void SetDividersEnabled(bool enabled) override;
        void ConfigureSlotGroup(SlotGroup group, SlotGroupConfiguration configuration) override;
        int GetSlotGroupDisplayOrder(SlotGroup group) const override;

        bool IsSlotGroupVisible(SlotGroup group) const override;
        void SetSlotGroupVisible(SlotGroup group, bool visible) override;
        void ClearSlotGroup(SlotGroup group) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

    protected:
        
        const AZ::EntityId& GetEntityId() const { return m_entityId; }

    private:

        bool DisplaySlot(const AZ::EntityId& slotId);
        bool RemoveSlot(const AZ::EntityId& slotId);

        void ActivateSlots();

        void ClearLayout();
        void UpdateLayout();

        void UpdateStyles();
        void RefreshDisplay();

        LinearSlotGroupWidget* FindCreateSlotGroupWidget(SlotGroup slotType);
        LayoutDividerWidget*   FindCreateDividerWidget(int index);

        GeneralSlotLayoutGraphicsWidget(const GeneralSlotLayoutComponent&) = delete;

        GeneralSlotLayoutComponent& m_nodeSlots;

        QGraphicsLinearLayout* m_groupLayout;

        AZStd::unordered_map< SlotGroup, LinearSlotGroupWidget* > m_slotGroups;
        AZStd::vector< LayoutDividerWidget* > m_dividers;

        Styling::StyleHelper m_styleHelper;
        AZ::EntityId         m_entityId;

        bool                 m_addedToScene;
    };
}

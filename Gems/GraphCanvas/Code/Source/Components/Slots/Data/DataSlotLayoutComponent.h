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

#include <AzToolsFramework/UI/Notifications/ToastBus.h>
#include <Components/Slots/SlotLayoutComponent.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphicsItems/GraphCanvasSceneEventFilter.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <Widgets/GraphCanvasLabel.h>
#include <Widgets/NodePropertyDisplayWidget.h>

namespace GraphCanvas
{
    class DataSlotLayoutComponent;
    class DataSlotConnectionPin;
    class GraphCanvasLabel;

    class DataSlotLayout
        : public QGraphicsLinearLayout
        , public SlotNotificationBus::Handler
        , public DataSlotLayoutRequestBus::Handler
        , public DataSlotNotificationBus::Handler
        , public NodeDataSlotRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public StyleNotificationBus::Handler
        , public VisualNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    private:

        class DataTypeConversionDataSlotDragDropInterface
            : public DataSlotDragDropInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(DataTypeConversionDataSlotDragDropInterface, AZ::SystemAllocator);

            DataTypeConversionDataSlotDragDropInterface(const SlotId& slotId);

            AZ::Outcome<DragDropState> OnDragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent) override;
            void OnDragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent) override;
            void OnDropEvent(QGraphicsSceneDragDropEvent* dropEvent) override;
            void OnDropCancelled() override;

        private:

            SlotId m_slotId;

            ViewId  m_viewId;
            AzToolsFramework::ToastId m_toastId;
        };

        class DoubleClickSceneEventFilter
            : public SceneEventFilter
        {
        public:
            AZ_CLASS_ALLOCATOR(DoubleClickSceneEventFilter, AZ::SystemAllocator);

            DoubleClickSceneEventFilter(DataSlotLayout& dataSlotLayout)
                : SceneEventFilter(nullptr)
                , m_owner(dataSlotLayout)
            {
            }

            bool sceneEventFilter(QGraphicsItem*, QEvent* sceneEvent) override
            {
                switch (sceneEvent->type())
                {
                case QEvent::GraphicsSceneMousePress:
                {
                    return true;
                }
                case QEvent::GraphicsSceneMouseDoubleClick:
                {
                    m_owner.OnSlotTextDoubleClicked();
                    return true;
                }
                }

                return false;
            }

        private:

            DataSlotLayout& m_owner;
        };

        friend class DoubleClickSceneEventFilter;

    public:
        AZ_CLASS_ALLOCATOR(DataSlotLayout, AZ::SystemAllocator);

        DataSlotLayout(DataSlotLayoutComponent& owner);
        ~DataSlotLayout();

        void Activate();
        void Deactivate();

        // SystemTickBus
        void OnSystemTick() override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId&) override;
        void OnSceneReady() override;
        ////

        // SlotNotificationBus
        void OnRegisteredToNode(const AZ::EntityId& nodeId) override;

        void OnNameChanged(const AZStd::string&) override;
        void OnTooltipChanged(const AZStd::string&) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // DataSlotLayoutRequestBus
        const DataSlotConnectionPin* GetConnectionPin() const override;

        void UpdateDisplay() override;

        QRectF GetWidgetSceneBoundingRect() const override;
        ////

        // DataSlotNotificationBus
        void OnDataSlotTypeChanged(const DataSlotType& dataSlotType) override;
        void OnDisplayTypeChanged(const AZ::Uuid& dataType, const AZStd::vector<AZ::Uuid>& typeIds) override;
        ////

        // NodeDataSlotRequestBus
        void RecreatePropertyDisplay() override;
        ////

        void OnDragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent);
        void OnDragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent);
        void OnDropEvent(QGraphicsSceneDragDropEvent* dragDropEvent);

    private:

        void SetTextDecoration(const AZStd::string& textDecoration, const AZStd::string& toolTip);
        void ClearTextDecoration();

        void ApplyTextStyle(GraphCanvasLabel* graphCanvasLabel);

        void UpdateFilterState();

        void RegisterDataSlotDragDropInterface(DataSlotDragDropInterface* dragDropInterface);
        void RemoveDataSlotDragDropInterface(DataSlotDragDropInterface* dragDropInterface);

        void SetDragDropState(DragDropState dragDropState);

        AZ::EntityId GetSceneId() const;

        void TryAndSetupSlot();

        void CreateDataDisplay();
        void UpdateLayout();
        void UpdateGeometry();

        void OnSlotTextDoubleClicked();

        AZStd::unordered_set< DataSlotDragDropInterface* >  m_dragDropInterfaces;
        DataSlotDragDropInterface*                          m_activeHandler;
        QGraphicsItem*                                      m_eventFilter;

        DragDropState                                       m_dragDropState;

        // Internal DragDrop Interface
        DataSlotDragDropInterface*                          m_valueReferenceInterface;

        ConnectionType m_connectionType;

        Styling::StyleHelper m_style;
        DataSlotLayoutComponent& m_owner;

        QGraphicsWidget*                                m_spacer;
        NodePropertyDisplayWidget*                      m_nodePropertyDisplay;
        DataSlotConnectionPin*                          m_slotConnectionPin;
        GraphCanvasLabel*                               m_slotText;
        DoubleClickSceneEventFilter*                    m_doubleClickFilter;

        GraphCanvasLabel* m_textDecoration = nullptr;

        bool m_isNameHidden = false;

        // track the last seen values of some members to prevent UpdateLayout doing unnecessary work
        struct
        {
            ConnectionType connectionType = CT_Invalid;
            DataSlotConnectionPin* slotConnectionPin = nullptr;
            GraphCanvasLabel* slotText = nullptr;
            NodePropertyDisplayWidget* nodePropertyDisplay = nullptr;
            QGraphicsWidget* spacer = nullptr;

            GraphCanvasLabel* textDecoration = nullptr;
        } m_atLastUpdate;
    };

    //! Lays out the parts of the Data Slot
    class DataSlotLayoutComponent
        : public SlotLayoutComponent
    {
    public:
        AZ_COMPONENT(DataSlotLayoutComponent, "{0DA3CBDA-1C43-4A18-8E01-AEEAA3C81882}");
        static void Reflect(AZ::ReflectContext* context);

        DataSlotLayoutComponent();
        virtual ~DataSlotLayoutComponent() = default;

        void Init();
        void Activate();
        void Deactivate();

    private:
        DataSlotLayout* m_layout;
    };
}

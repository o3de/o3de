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

#include <QGraphicsItem>

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Components/Slots/SlotLayoutItem.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

namespace GraphCanvas
{
    class SlotConnectionPin
        : public SlotLayoutItem
        , public SlotUIRequestBus::Handler
        , public SlotNotificationBus::Handler
    {
    public:
        AZ_RTTI(SlotConnectionPin, "{4E4A8C30-584A-434B-B8FC-0514C1E7D290}", QGraphicsLayoutItem, QGraphicsItem);
        AZ_CLASS_ALLOCATOR(SlotConnectionPin, AZ::SystemAllocator, 0);

        SlotConnectionPin(const AZ::EntityId& slotId);
        ~SlotConnectionPin() override;

        void Activate();
        void Deactivate();

        // SlotNotificationBus
        void OnRegisteredToNode(const AZ::EntityId& nodeId) override;
        ////

        // SlotLayoutItem
        void RefreshStyle() override final;

        AZ::EntityId GetEntityId() const override
        {
            return m_slotId;
        }
        ////

        // SlotUIRequestBus
        QPointF GetConnectionPoint() const override;
        QPointF GetJutDirection() const override;
        ////

    protected:

        virtual void OnSlotClicked();

        // QGraphicsItem
        QRectF boundingRect() const override;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        void keyPressEvent(QKeyEvent* keyEvent) override;
        void keyReleaseEvent(QKeyEvent* keyEvent) override;
        void hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent) override;
        void hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent) override;        
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        ////

        void OnMouseEnter(bool hasAltModifier);
        void OnMouseLeave();

        // QGraphicsLayoutItem
        void setGeometry(const QRectF& rect) override;
        QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint = {}) const override;
        ////

        void HandleNewConnection();

        virtual void DrawConnectionPin(QPainter *painter, QRectF drawRect, bool isConnected);
        virtual void OnRefreshStyle();

        ConnectionType m_connectionType;

        AZ::EntityId m_slotId;

        bool m_trackClick;
        bool m_deletionClick;

        bool m_hovered;
        StateSetter<RootGraphicsItemDisplayState> m_nodeDisplayStateStateSetter;
    };
}

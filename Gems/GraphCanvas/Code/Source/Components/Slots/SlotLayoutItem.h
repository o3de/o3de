/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsItem>
#include <QGraphicsLayoutItem>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    //! Generates Ebus notifications for QGraphicsItem event. 
    //! Requires that the derived class has a GetEntityId() function
    class SlotLayoutItem
        : public QGraphicsItem
        , public QGraphicsLayoutItem
    {
    public:

        AZ_RTTI(SlotLayoutItem, "{ED76860E-35B8-4FEE-A2A0-04B467F778B6}", QGraphicsLayoutItem, QGraphicsItem);
        AZ_CLASS_ALLOCATOR(SlotLayoutItem, AZ::SystemAllocator);

        SlotLayoutItem()
        {
            setGraphicsItem(this);
            setAcceptHoverEvents(true);
            setOwnedByLayout(false);
        }
        virtual ~SlotLayoutItem() = default;

        const Styling::StyleHelper& GetStyle() const { return m_style; }

    protected:
        // QGraphicsItem  
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMousePress, GetEntityId(), event);
            if (!result)
            {
                QGraphicsItem::mousePressEvent(event);
            }
        }

        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMouseRelease, GetEntityId(), event);
            if (!result)
            {
                QGraphicsItem::mouseReleaseEvent(event);
            }
        }

        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override
        {
            if (GetEntityId().IsValid())
            {
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, value);
            }
            return QGraphicsItem::itemChange(change, value);
        }

        virtual void RefreshStyle() { };
        virtual AZ::EntityId GetEntityId() const = 0;

        Styling::StyleHelper m_style;
    };
}

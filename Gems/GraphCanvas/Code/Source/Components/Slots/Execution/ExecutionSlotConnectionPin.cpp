/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QPainter>

#include <Components/Slots/Execution/ExecutionSlotConnectionPin.h>
#include <GraphCanvas/Styling/definitions.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // ExecutionSlotConnectionPin
    ///////////////////////////////
    
    ExecutionSlotConnectionPin::ExecutionSlotConnectionPin(const AZ::EntityId& slotId)
        : SlotConnectionPin(slotId)
    {
    }
    
    ExecutionSlotConnectionPin::~ExecutionSlotConnectionPin()
    {    
    }
    
    void ExecutionSlotConnectionPin::OnRefreshStyle()
    {
        m_style.SetStyle(m_slotId, Styling::Elements::ExecutionConnectionPin);
        m_connectedStyle.SetStyle(m_slotId, ".connected");
    }
    
    void ExecutionSlotConnectionPin::DrawConnectionPin(QPainter *painter, QRectF drawRect, bool isConnected)
    {

        // draw triangle, pointing to the right
        if (isConnected)
        {
            // Add fill color for slots if it is connected
            painter->setBrush(m_connectedStyle.GetBrush(Styling::Attribute::BackgroundColor, QColor{ 0xFF, 0xFF, 0xFF }));
        }
        
        QPen decorationBorder = m_style.GetBorder();
        decorationBorder.setJoinStyle(Qt::PenJoinStyle::MiterJoin);
        painter->setPen(decorationBorder);
        
        qreal sideLength = AZ::GetMin(drawRect.width(), drawRect.height());
        qreal halfLength = sideLength * 0.5;

        painter->drawConvexPolygon(QPolygonF({
            drawRect.center() + QPointF(-halfLength, -halfLength),
            drawRect.center() + QPointF(halfLength,0),
            drawRect.center() + QPointF(-halfLength, halfLength)
        }));
    }
}

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
#include "precompiled.h"

#include <QPainter>

#include <Components/Slots/Extender/ExtenderSlotConnectionPin.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/definitions.h>

namespace GraphCanvas
{
    //////////////////////////////
    // ExtenderSlotConnectionPin
    //////////////////////////////
    
    ExtenderSlotConnectionPin::ExtenderSlotConnectionPin(const AZ::EntityId& slotId)
        : SlotConnectionPin(slotId)
    {
    }
    
    ExtenderSlotConnectionPin::~ExtenderSlotConnectionPin()
    {    
    }
    
    void ExtenderSlotConnectionPin::OnRefreshStyle()
    {
        m_style.SetStyle(m_slotId, Styling::Elements::ExecutionConnectionPin);

        update();
    }
    
    void ExtenderSlotConnectionPin::DrawConnectionPin(QPainter *painter, QRectF drawRect, [[maybe_unused]] bool isConnected)
    {
        qreal radius = (AZ::GetMin(drawRect.width(), drawRect.height()) * 0.5) - m_style.GetBorder().width();

        QPen pen = m_style.GetBorder();

        QColor color = pen.color();
        color.setAlpha(255);
        pen.setColor(color);

        painter->setPen(pen);        
        
        QLineF horizontalLine(drawRect.center() - QPointF(radius, 0), drawRect.center() + QPointF(radius, 0));
        QLineF verticalLine(drawRect.center() - QPointF(0, radius), drawRect.center() + QPointF(0, radius));
        
        painter->drawLine(horizontalLine);
        painter->drawLine(verticalLine);
    }

    void ExtenderSlotConnectionPin::OnSlotClicked()
    {
        ExtenderSlotRequestBus::Event(GetEntityId(), &ExtenderSlotRequests::TriggerExtension);
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QPainter>

#include <Components/Slots/Data/DataSlotConnectionPin.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Styling/definitions.h>

#include <GraphCanvas/Utils/QtVectorMath.h>
#include <GraphCanvas/Utils/QtDrawingUtils.h>

namespace GraphCanvas
{
    //////////////////////////
    // DataSlotConnectionPin
    //////////////////////////
    
    DataSlotConnectionPin::DataSlotConnectionPin(const AZ::EntityId& slotId)
        : SlotConnectionPin(slotId)
        , m_colorPalette(nullptr)
    {
    }
    
    DataSlotConnectionPin::~DataSlotConnectionPin()
    {    
    }
    
    void DataSlotConnectionPin::OnRefreshStyle()
    {
        m_style.SetStyle(m_slotId, Styling::Elements::DataConnectionPin);

        size_t typeCount = 0;
        DataSlotRequestBus::EventResult(typeCount, GetEntityId(), &DataSlotRequests::GetContainedTypesCount);

        m_containerColorPalettes.clear();

        for (size_t i = 0; i < typeCount; ++i)
        {
            const Styling::StyleHelper* colorPalette = nullptr;
            DataSlotRequestBus::EventResult(colorPalette, GetEntityId(), &DataSlotRequests::GetContainedTypeColorPalette, i);
            if (colorPalette)
            {
                m_containerColorPalettes.emplace_back(colorPalette);
            }
        }
            
        DataSlotRequestBus::EventResult(m_colorPalette, GetEntityId(), &DataSlotRequests::GetDataColorPalette);

        update();
    }
    
    void DataSlotConnectionPin::DrawConnectionPin(QPainter *painter, QRectF drawRect, bool isConnected)
    {
        painter->save();

        DataSlotType dataType = DataSlotType::Unknown;
        DataSlotRequestBus::EventResult(dataType, GetEntityId(), &DataSlotRequests::GetDataSlotType);

        DataValueType valueType = DataValueType::Unknown;
        DataSlotRequestBus::EventResult(valueType, GetEntityId(), &DataSlotRequests::GetDataValueType);

        qreal radius = (AZ::GetMin(drawRect.width(), drawRect.height()) * 0.5) - m_style.GetBorder().width();

        QPen pen = m_style.GetBorder();
        QBrush brush = painter->brush();

        if (m_colorPalette)
        {            
            pen.setColor(m_colorPalette->GetColor(Styling::Attribute::LineColor));
            brush.setColor(m_colorPalette->GetColor(Styling::Attribute::BackgroundColor));
        }
        else
        {
            brush.setColor(pen.color());
        }

        QRectF finalRect(drawRect.center().x() - radius, drawRect.center().y() - radius, radius * 2, radius * 2);

        QLinearGradient penGradient;
        QLinearGradient fillGradient;

        if (m_containerColorPalettes.size() > 0)
        {
            QtDrawingUtils::GenerateGradients(m_containerColorPalettes, finalRect, penGradient, fillGradient);

            penGradient.setColorAt(0, m_containerColorPalettes[0]->GetColor(Styling::Attribute::LineColor));
            fillGradient.setColorAt(0, m_containerColorPalettes[0]->GetColor(Styling::Attribute::BackgroundColor));

            double transition = 0.1 * (1.0 / m_containerColorPalettes.size());

            for (size_t i = 1; i < m_containerColorPalettes.size(); ++i)
            {
                double transitionStart = AZStd::max(0.0, (double)i / m_containerColorPalettes.size() - (transition * 0.5));
                double transitionEnd = AZStd::min(1.0, (double)i / m_containerColorPalettes.size() + (transition * 0.5));

                penGradient.setColorAt(transitionStart, m_containerColorPalettes[i - 1]->GetColor(Styling::Attribute::LineColor));
                penGradient.setColorAt(transitionEnd, m_containerColorPalettes[i]->GetColor(Styling::Attribute::LineColor));

                fillGradient.setColorAt(transitionStart, m_containerColorPalettes[i - 1]->GetColor(Styling::Attribute::BackgroundColor));
                fillGradient.setColorAt(transitionEnd, m_containerColorPalettes[i]->GetColor(Styling::Attribute::BackgroundColor));
            }

            penGradient.setColorAt(1, m_containerColorPalettes[m_containerColorPalettes.size() - 1]->GetColor(Styling::Attribute::LineColor));
            fillGradient.setColorAt(1, m_containerColorPalettes[m_containerColorPalettes.size() - 1]->GetColor(Styling::Attribute::BackgroundColor));

            pen.setBrush(penGradient);            
        }

        // Add fill color for slots if it is connected
        if (m_containerColorPalettes.size() > 0)
        {
            brush = fillGradient;
        }
        else if (m_colorPalette)
        {
            brush.setColor(m_colorPalette->GetColor(Styling::Attribute::BackgroundColor));
        }
        else
        {
            brush.setColor(pen.color());
        }

        painter->setPen(pen);
        
        if (dataType == DataSlotType::Reference)
        {
            QRectF filledHalfRect = QRectF(drawRect.x(), drawRect.y(), drawRect.width() * 0.5, drawRect.height());
            QRectF unfilledHalfRect = filledHalfRect;
            unfilledHalfRect.moveLeft(drawRect.center().x());

            ConnectionType connectionType = CT_Invalid;
            SlotRequestBus::EventResult(connectionType, GetEntityId(), &SlotRequests::GetConnectionType);

            if (connectionType == CT_Output)
            {
                QRectF swapRect = filledHalfRect;
                filledHalfRect = unfilledHalfRect;
                unfilledHalfRect = swapRect;
            }

            // Draw the unfilled half
            painter->setClipRect(filledHalfRect);

            if (valueType == DataValueType::Container)
            {
                painter->drawRect(finalRect);
            }
            else
            {
                painter->drawEllipse(drawRect.center(), radius, radius);
            }

            painter->setClipRect(unfilledHalfRect);

            if (valueType == DataValueType::Container)
            {
                painter->fillRect(finalRect, brush);
            }
            else
            {
                painter->setBrush(brush);
                painter->drawEllipse(drawRect.center(), radius, radius);
            }            
        }        
        else if (dataType == DataSlotType::Value)
        {
            if (isConnected)
            {
                painter->setBrush(brush);
            }

            if (valueType == DataValueType::Primitive)
            {
                painter->drawEllipse(drawRect.center(), radius, radius);
            }
            else
            {
                painter->drawRect(finalRect);
            }
        }

        painter->restore();
    }

}

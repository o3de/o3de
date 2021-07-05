/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QPaintEvent>
#include <QStylePainter>
#include <QStyleOption>
#include <QImage>
#include <QPainter>

namespace AzQtComponents
{

    Swatch::Swatch(QWidget* parent /* = nullptr */)
        : Swatch({}, parent)
    {
    }

    Swatch::Swatch(const AZ::Color& color, QWidget* parent /* = nullptr */)
        : QFrame(parent)
        , m_color(color)
    {
        setMinimumSize({ 8, 8 });
    }

    Swatch::~Swatch()
    {
    }

    AZ::Color Swatch::color() const
    {
        return m_color;
    }

    void Swatch::setColor(const AZ::Color& color)
    {
        if (color.IsClose(m_color))
        {
            return;
        }

        m_color = color;
        emit colorChanged(m_color);
        update();
    }

    bool Swatch::drawSwatch(const QStyleOption* option, QPainter* painter, const QPoint& targetOffset) const
    {
        if (option->type != QStyleOption::SO_Frame)
        {
            return false;
        }

        painter->save();

        painter->translate(targetOffset);

        painter->setPen(Qt::NoPen);
        painter->fillRect(contentsRect(), MakeAlphaBrush(ToQColor(m_color)));
        style()->drawPrimitive(QStyle::PE_Frame, option, painter, this);
        
        painter->restore();
        return true;
    }

    void Swatch::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

        QPainter painter(this);
        QStyleOptionFrame option;
        option.initFrom(this);
        drawSwatch(&option, &painter);
    }

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_Swatch.cpp"

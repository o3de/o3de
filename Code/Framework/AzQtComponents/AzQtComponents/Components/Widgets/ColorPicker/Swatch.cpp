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
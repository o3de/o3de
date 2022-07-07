/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Casting/numeric_cast.h>


#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QKeyEvent>

namespace AzQtComponents
{

const char* GradientSlider::s_gradientSliderClass = "GradientSlider";

GradientSlider::GradientSlider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent)
{
    setProperty("class", s_gradientSliderClass);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true);

    m_colorFunction = [](qreal value) {
        return QColor::fromRgbF(value, value, value);
    };

    m_toolTipFunction = [this](qreal value) {
        QColor rgb = m_colorFunction(value);

        return QStringLiteral("RGB: %1, %2, %3").arg(rgb.red()).arg(rgb.green()).arg(rgb.blue());
    };
}

GradientSlider::GradientSlider(QWidget* parent)
    : GradientSlider(Qt::Horizontal, parent)
{
}

GradientSlider::~GradientSlider()
{
}

void GradientSlider::setColorFunction(GradientSlider::ColorFunction colorFunction)
{
    m_colorFunction = colorFunction;
    updateGradient();
}

void GradientSlider::setToolTipFunction(GradientSlider::ToolTipFunction toolTipFunction)
{
    m_toolTipFunction = toolTipFunction;
}

void GradientSlider::updateGradient()
{
    initGradientColors();
    update();
}

bool GradientSlider::GetIgnoreWheelEvents() const
{
    return m_ignoreWheelEvents;
}

void GradientSlider::SetIgnoreWheelEvents(bool shouldIgnore)
{
    m_ignoreWheelEvents = shouldIgnore;
}

QColor GradientSlider::colorAt(qreal position) const
{
    return m_colorFunction(position);
}

void GradientSlider::initGradientColors()
{
    if (m_colorFunction)
    {
        const int numGradientStops = 12;

        QGradientStops stops;
        stops.reserve(numGradientStops);

        for (int i = 0; i < numGradientStops; ++i)
        {
            const auto position = static_cast<qreal>(i)/(numGradientStops - 1);
            stops.append({ position, m_colorFunction(position) });
        }

        m_gradient.setStops(stops);
    }
}

void GradientSlider::resizeEvent(QResizeEvent* e)
{
    QSlider::resizeEvent(e);

    const QRect r = contentsRect();
    m_gradient.setStart(r.bottomLeft());
    m_gradient.setFinalStop(orientation() == Qt::Horizontal ? r.bottomRight() : r.topLeft());
}

void GradientSlider::mouseMoveEvent(QMouseEvent* event)
{
    int intValue = Slider::valueFromPosition(this, event->pos(), width(), height(), rect().bottom());

    qreal value = (aznumeric_cast<qreal>(intValue - minimum()) / aznumeric_cast<qreal>(maximum() - minimum()));

    const QString toolTipText = m_toolTipFunction(value);

    QPoint globalPosition = mapToGlobal(QPoint(0, 0));
    Slider::showHoverToolTip(toolTipText, globalPosition, this, this, width(), height(), m_toolTipOffset);

    QSlider::mouseMoveEvent(event);
}

void GradientSlider::wheelEvent(QWheelEvent* e)
{
    if (GetIgnoreWheelEvents())
    {
        e->ignore();
        return;
    }

    QSlider::wheelEvent(e);
}

void GradientSlider::keyPressEvent(QKeyEvent * e)
{
    switch (e->key())
    {
    case Qt::Key_Up:
    case Qt::Key_Right:
        if (e->modifiers() & Qt::ShiftModifier)
        {
            setValue(value() + pageStep());
            return;
        }
        break;
    case Qt::Key_Down:
    case Qt::Key_Left:
        if (e->modifiers() & Qt::ShiftModifier)
        {
            setValue(value() - pageStep());
            return;
        }
        break;
    }

    QSlider::keyPressEvent(e);
}

int GradientSlider::sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    return config.gradientSlider.thickness;
}

int GradientSlider::sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    return config.gradientSlider.length;
}

QRect GradientSlider::sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config)
{
    // start from slider handle rect from common style and change dimensions to our desired thickness/length
    QRect rect = style->QCommonStyle::subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderHandle, widget);

    if (option->orientation == Qt::Horizontal)
    {
        rect.setHeight(config.gradientSlider.thickness);
        rect.setWidth(config.gradientSlider.length);
        rect.moveTop(option->rect.center().y() - rect.height() / 2 + 1);
    }
    else
    {
        rect.setWidth(config.gradientSlider.thickness);
        rect.setHeight(config.gradientSlider.length);
        rect.moveLeft(option->rect.center().x() - rect.width() / 2 + 1);
    }

    return rect;
}

QRect GradientSlider::sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config)
{
    // start from slider groove rect from common style and change to our desired thickness
    QRect rect = style->QCommonStyle::subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderGroove, widget);

    if (option->orientation == Qt::Horizontal)
    {
        rect.setHeight(config.gradientSlider.thickness);
    }
    else
    {
        rect.setWidth(config.gradientSlider.thickness);
    }

    rect.moveCenter(option->rect.center());

    return rect;
}

bool GradientSlider::drawSlider(const Style* style, const QStyleOptionSlider* option, QPainter* painter, const QWidget* widget, const Slider::Config& config)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // groove
    {
        const auto& border = config.gradientSlider.grooveBorder;

        const QPen pen(border.color, border.thickness);

        const qreal w = 0.5*pen.widthF();
        const auto r = QRectF(style->subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderGroove, widget)).adjusted(w, w, -w, -w);

        painter->setPen(Qt::NoPen);
        painter->setBrush(Style::cachedPixmap(QStringLiteral(":/stylesheet/img/UI20/alpha-background.png")));
        painter->drawRoundedRect(r, border.radius, border.radius);

        painter->setPen(pen);

        const QBrush brush = static_cast<const GradientSlider*>(widget)->gradientBrush();
        painter->setBrush(brush);

        painter->drawRoundedRect(r, border.radius, border.radius);
    }

    // handle
    {
        const auto& border = config.gradientSlider.handleBorder;

        const QPen pen(border.color, border.thickness);

        const qreal w = 0.5*pen.widthF();
        const auto r = QRectF(style->subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderHandle, widget)).adjusted(w, w, -w, -w);

        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(r, border.radius, border.radius);
    }

    painter->restore();

    return true;
}

QBrush GradientSlider::gradientBrush() const
{
    return m_gradient;
}

} // namespace AzQtComponents

#include "Components/Widgets/moc_GradientSlider.cpp"

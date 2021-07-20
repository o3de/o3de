/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolTip>
#include <QVariant>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace AzQtComponents
{
static QString g_horizontalSliderClass = QStringLiteral("HorizontalSlider");
static QString g_verticalSliderClass = QStringLiteral("VerticalSlider");
static QPoint g_horizontalToolTip;
static QPoint g_verticalToolTip;

static void ReadBorder(QSettings& settings, const QString& name, Slider::Border& border)
{
    settings.beginGroup(name);
    ConfigHelpers::read<int>(settings, QStringLiteral("Thickness"), border.thickness);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), border.color);
    ConfigHelpers::read<qreal>(settings, QStringLiteral("Radius"), border.radius);
    settings.endGroup();
}

static void ReadGradientSlider(QSettings& settings, const QString& name, Slider::GradientSliderConfig& gradientSlider)
{
    settings.beginGroup(name);
    ConfigHelpers::read<int>(settings, QStringLiteral("Thickness"), gradientSlider.thickness);
    ConfigHelpers::read<int>(settings, QStringLiteral("Length"), gradientSlider.length);
    ReadBorder(settings, QStringLiteral("GrooveBorder"), gradientSlider.grooveBorder);
    ReadBorder(settings, QStringLiteral("HandleBorder"), gradientSlider.handleBorder);
    settings.endGroup();
}

static void ReadGroove(QSettings& settings, const QString& name, Slider::SliderConfig::GrooveConfig& groove)
{
    settings.beginGroup(name);

    ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), groove.color);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("ColorHovered"), groove.colorHovered);
    ConfigHelpers::read<int>(settings, QStringLiteral("Width"), groove.width);
    ConfigHelpers::read<int>(settings, QStringLiteral("MidMarkerHeight"), groove.midMarkerHeight);

    settings.endGroup();
}

static void ReadHandle(QSettings& settings, const QString& name, Slider::SliderConfig::HandleConfig& handle)
{
    settings.beginGroup(name);

    ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), handle.color);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("DisabledColor"), handle.colorDisabled);
    ConfigHelpers::read<int>(settings, QStringLiteral("Size"), handle.size);
    ConfigHelpers::read<int>(settings, QStringLiteral("SizeMinusMargin"), handle.sizeMinusMargin);
    ConfigHelpers::read<int>(settings, QStringLiteral("HoverSize"), handle.hoverSize);

    settings.endGroup();
}

static void ReadSlider(QSettings& settings, const QString& name, Slider::SliderConfig& slider)
{
    settings.beginGroup(name);

    ReadHandle(settings, QStringLiteral("Handle"), slider.handle);
    ReadGroove(settings, QStringLiteral("Groove"), slider.grove);

    settings.endGroup();
}

static QString g_midPointStyleClass = QStringLiteral("MidPoint");

CustomSlider::CustomSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
}

void CustomSlider::initStyleOption(QStyleOptionSlider& option)
{
    QSlider::initStyleOption(&option);
}

void CustomSlider::mousePressEvent(QMouseEvent* ev)
{
    Q_EMIT moveSlider(true);
    QSlider::mousePressEvent(ev);
}
void CustomSlider::mouseReleaseEvent(QMouseEvent* ev)
{
    Q_EMIT moveSlider(false);
    QSlider::mouseReleaseEvent(ev);
}

void CustomSlider::wheelEvent(QWheelEvent* ev)
{
    if (hasFocus())
    {
        QSlider::wheelEvent(ev);
    }
    else
    {
        ev->ignore();
    }
}

Slider::Slider(QWidget* parent)
    : Slider(Qt::Horizontal, parent)
{
}

Slider::Slider(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* noFrameLayout = new QVBoxLayout(this);
    noFrameLayout->setContentsMargins(0, 0, 0, 0);

    m_slider = new CustomSlider(orientation, this);
    noFrameLayout->addWidget(m_slider);
    connect(m_slider, &CustomSlider::moveSlider, this, &Slider::sliderIsInMoving);
    m_slider->installEventFilter(this);

    connect(m_slider, &QSlider::sliderPressed, this, &Slider::sliderPressed);
    connect(m_slider, &QSlider::sliderMoved, this, &Slider::sliderMoved);
    connect(m_slider, &QSlider::sliderReleased, this, &Slider::sliderReleased);
    connect(m_slider, &QSlider::actionTriggered, this, &Slider::actionTriggered);

    m_slider->setMouseTracking(true);
}


void Slider::initStaticVars(const QPoint& verticalToolTipOffset, const QPoint& horizontalToolTipOffset)
{
    g_horizontalToolTip = horizontalToolTipOffset;
    g_verticalToolTip = verticalToolTipOffset;
}

void Slider::sliderIsInMoving(bool b)
{
    m_moveSlider = b;
}

bool Slider::IsSliderBeingMoved() const
{
    return m_moveSlider;
}

QSize Slider::sizeHint() const
{
    return m_slider->sizeHint();
}

QSize Slider::minimumSizeHint() const
{
    return m_slider->minimumSizeHint();
}

void Slider::setFocusProxy(QWidget* proxy)
{
    m_slider->setFocusProxy(proxy);
}

QWidget* Slider::focusProxy() const
{
    return m_slider->focusProxy();
}

void Slider::setTracking(bool enable)
{
    m_slider->setTracking(enable);
}

bool Slider::hasTracking() const
{
    return m_slider->hasTracking();
}

void Slider::setOrientation(Qt::Orientation orientation)
{
    m_slider->setOrientation(orientation);
}

Qt::Orientation Slider::orientation() const
{
    return m_slider->orientation();
}

void Slider::applyMidPointStyle(Slider* slider)
{
    slider->setProperty("class", g_midPointStyleClass);
    slider->m_slider->setProperty("class", g_midPointStyleClass);
}

void Slider::setToolTipFormatting(const QString& prefix, const QString& postFix)
{
    m_toolTipPrefix = prefix;
    m_toolTipPostfix = postFix;
}

Slider::Config Slider::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();

    ReadGradientSlider(settings, "GradientSlider", config.gradientSlider);
    ReadSlider(settings, "Slider", config.slider);
    ConfigHelpers::read<QPoint>(settings, QStringLiteral("HorizontalToolTipOffset"), config.horizontalToolTipOffset);
    ConfigHelpers::read<QPoint>(settings, QStringLiteral("VerticalToolTipOffset"), config.verticalToolTipOffset);

    return config;
}

Slider::Config Slider::defaultConfig()
{

   Config config;

    config.gradientSlider.thickness = 16;
    config.gradientSlider.length = 6;
    config.gradientSlider.grooveBorder.thickness = 1;
    config.gradientSlider.grooveBorder.color = QColor(0x33, 0x33, 0x33);
    config.gradientSlider.grooveBorder.radius = 1.5;
    config.gradientSlider.handleBorder.thickness = 1;
    config.gradientSlider.handleBorder.color = QColor(0xff, 0xff, 0xff);
    config.gradientSlider.handleBorder.radius = 1.5;

    config.slider.handle.color = { "#0185D7" };
    config.slider.handle.colorDisabled = { "#999999" };
    config.slider.handle.size = 12;
    config.slider.handle.sizeMinusMargin = 8;
    config.slider.handle.hoverSize = 12;

    config.slider.grove.color = { "#999999" };
    config.slider.grove.colorHovered = { "#CCDFF5" };
    config.slider.grove.width = 2;
    config.slider.grove.midMarkerHeight = 16;

    // numbers chosen because they place the tooltip at a spot that looks decent
    config.horizontalToolTipOffset = QPoint(-8, -16);
    config.verticalToolTipOffset = QPoint(8, -24);

    return config;
}

int Slider::valueFromPosition(QSlider* slider, const QPoint& pos, int width, int height, int bottom)
{
    if (slider->orientation() == Qt::Horizontal)
    {
        return QStyle::sliderValueFromPosition(slider->minimum(), slider->maximum(), pos.x(), width);
    }
    else
    {
        return QStyle::sliderValueFromPosition(slider->minimum(), slider->maximum(), bottom - pos.y(), height);
    }
}

int Slider::valueFromPos(const QPoint& pos) const
{
    int localDistance = getDistanceFromVisibleGrooveStart(pos);
    int totalLength = getVisibleGrooveLength();

    // Vertical slider values are reversed so that min is at the bottom and max is at the top
    bool rangeReversed = (m_slider->orientation() == Qt::Vertical);
    return QStyle::sliderValueFromPosition(m_slider->minimum(), m_slider->maximum(), localDistance, totalLength, rangeReversed);
}

QRect Slider::getVisibleGrooveRect() const
{
    QStyleOptionSlider option;
    m_slider->initStyleOption(option);

    int handleThickness = m_slider->style()->pixelMetric(QStyle::PM_SliderThickness, &option, m_slider);
    QRect grooveRect = m_slider->style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, m_slider);
    QPoint grooveCenter = grooveRect.center();
    int grooveLength = orientation() == Qt::Horizontal ? grooveRect.width() : grooveRect.height();
    int visibleGrooveLength = grooveLength - handleThickness;
    if (m_slider->orientation() == Qt::Horizontal)
    {
        grooveRect.setWidth(visibleGrooveLength);
    }
    else
    {
        grooveRect.setHeight(visibleGrooveLength);
    }
    grooveRect.moveCenter(grooveCenter);
    return grooveRect;
}

int Slider::getVisibleGrooveLength() const
{
    QRect visibleGrooveRect = getVisibleGrooveRect();
    int visibleGrooveLength = orientation() == Qt::Horizontal ? visibleGrooveRect.width() : visibleGrooveRect.height();
    return visibleGrooveLength;
}

int Slider::getVisibleGrooveStart() const
{
    QStyleOptionSlider option;
    m_slider->initStyleOption(option);

    int handleThickness = m_slider->style()->pixelMetric(QStyle::PM_SliderThickness, &option, m_slider);
    return handleThickness / 2;
}

int Slider::getDistanceFromVisibleGrooveStart(const QPoint& point) const
{
    int distanceFromSlider = orientation() == Qt::Horizontal ? point.x() : point.y();
    int distanceFromVisibleGrooveStart = distanceFromSlider - getVisibleGrooveStart();
    return distanceFromVisibleGrooveStart;
}

bool Slider::isPointInVisibleGrooveArea(const QPoint& point) const
{
    int distance = getDistanceFromVisibleGrooveStart(point);
    return (distance >= 0 && distance <= getVisibleGrooveLength());
}

void Slider::showHoverToolTip(const QString& toolTipText, const QPoint& globalPosition, QSlider* slider, QWidget* toolTipParentWidget, int width, int height, const QPoint& toolTipOffset)
{
    QPoint toolTipPosition;

    // QToolTip::showText puts the tooltip (2, 16) pixels down from the specified point - it's hardcoded.
    // We want the tooltip to always appear above the slider, so we offset it according to our settings (which
    // are solely there to put the tooltip in a readable spot - somewhere tracking the mouse x position directly
    // above a horizontal slider, and tracking the mouse y position directly to the right of vertical sliders).
    if (slider->orientation() == Qt::Horizontal)
    {
        toolTipPosition = QPoint(QCursor::pos().x() + toolTipOffset.x(), globalPosition.y() - height + toolTipOffset.y());
    }
    else
    {
        toolTipPosition = QPoint(globalPosition.x() + width + toolTipOffset.x(), QCursor::pos().y() + toolTipOffset.y());
    }

    QToolTip::showText(toolTipPosition, toolTipText, toolTipParentWidget);

    slider->update();
}

void Slider::ShowValueToolTip(int sliderPos)
{
    const QString toolTipText = QStringLiteral("%1%2%3").arg(m_toolTipPrefix, hoverValueText(sliderPos), m_toolTipPostfix);

    QPoint globalPosition = parentWidget()->mapToGlobal(pos());

    QPoint offsetPos;
    if (m_slider->orientation() == Qt::Horizontal)
    {
        offsetPos = g_horizontalToolTip + m_toolTipOffset;
    }
    else
    {
        offsetPos = g_verticalToolTip + m_toolTipOffset;
    }
    showHoverToolTip(toolTipText, globalPosition, m_slider, this, width(), height(), offsetPos);
}

bool Slider::eventFilter(QObject* watched, QEvent* event)
{
    if (isEnabled() && !m_moveSlider)
    {
        switch (event->type())
        {
            case QEvent::Leave:
                m_mousePos = QPoint();
                break;

            case QEvent::ToolTip:
                return true;
                break;
        }
    }
    return QObject::eventFilter(watched, event);
}

int Slider::sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
{
    if (widget && style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderThickness(style, option, widget, config);
        }
    }
    else
    {
        return config.slider.handle.size;
    }

    return -1;
}

int Slider::sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
{
    if (widget && style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderLength(style, option, widget, config);
        }
    }
    else
    {
        return config.slider.handle.size;
    }

    return -1;
}

QRect Slider::sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config, bool isHovered)
{
    Q_ASSERT(widget);

    if (style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderHandleRect(style, option, widget, config);
        }
    }
    else
    {
        const QRect grooveRect = sliderGrooveRect(style, option, widget, config);
        QRect handleRect = style->QCommonStyle::subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderHandle, widget);
        QPoint handleCenter = handleRect.center();
        if (!isHovered)
        {
            // Adjust handle rect based on the smaller handle size when not hovered
            int newHandleSize = config.slider.handle.sizeMinusMargin;
            handleRect.setSize(QSize(newHandleSize, newHandleSize));
        }
        if (option->orientation == Qt::Horizontal)
        {
            handleRect.moveCenter({ handleCenter.x(), grooveRect.center().y() });
        }
        else
        {
            handleRect.moveCenter({ grooveRect.center().x(), handleCenter.y() });
        }

        return handleRect;
    }

    return {};
}

QRect Slider::sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config)
{
    Q_ASSERT(widget);

    if (style->hasClass(widget, GradientSlider::s_gradientSliderClass))
    {
        if (qobject_cast<const GradientSlider*>(widget))
        {
            return GradientSlider::sliderGrooveRect(style, option, widget, config);
        }
    }
    else
    {
        QRect grooveRect = style->QCommonStyle::subControlRect(QProxyStyle::CC_Slider, option, QProxyStyle::SC_SliderGroove, widget);        
        if (option->orientation == Qt::Horizontal)
        {
            grooveRect.setHeight(config.slider.grove.width);
        }
        else
        {
            grooveRect.setWidth(config.slider.grove.width);
        }
        grooveRect.moveCenter(widget->rect().center());
        return grooveRect;
    }

    return {};
}

bool Slider::polish(Style* style, QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(config);

    auto polishSlider = [style](auto slider)
    {
        // Qt's stylesheet parsing doesn't set custom properties on things specified via
        // pseudo-states, such as horizontal/vertical, so we implement our own
        if (slider->orientation() == Qt::Horizontal)
        {
            Style::removeClass(slider, g_verticalSliderClass);
            Style::addClass(slider, g_horizontalSliderClass);
        }
        else
        {
            Style::removeClass(slider, g_horizontalSliderClass);
            Style::addClass(slider, g_verticalSliderClass);
        }
    };

    // Apply the right styles to our QSlider objects, so that css rules can be properly applied
    if (auto slider = qobject_cast<Slider*>(widget))
    {
        polishSlider(slider);
    }

    // Also apply the right styles to our AzQtComponents::Slider container
    if (auto slider = qobject_cast<QSlider*>(widget))
    {
        polishSlider(slider);
    }

    return false;
}

bool Slider::drawSlider(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Slider::Config& config)
{
   const QStyleOptionSlider* sliderOption = static_cast<const QStyleOptionSlider*>(option);
   const QSlider* slider = qobject_cast<const QSlider*>(widget);
   if (slider != nullptr)
    {
        if (style->hasClass(widget, GradientSlider::s_gradientSliderClass))
        {
            Q_ASSERT(widget);
            return GradientSlider::drawSlider(style, static_cast<const QStyleOptionSlider*>(option), painter, widget, config);
        }
        else if (auto sliderWidget = qobject_cast<const Slider*>(widget->parentWidget()))
        {
            const QColor handleColor = option->state & QStyle::State_Enabled ? config.slider.handle.color : config.slider.handle.colorDisabled;
            const QRect visibleGrooveRect = sliderWidget->getVisibleGrooveRect();

            // only show the hover highlight if the mouse is hovered over the control, and the slider isn't being clicked on or dragged
            const bool mouseIsOnControl = !sliderWidget->m_mousePos.isNull();
            const bool isHovered = mouseIsOnControl && !(QApplication::mouseButtons() & Qt::MouseButton::LeftButton);
            const QRect handleRect = sliderHandleRect(style, sliderOption, widget, config, mouseIsOnControl);
            Qt::Orientation orientation = sliderOption->orientation;

            //draw the groove background, without highlight colors from the mouse hover or the selected value
            if (option->state & QStyle::State_Enabled)
            {
                painter->fillRect(visibleGrooveRect, config.slider.grove.color);

                if (style->hasClass(widget, g_midPointStyleClass))
                {
                    QPoint valueGrooveStart = orientation == Qt::Horizontal ? QPoint(visibleGrooveRect.center().x(), visibleGrooveRect.top()) : QPoint(visibleGrooveRect.left(), visibleGrooveRect.center().y());
                    QPoint handleMiddle = orientation == Qt::Horizontal ? QPoint(handleRect.x(), visibleGrooveRect.bottom()) : QPoint(visibleGrooveRect.right(), handleRect.bottom());

                    //color the grove from the mid to the handle to show the amount selected
                    painter->fillRect(QRect(valueGrooveStart, handleMiddle), handleColor);

                    drawMidPointMarker(painter, config, sliderOption->orientation, visibleGrooveRect, handleColor);
                }
                else
                {
                    QPoint valueGrooveStart = orientation == Qt::Horizontal ? visibleGrooveRect.topLeft() : visibleGrooveRect.bottomLeft();
                    QPoint valueGrooveEnd = orientation == Qt::Horizontal ? QPoint(handleRect.x(), visibleGrooveRect.bottom()) : QPoint(visibleGrooveRect.right(), handleRect.y());

                    //color the grove from the start to the handle
                    painter->fillRect(QRect(valueGrooveStart, valueGrooveEnd), handleColor);
                }
            }
            else
            {
                const int disabledGap = 5;
                QRect firstGrooveRect;
                QRect lastGrooveRect;
                if (orientation == Qt::Horizontal)
                {
                    firstGrooveRect = QRect(visibleGrooveRect.topLeft(), QPoint(handleRect.left() - disabledGap, visibleGrooveRect.bottom()));
                    lastGrooveRect = QRect(QPoint(handleRect.right() + disabledGap, visibleGrooveRect.top()), visibleGrooveRect.bottomRight());
                }
                else
                {
                    firstGrooveRect = QRect(visibleGrooveRect.topLeft(), QPoint(visibleGrooveRect.right(), handleRect.top() - disabledGap));
                    lastGrooveRect = QRect(QPoint(visibleGrooveRect.left(), handleRect.bottom() + disabledGap), visibleGrooveRect.bottomRight());
                }
                if(firstGrooveRect.width() > 0 && firstGrooveRect.height() > 0)
                    painter->fillRect(firstGrooveRect, config.slider.grove.color);
                if (lastGrooveRect.width() > 0 && lastGrooveRect.height() > 0)
                    painter->fillRect(lastGrooveRect, config.slider.grove.color);
            }

            bool drawHighlights = sliderWidget->isPointInVisibleGrooveArea(sliderWidget->m_mousePos) && isHovered;
            if (drawHighlights)
            {
                if (style->hasClass(widget, g_midPointStyleClass))
                {
                    drawMidPointGrooveHighlights(painter, config, visibleGrooveRect, handleRect, sliderWidget->m_mousePos, sliderOption->orientation);
                }
                else
                {
                    drawGrooveHighlights(painter, config, visibleGrooveRect, handleRect, sliderWidget->m_mousePos, sliderOption->orientation);
                }
            }

            drawHandle(option, painter, config, handleRect, handleColor);

            return true;
        }
    }
    return false;
}

void Slider::drawMidPointMarker(QPainter* painter, const Slider::Config& config, Qt::Orientation orientation, const QRect& grooveRect, const QColor& midMarkerColor)
{
    int width = orientation == Qt::Horizontal ? config.slider.grove.width : config.slider.grove.midMarkerHeight;
    int height = orientation == Qt::Horizontal ? config.slider.grove.midMarkerHeight : config.slider.grove.width;

    auto rect = QRect(0, 0, width, height);
    rect.moveCenter(grooveRect.center());
    painter->fillRect(rect, midMarkerColor);
}

void Slider::drawMidPointGrooveHighlights(QPainter* painter, const Slider::Config& config, const QRect& grooveRect, const QRect& handleRect, const QPoint& mousePos, Qt::Orientation orientation)
{
    QPoint valueGrooveStart = orientation == Qt::Horizontal ? QPoint(grooveRect.center().x(), grooveRect.top()) : QPoint(grooveRect.left(), grooveRect.center().y());

    QPoint hoveredEnd;
    QPoint hoveredStart = valueGrooveStart;

    if (orientation == Qt::Horizontal)
    {
        const auto x = qBound(grooveRect.left(), mousePos.x(), grooveRect.right());
        hoveredEnd = QPoint(x, grooveRect.bottom());

        // be careful not to completely cover the value highlight
        if (((handleRect.x() < valueGrooveStart.x()) && (mousePos.x() < handleRect.x())) ||
            ((handleRect.x() > valueGrooveStart.x()) && (mousePos.x() > handleRect.x())))
        {
            hoveredStart.setX(handleRect.x());
        }
    }
    else
    {
        const auto y = qBound(grooveRect.top(), mousePos.y(), grooveRect.bottom());
        hoveredEnd = QPoint(grooveRect.right(), y);

        // be careful not to completely cover the value highlight
        if (((handleRect.y() < valueGrooveStart.y()) && (mousePos.y() < handleRect.y())) ||
            ((handleRect.y() > valueGrooveStart.y()) && (mousePos.y() > handleRect.y())))
        {
            hoveredStart.setY(handleRect.y());
        }
    }

    painter->fillRect(QRect(hoveredStart, hoveredEnd), config.slider.grove.colorHovered);
}

void Slider::drawGrooveHighlights(QPainter* painter, const Slider::Config& config, const QRect& grooveRect, const QRect& handleRect,
                                  const QPoint& mousePos, Qt::Orientation orientation)
{
    QPoint valueGrooveStart = orientation == Qt::Horizontal ? grooveRect.topLeft() : grooveRect.bottomLeft();
    QPoint valueGrooveEnd = orientation == Qt::Horizontal ? QPoint(handleRect.x(), grooveRect.bottom()) : QPoint(grooveRect.right(), handleRect.y());

    QPoint hoveredStart = valueGrooveStart;
    QPoint hoveredEnd;

    if (orientation == Qt::Horizontal)
    {
        const auto x = qBound(grooveRect.left(), mousePos.x(), grooveRect.right());

        if (x > valueGrooveEnd.x())
        {
            hoveredStart.setX(valueGrooveEnd.x());
        }

        hoveredEnd = QPoint(x, grooveRect.bottom());
    }
    else
    {
        const auto y = qBound(grooveRect.top(), mousePos.y(), grooveRect.bottom());

        if (y < valueGrooveEnd.y())
        {
            hoveredStart.setY(valueGrooveEnd.y());
        }

        hoveredEnd = QPoint(grooveRect.right(), y);
    }

    painter->fillRect(QRect(hoveredStart, hoveredEnd), config.slider.grove.colorHovered);
}

void Slider::drawHandle(const QStyleOption* option, QPainter* painter, const Slider::Config& config, const QRect& handleRect, const QColor& handleColor)
{
    //handle
    Q_UNUSED(config);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (option->state & QStyle::State_Enabled)
    {
        painter->setPen(Qt::NoPen);
    }
    else
    {
        auto pen = painter->pen();
        pen.setBrush(option->palette.window());
        painter->setPen(pen);
    }
    painter->setBrush(handleColor);
    painter->drawRoundedRect(handleRect, 100.0, 100.0);

    painter->restore();
}

int Slider::styleHintAbsoluteSetButtons()
{
    // Make sliders jump to the value when the user clicks on them instead of the Qt default of moving closer to the clicked location
    return (Qt::LeftButton | Qt::MidButton | Qt::RightButton);
}


SliderInt::SliderInt(QWidget* parent)
    : SliderInt(Qt::Horizontal, parent)
{
}

SliderInt::SliderInt(Qt::Orientation orientation, QWidget* parent)
    : Slider(orientation, parent)
{
    connect(m_slider, &QSlider::valueChanged, this, [this](int newValue) {
        Q_EMIT valueChanged(newValue);
        if (IsSliderBeingMoved())
        {
            ShowValueToolTip(newValue);
        }
        });
    connect(m_slider, &QSlider::rangeChanged, this, &SliderInt::rangeChanged);
}

void SliderInt::setValue(int value)
{
    m_slider->setValue(value);
}

int SliderInt::value() const
{
    return m_slider->value();
}

void SliderInt::setMinimum(int min)
{
    m_slider->setMinimum(min);
}

int SliderInt::minimum() const
{
    return m_slider->minimum();
}

void SliderInt::setMaximum(int max)
{
    m_slider->setMaximum(max);
}

int SliderInt::maximum() const
{
    return m_slider->maximum();
}

void SliderInt::setRange(int min, int max)
{
    m_slider->setRange(min, max);
}

QString SliderInt::hoverValueText(int sliderValue) const
{
    return QStringLiteral("%1").arg(sliderValue);
}

SliderDouble::SliderDouble(QWidget* parent)
    : SliderDouble(Qt::Horizontal, parent)
{
}

static const int g_sliderDecimalPrecisonDefault = 7;
static const double g_defaultCurveMidpoint = 0.5;

SliderDouble::SliderDouble(Qt::Orientation orientation, QWidget* parent)
    : Slider(orientation, parent)
    , m_decimals(g_sliderDecimalPrecisonDefault)
{
    setRange(m_minimum, m_maximum, m_numSteps);

    connect(m_slider, &QSlider::valueChanged, this, [this](int newValue) {
        Q_EMIT valueChanged(calculateRealSliderValue(newValue));
        if (IsSliderBeingMoved())
        {
            ShowValueToolTip(newValue);
        }
    });
}

void SliderDouble::setValue(double value)
{
    m_slider->setValue(aznumeric_cast<int>(convertToSliderValue(value)));
}

double SliderDouble::value() const
{
    return calculateRealSliderValue(m_slider->value());
}

double SliderDouble::minimum() const
{
    return m_minimum;
}

void SliderDouble::setMinimum(double min)
{
    setRange(min, m_maximum, m_numSteps);
}

double SliderDouble::maximum() const
{
    return m_maximum;
}

void SliderDouble::setMaximum(double max)
{
    setRange(m_minimum, max, m_numSteps);
}

int SliderDouble::numSteps() const
{
    return m_numSteps;
}

void SliderDouble::setNumSteps(int steps)
{
    setRange(m_minimum, m_maximum, steps);
}

void SliderDouble::setRange(double min, double max, int numSteps)
{
    m_slider->setRange(0, numSteps);

    m_maximum = max;
    m_minimum = min;
    m_numSteps = numSteps;

    Q_ASSERT(m_numSteps != 0);
    if (m_numSteps == 0)
    {
        m_numSteps = 1;
    }

    Q_EMIT rangeChanged(min, max, numSteps);
}

void SliderDouble::setCurveMidpoint(double midpoint)
{
    QSignalBlocker block(this);
    const double currentValue = value();
    m_curveMidpoint = std::clamp<double>(midpoint, 0, 1);
    setValue(currentValue);
}

QString SliderDouble::hoverValueText(int sliderValue) const
{
    // maybe format this, max number of digits?
    QString valueText = locale().toString(calculateRealSliderValue(sliderValue), 'f', m_decimals);
    return QStringLiteral("%1").arg(valueText);
}

double SliderDouble::calculateRealSliderValue(int value) const
{
    const double sliderValue = static_cast<double>(value);
    const double range = maximum() - minimum();
    const double steps = aznumeric_cast<double>(numSteps());

    const double denormalizedValue = ((sliderValue / steps) * range) + minimum();

    return convertFromSliderValue(denormalizedValue);
}

double SliderDouble::convertToSliderValue(double value) const
{
    return convertPowerCurveValue(value, false);
}

double SliderDouble::convertFromSliderValue(double value) const
{
    return convertPowerCurveValue(value, true);
}

double SliderDouble::convertPowerCurveValue(double value, bool fromSlider) const
{
    // If the midpoint is set to the default (0.5), then ignore the curve logic
    // and just return the linear scale value. Also, having a midpoint value
    // of 0.5 would cause a divide by 0 error in the a/b coefficients.
    static const double epsilon = .001;

    const double normalizedValue = (value - m_minimum) / (m_maximum - m_minimum);
    if (AZ::IsClose(m_curveMidpoint, g_defaultCurveMidpoint, epsilon))
    {
        if (fromSlider)
        {
            return value;
        }
        else
        {
            return static_cast<int>(normalizedValue * aznumeric_cast<double>(m_numSteps));
        }
    }

    // Calculate a new slider value based on the curve given the current slider value
    // This part is executed when we get a new value from the user dragging the slider
    if (fromSlider)
    {
        // Calculate the midpoint value of our slider range based on the normalized midpoint curve value
        const double sliderMax = maximum();
        const double sliderMin = minimum();
        const double range = sliderMax - sliderMin;
        const double sliderMidpoint = sliderMin + (m_curveMidpoint * range);

        // Power curve variables based on the various slider min, midpoint, and max values
        const double a = ((sliderMin * sliderMax) - (sliderMidpoint * sliderMidpoint)) / (sliderMin - (2 * sliderMidpoint) + sliderMax);
        const double b = std::pow(sliderMidpoint - sliderMin, 2) / (sliderMin - (2 * sliderMidpoint) + sliderMax);
        const double c = 2 * std::log((sliderMax - sliderMidpoint) / (sliderMidpoint - sliderMin));

        const double newSliderValue = a + b * std::exp(c * normalizedValue);
        return newSliderValue;
    }
    // Calculate a new slider value based on the curve given the current spinbox value
    // This part is executed when we get a new value set externally
    else
    {
        // Calculate the midpoint value of our slider range based on the normalized midpoint curve value
        const double sliderMax = aznumeric_cast<double>(m_slider->maximum());
        const double sliderMin = aznumeric_cast<double>(m_slider->minimum());
        const double range = sliderMax - sliderMin;
        const double sliderMidpoint = sliderMin + (m_curveMidpoint * range);

        // Power curve variables based on the various slider min, midpoint, and max values
        const double a = ((sliderMin * sliderMax) - (sliderMidpoint * sliderMidpoint)) / (sliderMin - (2.0 * sliderMidpoint) + sliderMax);
        const double b = std::pow(sliderMidpoint - sliderMin, 2.0) / (sliderMin - (2.0 * sliderMidpoint) + sliderMax);
        const double c = 2.0 * std::log((sliderMax - sliderMidpoint) / (sliderMidpoint - sliderMin));

        const double sliderValue = normalizedValue * aznumeric_cast<double>(m_numSteps);
        const double newSliderValue = std::log((sliderValue - a) / b) / c;
        return sliderMin + (newSliderValue * range);
    }
}

} // namespace AzQtComponents

#include "Components/Widgets/moc_Slider.cpp"

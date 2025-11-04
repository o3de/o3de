/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <AzCore/Math/MathUtils.h>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QStyleOptionFrame>

namespace AzQtComponents
{

namespace
{
    const int cursorRadius = 5;
}

ColorGrid::ColorGrid(QWidget* parent)
    : QFrame(parent)
    , m_mode(Mode::SaturationValue)
    , m_hue(0.0)
    , m_saturation(0.0)
    , m_value(0.0)
    , m_defaultVForHsMode(0.85)
{
    setFocusPolicy(Qt::ClickFocus);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setMouseTracking(true);
}

ColorGrid::~ColorGrid()
{
}

qreal ColorGrid::hue() const
{
    return m_hue;
}

void ColorGrid::setHue(qreal hue)
{
    if (qFuzzyCompare(hue, m_hue))
    {
        return;
    }

    m_hue = hue;

    if (m_mode == Mode::SaturationValue)
    {
        initPixmap();
    }
    update();
}

qreal ColorGrid::saturation() const
{
    return m_saturation;
}

void ColorGrid::setSaturation(qreal saturation)
{
    if (qFuzzyCompare(saturation, m_saturation))
    {
        return;
    }

    m_saturation = saturation;

    update();
}

qreal ColorGrid::value() const
{
    return m_value;
}

void ColorGrid::setValue(qreal value)
{
    if (qFuzzyCompare(value, m_value))
    {
        return;
    }

    m_value = value;

    if (m_mode == Mode::SaturationValue)
    {
        update();
    }
}

qreal ColorGrid::defaultVForHsMode() const
{
    return m_defaultVForHsMode;
}

void ColorGrid::setDefaultVForHsMode(qreal value)
{
    m_defaultVForHsMode = value;
}

QPoint ColorGrid::cursorCenter() const
{
    const QRect r = contentsRect();
    const auto w = r.width();
    const auto h = r.height();

    qreal x, y;
    if (m_mode == Mode::SaturationValue)
    {
        x = m_saturation;
        y = m_value;
    }
    else
    {
        x = m_hue;
        y = m_saturation;
    }

    return r.topLeft() + QPoint(qRound(x*(w - 1)), h - 1 - qRound(y*(h - 1)));
}

ColorGrid::HSV ColorGrid::positionToColor(const QPoint& pos) const
{
    const QRect r = contentsRect();
    const auto w = r.width();
    const auto h = r.height();

    const QPoint p = pos - r.topLeft();
    const qreal y = AZ::GetClamp(static_cast<qreal>(h - 1 - p.y())/(h - 1), 0.0, 1.0);
    const qreal x = AZ::GetClamp(static_cast<qreal>(p.x())/(w - 1), 0.0, 1.0);

    if (m_mode == Mode::SaturationValue)
    {
        return { m_hue, x, y };
    }
    else
    {
        return { x, y, m_value };
    }
}

void ColorGrid::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.drawPixmap(contentsRect().topLeft(), m_pixmap);

    QPen pen(m_value <= 0.5 ? Qt::white : Qt::black);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.drawEllipse(cursorCenter(), cursorRadius, cursorRadius);

    QStyleOptionFrame option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Frame, &option, &p, this);
}

void ColorGrid::resizeEvent(QResizeEvent* e)
{
    QFrame::resizeEvent(e);
    initPixmap();
}

void ColorGrid::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        m_userIsSelecting = true;

        emit gridPressed();
        handleLeftButtonEvent(e->pos());
    }
}

void ColorGrid::mouseMoveEvent(QMouseEvent* e)
{
    if (m_userIsSelecting)
    {
        handleLeftButtonEvent(e->pos());

        if (e->buttons() == Qt::MouseButtons())
        {
            QPoint globalPosition = QCursor::pos();
            QPoint position = mapFromGlobal(globalPosition);

            auto hsv = positionToColor(position);
            QPoint toolTipPosition = mapToGlobal(QPoint(position.x(), height()));

            QColor rgb = toQColor(Internal::ColorController::fromHsv(hsv.hue, hsv.saturation, hsv.value));

            QString text = QStringLiteral("HSV: %1, %2, %3\nRGB: %4, %5, %6").arg(static_cast<int>(hsv.hue * 360.0)).arg(static_cast<int>(hsv.saturation * 100.0)).arg(static_cast<int>(hsv.value * 100.0)).arg(rgb.red()).arg(rgb.green()).arg(rgb.blue());
            QToolTip::showText(toolTipPosition, text, this);
        }
    }
}

void ColorGrid::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_userIsSelecting && e->button() == Qt::LeftButton)
    {
        StopSelection();
    }
}

void ColorGrid::StopSelection()
{
    m_userIsSelecting = false;
    emit gridReleased();
}

void ColorGrid::handleLeftButtonEvent(const QPoint& eventPos)
{
    const HSV hsv = positionToColor(eventPos);

    bool updated = false;

    if (!qFuzzyCompare(hsv.hue, m_hue))
    {
        m_hue = hsv.hue;
        updated = true;
    }

    if (!qFuzzyCompare(hsv.saturation, m_saturation))
    {
        m_saturation = hsv.saturation;
        updated = true;
    }

    if (!qFuzzyCompare(hsv.value, m_value))
    {
        m_value = hsv.value;
        updated = true;
    }

    if (updated)
    {
        emit hsvChanged(m_hue, m_saturation, m_value);
        update();
    }
}

void ColorGrid::initPixmap()
{
    const QRect rect = contentsRect();
    const int w = rect.width();
    const int h = rect.height();

    if (w == 0 || h == 0)
    {
        m_pixmap = {};
        return;
    }

    QImage image(w, h, QImage::Format_RGB32);

    for (int r = 0; r < h; ++r)
    {
        const qreal y = static_cast<qreal>(r)/(h - 1);
        auto pixel = reinterpret_cast<uint*>(image.scanLine(h - 1 - r));

        if (m_mode == Mode::SaturationValue)
        {
            for (int c = 0; c < w; ++c)
            {
                const qreal x = static_cast<qreal>(c)/(w - 1);
                *pixel++ = QColor::fromHsvF(m_hue, x, y).rgb();
            }
        }
        else
        {
            for (int c = 0; c < w; ++c)
            {
                const qreal x = static_cast<qreal>(c)/(w - 1);
                *pixel++ = QColor::fromHsvF(x, y, m_defaultVForHsMode).rgb();
            }
        }
    }

    m_pixmap = QPixmap::fromImage(image);
}

void ColorGrid::setMode(Mode mode)
{
    if (mode == m_mode)
    {
        return;
    }

    m_mode = mode;
    initPixmap();
    update();
}

ColorGrid::Mode ColorGrid::mode() const
{
    return m_mode;
}

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorGrid.cpp"

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorPreview.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Casting/numeric_cast.h>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QStyleOptionFrame>

namespace AzQtComponents
{
    namespace
    {
        const int DRAGGED_SWATCH_SIZE = 16;
    }

ColorPreview::ColorPreview(QWidget* parent)
    : QFrame(parent)
    , m_draggedSwatch(new Swatch(this))
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusPolicy(Qt::ClickFocus);

    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        emit colorContextMenuRequested(pos, colorUnderPoint(pos));
    });

    int size = aznumeric_cast<int>(static_cast<double>(DRAGGED_SWATCH_SIZE) * devicePixelRatioF());

    m_draggedSwatch->hide();
    m_draggedSwatch->setFixedSize({ size, size });
}

ColorPreview::~ColorPreview()
{
}

void ColorPreview::setCurrentColor(const AZ::Color& color)
{
    if (AreClose(color, m_currentColor))
    {
        return;
    }

    m_currentColor = color;
    update();
}

AZ::Color ColorPreview::currentColor() const
{
    return m_currentColor;
}

void ColorPreview::setSelectedColor(const AZ::Color& color)
{
    if (AreClose(color, m_selectedColor))
    {
        return;
    }

    m_selectedColor = color;
    update();
}

AZ::Color ColorPreview::selectedColor() const
{
    return m_selectedColor;
}

QSize ColorPreview::sizeHint() const
{
    return { 200 + 2*frameWidth(), 20 + 2*frameWidth() };
}

void ColorPreview::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    drawFrame(&p);
    
    const QRect r = contentsRect();
    const int w = r.width() / 2;

    p.fillRect(QRect(r.x(), r.y(), w, r.height()), MakeAlphaBrush(m_selectedColor));
    p.fillRect(QRect(r.x() + w, r.y(), r.width() - w, r.height()), MakeAlphaBrush(m_currentColor));

    QStyleOptionFrame option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Frame, &option, &p, this);
}

void ColorPreview::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragStartPosition = event->pos();
    }
}

void ColorPreview::mouseReleaseEvent(QMouseEvent* event)
{
    QFrame::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        emit colorSelected(colorUnderPoint(event->pos()));
    }
}

void ColorPreview::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton))
    {
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    {
        return;
    }

    Palette palette;
    QMimeData* mimeData = new QMimeData();
    QDrag* drag = new QDrag(this);

    const AZ::Color color = (event->pos().x() < contentsRect().width() / 2) ? selectedColor() : currentColor();
    palette.tryAppendColor(color);
    palette.save(mimeData);
    drag->setMimeData(mimeData);

    m_draggedSwatch->setColor(color);
    QPixmap pixmap(m_draggedSwatch->size());
    pixmap.setDevicePixelRatio(devicePixelRatioF());
    m_draggedSwatch->render(&pixmap);
    drag->setPixmap(pixmap);

    drag->exec(Qt::CopyAction);
}

AZ::Color ColorPreview::colorUnderPoint(const QPoint& p)
{
    return (p.x() < contentsRect().width() / 2) ? selectedColor() : currentColor();
}

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorPreview.cpp"

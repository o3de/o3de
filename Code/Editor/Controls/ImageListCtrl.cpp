/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ImageListCtrl.h"

// Qt
#include <QPainter>
#include <QScrollBar>

//////////////////////////////////////////////////////////////////////////
CImageListCtrl::CImageListCtrl(QWidget* parent)
    : QAbstractItemView(parent)
    , m_itemSize(60, 60)
    , m_borderSize(4, 4)
    , m_style(DefaultStyle)
{
    setItemDelegate(new QImageListDelegate(this));
    setAutoFillBackground(false);

    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(255, 55, 50));
    setPalette(p);

    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setRange(0, 0);
}

//////////////////////////////////////////////////////////////////////////
CImageListCtrl::~CImageListCtrl()
{
}

//////////////////////////////////////////////////////////////////////////
CImageListCtrl::ListStyle CImageListCtrl::Style() const
{
    return m_style;
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::SetStyle(ListStyle style)
{
    m_style = style;
    scheduleDelayedItemsLayout();
}

//////////////////////////////////////////////////////////////////////////
const QSize& CImageListCtrl::ItemSize() const
{
    return m_itemSize;
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::SetItemSize(QSize size)
{
    Q_ASSERT(size.isValid());
    m_itemSize = size;
    scheduleDelayedItemsLayout();
}

//////////////////////////////////////////////////////////////////////////
const QSize& CImageListCtrl::BorderSize() const
{
    return m_borderSize;
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::SetBorderSize(QSize size)
{
    Q_ASSERT(size.isValid());
    m_borderSize = size;
    scheduleDelayedItemsLayout();
}

//////////////////////////////////////////////////////////////////////////
QModelIndexList CImageListCtrl::ItemsInRect(const QRect& rect) const
{
    QModelIndexList list;

    if (!model())
    {
        return list;
    }

    QHash<int, QRect>::const_iterator i;
    QHash<int, QRect>::const_iterator c = m_geometry.cend();
    for (i = m_geometry.cbegin(); i != c; ++i)
    {
        if (i.value().intersects(rect))
        {
            list << model()->index(i.key(), 0, rootIndex());
        }
    }

    return list;
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::paintEvent(QPaintEvent* event)
{
    QAbstractItemView::paintEvent(event);

    if (!model())
    {
        return;
    }

    const int rowCount = model()->rowCount();

    if (m_geometry.isEmpty() && rowCount)
    {
        updateGeometries();
    }

    QPainter painter(viewport());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setBackground(palette().window());
    painter.setFont(font());

    QStyleOptionViewItem option;
    option.palette = palette();
    option.font = font();
    option.fontMetrics = fontMetrics();
    option.decorationAlignment = Qt::AlignCenter;

    const QRect visibleRect(QPoint(horizontalOffset(), verticalOffset()), viewport()->contentsRect().size());

    painter.translate(-horizontalOffset(), -verticalOffset());

    for (int r = 0; r < rowCount; ++r)
    {
        const QModelIndex& index = model()->index(r, 0, rootIndex());

        option.rect = m_geometry.value(r);
        if (!option.rect.intersects(visibleRect))
        {
            continue;
        }

        option.state = QStyle::State_None;

        if (selectionModel()->isSelected(index))
        {
            option.state |= QStyle::State_Selected;
        }

        if (currentIndex() == index)
        {
            option.state |= QStyle::State_HasFocus;
        }

        QAbstractItemDelegate* idt = itemDelegate(index);
        idt->paint(&painter, option, index);
    }
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::rowsInserted(const QModelIndex& parent, int start, int end)
{
    QAbstractItemView::rowsInserted(parent, start, end);

    if (isVisible())
    {
        scheduleDelayedItemsLayout();
    }
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::updateGeometries()
{
    ClearItemGeometries();

    if (!model())
    {
        return;
    }

    const int rowCount = model()->rowCount();

    const int nPageHorz = viewport()->width();
    const int nPageVert = viewport()->height();

    if (nPageHorz == 0 || nPageVert == 0 || rowCount <= 0)
    {
        return;
    }

    int x = m_borderSize.width();
    int y = m_borderSize.height();

    const int nItemWidth = m_itemSize.width() + m_borderSize.width();

    if (m_style == HorizontalStyle)
    {
        for (int row = 0; row < rowCount; ++row)
        {
            m_geometry.insert(row, QRect(QPoint(x, y), m_itemSize));
            x += nItemWidth;
        }

        horizontalScrollBar()->setPageStep(viewport()->width());
        horizontalScrollBar()->setRange(0, x - viewport()->width());
    }
    else
    {
        const int nTextHeight = fontMetrics().height();
        const int nItemHeight = m_itemSize.height() + m_borderSize.height() + nTextHeight;

        int nNumOfHorzItems = nPageHorz / nItemWidth;
        if (nNumOfHorzItems <= 0)
        {
            nNumOfHorzItems = 1;
        }

        for (int row = 0; row < rowCount; ++row)
        {
            m_geometry.insert(row, QRect(QPoint(x, y), m_itemSize));

            if ((row + 1) % nNumOfHorzItems == 0)
            {
                y += nItemHeight;
                x = m_borderSize.width();
            }
            else
            {
                x += nItemWidth;
            }
        }

        verticalScrollBar()->setPageStep(viewport()->height());
        verticalScrollBar()->setRange(0, (y + nItemHeight) - viewport()->height());
    }
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CImageListCtrl::indexAt(const QPoint& point) const
{
    if (!model())
    {
        return QModelIndex();
    }

    const QPoint p = point +
        QPoint(horizontalOffset(), verticalOffset());

    QHash<int, QRect>::const_iterator i;
    QHash<int, QRect>::const_iterator c = m_geometry.cend();
    for (i = m_geometry.cbegin(); i != c; ++i)
    {
        if (i.value().contains(p))
        {
            return model()->index(i.key(), 0, rootIndex());
        }
    }

    return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    if (!index.isValid())
    {
        return;
    }

    QRect rect = m_geometry.value(index.row());

    switch (hint)
    {
    case EnsureVisible:
        if (horizontalOffset() > rect.right())
        {
            horizontalScrollBar()->setValue(rect.left());
        }
        else if ((horizontalOffset() + viewport()->width()) < rect.left())
        {
            horizontalScrollBar()->setValue(rect.right() - viewport()->width());
        }

        if (verticalOffset() > rect.bottom())
        {
            verticalScrollBar()->setValue(rect.top());
        }
        else if ((verticalOffset() + viewport()->height()) < rect.top())
        {
            verticalScrollBar()->setValue(rect.bottom() - viewport()->height());
        }
        break;

    case PositionAtTop:
        horizontalScrollBar()->setValue(rect.left());
        verticalScrollBar()->setValue(rect.top());
        break;

    case PositionAtBottom:
        horizontalScrollBar()->setValue(rect.right() - viewport()->width());
        verticalScrollBar()->setValue(rect.bottom() - viewport()->height());
        break;

    case PositionAtCenter:
        horizontalScrollBar()->setValue(rect.center().x() - (viewport()->width() / 2));
        verticalScrollBar()->setValue(rect.center().y() - (viewport()->height() / 2));
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
QRect CImageListCtrl::visualRect(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QRect();
    }


    if (!m_geometry.contains(index.row()))
    {
        return QRect();
    }

    return m_geometry.value(index.row())
               .translated(-horizontalOffset(), -verticalOffset());
}

//////////////////////////////////////////////////////////////////////////
QRect CImageListCtrl::ItemGeometry(const QModelIndex& index) const
{
    Q_ASSERT(index.model() == model());
    Q_ASSERT(m_geometry.contains(index.row()));

    return m_geometry.value(index.row());
}

void CImageListCtrl::SetItemGeometry(const QModelIndex& index, const QRect& rect)
{
    Q_ASSERT(index.model() == model());
    m_geometry.insert(index.row(), rect);
    update(rect);
}

void CImageListCtrl::ClearItemGeometries()
{
    m_geometry.clear();
}

//////////////////////////////////////////////////////////////////////////
int CImageListCtrl::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

//////////////////////////////////////////////////////////////////////////
int CImageListCtrl::verticalOffset() const
{
    return verticalScrollBar()->value();
}

//////////////////////////////////////////////////////////////////////////
bool CImageListCtrl::isIndexHidden([[maybe_unused]] const QModelIndex& index) const
{
    return false; /* not supported */
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CImageListCtrl::moveCursor(CursorAction cursorAction, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    if (!model())
    {
        return QModelIndex();
    }

    const int rowCount = model()->rowCount();

    if (0 == rowCount)
    {
        return QModelIndex();
    }

    switch (cursorAction)
    {
    case MoveHome:
        return model()->index(0, 0, rootIndex());

    case MoveEnd:
        return model()->index(rowCount - 1, 0, rootIndex());

    case MovePrevious:
    {
        QModelIndex current = currentIndex();
        if (current.isValid())
        {
            return model()->index((current.row() - 1) % rowCount, 0, rootIndex());
        }
    } break;

    case MoveNext:
    {
        QModelIndex current = currentIndex();
        if (current.isValid())
        {
            return model()->index((current.row() + 1) % rowCount, 0, rootIndex());
        }
    } break;

    case MoveUp:
    case MoveDown:
    case MoveLeft:
    case MoveRight:
    case MovePageUp:
    case MovePageDown:
        /* TODO */
        break;
    }
    return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
void CImageListCtrl::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags)
{
    if (!model())
    {
        return;
    }

    const QRect lrect =
        rect.translated(horizontalOffset(), verticalOffset());

    QHash<int, QRect>::const_iterator i;
    QHash<int, QRect>::const_iterator c = m_geometry.cend();
    for (i = m_geometry.cbegin(); i != c; ++i)
    {
        if (i.value().intersects(lrect))
        {
            selectionModel()->select(model()->index(i.key(), 0, rootIndex()), flags);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QRegion CImageListCtrl::visualRegionForSelection(const QItemSelection& selection) const
{
    QRegion region;

    foreach(const QModelIndex &index, selection.indexes())
    {
        region += visualRect(index);
    }

    return region;
}

//////////////////////////////////////////////////////////////////////////
QImageListDelegate::QImageListDelegate(QObject* parent)
    : QAbstractItemDelegate(parent)
{
}

//////////////////////////////////////////////////////////////////////////
void QImageListDelegate::paint(QPainter* painter,
    const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    painter->setFont(option.font);

    if (option.rect.isValid())
    {
        painter->setClipRect(option.rect);
    }

    QRect innerRect = option.rect.adjusted(1, 1, -1, -1);

    QRect textRect(innerRect.left(), innerRect.bottom() - option.fontMetrics.height(),
        innerRect.width(), option.fontMetrics.height() + 1);

    /* fill item background */

    painter->fillRect(option.rect, option.palette.color(QPalette::Base));

    /* draw image */

    if (index.data(Qt::DecorationRole).isValid())
    {
        const QPixmap& p = index.data(Qt::DecorationRole).value<QPixmap>();
        if (p.isNull() || p.size() == QSize(1, 1))
        {
            emit InvalidPixmapGenerated(index);
        }
        else
        {
            painter->drawPixmap(innerRect, p);
        }
    }

    /* draw text */

    const QColor trColor = option.palette.color(QPalette::Shadow);
    painter->fillRect(textRect, (option.state & QStyle::State_Selected) ?
        trColor.lighter() : trColor);

    if (option.state & QStyle::State_Selected)
    {
        painter->setPen(QPen(option.palette.color(QPalette::HighlightedText)));

        QFont f = painter->font();
        f.setBold(true);
        painter->setFont(f);
    }
    else
    {
        painter->setPen(QPen(option.palette.color(QPalette::Text)));
    }

    painter->drawText(textRect, index.data(Qt::DisplayRole).toString(),
        QTextOption(option.decorationAlignment));

    painter->setPen(QPen(option.palette.color(QPalette::Shadow)));
    painter->drawRect(textRect);

    /* draw border */

    if (option.state & QStyle::State_Selected)
    {
        QPen pen(option.palette.color(QPalette::Highlight));
        pen.setWidth(2);
        painter->setPen(pen);
        painter->drawRect(innerRect);
    }
    else
    {
        painter->setPen(QPen(option.palette.color(QPalette::Shadow)));
        painter->drawRect(option.rect);
    }

    if (option.state & QStyle::State_HasFocus)
    {
        QPen pen(Qt::DotLine);
        pen.setColor(option.palette.color(QPalette::AlternateBase));
        painter->setPen(pen);
        painter->drawRect(option.rect);
    }

    painter->restore();
}

//////////////////////////////////////////////////////////////////////////
QSize QImageListDelegate::sizeHint(const QStyleOptionViewItem& option,
    [[maybe_unused]] const QModelIndex& index) const
{
    return option.rect.size();
}

//////////////////////////////////////////////////////////////////////////
QVector<int> QImageListDelegate::paintingRoles() const
{
    return QVector<int>() << Qt::DecorationRole << Qt::DisplayRole;
}

#include <Controls/moc_ImageListCtrl.cpp>

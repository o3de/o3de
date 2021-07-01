/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ColumnGroupHeaderView.h"

// Qt
#include <QPainter>

// Editor
#include "Util/ColumnGroupProxyModel.h"


ColumnGroupHeaderView::ColumnGroupHeaderView(QWidget* parent)
    : QHeaderView(Qt::Horizontal, parent)
    , m_groupModel(nullptr)
    , m_showGroups(false)
{
    setSectionsMovable(true);
    setStretchLastSection(true);
    setSortIndicatorShown(false);
}

void ColumnGroupHeaderView::setModel(QAbstractItemModel* model)
{
    QHeaderView::setModel(model);
    m_groupModel = qobject_cast<ColumnGroupProxyModel*>(model);
    if (m_groupModel)
    {
        connect(m_groupModel, SIGNAL(SortChanged()), this, SLOT(update()));
    }
}

QSize ColumnGroupHeaderView::sizeHint() const
{
    QSize s = QHeaderView::sizeHint();
    if (m_showGroups)
    {
        s.setHeight(s.height() + GroupViewHeight());
    }
    return s;
}

bool ColumnGroupHeaderView::IsGroupsShown() const
{
    return m_showGroups;
}

bool ColumnGroupHeaderView::event(QEvent* event)
{
    switch (event->type())
    {
    case QEvent::Paint:
    {
        if (!m_showGroups || !m_groupModel)
        {
            return true;
        }

        QPainter painter(this);
        painter.fillRect(rect(), QColor(145, 145, 145));
        int xOffset = 10;
        int yOffset = 10;

        auto groups = m_groupModel->Groups();

        m_groups.clear();
        foreach(int column, groups)
        {
            const int width = sectionSize(column);
            const QRect r(xOffset, yOffset, width == 0 ? defaultSectionSize() : width, QHeaderView::sizeHint().height());
            xOffset += r.width() + 10;
            yOffset += 10;

            if (column != groups.last())
            {
                painter.setPen(Qt::black);
                painter.drawLine(r.bottomRight() + QPoint(-3, 0), r.bottomRight() + QPoint(-3, 3));
                painter.drawLine(r.bottomRight() + QPoint(-3, 3), r.bottomRight() + QPoint(10, 3));
            }
            m_groups.push_back({ r, column });
            paintSection(&painter, r, column);
        }
        break;
    }
    case QEvent::MouseButtonRelease:
    {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (m_showGroups && m_groupModel)
        {
            foreach(const Group &group, m_groups)
            {
                if (group.rect.contains(mouseEvent->pos()))
                {
                    auto sortOrder = m_groupModel->SortOrder(group.col);
                    m_groupModel->sort(group.col, (sortOrder == Qt::AscendingOrder) ?
                        Qt::DescendingOrder : Qt::AscendingOrder);
                }
            }
        }
        break;
    }

    default:
        return QHeaderView::event(event);
        break;
    }

    return true;
}

void ColumnGroupHeaderView::ShowGroups(bool showGroups)
{
    m_showGroups = showGroups;
    Q_EMIT geometriesChanged();
}

void ColumnGroupHeaderView::updateGeometries()
{
    setViewportMargins(0, m_showGroups ? GroupViewHeight() : 0, 0, 0);
    QHeaderView::updateGeometries();
}

int ColumnGroupHeaderView::GroupViewHeight() const
{
    if (!m_groupModel)
    {
        return 0;
    }
    int groupCount = m_groupModel->Groups().size();
    return QHeaderView::sizeHint().height() + qMax(0, groupCount - 1) * 10 + 20;
}


#include <Util/moc_ColumnGroupHeaderView.cpp>

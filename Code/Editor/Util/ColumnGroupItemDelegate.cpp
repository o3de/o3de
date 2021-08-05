/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ColumnGroupItemDelegate.h"

// Qt
#include <QHeaderView>
#include <QPainter>
#include <QTreeView>



ColumnGroupItemDelegate::ColumnGroupItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QSize ColumnGroupItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // group title indexes have no own width, their text is drawn over all columns
    if (index.model()->hasChildren(index) && index.column() == 0)
    {
        return QSize(32, QStyledItemDelegate::sizeHint(option, index).height());
    }
    return QStyledItemDelegate::sizeHint(option, index);
}

void ColumnGroupItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.model()->hasChildren(index))
    {
        painter->setClipping(false);
        painter->setPen(option.palette.text().color());
        if (option.state & QStyle::State_Selected)
        {
            painter->setPen(option.palette.highlightedText().color());
        }
        QRect textRect = option.rect;
        textRect.setRight(qobject_cast<QWidget*>(parent())->width());
        if (option.state & QStyle::State_Selected && index.column() == 0)
        {
            painter->fillRect(textRect, option.palette.highlight());
        }
        // there's just one text - somewhere in the model row, show it!
        QString content;
        const int columnCount = index.model()->columnCount(index.parent());
        for (int column = 0; column < columnCount; ++column)
        {
            content += index.sibling(index.row(), column).data().toString();
        }
        int alignment = index.data(Qt::TextAlignmentRole).toInt();
        if (index.column() == 0)
        {
            painter->drawText(textRect, (alignment == 0 ? Qt::AlignLeft | Qt::AlignVCenter : alignment) | Qt::ElideRight, content);
        }

        if (!index.parent().isValid() && index.row() > 0)
        {
            QTreeView* tv = qobject_cast<QTreeView*>(option.styleObject);
            if (tv)
            {
                // draw a line in the same color as the table header for separation between groups
                QStyleOptionHeader header;
                header.rect = QRect(QPoint(1, textRect.top()), textRect.topRight() - QPoint(1,0));
                tv->style()->drawControl(QStyle::CE_HeaderSection, &header, painter, tv->header());
            }
        }
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

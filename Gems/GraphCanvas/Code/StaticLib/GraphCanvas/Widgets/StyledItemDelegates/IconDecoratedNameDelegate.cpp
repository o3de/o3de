/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyleManager.h>

#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

namespace GraphCanvas
{
    IconDecoratedNameDelegate::IconDecoratedNameDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
    {
    }
    
    void IconDecoratedNameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        bool fallback = true;

        if (option.state & QStyle::State_Selected)
        {
            QVariant iconData = index.data(Qt::DecorationRole);

            if (iconData.canConvert<QPixmap>())
            {
                QPixmap icon = qvariant_cast<QPixmap>(iconData);

                // Magic decorator width offset to make sure the overlaid pixmap is in the right place
                int decoratorWidth = icon.width() + m_paddingOffset;

                QStyleOptionViewItem opt = option;
                initStyleOption(&opt, index);

                // Draw the original widget
                QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
                style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

                // Redraw the decorator to avoid the selection tinting
                QRect decoratorRect = QRect(opt.rect.x(), opt.rect.y(), decoratorWidth, opt.rect.height());
                style->drawItemPixmap(painter, decoratorRect, Qt::AlignCenter, icon);

                fallback = false;
            }
        }

        if(fallback)
        {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
}

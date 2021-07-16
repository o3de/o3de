/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"
#include "EditorPreferencesTreeWidgetItemDelegate.h"
#include <QIcon>
#include <QPainter>
#include <QStyledItemDelegate>

static const int IconX = 26;
static const int TextX = 27;

EditorPreferencesTreeWidgetItemDelegate::EditorPreferencesTreeWidgetItemDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
{
    m_mouseOverBrush = QBrush(QColor(0x40, 0x40, 0x40));
    m_selectedBrush = QBrush(QColor(0x47, 0x47, 0x47));
}

void EditorPreferencesTreeWidgetItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setPen(Qt::NoPen);

    const auto textHeight = option.fontMetrics.height();
    int vCenter = option.rect.bottom() - option.rect.height() / 2;
    int iconSize = option.decorationSize.width();

    if ((option.state & QStyle::State_Selected) || (option.state & QStyle::State_MouseOver))
    {
        if ((option.state & QStyle::State_Selected))
        {
            painter->setBrush(m_selectedBrush);
        }
        else if ((option.state & QStyle::State_MouseOver))
        {
            painter->setBrush(m_mouseOverBrush);
        }

        const QRect backGroundRect(option.rect.left(), option.rect.top(), option.rect.width(), option.rect.height());
        painter->drawRect(backGroundRect);
    }

    const auto& iconVariant = index.data(Qt::DecorationRole);
    if (!iconVariant.isNull())
    {
        const auto& icon = iconVariant.value<QIcon>();
        QRect r(IconX, vCenter - iconSize/2, iconSize, iconSize);
        r.setX(-r.width());
        icon.paint(painter, r);
    }

    const QString& text = index.data(Qt::DisplayRole).toString();
    painter->setPen(Qt::white);
    const auto textRect = QRect{ TextX, vCenter - textHeight/2, option.rect.width() - TextX, textHeight };
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

QSize EditorPreferencesTreeWidgetItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}

#include <moc_EditorPreferencesTreeWidgetItemDelegate.cpp>

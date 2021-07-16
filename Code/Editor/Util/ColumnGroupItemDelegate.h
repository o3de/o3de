/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef COLUMNGROUPITEMDELEGATE_H
#define COLUMNGROUPITEMDELEGATE_H

#include <QStyledItemDelegate>

class ColumnGroupItemDelegate
    : public QStyledItemDelegate
{
public:
    ColumnGroupItemDelegate(QObject* parent = 0);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif // COLUMNGROUPITEMDELEGATE_H

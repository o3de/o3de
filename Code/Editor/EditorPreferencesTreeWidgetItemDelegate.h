/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QStyledItemDelegate>

class EditorPreferencesTreeWidgetItemDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit EditorPreferencesTreeWidgetItemDelegate(QWidget* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QBrush m_selectedBrush;
    QBrush m_mouseOverBrush;
};


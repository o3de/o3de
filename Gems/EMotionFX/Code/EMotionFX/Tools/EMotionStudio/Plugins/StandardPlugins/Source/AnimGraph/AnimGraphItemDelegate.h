/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QStyledItemDelegate>
#include <QPixmap>
#endif

namespace EMStudio
{
    // This class is a custom item delegate to handle painting of elements from AnimGraphModel
    //
    class AnimGraphItemDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        AnimGraphItemDelegate(QWidget* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

    signals:
        void linkActivated(const QString& link);

    private:
        QPixmap m_referenceBackground;
        QPixmap m_referenceArrow;
    };

}   // namespace EMStudio

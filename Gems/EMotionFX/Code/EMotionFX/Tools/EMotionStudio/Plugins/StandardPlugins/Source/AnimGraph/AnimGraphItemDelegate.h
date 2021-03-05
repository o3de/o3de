/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

    signals:
        void linkActivated(const QString& link);

    private:
        QPixmap m_referenceBackground;
        QPixmap m_referenceArrow;
    };

}   // namespace EMStudio

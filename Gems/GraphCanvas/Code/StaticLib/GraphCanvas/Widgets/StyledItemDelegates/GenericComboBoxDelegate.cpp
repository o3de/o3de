/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "GenericComboBoxDelegate.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QAction>


namespace GraphCanvas
{
    GenericComboBoxDelegate::GenericComboBoxDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
    {
    }

    QWidget* GenericComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QVariant variant = index.data(ComboBoxDelegateRole);

        QStringList variantList = variant.toStringList();

        if (variantList.isEmpty())
        {
            return QStyledItemDelegate::createEditor(parent, option, index);
        }

        QComboBox* combo = new QComboBox(parent);

        connect(combo, SIGNAL(activated(int)), this, SLOT(dismissComboBox()));
        connect(combo, SIGNAL(activated(int)), this, SLOT(dismissComboBox()));

        combo->addItems(variantList);
        
        return combo;
    }

    void GenericComboBoxDelegate::dismissComboBox()
    {
        auto widget = qobject_cast<QWidget*>(sender());
        if (widget)
        {
            Q_EMIT commitData(widget);
            Q_EMIT closeEditor(widget);
        }
    }

    void GenericComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
        {
            // get the index of the text in the combobox that matches the current value of the itenm
            QString currentText = index.data(Qt::EditRole).toString();
            int cbIndex = cb->findText(currentText);
            // if it is valid, adjust the combobox
            if (cbIndex >= 0)
            {
                cb->setCurrentIndex(cbIndex);
            }

            cb->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
        }
        else
        {
            QStyledItemDelegate::setEditorData(editor, index);
        }
    }

    void GenericComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
        {
            model->setData(index, cb->currentText(), Qt::EditRole);
        }
        else
        {
            QStyledItemDelegate::setModelData(editor, model, index);
        }
    }

    void GenericComboBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
    {
        editor->setGeometry(option.rect);

        if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
        {
            cb->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
            cb->setMinimumHeight(option.rect.height());
            cb->setMinimumWidth(option.rect.width());

            cb->update();
            cb->updateGeometry();

            cb->view()->updateGeometry();
            cb->view()->setSizeAdjustPolicy(QAbstractItemView::SizeAdjustPolicy::AdjustToContents);
            cb->view()->setTextElideMode(Qt::ElideNone);
            cb->view()->adjustSize();
        }
    }
}

#include <StaticLib/GraphCanvas/Widgets/StyledItemDelegates/moc_GenericComboBoxDelegate.cpp>

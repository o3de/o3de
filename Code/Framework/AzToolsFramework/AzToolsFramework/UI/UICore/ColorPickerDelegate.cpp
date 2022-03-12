/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QtCore/QAbstractItemModel>
#include "ColorPickerDelegate.hxx"

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

namespace AzToolsFramework
{
    ColorPickerDelegate::ColorPickerDelegate(QObject* pParent)
        : QStyledItemDelegate(pParent)
    {
    }

    QWidget* ColorPickerDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        (void)index;
        (void)option;
        AzQtComponents::ColorPicker* ptrDialog = new AzQtComponents::ColorPicker(AzQtComponents::ColorPicker::Configuration::RGB,
                tr("Select Color"), parent);
        ptrDialog->setWindowFlags(Qt::Tool);
        return ptrDialog;
    }

    void ColorPickerDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        AzQtComponents::ColorPicker* colorEditor = qobject_cast<AzQtComponents::ColorPicker*>(editor);

        if (!editor)
        {
            return;
        }

        QVariant colorResult = index.data(COLOR_PICKER_ROLE);
        if (colorResult == QVariant())
        {
            return;
        }

        const QColor pickedColor = qvariant_cast<QColor>(colorResult);
        colorEditor->setCurrentColor(AzQtComponents::fromQColor(pickedColor));
    }

    void ColorPickerDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        AzQtComponents::ColorPicker* colorEditor = qobject_cast<AzQtComponents::ColorPicker*>(editor);

        if (!editor)
        {
            return;
        }

        const QVariant colorVariant = AzQtComponents::toQColor(colorEditor->currentColor());
        model->setData(index, colorVariant, COLOR_PICKER_ROLE);
    }

    void ColorPickerDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        (void)index;
        QRect pickerpos = option.rect;

        pickerpos.setTopLeft(editor->parentWidget()->mapToGlobal(pickerpos.topLeft()));
        pickerpos.adjust(64, 0, 0, 0);
        editor->setGeometry(pickerpos);
    }

} // namespace AzToolsFramework

#include "UI/UICore/moc_ColorPickerDelegate.cpp"

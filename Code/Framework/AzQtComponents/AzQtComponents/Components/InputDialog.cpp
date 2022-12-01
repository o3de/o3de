/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InputDialog.h"

#include <QEventLoop>
#include <QInputDialog>
#include <QRegularExpressionValidator>

namespace AzQtComponents
{
    InputDialog::InputDialog(QWidget* parent)
        : QInputDialog(parent)
        , m_validator(nullptr)
    {
    }

    void InputDialog::setValidator(QValidator* validator)
    {
        m_validator = validator;

        // If this is called after show has been called, the QLineEdit will be available to patch.
        QLineEdit* lineEdit = this->findChild<QLineEdit*>();
        if (lineEdit)
        {
            lineEdit->setValidator(m_validator);
        }
    }

    void InputDialog::show()
    {
        QInputDialog::show();

        if (m_validator)
        {
            QLineEdit* lineEdit = this->findChild<QLineEdit*>();
            if (lineEdit)
            {
                lineEdit->setValidator(m_validator);
            }
        }
    }

    int InputDialog::exec()
    {
        show();

        this->setWindowModality(Qt::WindowModality::WindowModal);
        return QInputDialog::exec();
    }

    QString InputDialog::getText(
        QWidget* parent,
        const QString& title,
        const QString& label,
        QLineEdit::EchoMode mode,
        const QString& text,
        const QString& validationRegExp)
    {
        InputDialog dialog(parent);
        dialog.setWindowTitle(title);
        dialog.setLabelText(label);
        dialog.setTextValue(text);
        dialog.setTextEchoMode(mode);

        if (!validationRegExp.isEmpty())
        {
            dialog.setValidator(new QRegularExpressionValidator(QRegularExpression(validationRegExp), &dialog));
        }

        const int ret = dialog.exec();
        if (ret)
        {
            return dialog.textValue();
        }
        else
        {
            return QString();
        }
    }

} // namespace AzQtComponents

#include "Components/moc_InputDialog.cpp"

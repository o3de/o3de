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

    void InputDialog::UpdateLineEdit()
    {
        // This will succeed only if show() has happened, which sets up the UI.
        QLineEdit* lineEdit = this->findChild<QLineEdit*>();
        if (lineEdit)
        {
            lineEdit->setValidator(m_validator);

            if (m_maxLength > 0)
            {
                lineEdit->setMaxLength(m_maxLength);
            }
        }
    }

    void InputDialog::SetValidator(QValidator* validator)
    {
        m_validator = validator;

        UpdateLineEdit();
    }

    void InputDialog::SetRegularExpressionValidator(const QString& pattern)
    {
        SetValidator(new QRegularExpressionValidator(QRegularExpression(pattern), this));
    }

    void InputDialog::SetMaxLength(int length)
    {
        m_maxLength = length;

        UpdateLineEdit();
    }

    void InputDialog::show()
    {
        QInputDialog::show();

        UpdateLineEdit();
    }

    int InputDialog::exec()
    {
        setWindowModality(Qt::WindowModality::WindowModal);
        show();
 
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
            dialog.SetRegularExpressionValidator(validationRegExp);
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

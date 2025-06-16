/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorGoToLineDialog.hxx"

#include <Source/LUA/moc_LUAEditorGoToLineDialog.cpp>
#include <Source/LUA/ui_LUAEditorGoToLineDialog.h>

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExp>
#include <QRegExpValidator>

namespace LUAEditor
{
    static constexpr const char* GotoLineDialogRegEx = "(^\\d+(:\\d+)?$)";

    LUAEditorGoToLineDialog::LUAEditorGoToLineDialog(QWidget* parent)
        : QDialog(parent)
    {
        m_lineNumber = 0;
        m_gui = azcreate(Ui::goToLineDlg, ());
        m_gui->setupUi(this);
        this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

        const QRegExp rx(GotoLineDialogRegEx);
        QRegExpValidator* const validator = new QRegExpValidator(rx, this);
        m_gui->lineNumber->setValidator(validator);

        connect(m_gui->lineNumber, &QLineEdit::textChanged, this, &LUAEditorGoToLineDialog::handleLineNumberInput);
        connect(m_gui->buttonBox, &QDialogButtonBox::accepted, this, &LUAEditorGoToLineDialog::validateAndAccept);
        connect(m_gui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    LUAEditorGoToLineDialog::~LUAEditorGoToLineDialog()
    {
        azdestroy(m_gui);
    }

    void LUAEditorGoToLineDialog::setLineNumber(int line, int newColumn) const
    {
        const QString text = QString::number(line) + QString(":") + QString::number(newColumn);
        m_gui->lineNumber->setText(text);
        m_gui->lineNumber->setFocus();
        m_gui->lineNumber->selectAll();
    }

    void LUAEditorGoToLineDialog::handleLineNumberInput(const QString& input)
    {
        const QRegularExpression re(GotoLineDialogRegEx);
        QPushButton* const button = m_gui->buttonBox->button(QDialogButtonBox::StandardButton::Ok);
        button->setEnabled(re.match(input).hasMatch());
    }

    void LUAEditorGoToLineDialog::validateAndAccept()
    {
        m_columnNumber = 0;

        const QString input = m_gui->lineNumber->text();
        const QStringList parts = input.split(':');

        if (parts.size() < 1) {
            return;
        }

        bool lineValid = false;
        bool columnValid = false;

        const int lineNumber = parts[0].toInt(&lineValid);
        if (lineValid && !parts[0].isEmpty()) {
            m_lineNumber = AZStd::max(lineNumber, 1);
        }

        if (parts.size() == 2) {
            const int columnNumber = parts[1].toInt(&columnValid);
            if (columnValid && !parts[1].isEmpty()) {
                m_columnNumber = AZStd::max(columnNumber, 0);
            }
        }
    }
}

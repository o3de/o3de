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

namespace LUAEditor
{
    LUAEditorGoToLineDialog::LUAEditorGoToLineDialog(QWidget* parent)
        : QDialog(parent)
    {
        m_lineNumber = 0;
        m_gui = azcreate(Ui::goToLineDlg, ());
        m_gui->setupUi(this);
        this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        connect(m_gui->lineNumberSpinBox, SIGNAL(valueChanged (int)), this, SLOT(spinBoxLineNumberChanged(int)));
    }

    LUAEditorGoToLineDialog::~LUAEditorGoToLineDialog()
    {
        azdestroy(m_gui);
    }

    void LUAEditorGoToLineDialog::setLineNumber(int newNumber)
    {
        m_gui->lineNumberSpinBox->setValue(newNumber);
        m_gui->lineNumberSpinBox->setFocus();
        m_gui->lineNumberSpinBox->selectAll();
    }

    void LUAEditorGoToLineDialog::spinBoxLineNumberChanged(int newNumber)
    {
        m_lineNumber = newNumber;
        emit lineNumberChanged(newNumber);
    }
}

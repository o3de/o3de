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

#include "Woodpecker_precompiled.h"

#include "LUAEditorGoToLineDialog.hxx"
#include <Woodpecker/LUA/moc_LUAEditorGoToLineDialog.cpp>

#include <Woodpecker/LUA/ui_LUAEditorGoToLineDialog.h>

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

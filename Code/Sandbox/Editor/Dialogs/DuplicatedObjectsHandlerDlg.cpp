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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "DuplicatedObjectsHandlerDlg.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Dialogs/ui_DuplicatedObjectsHandlerDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


CDuplicatedObjectsHandlerDlg::CDuplicatedObjectsHandlerDlg(const QString& msg, QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::DuplicatedObjectsHandlerDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->textBrowser->setPlainText(msg);

    connect(m_ui->buttonOverride, &QPushButton::clicked, this, &CDuplicatedObjectsHandlerDlg::OnBnClickedOverrideBtn);
    connect(m_ui->buttonCreateCopies, &QPushButton::clicked, this, &CDuplicatedObjectsHandlerDlg::OnBnClickedCreateCopiesBtn);
}

CDuplicatedObjectsHandlerDlg::~CDuplicatedObjectsHandlerDlg()
{
}

void CDuplicatedObjectsHandlerDlg::OnBnClickedOverrideBtn()
{
    m_result = eResult_Override;
    accept();
}

void CDuplicatedObjectsHandlerDlg::OnBnClickedCreateCopiesBtn()
{
    m_result = eResult_CreateCopies;
    accept();
}

#include <Dialogs/moc_DuplicatedObjectsHandlerDlg.cpp>

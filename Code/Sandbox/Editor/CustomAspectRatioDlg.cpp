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

// Description : A dialog for getting an aspect ratio info from users
// Notice      : Refer to ViewportTitleDlg cpp for a use case


#include "EditorDefs.h"

#include "CustomAspectRatioDlg.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_CustomAspectRatioDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#define MIN_ASPECT 1
#define MAX_ASPECT 16384

CCustomAspectRatioDlg::CCustomAspectRatioDlg(int x, int y, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_xDefault(x)
    , m_yDefault(y)
    , m_ui(new Ui::CustomAspectRatioDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    OnInitDialog();
}

CCustomAspectRatioDlg::~CCustomAspectRatioDlg()
{
}

void CCustomAspectRatioDlg::OnInitDialog()
{
    m_ui->m_x->setRange(MIN_ASPECT, MAX_ASPECT);
    m_ui->m_x->setValue(m_xDefault);

    m_ui->m_y->setRange(MIN_ASPECT, MAX_ASPECT);
    m_ui->m_y->setValue(m_yDefault);
}

int CCustomAspectRatioDlg::GetX() const
{
    return m_ui->m_x->value();
}

int CCustomAspectRatioDlg::GetY() const
{
    return m_ui->m_y->value();
}

#include <moc_CustomAspectRatioDlg.cpp>

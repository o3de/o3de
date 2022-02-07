/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A dialog for getting an aspect ratio info from users
// Notice      : Refer to ViewportTitleDlg cpp for a use case


#include "EditorDefs.h"

#include "CustomAspectRatioDlg.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_CustomAspectRatioDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#define MIN_ASPECT 1
#define MAX_ASPECT 16384

CCustomAspectRatioDlg::CCustomAspectRatioDlg(int x, int y, QWidget* pParent /*=nullptr*/)
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

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

// Description : A dialog for getting a resolution info from users
// Notice      : Refer to ViewportTitleDlg cpp for a use case


#include "EditorDefs.h"

#include "CustomResolutionDlg.h"

// Qt
#include <QTextStream>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_CustomResolutionDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#define MIN_RES 64
#define MAX_RES 8192

CCustomResolutionDlg::CCustomResolutionDlg(int w, int h, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_wDefault(w)
    , m_hDefault(h)
    , m_ui(new Ui::CustomResolutionDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    OnInitDialog();
}

CCustomResolutionDlg::~CCustomResolutionDlg()
{
}

void CCustomResolutionDlg::OnInitDialog()
{
    m_ui->m_width->setRange(MIN_RES, MAX_RES);
    m_ui->m_width->setValue(m_wDefault);

    m_ui->m_height->setRange(MIN_RES, MAX_RES);
    m_ui->m_height->setValue(m_hDefault);

    QString maxDimensionString;
    QTextStream(&maxDimensionString) 
        << "Maximum Dimension: " << MAX_RES << Qt::endl 
        << Qt::endl
        << "Note: Dimensions over 8K may be" << Qt::endl
        << "unstable depending on hardware.";
    
    m_ui->m_maxDimension->setText(maxDimensionString);
}

int CCustomResolutionDlg::GetWidth() const
{
    return m_ui->m_width->value();
}

int CCustomResolutionDlg::GetHeight() const
{
    return m_ui->m_height->value();
}

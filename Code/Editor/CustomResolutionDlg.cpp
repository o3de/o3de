/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

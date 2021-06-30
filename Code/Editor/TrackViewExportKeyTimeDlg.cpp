/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewExportKeyTimeDlg.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_TrackViewExportKeyTimeDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

//////////////////////////////////////////////////////////////////////////
CTrackViewExportKeyTimeDlg::CTrackViewExportKeyTimeDlg(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::TrackViewExportKeyTimeDlg)
{
    m_ui->setupUi(this);
    m_ui->m_animationTimeExport->setChecked(true);
    m_ui->m_soundTimeExport->setChecked(true);
}

CTrackViewExportKeyTimeDlg::~CTrackViewExportKeyTimeDlg()
{
}

bool CTrackViewExportKeyTimeDlg::IsAnimationExportChecked() const
{
    return m_ui->m_animationTimeExport->isChecked();
}

bool CTrackViewExportKeyTimeDlg::IsSoundExportChecked() const
{
    return m_ui->m_soundTimeExport->isChecked();
}

#include <moc_TrackViewExportKeyTimeDlg.cpp>

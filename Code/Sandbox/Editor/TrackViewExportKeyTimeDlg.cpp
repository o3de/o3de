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

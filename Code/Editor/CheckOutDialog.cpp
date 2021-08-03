/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "CheckOutDialog.h"

// Qt
#include <QStyle>

// AzToolsFramework
#include <AzToolsFramework/SourceControl/SourceControlAPI.h> // for AzToolsFramework::SourceControlConnectionRequestBus


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_CheckOutDialog.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING



// CCheckOutDialog dialog
int CCheckOutDialog::m_lastResult = CCheckOutDialog::CANCEL;

CCheckOutDialog::CCheckOutDialog(const QString& file, QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::CheckOutDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_file = file;

    m_ui->icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(m_ui->icon->width()));

    OnInitDialog();

    connect(m_ui->buttonCancel, &QPushButton::clicked, this, &CCheckOutDialog::OnBnClickedCancel);
    connect(m_ui->buttonCheckout, &QPushButton::clicked, this, &CCheckOutDialog::OnBnClickedCheckout);
    connect(m_ui->buttonOverwrite, &QPushButton::clicked, this, &CCheckOutDialog::OnBnClickedOverwrite);
}

CCheckOutDialog::~CCheckOutDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CCheckOutDialog::OnBnClickedCancel()
{
    // Cancel operation
    HandleResult(CANCEL);
}

//////////////////////////////////////////////////////////////////////////
// CCheckOutDialog message handlers
void CCheckOutDialog::OnBnClickedCheckout()
{
    // Check out this file.
    HandleResult(CHECKOUT);
}

//////////////////////////////////////////////////////////////////////////
void CCheckOutDialog::OnBnClickedOverwrite()
{
    // Overwrite this file.
    HandleResult(OVERWRITE);
}

//////////////////////////////////////////////////////////////////////////
void CCheckOutDialog::HandleResult(int result)
{
    m_lastResult = result;
    InstanceIsForAll() = m_ui->chkForAll->isChecked();
    done(result);
}

//////////////////////////////////////////////////////////////////////////
void CCheckOutDialog::OnInitDialog()
{
    setWindowTitle(tr("Source Control"));

    using namespace AzToolsFramework;
    SourceControlState state = SourceControlState::Disabled;
    SourceControlConnectionRequestBus::BroadcastResult(state, &SourceControlConnectionRequestBus::Events::GetSourceControlState);
    bool sccAvailable = state == SourceControlState::Active ? true : false;

    QString text(tr("%1\n\nis read-only, and needs to be writable to continue.").arg(m_file));

    if (!sccAvailable)
    {
        text.append("\nEnable and connect to source control for more options.");
    }

    m_ui->m_text->setText(text);

    m_ui->chkForAll->setEnabled(InstanceEnableForAll());
    m_ui->chkForAll->setChecked(InstanceIsForAll());
    m_ui->buttonCheckout->setEnabled(sccAvailable);

    adjustSize();
}

//static ////////////////////////////////////////////////////////////////
bool& CCheckOutDialog::InstanceEnableForAll()
{
    static bool isEnableForAll = false;
    return isEnableForAll;
}

//static ////////////////////////////////////////////////////////////////
bool& CCheckOutDialog::InstanceIsForAll()
{
    static bool isForAll = false;
    return isForAll;
}

//static ////////////////////////////////////////////////////////////////
bool CCheckOutDialog::EnableForAll(bool isEnable)
{
    bool bPrevEnable = InstanceEnableForAll();
    InstanceEnableForAll() = isEnable;
    if (!bPrevEnable || !isEnable)
    {
        InstanceIsForAll() = false;
        m_lastResult = CANCEL;
    }
    return bPrevEnable;
}

#include <moc_CheckOutDialog.cpp>

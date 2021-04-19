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

// Description : implementation file


#include "EditorDefs.h"

#include "StartupLogoDialog.h"

// Qt
#include <QPainter>
#include <QThread>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_StartupLogoDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING



/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog dialog

CStartupLogoDialog* CStartupLogoDialog::s_pLogoWindow = 0;

CStartupLogoDialog::CStartupLogoDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent /*=NULL*/)
    : QWidget(pParent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_ui(new Ui::StartupLogoDialog)
{
    m_ui->setupUi(this);
 
    s_pLogoWindow = this;

    m_backgroundImage = QPixmap(QStringLiteral(":/StartupLogoDialog/splashscreen_1_27.png"));
    setFixedSize(QSize(600, 300));

    // Prepare background image
    QImage backgroundImage(QStringLiteral(":/StartupLogoDialog/splashscreen_1_27.png"));
    m_backgroundImage = QPixmap::fromImage(backgroundImage.scaled(m_enforcedWidth, m_enforcedHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    // Draw the Open 3D Engine logo from svg
    m_ui->m_logo->load(QStringLiteral(":/StartupLogoDialog/o3de_logo.svg"));

    m_ui->m_TransparentConfidential->setObjectName("copyrightNotice");
    m_ui->m_TransparentConfidential->setTextFormat(Qt::RichText);
    m_ui->m_TransparentConfidential->setText(richTextCopyrightNotice);

    setWindowTitle(tr("Starting Open 3D Engine Editor"));

    setStyleSheet( "CStartupLogoDialog > QLabel { background: transparent; color: 'white' }\
                    CStartupLogoDialog > QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; } ");

    m_ui->m_TransparentVersion->setText(QString("BETA - ") + versionText);
}

CStartupLogoDialog::~CStartupLogoDialog()
{
    s_pLogoWindow = 0;
}

void CStartupLogoDialog::SetText(const char* text)
{
    if (s_pLogoWindow)
    {
        s_pLogoWindow->SetInfoText(text);
    }
}

void CStartupLogoDialog::SetInfoText(const char* text)
{
    m_ui->m_TransparentText->setText(text);

    if (QThread::currentThread() == thread())
    {
        m_ui->m_TransparentText->repaint();
    }

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);  // if you don't process events, repaint does not function correctly.
}

void CStartupLogoDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), m_backgroundImage);
}

#include <moc_StartupLogoDialog.cpp>

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation file

#include "EditorDefs.h"
#include "StartupLogoDialog.h"

#include <AzQtComponents/Utilities/PixmapScaleUtilities.h>

// Qt
#include <QPainter>
#include <QThread>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_StartupLogoDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog dialog

CStartupLogoDialog* CStartupLogoDialog::s_pLogoWindow = nullptr;

CStartupLogoDialog::CStartupLogoDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent /*=nullptr*/)
    : QWidget(pParent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_ui(new Ui::StartupLogoDialog)
{
    m_ui->setupUi(this);

    s_pLogoWindow = this;
    setFixedSize(QSize(m_enforcedWidth, m_enforcedHeight));
    setAttribute(Qt::WA_TranslucentBackground, true);

    // Prepare background image
    m_backgroundImage = AzQtComponents::ScalePixmapForScreenDpi(
        QPixmap(QStringLiteral(":/StartupLogoDialog/splashscreen_background_2021_11.jpg")),
        screen(),
        QSize(m_enforcedWidth, m_enforcedHeight),
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
    );

    // Draw the Open 3D Engine logo from svg
    m_ui->m_logo->load(QStringLiteral(":/StartupLogoDialog/o3de_logo.svg"));

    m_ui->m_TransparentConfidential->setObjectName("copyrightNotice");
    m_ui->m_TransparentConfidential->setTextFormat(Qt::RichText);
    m_ui->m_TransparentConfidential->setText(richTextCopyrightNotice);

    setWindowTitle(tr("Starting Open 3D Engine Editor"));

    setStyleSheet( "CStartupLogoDialog > QLabel { background: transparent; color: 'white' }\
                    CStartupLogoDialog > QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; } ");

    m_ui->m_TransparentVersion->setText(versionText);
}

CStartupLogoDialog::~CStartupLogoDialog()
{
    s_pLogoWindow = nullptr;
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StartupLogoDialog.h"
#include "EditorDefs.h"

#include <AzQtComponents/Utilities/PixmapScaleUtilities.h>

// Qt
#include <QPainter>
#include <QThread>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_StartupLogoDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

CStartupLogoDialog* CStartupLogoDialog::s_pLogoWindow = nullptr;

CStartupLogoDialog::CStartupLogoDialog(
    DialogType dialogType, QString versionText, QString richTextCopyrightNotice, QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui::StartupLogoDialog)
    , m_dialogType(dialogType)
{
    m_ui->setupUi(this);

    s_pLogoWindow = this;
    setFixedSize(QSize(EnforcedWidth, EnforcedHeight));
    setAttribute(Qt::WA_TranslucentBackground, true);

    // Prepare background image
    m_backgroundImage = AzQtComponents::ScalePixmapForScreenDpi(
        QPixmap(QStringLiteral(":/StartupLogoDialog/splashscreen_background.png")),
        screen(),
        QSize(EnforcedWidth, EnforcedHeight),
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation);

    m_ui->m_transparentAgreement->setObjectName("link");

    switch (m_dialogType)
    {
    case Loading:
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        m_ui->m_pages->setCurrentIndex(0);
        setWindowTitle(tr("Starting Open 3D Engine Editor"));
        m_ui->m_TransparentConfidential->setObjectName("copyrightNotice");
        m_ui->m_TransparentConfidential->setTextFormat(Qt::RichText);
        m_ui->m_TransparentConfidential->setText(richTextCopyrightNotice);
        m_ui->m_TransparentVersion->setText(versionText);
        setStyleSheet("QLabel { background: transparent; color: 'white' }\
                            QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; } ");

        break;
    case About:
        setWindowFlags(Qt::FramelessWindowHint | Qt::Popup | Qt::NoDropShadowWindowHint);
        m_ui->m_pages->setCurrentIndex(1);
        m_ui->m_transparentAllRightReserved->setObjectName("copyrightNotice");
        m_ui->m_transparentAllRightReserved->setTextFormat(Qt::RichText);
        m_ui->m_transparentAllRightReserved->setText(richTextCopyrightNotice);
        m_ui->m_transparentTrademarks->setText(versionText);
        setStyleSheet("QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; }\
                            QLabel#link { text-decoration: underline; color: #94D2FF; }");
        break;
    }

    // Draw the Open 3D Engine logo from svg
    m_ui->m_logo->load(QStringLiteral(":/StartupLogoDialog/o3de_logo.svg"));
}

CStartupLogoDialog::~CStartupLogoDialog()
{
    s_pLogoWindow = nullptr;
}

void CStartupLogoDialog::focusOutEvent([[maybe_unused]] QFocusEvent*)
{
    if (m_dialogType == About)
    {
        accept();
    }
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
    QMetaObject::invokeMethod(
        this,
        [this, text = QString(text)]()
        {
            m_ui->m_TransparentText->setText(text);
        });
}

void CStartupLogoDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), m_backgroundImage);
}

#include <moc_StartupLogoDialog.cpp>

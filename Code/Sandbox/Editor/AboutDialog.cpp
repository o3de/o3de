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

#include "AboutDialog.h"

// Qt
#include <QPainter>
#include <QDesktopServices>
#include <QSvgWidget>

// AzCore
#include <AzCore/Casting/numeric_cast.h>    // for aznumeric_cast


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_AboutDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

CAboutDialog::CAboutDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::CAboutDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(m_ui->m_transparentAgreement, &QLabel::linkActivated, this, &CAboutDialog::OnCustomerAgreement);
    connect(m_ui->m_transparentNotice, &QLabel::linkActivated, this, &CAboutDialog::OnPrivacyNotice);

    m_ui->m_transparentTrademarks->setText(versionText);

    m_ui->m_transparentAllRightReserved->setObjectName("copyrightNotice");
    m_ui->m_transparentAllRightReserved->setTextFormat(Qt::RichText);
    m_ui->m_transparentAllRightReserved->setText(richTextCopyrightNotice);

    m_ui->m_transparentAgreement->setObjectName("link");
    m_ui->m_transparentNotice->setObjectName("link");

    setStyleSheet( "CAboutDialog > QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; }\
                    CAboutDialog > QLabel#link { text-decoration: underline; color: #00A1C9; }");

    // Prepare background image
    QImage backgroundImage(QStringLiteral(":/StartupLogoDialog/splashscreen_1_27.png"));
    m_backgroundImage = QPixmap::fromImage(backgroundImage.scaled(m_enforcedWidth, m_enforcedHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    // Draw the Lumberyard logo from svg
    m_ui->m_logo->load(QStringLiteral(":/StartupLogoDialog/lumberyard_logo.svg"));

    // Prevent re-sizing
    setFixedSize(m_enforcedWidth, m_enforcedHeight);
}

CAboutDialog::~CAboutDialog()
{
}

void CAboutDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    QRect drawTarget = rect();
    painter.drawPixmap(drawTarget, m_backgroundImage);
}

void CAboutDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        accept();
    }

    QDialog::mouseReleaseEvent(event);
}

void CAboutDialog::OnCustomerAgreement()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("http://aws.amazon.com/agreement/")));
}

void CAboutDialog::OnPrivacyNotice()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("http://aws.amazon.com/privacy/")));
}

#include <moc_AboutDialog.cpp>

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include "EditorDefs.h"

#include "AboutDialog.h"

// Qt
#include <QPainter>
#include <QDesktopServices>
#include <QSvgWidget>

// AzCore
#include <AzCore/Casting/numeric_cast.h>    // for aznumeric_cast

#include <AzQtComponents/Utilities/PixmapScaleUtilities.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_AboutDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

CAboutDialog::CAboutDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui::CAboutDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(m_ui->m_transparentAgreement, &QLabel::linkActivated, this, &CAboutDialog::OnCustomerAgreement);

    m_ui->m_transparentTrademarks->setText(versionText);

    m_ui->m_transparentAllRightReserved->setObjectName("copyrightNotice");
    m_ui->m_transparentAllRightReserved->setTextFormat(Qt::RichText);
    m_ui->m_transparentAllRightReserved->setText(richTextCopyrightNotice);

    m_ui->m_transparentAgreement->setObjectName("link");

    setStyleSheet( "CAboutDialog > QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; }\
                    CAboutDialog > QLabel#link { text-decoration: underline; color: #94D2FF; }");

    // Prepare background image
    m_backgroundImage = AzQtComponents::ScalePixmapForScreenDpi(
        QPixmap(QStringLiteral(":/StartupLogoDialog/splashscreen_background_developer_preview.jpg")),
        screen(),
        QSize(m_enforcedWidth, m_enforcedHeight),
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
    );

    // Draw the Open 3D Engine logo from svg
    m_ui->m_logo->load(QStringLiteral(":/StartupLogoDialog/o3de_logo.svg"));

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
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.o3debinaries.org/license")));
}

#include <moc_AboutDialog.cpp>

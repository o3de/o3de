/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StartupLogoDialog.h"
#include "EditorDefs.h"
#include "EditorFlatSheet.h"
#include "EditorFlatValues.h"

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/PixmapScaleUtilities.h>

// Qt
#include <QPainter>
#include <QStyle>
#include <QThread>

#include <ui_StartupLogoDialog.h>

CStartupLogoDialog* CStartupLogoDialog::s_pLogoWindow = nullptr;

CStartupLogoDialog::CStartupLogoDialog(
    DialogType dialogType, QString versionText, QString richTextCopyrightNotice, QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui::StartupLogoDialog)
    , m_dialogType(dialogType)
{
    // Flattened GUI only: the elements that carry the sheets set below. The splash is made before the main window sets Editor.qss
    // (CryEdit.cpp), so the dialog registers them itself, before its form is built; once for the process.
    EditorFlatSheet::RegisterStartupLogoElements();

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
        // The sheet of the splash screen, for the legacy GUI only: the flattened GUI sets none, and the elements of EditorFlatSheet
        // (RegisterStartupLogoElements) carry the notice. Its label rule equals the universal rule (BaseStyleSheet.qss:17-18).
        AzQtComponents::StyleManager::setLegacyStyleSheet(this, EditorFlatValues::StartupLogoLoadingSheet());
        break;
    case About:
        setWindowFlags(Qt::FramelessWindowHint | Qt::Popup | Qt::NoDropShadowWindowHint);
        m_ui->m_pages->setCurrentIndex(1);
        m_ui->m_transparentAllRightReserved->setObjectName("copyrightNotice");
        m_ui->m_transparentAllRightReserved->setTextFormat(Qt::RichText);
        m_ui->m_transparentAllRightReserved->setText(richTextCopyrightNotice);
        m_ui->m_transparentTrademarks->setText(versionText);
        // The sheet of the About box, for the legacy GUI only. Its link rule reaches the box's own link and not the one on the hidden
        // page of the splash, which the flattened GUI's element tells by the mode of the dialog (the legacy GUI has a sheet per mode).
        AzQtComponents::StyleManager::setLegacyStyleSheet(this, EditorFlatValues::StartupLogoAboutSheet());
        break;
    }

    // Flattened GUI only. The form polished its labels while it was built (the stack shows its first page when the page is added to it,
    // and showing a widget polishes everything below it), and the switch above showed the About page: before the notice was named and
    // before the dialog had its tag, which the matchers of the elements read. The sheet of the legacy GUI repolished the whole dialog
    // when it was set (QStyleSheetStyle::repolish); here the About dialog is tagged and the labels below it, in either mode, are
    // polished again, those that are polished already, to resolve their elements. The legacy GUI sets nothing.
    if (AzQtComponents::StyleManager::flatStylingActive())
    {
        if (m_dialogType == About)
        {
            AzQtComponents::Style::addClass(this, QString::fromLatin1(EditorFlatSheet::StartupLogoAboutTag));
        }
        for (QWidget* each : findChildren<QWidget*>())
        {
            if (each->testAttribute(Qt::WA_WState_Polished))
            {
                each->style()->unpolish(each);
                each->style()->polish(each);
            }
        }
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


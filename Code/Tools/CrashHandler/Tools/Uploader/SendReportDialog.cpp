/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SendReportDialog.h"
#include "UI/ui_submit_report.h"

#include <QDesktopServices>
#include <QLabel>
#include <QUrl>

namespace CrashUploader
{

    SendReportDialog::SendReportDialog(bool manualReport, QWidget* parent)
        : QDialog(parent)
        , ui(new Ui::SendReportDialog)
        , m_manualReport(manualReport)
    {
        ui->setupUi(this);

        if (manualReport)
        {
            ui->comment_prompt_label->setText("Would you like to manually report the issue?");
        }

        connect(ui->diagnostics_label, &QLabel::linkActivated, this, [this](const QString&)
        {
            if (!m_reportDirectory.isEmpty())
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(m_reportDirectory));
            }
        });

        connect(ui->send_close_button, &QPushButton::clicked, this, [this]()
        {
            m_wantsRestart = false;
            accept();
        });

        connect(ui->send_restart_button, &QPushButton::clicked, this, [this]()
        {
            m_wantsRestart = true;
            accept();
        });
    }

    SendReportDialog::~SendReportDialog()
    {
    }

    void SendReportDialog::SetReportDirectory(const QString& reportDirectory)
    {
        m_reportDirectory = reportDirectory;
    }

    void SendReportDialog::SetSummaryText(const QString& summaryText)
    {
        ui->summary_edit->setPlainText(summaryText);
    }

    void SendReportDialog::SetApplicationName(const char* applicationName)
    {
        QString errorString{ applicationName };
        errorString += " has crashed";
        ui->header_label->setText(errorString);
    }

    QString SendReportDialog::GetUserComments() const
    {
        return ui->comments_edit->toPlainText();
    }

    bool SendReportDialog::WantsContact() const
    {
        return ui->contact_checkbox->isChecked();
    }
}

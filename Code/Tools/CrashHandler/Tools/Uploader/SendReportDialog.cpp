/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SendReportDialog.h"
#include "UI/ui_submit_report.h"

#include <QLabel>

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
            ui->question_label->setText("Would you like to manually report the issue ?");
        }
    }

    SendReportDialog::~SendReportDialog()
    {
    }

    void SendReportDialog::SetReportText(const QString& reportText)
    {
        ui->dump_label->setText(reportText);
    }

    void SendReportDialog::SetApplicationName(const char* applicationName)
    {
        QString errorString{ applicationName };
        errorString += " has encountered a fatal error.  We're sorry for the inconvenience.";
        ui->label->setText(errorString);
    }
}

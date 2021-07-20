/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SendReportDialog.h"
#include "UI/ui_submit_report.h"

namespace CrashUploader
{

    SendReportDialog::SendReportDialog(QWidget* parent)
        : QDialog(parent)
        , ui(new Ui::SendReportDialog)
    {
        ui->setupUi(this);
    }

    SendReportDialog::~SendReportDialog()
    {
    }

    void SendReportDialog::SetReportText(const char* reportText)
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

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

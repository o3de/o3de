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

#include "EditorDefs.h"

#include "FeedbackDialog.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <FeedbackDialog/ui_FeedbackDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


namespace {
    QString feedbackText = "<h3>We love getting feedback from our customers.</h3>"
        "Feedback from our community helps us to constantly improve Lumberyard.<br/><br/>"
        "In addition to using our forums and AWS support channels, you can always email us with your comments and suggestions at "
        "<a href=\"mailto:lumberyard-feedback@amazon.com\" style=\"color: #4285F4;\">lumberyard-feedback@amazon.com</a>.  "
        "While we do not respond to everyone who submits feedback, we read everything and aspire to use your feedback to improve Lumberyard for everyone.";
}


FeedbackDialog::FeedbackDialog(QWidget* pParent)
    : QDialog(pParent)
    , ui(new Ui::FeedbackDialog)
{
    ui->setupUi(this);
    ui->feedbackLabel->setText(feedbackText);
    resize(size().expandedTo(sizeHint()));
}


FeedbackDialog::~FeedbackDialog()
{
}


#include <FeedbackDialog/moc_FeedbackDialog.cpp>

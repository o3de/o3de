/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "FeedbackDialog.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <FeedbackDialog/ui_FeedbackDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


namespace {
    QString feedbackText = "<h3>We love getting feedback from our customers.</h3>"
        "Feedback from our community helps us to constantly improve Open 3D Engine.<br/><br/>"
        "In addition to using our forums and AWS support channels, you can always email us with your comments and suggestions at "
        "<a href=\"mailto:o3de-feedback@amazon.com\" style=\"color: #4285F4;\">o3de-feedback@amazon.com</a>.  "
        "While we do not respond to everyone who submits feedback, we read everything and aspire to use your feedback to improve Open 3D Engine for everyone.";
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

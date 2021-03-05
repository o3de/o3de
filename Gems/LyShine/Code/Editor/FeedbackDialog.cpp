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
#include "UiCanvasEditor_precompiled.h"
#include "FeedbackDialog.h"

#include <QBoxLayout>
#include <QLabel>

namespace {
    QString feedbackText = "<h3>We love getting feedback from our customers.</h3>"
        "Feedback from our community helps us to constantly improve the UI Editor.<br/><br/>"
        "In addition to using our forums and AWS support channels, you can always email us with your comments and suggestions at "
        "<a href=\"mailto:lumberyard-feedback@amazon.com?subject=UI Editor Feedback\" style=\"color: #4285F4;\">lumberyard-feedback@amazon.com</a>.  "
        "While we do not respond to everyone who submits feedback, we read everything and aspire to use your feedback to improve the UI Editor for everyone.";
}

FeedbackDialog::FeedbackDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Give Us Feedback");

    setMinimumSize(580, 204);

    QVBoxLayout* verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(20, 20, 20, 20);

    QLabel* feedbackLabel = new QLabel(this);
    feedbackLabel->setTextFormat(Qt::RichText);
    feedbackLabel->setAlignment(Qt::AlignCenter);
    feedbackLabel->setWordWrap(true);
    feedbackLabel->setOpenExternalLinks(true);
    feedbackLabel->setText(feedbackText);
    verticalLayout->addWidget(feedbackLabel);
}

#include <moc_FeedbackDialog.cpp>

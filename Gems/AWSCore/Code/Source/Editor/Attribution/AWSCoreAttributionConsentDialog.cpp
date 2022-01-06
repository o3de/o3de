/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Attribution/AWSCoreAttributionConsentDialog.h>

#include <QCheckBox>
#include <QLayout>

namespace AWSCore
{
    constexpr const char* AWSAttributionConsentDialogTitle = "AWS Core Gem Usage Agreement";
    constexpr const char* AWSAttributionConsentDialogMessage = "<nobr>The AWS Core Gem has detected credentials for an Amazon Web Services account for this</nobr><br>\
                           <nobr>instance of O3DE. <a href=\"https://o3de.org/docs/user-guide/gems/reference/aws/aws-core/configuring-credentials\">Click here</a> to learn more about AWS integration, including how to</nobr><br>\
                           <nobr>manage your AWS credentials.</nobr><br><br>\
                           <nobr>Please note: when credentials are detected, AWS Core Gem sends telemetry data to AWS,</nobr><br>\
                           <nobr>which helps us improve AWS services for O3DE. You can change this setting below, and at</nobr><br>\
                           <nobr>any time in Settings: Global Preferences. Data sent is subject to the <a href=\"https://aws.amazon.com/privacy\">AWS Privacy Policy</a>.</nobr><br>\
                           <nobr><a href=\"https://o3de.org/docs/user-guide/gems/reference/aws/aws-core/telemetry-data-collection\">Click here</a> to learn more about what data is sent to AWS.</nobr>";

    constexpr const char* AWSAttributionConsentDialogCheckboxText = "Please share the information about my use of AWS Core Gem with AWS.";

    AWSCoreAttributionConsentDialog::AWSCoreAttributionConsentDialog()
    {
        this->setWindowTitle(AWSAttributionConsentDialogTitle);
        this->setText(AWSAttributionConsentDialogMessage);
        QCheckBox* checkBox = new QCheckBox(AWSAttributionConsentDialogCheckboxText);
        checkBox->setChecked(true);
        this->setCheckBox(checkBox);
        this->setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        this->setDefaultButton(QMessageBox::Save);
        this->button(QMessageBox::Cancel)->hide();
        this->setIcon(QMessageBox::Information);
        if (QGridLayout* layout = static_cast<QGridLayout*>(this->layout()))
        {
            layout->setVerticalSpacing(20);
            layout->setHorizontalSpacing(10);
        }
    }
} // namespace AWSCore

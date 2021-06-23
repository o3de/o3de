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

#include <FormBrowseEditWidget.h>

#include <QPushButton>
#include <QHBoxLayout>

namespace O3DE::ProjectManager
{
    FormBrowseEditWidget::FormBrowseEditWidget(const QString& labelText, const QString& valueText, QWidget* parent)
        : FormLineEditWidget(labelText, valueText, parent)
    {
        setObjectName("formBrowseEditWidget");

        QPushButton* browseButton = new QPushButton(this);
        connect(browseButton, &QPushButton::pressed, this, &FormBrowseEditWidget::HandleBrowseButton);
        m_frameLayout->addWidget(browseButton); 
    }
} // namespace O3DE::ProjectManager

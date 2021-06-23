/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

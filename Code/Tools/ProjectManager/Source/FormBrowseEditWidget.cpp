/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    FormBrowseEditWidget::FormBrowseEditWidget(const QString& labelText, QWidget* parent)
        : FormBrowseEditWidget(labelText, "", parent)
    {
    }
} // namespace O3DE::ProjectManager

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FormBrowseEditWidget.h>

#include <QPushButton>
#include <QHBoxLayout>
#include <QKeyEvent>

namespace O3DE::ProjectManager
{
    FormBrowseEditWidget::FormBrowseEditWidget(
        const QString& labelText,
        const QString& valueText,
        const QString& placeholderText,
        const QString& errorText,
        QWidget* parent)
        : FormLineEditWidget(labelText, valueText, placeholderText, errorText, parent)
    {
        setObjectName("formBrowseEditWidget");

        QPushButton* browseButton = new QPushButton(this);
        connect(
            browseButton,
            &QPushButton::pressed,
            [this]
            {
                emit OnBrowse();
            });
        connect(this, &FormBrowseEditWidget::OnBrowse, this, &FormBrowseEditWidget::HandleBrowseButton);
        m_frameLayout->addWidget(browseButton); 
    }

    FormBrowseEditWidget::FormBrowseEditWidget(const QString& labelText, const QString& valueText, QWidget* parent)
        : FormBrowseEditWidget(labelText, valueText, "", "", parent)
    {
        
    }

    FormBrowseEditWidget::FormBrowseEditWidget(const QString& labelText, QWidget* parent)
        : FormBrowseEditWidget(labelText, "", "", "", parent)
    {
    }

    void FormBrowseEditWidget::keyPressEvent(QKeyEvent* event)
    {
        int key = event->key();
        if (key == Qt::Key_Return || key == Qt::Key_Enter)
        {
            emit OnBrowse();
        }
    }

} // namespace O3DE::ProjectManager

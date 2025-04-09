/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FormFolderBrowseEditWidget.h>
#include <AzQtComponents/Components/StyledLineEdit.h>

#include <QFileDialog>
#include <QLineEdit>
#include <QStandardPaths>

namespace O3DE::ProjectManager
{
    FormFolderBrowseEditWidget::FormFolderBrowseEditWidget(
        const QString& labelText,
        const QString& valueText,
        const QString& placeholderText,
        const QString& errorText,
        QWidget* parent)
        : FormBrowseEditWidget(labelText, valueText, placeholderText, errorText, parent)
    {
        setText(valueText);
    }

    FormFolderBrowseEditWidget::FormFolderBrowseEditWidget(const QString& labelText, const QString& valueText, QWidget* parent)
        : FormFolderBrowseEditWidget(labelText, valueText, "", "", parent)
    {
    }

    void FormFolderBrowseEditWidget::HandleBrowseButton()
    {
        QString defaultPath = m_lineEdit->text();
        if (defaultPath.isEmpty())
        {
            defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }

        QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Browse"), defaultPath));
        if (!directory.isEmpty())
        {
            setText(directory);
        }
    }

    void FormFolderBrowseEditWidget::setText(const QString& text)
    {
        QString path = QDir::toNativeSeparators(text);
        FormBrowseEditWidget::setText(path);
    }

} // namespace O3DE::ProjectManager

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

#include <FormFolderBrowseEditWidget.h>
#include <AzQtComponents/Components/StyledLineEdit.h>

#include <QFileDialog>
#include <QLineEdit>
#include <QStandardPaths>

namespace O3DE::ProjectManager
{
    FormFolderBrowseEditWidget::FormFolderBrowseEditWidget(const QString& labelText, const QString& valueText, QWidget* parent)
        : FormBrowseEditWidget(labelText, valueText, parent)
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
            m_lineEdit->setText(directory);
        }

    }
} // namespace O3DE::ProjectManager

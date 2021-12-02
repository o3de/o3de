/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "AutoDirectoryRestoreFileDialog.h"

// Qt
#include <QMessageBox>


CAutoDirectoryRestoreFileDialog::CAutoDirectoryRestoreFileDialog(
    QFileDialog::AcceptMode acceptMode,
    QFileDialog::FileMode fileMode,
    const QString& defaultSuffix,
    const QString& directory /* = {} */,
    const QString& filter /* = {} */,
    QFileDialog::Options options /* = {} */,
    const QString& caption /* = {} */,
    QWidget* parent /* = nullptr */)
    : QFileDialog(parent, caption, QString(""), filter)
{
    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(directory.toUtf8().data(), resolvedPath, AZ_MAX_PATH_LEN);
    setDirectory(QString::fromUtf8(resolvedPath));

    setAcceptMode(acceptMode);
    setDefaultSuffix(defaultSuffix);
    setFileMode(fileMode);
    setOptions(options);
}

int CAutoDirectoryRestoreFileDialog::exec()
{
    int result = -1;
    while ((result = QFileDialog::exec()) == QDialog::Accepted)
    {
        bool problem = false;
        foreach(const QString&fileName, selectedFiles())
        {
            QFileInfo info(fileName);
            if (!AZ::StringFunc::Path::IsValid(info.fileName().toStdString().c_str()))
            {
                QMessageBox::warning(this, tr("Error"), tr("Please select a valid file name (standard English alphanumeric characters only)"));
                problem = true;
                break;
            }
        }

        if (!problem)
        {
            return result;
        }
    }

    return result;
}

#include <Util/moc_AutoDirectoryRestoreFileDialog.cpp>

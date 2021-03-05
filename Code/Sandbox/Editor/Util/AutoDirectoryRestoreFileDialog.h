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

#ifndef CRYINCLUDE_EDITOR_UTIL_AUTODIRECTORYRESTOREFILEDIALOG_H
#define CRYINCLUDE_EDITOR_UTIL_AUTODIRECTORYRESTOREFILEDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QFileDialog>
#endif

class CAutoDirectoryRestoreFileDialog
    : public QFileDialog
{
    Q_OBJECT

public:
    explicit CAutoDirectoryRestoreFileDialog(
        QFileDialog::AcceptMode acceptMode,
        QFileDialog::FileMode fileMode = QFileDialog::AnyFile,
        const QString& defaultSuffix = {},
        const QString& directory = {},
        const QString& filter = {},
        QFileDialog::Options options = QFileDialog::Options(),
        const QString& caption = {},
        QWidget* parent = nullptr);
    virtual ~CAutoDirectoryRestoreFileDialog() {}

    int exec() override;
};


#endif // CRYINCLUDE_EDITOR_UTIL_AUTODIRECTORYRESTOREFILEDIALOG_H

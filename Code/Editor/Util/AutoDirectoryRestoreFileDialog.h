/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>

#include <QWidget>
#include <QString>

namespace Ui {
    class CPythonScriptsDialog;
}

class CPythonScriptsDialog
    : public QWidget
{
    Q_OBJECT
public:
    explicit CPythonScriptsDialog(QWidget* parent = nullptr);
    ~CPythonScriptsDialog();

    static void RegisterViewClass();

private slots:
    void OnExecute();

protected:
    void ScanFolderForScripts(QString path, AZStd::vector<QString>& scriptFolders) const;

private:
    Ui::CPythonScriptsDialog* ui;
};

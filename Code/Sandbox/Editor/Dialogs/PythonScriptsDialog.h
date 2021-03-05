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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_DIALOGS_PYTHONSCRIPTSDIALOG_H
#define CRYINCLUDE_EDITOR_DIALOGS_PYTHONSCRIPTSDIALOG_H
#pragma once


#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#endif

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

    static const GUID& GetClassID()
    {
        // {C61C9C4C-CFED-47C4-8FE1-79069D0284E1}
        static const GUID guid = {
            0xc61c9c4c, 0xcfed, 0x47c4, { 0x8f, 0xe1, 0x79, 0x6, 0x9d, 0x2, 0x84, 0xe1 }
        };

        return guid;
    }

    static void RegisterViewClass();

private slots:
    void OnExecute();

protected:
    void ScanFolderForScripts(QString path, QStringList& scriptFolders) const;

private:
    QScopedPointer<Ui::CPythonScriptsDialog> ui;
};


#endif // CRYINCLUDE_EDITOR_DIALOGS_PYTHONSCRIPTSDIALOG_H

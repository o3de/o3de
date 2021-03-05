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

// Description : Dialog for python script terminal

#ifndef CRYINCLUDE_EDITOR_SCRIPTTERMDIALOG_H
#define CRYINCLUDE_EDITOR_SCRIPTTERMDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>

#include <QWidget>

#include <QColor>
#include <QScopedPointer>
#endif


#define SCRIPT_TERM_WINDOW_NAME "Python Console"

class QStringListModel;
namespace Ui {
    class CScriptTermDialog;
}

namespace AzToolsFramework
{
    class CScriptTermDialog
        : public QWidget
        , protected EditorPythonConsoleNotificationBus::Handler
        , protected EditorPreferencesNotificationBus::Handler
    {
        Q_OBJECT
    public:
        explicit CScriptTermDialog(QWidget* parent = nullptr);
        ~CScriptTermDialog();

        void AppendText(const char* pText);
        static void RegisterViewClass();

    protected:
        bool eventFilter(QObject* obj, QEvent* e) override;

        //! EditorPythonConsoleNotificationBus::Handler
        void OnTraceMessage(AZStd::string_view message) override;
        void OnErrorMessage(AZStd::string_view message) override;
        void OnExceptionMessage(AZStd::string_view message) override;

        //! EditorPreferencesNotificationBus
        void OnEditorPreferencesChanged() override;

    private slots:
        void OnScriptHelp();
        void OnOK();
        void OnScriptInputTextChanged(const QString& text);

    private:
        void RefreshStyle();
        void InitCompleter();
        void ExecuteAndPrint(const char* cmd);
        void AppendToConsole(const QString& string, const QColor& color, bool bold = false);

        QScopedPointer<Ui::CScriptTermDialog> ui;
        QStringListModel* m_completionModel;
        QStringListModel* m_lastCommandModel;
        QStringList m_lastCommands;

        QColor m_textColor;
        QColor m_warningColor;
        QColor m_errorColor;

        int m_upArrowLastCommandIndex = -1;
    };
} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_SCRIPTTERMDIALOG_H

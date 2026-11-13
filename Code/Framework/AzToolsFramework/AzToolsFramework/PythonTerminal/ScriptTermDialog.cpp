/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Dialog for python script terminal

#include "ScriptTermDialog.h"

// Qt
#include <QCompleter>
#include <QDesktopServices>
#include <QStringListModel>
#include <QStringBuilder>
#include <QToolButton>
#include <QLineEdit>

// AzToolsFramework
#include <AzToolsFramework/API/ViewPaneOptions.h>                   // for AzToolsFramework::ViewPaneOptions
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>     // for AzToolsFramework::EditorPythonRunnerRequestBus
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/ScrollBar.h>

// Editor
#include "ScriptHelpDialog.h"

#include <AzToolsFramework/PythonTerminal/ui_ScriptTermDialog.h>

namespace AzToolsFramework
{
    // =======================================================================
    // Console Colours
    // =======================================================================

    //! Every colour the Python Console sets for its output and its background, per console colour theme, named, so the legacy and the
    //! flattened GUI read one value. A named namespace, not an anonymous one: a unity build joins files into one translation unit,
    //! where anonymous namespaces merge.
    namespace ScriptTermDialogFlatInternal
    {
        // The light console colour theme
        const QColor LightText(Qt::black);                  // ScriptTermDialog.cpp:78. TOKENIZE: PythonConsoleLightText
        constexpr QColor LightError(200, 0, 0);             // ScriptTermDialog.cpp:79. TOKENIZE: PythonConsoleLightError
        constexpr QColor LightWarning(128, 112, 0);         // ScriptTermDialog.cpp:80. TOKENIZE: PythonConsoleLightWarning
        const QColor LightBackground(Qt::white);            // ScriptTermDialog.cpp:100. TOKENIZE: PythonConsoleLightBackground

        // The dark console colour theme
        const QColor DarkText(Qt::white);                   // ScriptTermDialog.cpp:87. TOKENIZE: PythonConsoleDarkText
        constexpr QColor DarkError(0xfa, 0x27, 0x27);       // ScriptTermDialog.cpp:88. TOKENIZE: PythonConsoleDarkError
        constexpr QColor DarkWarning(0xff, 0xaa, 0x22);     // ScriptTermDialog.cpp:89. TOKENIZE: PythonConsoleDarkWarning
        constexpr QColor DarkBackground(0x22, 0x22, 0x22);  // ScriptTermDialog.cpp:95. TOKENIZE: PythonConsoleDarkBackground
    }

    static constexpr const char* OtherCategory = "Other";

    CScriptTermDialog::CScriptTermDialog(QWidget* parent)
        : QWidget(parent)
        , ui(new Ui::CScriptTermDialog)
    {
        ui->setupUi(this);
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();

        ui->SCRIPT_INPUT->installEventFilter(this);

        connect(ui->SCRIPT_INPUT, &QLineEdit::returnPressed, this, &CScriptTermDialog::OnOK);
        connect(ui->SCRIPT_INPUT, &QLineEdit::textChanged, this, &CScriptTermDialog::OnScriptInputTextChanged);
        connect(ui->SCRIPT_HELP, &QToolButton::clicked, this, &CScriptTermDialog::OnScriptHelp);
        connect(ui->SCRIPT_DOCS, &QToolButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl("https://o3de.org/docs/user-guide/scripting/"));
            });

        InitCompleter();
        RefreshStyle();

        EditorPreferencesNotificationBus::Handler::BusConnect();
    }

    CScriptTermDialog::~CScriptTermDialog()
    {
        EditorPreferencesNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
    }

    void CScriptTermDialog::RegisterViewClass()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.canHaveMultipleInstances = true;
        AzToolsFramework::RegisterViewPane<CScriptTermDialog>(SCRIPT_TERM_WINDOW_NAME, OtherCategory, options);
    }

    void CScriptTermDialog::RefreshStyle()
    {
        // Set the debug/warning text colors appropriately for the background theme
        // (e.g. not have black text on black background)
        m_textColor = ScriptTermDialogFlatInternal::LightText;
        m_errorColor = ScriptTermDialogFlatInternal::LightError; // Error (Red)
        m_warningColor = ScriptTermDialogFlatInternal::LightWarning; // Warning (Yellow)

        ConsoleColorTheme consoleColorTheme(ConsoleColorTheme::Dark);
        EditorSettingsAPIBus::BroadcastResult(consoleColorTheme, &EditorSettingsAPIBus::Handler::GetConsoleColorTheme);

        if (consoleColorTheme == ConsoleColorTheme::Dark)
        {
            m_textColor = ScriptTermDialogFlatInternal::DarkText;
            m_errorColor = ScriptTermDialogFlatInternal::DarkError; // Error (Red)
            m_warningColor = ScriptTermDialogFlatInternal::DarkWarning; // Warning (Yellow)
        }

        QColor bgColor;
        if (consoleColorTheme == ConsoleColorTheme::Dark)
        {
            bgColor = ScriptTermDialogFlatInternal::DarkBackground;
            AzQtComponents::ScrollBar::applyLightStyle(ui->SCRIPT_OUTPUT);
        }
        else
        {
            bgColor = ScriptTermDialogFlatInternal::LightBackground;
            AzQtComponents::ScrollBar::applyDarkStyle(ui->SCRIPT_OUTPUT);
        }
        AzQtComponents::ScrollBar::setBackgroundColor(ui->SCRIPT_OUTPUT, bgColor);

        // Clear out the console text when we change our background color since
        // some of the previous text colors may not be appropriate for the
        // new background color
        QString text = ui->SCRIPT_OUTPUT->toPlainText();
        ui->SCRIPT_OUTPUT->clear();
        AppendToConsole(text, m_textColor);
    }

    void CScriptTermDialog::InitCompleter()
    {
        QStringList inputs;

        AzToolsFramework::EditorPythonConsoleInterface* editorPythonConsoleInterface = AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Get();
        if (editorPythonConsoleInterface)
        {
            AzToolsFramework::EditorPythonConsoleInterface::GlobalFunctionCollection globalFunctionCollection;
            editorPythonConsoleInterface->GetGlobalFunctionList(globalFunctionCollection);
            for (const AzToolsFramework::EditorPythonConsoleInterface::GlobalFunction& globalFunction : globalFunctionCollection)
            {
                inputs.append(QString("%1.%2()").arg(globalFunction.m_moduleName.data()).arg(globalFunction.m_functionName.data()));
            }
        }

        m_lastCommandModel = new QStringListModel(ui->SCRIPT_INPUT);
        m_completionModel = new QStringListModel(inputs, ui->SCRIPT_INPUT);
        auto completer = new QCompleter(m_completionModel, ui->SCRIPT_INPUT);
        ui->SCRIPT_INPUT->setCompleter(completer);
    }

    void CScriptTermDialog::OnOK()
    {
        const QString command = ui->SCRIPT_INPUT->text();
        QString command2 = QLatin1String("] ")
            % ui->SCRIPT_INPUT->text()
            % QLatin1String("\r\n");
        AppendToConsole(command2, m_textColor);

        // Add the command to the history.
        m_lastCommands.removeOne(command);
        if (!command.isEmpty())
        {
            m_lastCommands.prepend(command);
        }

        ExecuteAndPrint(command.toLocal8Bit().data());
        //clear script input via a QueuedConnection because completer sets text when it's done, undoing our clear.
        QMetaObject::invokeMethod(ui->SCRIPT_INPUT, "setText", Qt::QueuedConnection, Q_ARG(QString, ""));
    }

    void CScriptTermDialog::OnScriptInputTextChanged(const QString& text)
    {
        if (text.isEmpty())
        {
            ui->SCRIPT_INPUT->completer()->setModel(m_completionModel);
        }
    }

    void CScriptTermDialog::OnScriptHelp()
    {
        CScriptHelpDialog::GetInstance()->show();
    }

    void CScriptTermDialog::ExecuteAndPrint(const char* cmd)
    {
        if (AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers())
        {
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, cmd, true);
        }
        else
        {
            AZ_Warning("python", false, "EditorPythonRunnerRequestBus has no handlers");
        }
    }

    void CScriptTermDialog::AppendText(const char* pText)
    {
        AppendToConsole(pText, m_textColor);
    }

    void CScriptTermDialog::OnTraceMessage(AZStd::string_view message)
    {
        AppendToConsole(message.data(), m_textColor);
    }

    void CScriptTermDialog::OnErrorMessage(AZStd::string_view message)
    {
        AppendToConsole(message.data(), m_errorColor, true);
    }

    void CScriptTermDialog::OnExceptionMessage(AZStd::string_view message)
    {
        AppendToConsole(message.data(), m_warningColor, true);
    }

    void CScriptTermDialog::OnEditorPreferencesChanged()
    {
        RefreshStyle();
    }

    void CScriptTermDialog::AppendToConsole(const QString& string, const QColor& color, bool bold)
    {
        QTextCharFormat format;
        format.setForeground(color);
        if (bold)
        {
            format.setFontWeight(QFont::Bold);
        }

        QTextCursor cursor(ui->SCRIPT_OUTPUT->document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(string, format);
    }

    bool CScriptTermDialog::eventFilter(QObject* obj, QEvent* e)
    {
        if (e->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if (keyEvent->key() == Qt::Key_Down)
            {
                m_lastCommandModel->setStringList(m_lastCommands);
                ui->SCRIPT_INPUT->completer()->setModel(m_lastCommandModel);
                ui->SCRIPT_INPUT->completer()->setCompletionPrefix("");
                ui->SCRIPT_INPUT->completer()->complete();
                m_upArrowLastCommandIndex = -1;
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Up)
            {
                if (!m_lastCommands.isEmpty())
                {
                    m_upArrowLastCommandIndex++;
                    if (m_upArrowLastCommandIndex < 0)
                    {
                        m_upArrowLastCommandIndex = 0;
                    }
                    else if (m_upArrowLastCommandIndex >= m_lastCommands.size())
                    {
                        // Already at the last item, nothing to do
                        return true;
                    }

                    ui->SCRIPT_INPUT->setText(m_lastCommands.at(m_upArrowLastCommandIndex));
                }
            }
            else
            {
                // Reset cycling, we only want for sequential up arrow presses
                m_upArrowLastCommandIndex = -1;
            }
        }
        return QObject::eventFilter(obj, e);
    }
} // namespace AzToolsFramework

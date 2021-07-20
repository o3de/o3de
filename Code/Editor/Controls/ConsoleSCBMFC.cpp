/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "ConsoleSCBMFC.h"
#include "PropertiesDialog.h"
#include "QtViewPaneManager.h"
#include "Core/QtEditorApplication.h"

#include <Controls/ui_ConsoleSCBMFC.h>

#include <QtUtil.h>
#include <QtUtilWin.h>

#include <QtCore/QStringList>
#include <QtCore/QScopedPointer>
#include <QtCore/QPoint>
#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QVBoxLayout>

#include <vector>
#include <iostream>

namespace MFC
{

static CPropertiesDialog* gPropertiesDlg = nullptr;
static CString mfc_popup_helper(HWND hwnd, int x, int y);
static CConsoleSCB* s_consoleSCB = nullptr;

static QString RemoveColorCode(const QString& text, int& iColorCode)
{
    QString cleanString;
    cleanString.reserve(text.size());

    const int textSize = text.size();
    for (int i = 0; i < textSize; ++i)
    {
        QChar c = text.at(i);
        bool isLast = i == textSize - 1;
        if (c == '$' && !isLast && text.at(i + 1).isDigit())
        {
            if (iColorCode == 0)
            {
                iColorCode = text.at(i + 1).digitValue();
            }
            ++i;
            continue;
        }

        if (c == '\r' || c == '\n')
        {
            ++i;
            continue;
        }

        cleanString.append(c);
    }

    return cleanString;
}

ConsoleLineEdit::ConsoleLineEdit(QWidget* parent)
    : QLineEdit(parent)
    , m_historyIndex(0)
    , m_bReusedHistory(false)
{
}

void ConsoleLineEdit::mousePressEvent(QMouseEvent* ev)
{
    if (ev->type() == QEvent::MouseButtonPress && ev->button() & Qt::RightButton)
    {
        Q_EMIT variableEditorRequested();
    }

    QLineEdit::mousePressEvent(ev);
}

void ConsoleLineEdit::mouseDoubleClickEvent(QMouseEvent* ev)
{
    Q_EMIT variableEditorRequested();
}

bool ConsoleLineEdit::event(QEvent* ev)
{
    // Tab key doesn't go to keyPressEvent(), must be processed here

    if (ev->type() != QEvent::KeyPress)
    {
        return QLineEdit::event(ev);
    }

    QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
    if (ke->key() != Qt::Key_Tab)
    {
        return QLineEdit::event(ev);
    }

    QString inputStr = text();
    QString newStr;

    QStringList tokens = inputStr.split(" ");
    inputStr = tokens.isEmpty() ? QString() : tokens.first();
    IConsole* console = GetIEditor()->GetSystem()->GetIConsole();

    const bool ctrlPressed = ke->modifiers() & Qt::ControlModifier;
    CString cstring = QtUtil::ToCString(inputStr); // TODO: Use QString once the backend stops using QString
    if (ctrlPressed)
    {
        newStr = QtUtil::ToString(console->AutoCompletePrev(cstring));
    }
    else
    {
        newStr = QtUtil::ToString(console->ProcessCompletion(cstring));
        newStr = QtUtil::ToString(console->AutoComplete(cstring));

        if (newStr.isEmpty())
        {
            newStr = QtUtil::ToQString(GetIEditor()->GetCommandManager()->AutoComplete(QtUtil::ToString(newStr)));
        }
    }

    if (!newStr.isEmpty())
    {
        newStr += " ";
        setText(newStr);
    }

    deselect();
    return true;
}

void ConsoleLineEdit::keyPressEvent(QKeyEvent* ev)
{
    IConsole* console = GetIEditor()->GetSystem()->GetIConsole();
    auto commandManager = GetIEditor()->GetCommandManager();

    console->ResetAutoCompletion();

    switch (ev->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    {
        QString str = text().trimmed();
        if (!str.isEmpty())
        {
            if (commandManager->IsRegistered(QtUtil::ToCString(str)))
            {
                commandManager->Execute(QtUtil::ToString(str));
            }
            else
            {
                CLogFile::WriteLine(QtUtil::ToCString(str));
                GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(QtUtil::ToCString(str));
            }

            // If a history command was reused directly via up arrow enter, do not reset history index
            if (m_history.size() > 0 && m_historyIndex < m_history.size() && m_history[m_historyIndex] == str)
            {
                m_bReusedHistory = true;
            }
            else
            {
                m_historyIndex = m_history.size();
            }

            // Do not add the same string if it is the top of the stack, but allow duplicate entries otherwise
            if (m_history.isEmpty() || m_history.back() != str)
            {
                m_history.push_back(str);
                if (!m_bReusedHistory)
                {
                    m_historyIndex = m_history.size();
                }
            }
        }
        else
        {
            m_historyIndex = m_history.size();
        }

        setText(QString());
        break;
    }
    case Qt::Key_AsciiTilde: // ~
    case Qt::Key_Agrave: // `
        // disable log.
        GetIEditor()->ShowConsole(false);
        setText(QString());
        m_historyIndex = m_history.size();
        break;
    case Qt::Key_Escape:
        setText(QString());
        m_historyIndex = m_history.size();
        break;
    case Qt::Key_Up:
        DisplayHistory(false /*bForward*/);
        break;
    case Qt::Key_Down:
        DisplayHistory(true /*bForward*/);
        break;
    default:
        QLineEdit::keyPressEvent(ev);
    }
}

void ConsoleLineEdit::DisplayHistory(bool bForward)
{
    if (m_history.isEmpty())
    {
        return;
    }

    // Immediately after reusing a history entry, ensure up arrow re-displays command just used
    if (!m_bReusedHistory || bForward)
    {
        m_historyIndex = static_cast<unsigned int>(clamp_tpl(static_cast<int>(m_historyIndex) + (bForward ? 1 : -1), 0, m_history.size() - 1));
    }
    m_bReusedHistory = false;

    setText(m_history[m_historyIndex]);
}

ConsoleTextEdit::ConsoleTextEdit(QWidget* parent)
    : QTextEdit(parent)
{
}


Lines CConsoleSCB::s_pendingLines;

CConsoleSCB::CConsoleSCB(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ConsoleMFC())
    , m_richEditTextLength(0)
    , m_backgroundTheme(gSettings.consoleBackgroundColorTheme)
{
    m_lines = s_pendingLines;
    s_pendingLines.clear();
    s_consoleSCB = this;
    ui->setupUi(this);
    setMinimumHeight(120);

    // Setup the color table for the default (light) theme
    m_colorTable << QColor(0, 0, 0)
        << QColor(0, 0, 0)
        << QColor(0, 0, 200) // blue
        << QColor(0, 200, 0) // green
        << QColor(200, 0, 0) // red
        << QColor(0, 200, 200) // cyan
        << QColor(128, 112, 0) // yellow
        << QColor(200, 0, 200) // red+blue
        << QColor(0x000080ff)
        << QColor(0x008f8f8f);
    OnStyleSettingsChanged();

    connect(ui->button, &QPushButton::clicked, this, &CConsoleSCB::showVariableEditor);
    connect(ui->lineEdit, &MFC::ConsoleLineEdit::variableEditorRequested, this, &MFC::CConsoleSCB::showVariableEditor);
    connect(Editor::EditorQtApplication::instance(), &Editor::EditorQtApplication::skinChanged, this, &MFC::CConsoleSCB::OnStyleSettingsChanged);

    if (GetIEditor()->IsInConsolewMode())
    {
        // Attach / register edit box
        //CLogFile::AttachEditBox(m_edit.GetSafeHwnd()); // FIXME
    }
}

CConsoleSCB::~CConsoleSCB()
{
    s_consoleSCB = nullptr;
    delete gPropertiesDlg;
    gPropertiesDlg = nullptr;
    CLogFile::AttachEditBox(nullptr);
}

void CConsoleSCB::RegisterViewClass()
{
    QtViewOptions opts;
    opts.preferedDockingArea = Qt::BottomDockWidgetArea;
    opts.isDeletable = false;
    opts.isStandard = true;
    opts.showInMenu = true;
    opts.builtInActionId = ID_VIEW_CONSOLEWINDOW;
    opts.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    RegisterQtViewPane<CConsoleSCB>(GetIEditor(), LyViewPane::Console, LyViewPane::CategoryTools, opts);
}

void CConsoleSCB::OnStyleSettingsChanged()
{
    ui->button->setIcon(QIcon(QString(":/controls/img/cvar_dark.bmp")));

    // Set the debug/warning text colors appropriately for the background theme
    // (e.g. not have black text on black background)
    QColor textColor = Qt::black;
    m_backgroundTheme = gSettings.consoleBackgroundColorTheme;
    if (m_backgroundTheme == SEditorSettings::ConsoleColorTheme::Dark)
    {
        textColor = Qt::white;
    }
    m_colorTable[0] = textColor;
    m_colorTable[1] = textColor;

    QColor bgColor;
    if (!GetIEditor()->IsInConsolewMode() && CConsoleSCB::GetCreatedInstance() && m_backgroundTheme == SEditorSettings::ConsoleColorTheme::Dark)
    {
        bgColor = Qt::black;
    }
    else
    {
        bgColor = Qt::white;
    }

    ui->textEdit->setStyleSheet(QString("QTextEdit{ background: %1 }").arg(bgColor.name(QColor::HexRgb)));

    // Clear out the console text when we change our background color since
    // some of the previous text colors may not be appropriate for the
    // new background color
    ui->textEdit->clear();
}

void CConsoleSCB::showVariableEditor()
{
    const QPoint cursorPos = QCursor::pos();
    CString str = mfc_popup_helper(0, cursorPos.x(), cursorPos.y());
    if (!str.IsEmpty())
    {
        ui->lineEdit->setText(QtUtil::ToQString(str));
    }
}

void CConsoleSCB::SetInputFocus()
{
    ui->lineEdit->setFocus();
    ui->lineEdit->setText(QString());
}

void CConsoleSCB::AddToConsole(const QString& text, bool bNewLine)
{
    m_lines.push_back({ text, bNewLine });
    FlushText();
}

void CConsoleSCB::FlushText()
{
    if (m_lines.empty())
    {
        return;
    }

    // Store our current cursor in case we need to restore it, and check if
    // the user has scrolled the text edit away from the bottom
    const QTextCursor oldCursor = ui->textEdit->textCursor();
    QScrollBar* scrollBar = ui->textEdit->verticalScrollBar();
    const int oldScrollValue = scrollBar->value();
    bool scrolledOffBottom = oldScrollValue != scrollBar->maximum();

    ui->textEdit->moveCursor(QTextCursor::End);
    QTextCursor textCursor = ui->textEdit->textCursor();

    while (!m_lines.empty())
    {
        ConsoleLine line = m_lines.front();
        m_lines.pop_front();

        int iColor = 0;
        QString text = MFC::RemoveColorCode(line.text, iColor);
        if (iColor < 0 || iColor >= m_colorTable.size())
        {
            iColor = 0;
        }

        if (line.newLine)
        {
            text = QtUtil::trimRight(text);
            text = "\r\n" + text;
        }

        QTextCharFormat format;
        const QColor color(m_colorTable[iColor]);
        format.setForeground(color);

        if (iColor != 0)
        {
            format.setFontWeight(QFont::Bold);
        }

        textCursor.setCharFormat(format);
        textCursor.insertText(text);
    }

    // If the user has selected some text in the text edit area or has scrolled
    // away from the bottom, then restore the previous cursor and keep the scroll
    // bar in the same location
    if (oldCursor.hasSelection() || scrolledOffBottom)
    {
        ui->textEdit->setTextCursor(oldCursor);
        scrollBar->setValue(oldScrollValue);
    }
    // Otherwise scroll to the bottom so the latest text can be seen
    else
    {
        scrollBar->setValue(scrollBar->maximum());
    }
}

QSize CConsoleSCB::minimumSizeHint() const
{
    return QSize(-1, -1);
}

QSize CConsoleSCB::sizeHint() const
{
    return QSize(100, 100);
}

/** static */
void CConsoleSCB::AddToPendingLines(const QString& text, bool bNewLine)
{
    s_pendingLines.push_back({ text, bNewLine });
}

static CVarBlock* VarBlockFromConsoleVars()
{
    IConsole* console = GetIEditor()->GetSystem()->GetIConsole();
    std::vector<const char*> cmds;
    cmds.resize(console->GetNumVars());
    size_t cmdCount = console->GetSortedVars(&cmds[0], cmds.size());

    CVarBlock* vb = new CVarBlock;
    IVariable* pVariable = 0;
    for (int i = 0; i < cmdCount; i++)
    {
        ICVar* pCVar = console->GetCVar(cmds[i]);
        if (!pCVar)
        {
            continue;
        }
        int varType = pCVar->GetType();

        switch (varType)
        {
        case CVAR_INT:
            pVariable = new CVariable<int>();
            pVariable->Set(pCVar->GetIVal());
            break;
        case CVAR_FLOAT:
            pVariable = new CVariable<float>();
            pVariable->Set(pCVar->GetFVal());
            break;
        case CVAR_STRING:
            pVariable = new CVariable<CString>();
            pVariable->Set(pCVar->GetString());
            break;
        default:
            assert(0);
        }

        pVariable->SetDescription(pCVar->GetHelp());
        pVariable->SetName(cmds[i]);

        if (pVariable)
        {
            vb->AddVariable(pVariable);
        }
    }
    return vb;
}

static void OnConsoleVariableUpdated(IVariable* pVar)
{
    if (!pVar)
    {
        return;
    }
    CString varName = pVar->GetName();
    ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(varName);
    if (!pCVar)
    {
        return;
    }
    if (pVar->GetType() == IVariable::INT)
    {
        int val;
        pVar->Get(val);
        pCVar->Set(val);
    }
    else if (pVar->GetType() == IVariable::FLOAT)
    {
        float val;
        pVar->Get(val);
        pCVar->Set(val);
    }
    else if (pVar->GetType() == IVariable::STRING)
    {
        CString val;
        pVar->Get(val);
        pCVar->Set(val);
    }
}

static CString mfc_popup_helper(HWND hwnd, int x, int y)
{
    IConsole* console = GetIEditor()->GetSystem()->GetIConsole();

    TSmartPtr<CVarBlock> vb = VarBlockFromConsoleVars();
    XmlNodeRef node;
    if (!gPropertiesDlg)
    {
        gPropertiesDlg = new CPropertiesDialog("Console Variables", node, AfxGetMainWnd(), true);
    }
    if (!gPropertiesDlg->m_hWnd)
    {
        gPropertiesDlg->Create(CPropertiesDialog::IDD, AfxGetMainWnd());
        gPropertiesDlg->SetUpdateCallback(AZStd::bind(OnConsoleVariableUpdated, AZStd::placeholders::_1));
    }
    gPropertiesDlg->ShowWindow(SW_SHOW);
    gPropertiesDlg->BringWindowToTop();
    gPropertiesDlg->GetPropertyCtrl()->AddVarBlock(vb);

    return "";
}

CConsoleSCB* CConsoleSCB::GetCreatedInstance()
{
    return s_consoleSCB;
}

} // namespace MFC

#include <Controls/moc_ConsoleSCBMFC.cpp>

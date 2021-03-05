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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCBMFC_H
#define CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCBMFC_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>

#include "ConsoleSCB.h"
#endif

class QMenu;
class ConsoleWidget;
class QFocusEvent;

namespace Ui {
    class ConsoleMFC;
}

namespace MFC
{

struct ConsoleLine
{
    QString text;
    bool newLine;
};
typedef std::deque<ConsoleLine> Lines;

class ConsoleLineEdit
    : public QLineEdit
{
    Q_OBJECT
public:
    explicit ConsoleLineEdit(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    bool event(QEvent* ev) override;

signals:
    void variableEditorRequested();
    void setWindowTitle(const QString&);

private:
    void DisplayHistory(bool bForward);
    QStringList m_history;
    unsigned int m_historyIndex;
    bool m_bReusedHistory;
};

class ConsoleTextEdit
    : public QTextEdit
{
    Q_OBJECT
public:
    explicit ConsoleTextEdit(QWidget* parent = nullptr);
};

class CConsoleSCB
    : public QWidget
{
    Q_OBJECT
public:
    explicit CConsoleSCB(QWidget* parent = nullptr);
    ~CConsoleSCB();

    static void RegisterViewClass();
    void SetInputFocus();
    void AddToConsole(const QString& text, bool bNewLine);
    void FlushText();
    void showPopupAndSetTitle();
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    static CConsoleSCB* GetCreatedInstance();

    static void AddToPendingLines(const QString& text, bool bNewLine);        // call this function instead of AddToConsole() until an instance of CConsoleSCB exists to prevent messages from getting lost

public Q_SLOTS:
    void OnStyleSettingsChanged();

private Q_SLOTS:
    void showVariableEditor();

private:
    QScopedPointer<Ui::ConsoleMFC> ui;
    int m_richEditTextLength;

    Lines m_lines;
    static Lines s_pendingLines;

    QList<QColor> m_colorTable;
    SEditorSettings::ConsoleColorTheme m_backgroundTheme;
};

} // namespace MFC

#endif // CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCB_H


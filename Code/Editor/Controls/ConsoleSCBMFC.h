/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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


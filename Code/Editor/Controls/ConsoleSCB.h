/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCB_H
#define CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCB_H
#pragma once

#if !defined(Q_MOC_RUN)

#include "Settings.h"
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>

#include <QLineEdit>
#include <QPlainTextEdit>

#include <QAbstractTableModel>
#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopedPointer>
#include <QStyledItemDelegate>
#endif

class QMenu;
class ConsoleWidget;
class QFocusEvent;
class QTableView;
class CVarBlock;

namespace Ui {
    class Console;
}

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
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    bool event(QEvent* ev) override;

signals:
    void variableEditorRequested();

private:
    void DisplayHistory(bool bForward);
    void ResetHistoryIndex();

    QStringList m_history;
    unsigned int m_historyIndex;
    bool m_bReusedHistory;
};

class ConsoleVariableItemDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ConsoleVariableItemDelegate(QObject* parent = nullptr);

    // Item delegate overrides for creating the custom editor widget and
    // setting/retrieving data to/from it
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void SetVarBlock(CVarBlock* varBlock);

private:
    CVarBlock* m_varBlock;
};

class ConsoleVariableModel
    : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum CustomRoles
    {
        VariableCustomRole = Qt::UserRole
    };

    explicit ConsoleVariableModel(QObject* parent = nullptr);

    // Table model overrides
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    int rowCount(const QModelIndex& = {}) const override;
    int columnCount(const QModelIndex& = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void SetVarBlock(CVarBlock* varBlock);
    void ClearModifiedRows();

private:
    CVarBlock* m_varBlock;
    QList<int> m_modifiedRows;
};

class ConsoleVariableEditor
    : public QWidget
{
    Q_OBJECT
public:
    explicit ConsoleVariableEditor(QWidget* parent = nullptr);

    static void RegisterViewClass();
    void HandleVariableRowUpdated(ICVar* pCVar);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void SetVarBlock(CVarBlock* varBlock);

    QTableView* m_tableView;
    ConsoleVariableModel* m_model;
    ConsoleVariableItemDelegate* m_itemDelegate;
    CVarBlock* m_varBlock;
    static AZ::ConsoleCommandInvokedEvent::Handler m_commandInvokedHandler;
};

class CConsoleSCB
    : public QWidget
    , private AzToolsFramework::EditorPreferencesNotificationBus::Handler
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    explicit CConsoleSCB(QWidget* parent = nullptr);
    ~CConsoleSCB();

    static void RegisterViewClass();
    void SetInputFocus();
    void AddToConsole(const QString& text, bool bNewLine);
    void FlushText();
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    static CConsoleSCB* GetCreatedInstance();

    static void AddToPendingLines(const QString& text, bool bNewLine);        // call this function instead of AddToConsole() until an instance of CConsoleSCB exists to prevent messages from getting lost

    // EditorPreferencesNotificationBus...
    void OnEditorPreferencesChanged() override;

    void RefreshStyle();

private Q_SLOTS:
    void showVariableEditor();
    void toggleConsoleSearch();
    void findPrevious();
    void findNext();
    void toggleClearOnPlay();

private:
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    void SetupOptionsMenu();
    void UpdateOptionsMenu();

    QScopedPointer<Ui::Console> ui;

    Lines m_lines;
    static Lines s_pendingLines;

    QList<QColor> m_colorTable;
    AzToolsFramework::ConsoleColorTheme m_backgroundTheme;

    class SearchHighlighter;
    SearchHighlighter* m_highlighter;

    QMenu* m_optionsMenu;
    QAction* m_clearOnPlayAction;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCB_H


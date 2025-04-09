/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/UI/UICore/ConsoleTextEdit.hxx>

#include <QMenu>

namespace AzToolsFramework
{
    ConsoleTextEdit::ConsoleTextEdit(QWidget* parent)
        : QPlainTextEdit(parent)
        , m_contextMenu(new QMenu(this))
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QPlainTextEdit::customContextMenuRequested, this, &ConsoleTextEdit::showContextMenu);

        // Make sure to add the actions to this widget, so that the ShortCutDispatcher picks them up properly

        QAction* copyAction = m_contextMenu->addAction(tr("&Copy"));
        copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        copyAction->setShortcut(QKeySequence::Copy);
        copyAction->setEnabled(false);
        connect(copyAction, &QAction::triggered, this, &QPlainTextEdit::copy);
        addAction(copyAction);

        QAction* selectAllAction = m_contextMenu->addAction(tr("Select &All"));
        selectAllAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        selectAllAction->setShortcut(QKeySequence::SelectAll);
        selectAllAction->setEnabled(false);
        connect(selectAllAction, &QAction::triggered, this, &QPlainTextEdit::selectAll);
        addAction(selectAllAction);

        m_contextMenu->addSeparator();

        QAction* deleteAction = m_contextMenu->addAction(tr("Delete"));
        deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        deleteAction->setShortcut(QKeySequence::Delete);
        deleteAction->setEnabled(false);
        connect(
            deleteAction,
            &QAction::triggered,
            this,
            [=]()
            {
                textCursor().removeSelectedText();
            });
        addAction(deleteAction);

        QAction* clearAction = m_contextMenu->addAction(tr("Clear"));
        clearAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        clearAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C);
        clearAction->setEnabled(false);
        connect(clearAction, &QAction::triggered, this, &QPlainTextEdit::clear);
        addAction(clearAction);

        m_searchAction = m_contextMenu->addAction(tr("Find"));
        m_searchAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        m_searchAction->setShortcut(QKeySequence::Find);
        m_searchAction->setEnabled(true);
        connect(m_searchAction, &QAction::triggered, this, &ConsoleTextEdit::searchBarRequested);
        addAction(m_searchAction);

        connect(this, &QPlainTextEdit::copyAvailable, copyAction, &QAction::setEnabled);
        connect(this, &QPlainTextEdit::copyAvailable, deleteAction, &QAction::setEnabled);
        connect(
            this,
            &QPlainTextEdit::textChanged,
            selectAllAction,
            [=]
            {
                if (document() && !document()->isEmpty())
                {
                    clearAction->setEnabled(true);
                    selectAllAction->setEnabled(true);
                }
                else
                {
                    clearAction->setEnabled(false);
                    selectAllAction->setEnabled(false);
                }
            }
        );

        AssignWidgetToActionContextHelper(EditorIdentifiers::EditorConsoleActionContextIdentifier, this);
    }

    ConsoleTextEdit::~ConsoleTextEdit()
    {
        RemoveWidgetFromActionContextHelper(EditorIdentifiers::EditorConsoleActionContextIdentifier, this);
    }

    bool ConsoleTextEdit::searchEnabled()
    {
        AZ_Assert(m_searchAction, "Search action is missing.");
        return m_searchAction->isVisible();
    }

    void ConsoleTextEdit::setSearchEnabled(bool enabled)
    {
        AZ_Assert(m_searchAction, "Search action is missing.");
        return m_searchAction->setVisible(enabled);
    }

    bool ConsoleTextEdit::event(QEvent* theEvent)
    {
        if (theEvent->type() == QEvent::ShortcutOverride)
        {
            // ignore several possible key combinations to prevent them bubbling up to the main editor
            QKeyEvent* shortcutEvent = static_cast<QKeyEvent*>(theEvent);

            QKeySequence::StandardKey ignoredKeys[] = { QKeySequence::Backspace };

            for (auto& currKey : ignoredKeys)
            {
                if (shortcutEvent->matches(currKey))
                {
                    // these shortcuts are ignored. Accept them and do nothing.
                    theEvent->accept();
                    return true;
                }
            }
        }

        return QPlainTextEdit::event(theEvent);
    }

    void ConsoleTextEdit::showContextMenu(const QPoint& pt)
    {
        m_contextMenu->exec(mapToGlobal(pt));
    }
} // namespace AzToolsFramework

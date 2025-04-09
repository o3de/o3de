/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorPlainTextEdit.hxx"
#include "LUAEditorBlockState.h"
#include "LUAEditorStyleMessages.h"
#include <Source/LUA/CodeCompletion/LUACompletionModel.hxx>
#include <Source/LUA/CodeCompletion/LUACompleter.hxx>

#include <Source/LUA/moc_LUAEditorPlainTextEdit.cpp>
#include <Source/LUA/LUAEditorView.hxx>
#include "LUAEditorContextMessages.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QApplication>
#include <QTextDocumentFragment>
#include <QMimeData>
#include <QClipboard>

namespace LUAEditor
{
    LUAEditorPlainTextEdit::LUAEditorPlainTextEdit(QWidget* pParent)
        : QPlainTextEdit(pParent)
    {
        m_completionModel = aznew CompletionModel(this);
        m_completer = aznew Completer(m_completionModel, this);
        m_completer->setWidget(this);

        connect(m_completer, static_cast<void (QCompleter::*)(const QString&)>(&QCompleter::activated), this, &LUAEditorPlainTextEdit::CompletionSelected);
    }

    LUAEditorPlainTextEdit::~LUAEditorPlainTextEdit()
    {
        delete m_completer;
        delete m_completionModel;
    }

    QRectF LUAEditorPlainTextEdit::GetBlockBoundingGeometry(const QTextBlock& block) const
    {
        auto result = blockBoundingGeometry(block);
        result.translate(contentOffset());
        return result;
    }

    void LUAEditorPlainTextEdit::ForEachVisibleBlock(AZStd::function<void(QTextBlock& block, const QRectF&)> operation) const
    {
        for (auto block = document()->begin(); block != document()->end(); block = block.next())
        {
            auto blockRect = GetBlockBoundingGeometry(block);
            if (block.isVisible() && blockRect.bottom() > 0 && blockRect.top() < size().height())
            {
                operation(block, blockRect);
            }
        }
    }

    void LUAEditorPlainTextEdit::mouseDoubleClickEvent(QMouseEvent* event)
    {
        auto& mousePos = event->localPos();

        QTextBlock blockClicked;
        // any block could be hidden, so have to loop through to be sure
        ForEachVisibleBlock(
            [&](const QTextBlock& block, const QRectF& blockRect)
            {
                if (mousePos.y() >= blockRect.top() && mousePos.y() <= blockRect.bottom())
                {
                    blockClicked = block;
                }
            });

        event->ignore();

        if (blockClicked.isValid())
        {
            emit BlockDoubleClicked(event, blockClicked);
        }

        if (!event->isAccepted())
        {
            QPlainTextEdit::mouseDoubleClickEvent(event);
        }
        event->accept();
    }

    void LUAEditorPlainTextEdit::scrollContentsBy(int x, int y) 
    {
        QPlainTextEdit::scrollContentsBy(x, y);

        emit Scrolled();
    }

    void LUAEditorPlainTextEdit::focusInEvent(QFocusEvent* pEvent)
    {
        QPlainTextEdit::focusInEvent(pEvent);
        emit FocusChanged(true);
    }

    void LUAEditorPlainTextEdit::focusOutEvent(QFocusEvent* pEvent)
    {
        QPlainTextEdit::focusOutEvent(pEvent);
        emit FocusChanged(false);
    }

    void LUAEditorPlainTextEdit::paintEvent(QPaintEvent* event)
    {
        QPlainTextEdit::paintEvent(event);

        auto colors = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        QPainter painter(viewport());

        auto metrics = QFontMetrics(font());
        int descent = metrics.descent() - 1;

        auto oldPen = painter.pen();

        auto cursor = textCursor();
        auto currentBlock = document()->findBlock(cursor.position());

        ForEachVisibleBlock([&](QTextBlock& block, const QRectF& bounds)
            {
                QTBlockState state;
                state.m_qtBlockState = block.userState();
                if (state.m_blockState.m_folded)
                {
                    painter.setPen(colors->GetFoldingLineColor());
                    auto iBounds = bounds.toRect();
                    painter.drawLine(iBounds.left(), iBounds.bottom() + descent, iBounds.right(), iBounds.bottom() + descent);
                }
                if (currentBlock.isValid() && currentBlock.blockNumber() == block.blockNumber())
                {
                    painter.setPen(colors->GetCurrentLineOutlineColor());
                    auto iBounds = bounds.toRect();
                    iBounds.setLeft(iBounds.left() + horizontalScrollBar()->value());
                    iBounds.setWidth(viewport()->size().width() - 1);
                    painter.drawRect(iBounds);
                }
            });

        painter.setPen(oldPen);
    }

    void LUAEditorPlainTextEdit::keyPressEvent(QKeyEvent* event)
    {
        if (event == QKeySequence::Cut)
        {
            auto cursor = textCursor();
            if (cursor.hasSelection())
            {
                QPlainTextEdit::keyPressEvent(event);
            }
            else
            {
                auto block = document()->findBlock(cursor.position());
                if (block.isValid())
                {
                    cursor.setPosition(block.position() - 1);
                    cursor.setPosition(block.position() + block.length() - 1, QTextCursor::MoveMode::KeepAnchor);
                    if (!block.text().trimmed().isEmpty())
                    {
                        QApplication::clipboard()->setText(cursor.selectedText());
                    }
                    cursor.removeSelectedText();
                }
            }
        }
        else
        {
            if (m_completer->popup()->isVisible())
            {
                //let completer handle accepting the selected completion

                //completion selected
                if (m_completer->popup()->currentIndex().isValid())
                {
                    switch (event->key())
                    {
                    case Qt::Key_Enter:
                    case Qt::Key_Return:
                    case Qt::Key_Escape:
                    case Qt::Key_Tab:
                    case Qt::Key_Backtab:
                        if (m_completer->currentCompletion() != m_completer->completionPrefix())
                        {
                            event->ignore();
                            return;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else //completion not selected
                {
                    //let completer handle accepting the selected completion
                    switch (event->key())
                    {
                    case Qt::Key_Escape:
                    {
                        event->ignore();
                        return;
                    }
                    break;
                    default:
                        break;
                    }
                }
            }

            if (HandleNewline(event))
            {
                return;
            }

            if (HandleHomeKeyPress(event))
            {
                return;
            }

            if (HandleIndentKeyPress(event))
            {
                return;
            }

            if ( (m_getLUAName && event->key() == Qt::Key::Key_Space) && (event->modifiers() & Qt::KeyboardModifier::ControlModifier) )
            {
                auto bounds = cursorRect();
                bounds.setLeft(bounds.left());
                bounds.setRight(bounds.left() + 250);

                m_completer->setCompletionPrefix(m_getLUAName(textCursor()));
                m_completer->complete(bounds);
            }
            else
            {
                QPlainTextEdit::keyPressEvent(event);

                if (0 == (event->modifiers() & (Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::MetaModifier)))
                {
                    // don't proceed into this block and pop up the completer if a hotkey like CTRL+C / CTRL+V is being used.
                    LUAViewWidget* parentWidget = qobject_cast<LUAViewWidget*>(parent());

                    auto luaName = m_getLUAName(textCursor());
                    if (!luaName.isEmpty() && parentWidget && parentWidget->IsAutoCompletionEnabled())
                    {
                        m_completer->setCompletionPrefix(luaName);
                        if ((event->key() >= Qt::Key::Key_0 && event->key() <= Qt::Key::Key_9) ||
                            (event->key() >= Qt::Key::Key_A && event->key() <= Qt::Key::Key_Z) ||
                            (event->key() == Qt::Key::Key_Period) || (event->key() == Qt::Key::Key_Colon) ||
                            (event->key() == Qt::Key::Key_Backspace) || (event->key() == Qt::Key::Key_Delete) ||
                            m_completer->popup()->isVisible())
                        {
                            if (m_completer->completionCount() == 1 && m_completer->currentCompletion() == m_completer->completionPrefix()) //we have a match already
                            {
                                m_completer->popup()->hide();
                            }
                            else
                            {
                                auto bounds = cursorRect();
                                bounds.setLeft(bounds.left());
                                bounds.setRight(bounds.left() + 250);
                                m_completer->complete(bounds);
                            }
                        }
                    }
                    else
                    {
                        m_completer->popup()->hide();
                    }
                }
            }
        }
    }

    bool LUAEditorPlainTextEdit::HandleNewline(QKeyEvent* event)
    {
        if (isReadOnly())
        {
            return false;
        }

        if (event->key() == Qt::Key::Key_Enter || event->key() == Qt::Key::Key_Return)
        {

            auto cursor = textCursor();
            auto block = document()->findBlock(cursor.position());
            if (block.isValid())
            {
                int cursorStartColumn = cursor.columnNumber();

                cursor.beginEditBlock();

                cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

                block = document()->findBlock(cursor.position());
                if (block.isValid())
                {
                    QString text = block.text();

                    QString leadingSpacing;

                    // Count the number of leading tabs
                    int position = 0;
                    while (position < text.size() && (text.at(position) == '\t' || text.at(position) == ' ')) { leadingSpacing += text.at(position); ++position; }

                    QString left = text.left(cursorStartColumn);
                    QString trail = text.right(text.size() - cursorStartColumn);

                    // Add the newline and as many tabs leading tabs as we counted
                    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();

                    cursor.insertText(left);
                    cursor.insertText("\n");
                    if (leadingSpacing.size() > 0)
                    {
                        cursor.insertText(leadingSpacing);
                    }
                    cursor.insertText(trail);

                    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, position);

                    cursor.endEditBlock();

                    setTextCursor(cursor);

                    return true;
                }

                cursor.endEditBlock();

            }
        }

        return false;
    }

    bool LUAEditorPlainTextEdit::HandleHomeKeyPress(QKeyEvent* event)
    {
        if (event->key() == Qt::Key::Key_Home)
        {
            auto cursor = textCursor();
            auto block = document()->findBlock(cursor.position());
            if (block.isValid())
            {
                // Ctrl+Home goes to start of document.
                if (event->modifiers() & Qt::KeyboardModifier::ControlModifier)
                {
                    if (event->modifiers() & Qt::KeyboardModifier::ShiftModifier)
                    {
                        int position = cursor.position();
                        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
                        cursor.setPosition(position, QTextCursor::KeepAnchor);
                    }
                    else
                    {
                        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
                    }

                    setTextCursor(cursor);
                    return true;
                }

                if (cursor.columnNumber() > 0)
                {
                    QString text = block.text();
                    QStringRef line = QStringRef(&text, 0, cursor.columnNumber());

                    // Count the number of leading whitespace characters
                    int offset = 0;
                    while (offset < line.size() && (line.at(offset) == ' ' || line.at(offset) == '\t')) { ++offset; }

                    int column = cursor.columnNumber();

                    QTextCursor::MoveMode moveMode = QTextCursor::MoveMode::MoveAnchor;
                    if (event->modifiers() & Qt::KeyboardModifier::ShiftModifier)
                    {
                        moveMode = QTextCursor::MoveMode::KeepAnchor;
                    }

                    cursor.movePosition(QTextCursor::StartOfLine, moveMode);

                    // If the cursor was originally beyond the first white space, stop it at the first non-whitespace character.
                    if (column > offset)
                    {
                        cursor.movePosition(QTextCursor::Right, moveMode, offset);
                    }
                }
                else
                {
                    QString text = block.text();
                    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                    int position = cursor.position();

                    QStringRef line = QStringRef(&text, 0, position);

                    int offset = 0;
                    while (offset < line.size() && (line.at(offset) == ' ' || line.at(offset) == '\t')) { ++offset; }

                    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveMode::MoveAnchor, offset);
                }

                setTextCursor(cursor);

                return true;

            }
        }

        return false;
    }

    bool LUAEditorPlainTextEdit::HandleIndentKeyPress(QKeyEvent* event)
    {
        if (event->key() == Qt::Key::Key_Tab || event->key() == Qt::Key::Key_Backtab)
        {
            bool addIndent = (event->key() == Qt::Key::Key_Tab);
            QString tabString = m_useSpaces ? QString(" ").repeated(m_tabSize) : "\t";

            auto cursor = textCursor();
            if (cursor.hasSelection())
            {
                int anchor = cursor.anchor();
                int position = cursor.position();

                if (anchor > position)
                {
                    std::swap(anchor, position);
                }

                cursor.setPosition(anchor);
                cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

                anchor = cursor.position();

                // save a new anchor at the beginning of the line of the selected text
                cursor.setPosition(anchor);
                cursor.setPosition(position, QTextCursor::KeepAnchor);

                // set a new selection with the new beginning
                QString text = cursor.selection().toPlainText();

                if (m_useSpaces)
                {
                    // Replace tabs with spaces and adjust position
                    int tabs = text.count("\t");
                    text.replace("\t", tabString);
                    position += tabs * (m_tabSize - 1);
                }

                QStringList list = text.split("\n");

                // get the selected text and split into lines
                for (int i = 0; i < list.count(); i++)
                {
                    QString& line = list[i];

                    if (addIndent)
                    {
                        line.insert(0, tabString);
                        position += tabString.length();
                    }
                    else
                    {
                        if (line.startsWith(tabString))
                        {
                            line.remove(0, tabString.length());
                            position -= tabString.length();
                        }
                    }
                }

                text = list.join("\n");

                cursor.beginEditBlock();
                {
                    cursor.removeSelectedText();
                    cursor.insertText(text);
                }
                cursor.endEditBlock();

                // put the new text back
                cursor.setPosition(anchor);
                cursor.setPosition(position, QTextCursor::KeepAnchor);

                // reselect the text for more indents
                setTextCursor(cursor);

                return true;
            }
            else
            {
                if (addIndent && !isReadOnly())
                {
                    cursor.insertText(tabString);
                    return true;
                }
                else if (event->key() == Qt::Key::Key_Backtab)
                {
                    int position = cursor.position();
                    int columnNumber = cursor.columnNumber();
                    int removeCount = 1;

                    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

                    QString text = cursor.block().text();

                    if (text[columnNumber] != '\t' || text.midRef(columnNumber, m_tabSize) != tabString)
                    {
                        if (columnNumber > 0 && text[columnNumber - 1] == '\t')
                        {
                            --columnNumber;
                            --position;
                        }
                        else if (columnNumber > 0 && text.midRef(columnNumber - m_tabSize, m_tabSize) == tabString)
                        {
                            columnNumber -= tabString.length();
                            position -= tabString.length();
                            removeCount = tabString.length();
                        }
                        else
                        {
                            // No tab to remove, but consider it handled to avoid adding a tab
                            return true;
                        }
                    }

                    // Remove the tab
                    text.remove(columnNumber, removeCount);

                    // Select the entire line, we'll replace it with the modified text
                    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

                    // Replace the line with the new text
                    cursor.beginEditBlock();
                    {
                        cursor.removeSelectedText();
                        cursor.insertText(text);
                    }
                    cursor.endEditBlock();

                    // Restore the position to where it was
                    cursor.setPosition(position);

                    setTextCursor(cursor);

                    return true;
                }
            }
        }

        return false;
    }

    void LUAEditorPlainTextEdit::wheelEvent(QWheelEvent* event)
    {
        if (event->modifiers() & Qt::ControlModifier)
        {
            const int delta = event->angleDelta().y();
            if (delta < 0)
            {
                ZoomOut();
            }
            else if (delta > 0)
            {
                ZoomIn();
            }

            return;
        }

        QPlainTextEdit::wheelEvent(event);
    }

    void LUAEditorPlainTextEdit::UpdateFont(QFont font, int tabSize)
    {
        QTextCursor currentCursor = textCursor();
        QFontMetrics metrics(font);
        QTextCharFormat characterFormat;
        characterFormat.setFont(font);
        selectAll();
        setCurrentCharFormat(characterFormat);
        setTabStopDistance(metrics.horizontalAdvance(' ') * tabSize);
        setTextCursor(currentCursor);
    }

    void LUAEditorPlainTextEdit::CompletionSelected(const QString& text)
    {
        QString prefix = m_completer->completionPrefix();

        QTextCursor cursor = textCursor();

        // lastIndexOf will return -1 if not found, so subtracting index of lastPeriod + 1, 
        // the math will always work regardless of whether the prefix contains a . or not
        int lastPeriod = prefix.lastIndexOf('.');
        int charactersToReplace = prefix.length() - (lastPeriod + 1);

        cursor.setPosition(cursor.position() - charactersToReplace, QTextCursor::KeepAnchor);
        cursor.insertText(text);
    }

    void LUAEditorPlainTextEdit::OnScopeNamesUpdated(const QStringList& scopeNames)
    {
        m_completionModel->OnScopeNamesUpdated(scopeNames);
    }

    void LUAEditorPlainTextEdit::dropEvent(QDropEvent* e)
    {
        if (e->mimeData()->hasUrls())
        {
            QList<QUrl> urls = e->mimeData()->urls();
            for (int idx = 0; idx < urls.count(); ++idx)
            {
                QString path = urls[idx].toLocalFile();
                AZ_TracePrintf("Debug", "URL: %s\n", path.toUtf8().data());

                AZStd::string assetId(path.toUtf8().data());
                Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnLoadDocument, assetId, true);
            }
        }
        else
        {
            QPlainTextEdit::dropEvent(e);
        }
    }
}

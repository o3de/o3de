/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorFindResults.hxx"
#include "LUAEditorStyleMessages.h"
#include "LUAEditorBlockState.h"

#include <Source/LUA/moc_LUAEditorFindResults.cpp>

#include <Source/LUA/ui_LUAEditorFindResults.h>

namespace LUAEditor
{
    void FindResultsHighlighter::SetSearchString(const QString& searchString, bool regEx, bool wholeWord, bool caseSensitive)
    {
        m_searchString = searchString;
        m_regEx = regEx;
        m_wholeWord = wholeWord;
        m_caseSensitive = caseSensitive;
    }

    void FindResultsHighlighter::highlightBlock(const QString& text)
    {
        auto colors = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        auto block = currentBlock();
        QTBlockState blockState;
        blockState.m_qtBlockState = block.userState();

        QTextCharFormat textFormat;
        textFormat.setFont(m_font);
        if (!blockState.m_blockState.m_uninitialized)
        {
            if (blockState.m_blockState.m_syntaxHighlighterState == 0)
            {
                textFormat.setForeground(colors->GetFindResultsHeaderColor());
                setFormat(0, block.length(), textFormat);
            }
            else if (blockState.m_blockState.m_syntaxHighlighterState == 1)
            {
                textFormat.setForeground(colors->GetFindResultsFileColor());
                setFormat(0, block.length(), textFormat);
            }
            else
            {
                textFormat.setForeground(colors->GetTextColor());
                setFormat(0, block.length(), textFormat);

                textFormat.setForeground(colors->GetFindResultsMatchColor());
                QRegExp regex(m_searchString, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                int index = 0;
                if (m_regEx || m_wholeWord)
                {
                    index = text.indexOf(regex, index);
                }
                else
                {
                    index = text.indexOf(m_searchString, index, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                }
                while (index > 1)
                {
                    if (m_regEx || m_wholeWord)
                    {
                        setFormat(index, regex.matchedLength(), textFormat);
                    }
                    else
                    {
                        setFormat(index, m_searchString.length(), textFormat);
                    }

                    ++index;
                    if (m_regEx || m_wholeWord)
                    {
                        index = text.indexOf(regex, index);
                    }
                    else
                    {
                        index = text.indexOf(m_searchString, index, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                    }
                }
            }
        }
    }

    FindResults::FindResults(QWidget* parent)
        : QWidget(parent)
    {
        m_gui = azcreate(Ui::LUAEditorFindResults, ());
        m_gui->setupUi(this);

        m_gui->m_foldingWidget->setEnabled(true);
        m_gui->m_foldingWidget->SetTextEdit(m_gui->m_resultsList);
        connect(m_gui->m_resultsList, &LUAEditor::LUAEditorPlainTextEdit::BlockDoubleClicked, this, &FindResults::OnBlockDoubleClicked);
        connect(m_gui->m_resultsList, &LUAEditor::LUAEditorPlainTextEdit::cursorPositionChanged, this, [&]() { m_gui->m_foldingWidget->update(); });
        connect(m_gui->m_resultsList, &LUAEditor::LUAEditorPlainTextEdit::Scrolled, this, [&]() { m_gui->m_foldingWidget->update(); });
        connect(m_gui->m_foldingWidget, &FoldingWidget::TextBlockFoldingChanged, this, [&]() {m_gui->m_resultsList->update(); });
        connect(m_gui->m_resultsList->document(), &QTextDocument::contentsChange, m_gui->m_foldingWidget, &FoldingWidget::OnContentChanged);

        auto settings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);
        m_gui->m_resultsList->setFont(settings->GetFont());
        m_gui->m_foldingWidget->SetFont(settings->GetFont());

        QString styleSheet = QString(R"(
            QPlainTextEdit[readOnly="true"]:focus
            {
                background-color: %1;
                selection-color: %3;
                selection-background-color: %4;
            }

            QPlainTextEdit[readOnly="true"]:!focus
            {
                background-color: %2;
                selection-color: %3;
                selection-background-color: %4;
            }
        )");
        styleSheet = styleSheet.arg(settings->GetTextReadOnlyFocusedBackgroundColor().name())
                               .arg(settings->GetTextReadOnlyUnfocusedBackgroundColor().name())
                               .arg(settings->GetTextSelectedColor().name())
                               .arg(settings->GetTextSelectedBackgroundColor().name());

        m_gui->m_resultsList->setStyleSheet(styleSheet);
        m_resultLineHighlightColor = settings->GetFindResultsLineHighlightColor();

        m_highlighter = aznew FindResultsHighlighter(m_gui->m_resultsList->document());
    }

    FindResults::~FindResults()
    {
        delete m_highlighter;
        azdestroy(m_gui);
    }

    void FindResults::OnBlockDoubleClicked(QMouseEvent* event, const QTextBlock& block)
    {
        if (block.isValid())
        {
            auto userData = block.userData();
            if (userData)
            {
                auto blockInfo = static_cast<FindResultsBlockInfo*>(userData);

                // highlight the line
                QTextEdit::ExtraSelection selection;
                selection.format.setBackground(m_resultLineHighlightColor);
                selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                selection.cursor = m_gui->m_resultsList->textCursor();
                selection.cursor.clearSelection();
                QList<QTextEdit::ExtraSelection> extraSelections;
                extraSelections.append(selection);
                m_gui->m_resultsList->setExtraSelections(extraSelections);

                emit ResultSelected(*blockInfo);
                event->accept();
            }
        }
    }

    void FindResults::AssignAssetId(const AZStd::string& assetName, const AZStd::string& assetId)
    {
        for (auto block = m_gui->m_resultsList->document()->begin(); block != m_gui->m_resultsList->document()->end(); block = block.next())
        {
            auto userData = block.userData();
            if (userData)
            {
                auto blockInfo = static_cast<FindResultsBlockInfo*>(userData);
                if (blockInfo->m_assetName == assetName)
                {
                    blockInfo->m_assetId = assetId;
                }
            }
        }
    }

    void FindResults::FinishedAddingText(const QString& searchString, bool regEx, bool wholeWord, bool caseSensitive)
    {
        m_highlighter->SetSearchString(searchString, regEx, wholeWord, caseSensitive);
        m_highlighter->rehighlight();
    }

    void FindResults::Clear()
    {
        m_gui->m_resultsList->clear();
    }

    QTextDocument* FindResults::Document()
    {
        return m_gui->m_resultsList->document();
    }

    void FindResults::AppendPlainText(const QString& text)
    {
        m_gui->m_resultsList->appendPlainText(text);
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorFindResults.hxx"
#include "LUAEditorPlainTextEdit.hxx"
#include "LUAEditorStyleMessages.h"
#include "LUAEditorBlockState.h"

#include <AzQtComponents/Components/StyleManager.h>

#include <Source/LUA/ui_LUAEditorFindResults.h>

#include <QColor>
#include <QEvent>
#include <QObject>
#include <QPalette>
#include <QRegularExpression>

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
                if (m_regEx || m_wholeWord)
                {
                    QRegularExpression regex(
                        m_searchString, m_caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch match = regex.match(text);
                    int index = static_cast<int>(match.capturedStart());
                    while (match.hasMatch())
                    {
                        const int length = static_cast<int>(match.capturedLength());
                        setFormat(index, length, textFormat);

                        match = regex.match(text, index + length);
                        index = static_cast<int>(match.capturedStart());
                    }
                }
                else
                {
                    int index = static_cast<int>(text.indexOf(m_searchString, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive));
                    while (index > 1)
                    {
                        setFormat(index, static_cast<int>(m_searchString.length()), textFormat);
                        ++index;
                        index = static_cast<int>(text.indexOf(m_searchString, index, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive));
                    }
                }
 
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Flattened GUI: the colours of the results list
    //
    // The legacy GUI gives the list its colours with a sheet of two rules (the constructor below sets it), built from four colours of
    // the syntax style settings, which are user data. The flattened GUI sets no sheet: the list's palette carries the same four
    // colours, in the roles the sheet's declarations set (QRenderRule::configurePalette). Style gives every text edit the field's own
    // palette when it polishes the widget (Style.cpp, applyFlattenedTextEntryPalette), over whatever the code set before, so the
    // colours are set again whenever the palette changes.
    //
    // What the application sheet gave the list besides (the text colour of BaseStyleSheet.qss:91 in the roles Style's text-entry
    // palette leaves out: ButtonText, WindowText and PlaceholderText; a list that does not fill its own background; a margin of 0;
    // the hover and focus borders of BaseStyleSheet.qss:97-115) is not this sheet's: it stays with the flattened Style, which
    // draws every text edit the same way.
    // ----------------------------------------------------------------------------------

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation
    //! unit, where anonymous namespaces merge.
    namespace FindResultsFlatInternal
    {
        //! The four colours the sheet takes, as the sheet carried them: `#rrggbb`, 8 bits a channel
        struct ResultsListColors
        {
            QColor m_focusedBackground;     //!< `:focus { background-color }`, the sheet's %1 (SyntaxStyleSettings, read-only focused)
            QColor m_unfocusedBackground;   //!< `:!focus { background-color }`, %2 (read-only unfocused)
            QColor m_selectedText;          //!< `selection-color` of both rules, %3
            QColor m_selectedBackground;    //!< `selection-background-color` of both rules, %4
        };

        //! The colour as the sheet's text carried it (QColor::name)
        QColor asSheetColor(const QColor& color)
        {
            return QColor(color.rgb());
        }

        //! Keeps the colours of a results list on its palette, in every colour group (the rule of `:!focus` reaches the disabled
        //! group too). `background-color` sets Base, Button and Window; `selection-color` and `selection-background-color` set
        //! HighlightedText and Highlight. The background follows the focus as the sheet's `:focus` and `:!focus` rules do: the sheet
        //! style paints each in its own state, so the legacy list changes colour when it gains or loses the focus.
        class ResultsListPalette
            : public QObject
        {
        public:
            ResultsListPalette(LUAEditorPlainTextEdit* list, const ResultsListColors& colors)
                : QObject(list)
                , m_list(list)
                , m_colors(colors)
            {
                list->installEventFilter(this);
                QObject::connect(list, &LUAEditorPlainTextEdit::FocusChanged, this,
                    [this](bool focused)
                    {
                        m_focused = focused;
                        apply();
                    });
                apply();
            }

            //! Whenever the palette changes (the style writes its own at the first polish), the colours are put over it again
            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (watched == m_list && event->type() == QEvent::PaletteChange)
                {
                    apply();
                }
                return false;
            }

        private:
            void apply()
            {
                const QColor& background = m_focused ? m_colors.m_focusedBackground : m_colors.m_unfocusedBackground;
                QPalette palette = m_list->palette();
                for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
                {
                    palette.setColor(group, QPalette::Base, background);
                    palette.setColor(group, QPalette::Button, background);
                    palette.setColor(group, QPalette::Window, background);
                    palette.setColor(group, QPalette::Highlight, m_colors.m_selectedBackground);
                    palette.setColor(group, QPalette::HighlightedText, m_colors.m_selectedText);
                }
                // Equal colours set nothing, so a palette change that these colours made does not come back here
                if (palette != m_list->palette())
                {
                    m_list->setPalette(palette);
                }
            }

            LUAEditorPlainTextEdit* m_list;
            ResultsListColors m_colors;
            bool m_focused = false;
        };
    } // namespace FindResultsFlatInternal

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

        // The legacy GUI sets the sheet. The flattened GUI sets none: the list's palette carries the same four colours. Not at the
        // passthrough rung, where Style hands every call to its base style and a widget leaves out what it sets on itself.
        AzQtComponents::StyleManager::setLegacyStyleSheet(m_gui->m_resultsList, styleSheet);
        if (AzQtComponents::StyleManager::flatStylingActive())
        {
            using FindResultsFlatInternal::asSheetColor;
            new FindResultsFlatInternal::ResultsListPalette(m_gui->m_resultsList,
                FindResultsFlatInternal::ResultsListColors{
                    .m_focusedBackground = asSheetColor(settings->GetTextReadOnlyFocusedBackgroundColor()),
                    .m_unfocusedBackground = asSheetColor(settings->GetTextReadOnlyUnfocusedBackgroundColor()),
                    .m_selectedText = asSheetColor(settings->GetTextSelectedColor()),
                    .m_selectedBackground = asSheetColor(settings->GetTextSelectedBackgroundColor()),
                });
        }
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

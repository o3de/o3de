/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RichTextHighlighter.h"

namespace AzToolsFramework
{
    // =======================================================================
    // Rich Text Colours
    // =======================================================================

    //! The colours of the highlighted rich text: the background of a match, and the text colour of the document by the item state,
    //! written into its HTML and CSS text. A named namespace, not an anonymous one: a unity build joins files into one translation unit,
    //! where anonymous namespaces merge.
    namespace RichTextHighlighterFlatInternal
    {
        constexpr QColor MatchBackground(0x70, 0x70, 0x70);   // RichTextHighlighter.cpp:23. TOKENIZE: RichTextMatchBackground
        const QColor EnabledText(Qt::white);                  // RichTextHighlighter.cpp:41. TOKENIZE: RichTextEnabledText
        constexpr QColor DisabledText(0x7C, 0x7C, 0x7C);      // RichTextHighlighter.cpp:45. TOKENIZE: RichTextDisabledText
    }

    QString RichTextHighlighter::HighlightText(const QString& displayString, const QString& matchingSubstring)
    {
        QString highlightedString = displayString;
        int highlightTextIndex = 0;
        do
        {
            highlightTextIndex = static_cast<int>(highlightedString.lastIndexOf(matchingSubstring, highlightTextIndex - 1, Qt::CaseInsensitive));
            if (highlightTextIndex >= 0)
            {
                const QString backgroundColor{ RichTextHighlighterFlatInternal::MatchBackground.name() };
                highlightedString.insert(static_cast<int>(highlightTextIndex + matchingSubstring.length()), "</span>");
                highlightedString.insert(highlightTextIndex, "<span style=\"background-color: " + backgroundColor + "\">");
            }
        } while (highlightTextIndex > 0);

        return highlightedString;
    }

    void RichTextHighlighter::PaintHighlightedRichText(const QString& highlightedString,QPainter* painter, QStyleOptionViewItem option, QRect availableRect, QPoint offset /* = QPoint()*/)
    {
        // Now we setup a Text Document so it can draw the rich text
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QTextDocument textDoc;
        textDoc.setDefaultFont(option.font);
        if (option.state & QStyle::State_Enabled)
        {
            textDoc.setDefaultStyleSheet(QString("body {color: %1}").arg(RichTextHighlighterFlatInternal::EnabledText.name()));
        }
        else
        {
            textDoc.setDefaultStyleSheet(QString("body {color: %1}").arg(RichTextHighlighterFlatInternal::DisabledText.name().toUpper()));
        }
        textDoc.setHtml("<body>" + highlightedString + "</body>");
        painter->translate(availableRect.topLeft() + offset);
        textDoc.setTextWidth(availableRect.width());
        textDoc.drawContents(painter, QRectF(0, 0, availableRect.width(), availableRect.height()));
        painter->restore();
    }
} // namespace AzToolsFramework

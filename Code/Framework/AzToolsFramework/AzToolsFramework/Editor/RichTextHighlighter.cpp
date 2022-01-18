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

    QString RichTextHighlighter::HighlightText(const QString& displayString, const QString& matchingSubstring)
    {
        QString highlightedString = displayString;
        int highlightTextIndex = 0;
        do
        {
            highlightTextIndex = highlightedString.lastIndexOf(matchingSubstring, highlightTextIndex - 1, Qt::CaseInsensitive);
            if (highlightTextIndex >= 0)
            {
                const QString backgroundColor{ "#707070" };
                highlightedString.insert(static_cast<int>(highlightTextIndex + matchingSubstring.length()), "</span>");
                highlightedString.insert(highlightTextIndex, "<span style=\"background-color: " + backgroundColor + "\">");
            }
        } while (highlightTextIndex > 0);

        return highlightedString;
    }

    void RichTextHighlighter::PaintHighlightedRichText(const QString& highlightedString,QPainter* painter, QStyleOptionViewItem option, QRect availableRect)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // Now we setup a Text Document so it can draw the rich text
        QTextDocument textDoc;
        textDoc.setDefaultFont(option.font);
        if (option.state & QStyle::State_Enabled)
        {
            textDoc.setDefaultStyleSheet("body {color: white}");
        }
        else
        {
            textDoc.setDefaultStyleSheet("body {color: #7C7C7C}");
        }
        textDoc.setHtml("<body>" + highlightedString + "</body>");
        painter->translate(availableRect.topLeft());
        textDoc.setTextWidth(availableRect.width());
        textDoc.drawContents(painter, QRectF(0, 0, availableRect.width(), availableRect.height()));

        painter->restore();
    }
} // namespace AzToolsFramework

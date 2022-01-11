#include "RichTextHighlighter.h"

namespace AzToolsFramework
{
    RichTextHighlighter::RichTextHighlighter(QString displayString)
        : m_displayString(displayString){};

     QString RichTextHighlighter::HighlightText(const QString& displayString, const QString& matchingSubstring)
    {
        m_displayString = displayString;
        int highlightTextIndex = 0;
        do
        {
            highlightTextIndex = m_displayString.lastIndexOf(matchingSubstring, highlightTextIndex - 1, Qt::CaseInsensitive);
            if (highlightTextIndex >= 0)
            {
                const QString BACKGROUND_COLOR{ "#707070" };
                m_displayString.insert(static_cast<int>(highlightTextIndex + matchingSubstring.length()), "</span>");
                m_displayString.insert(highlightTextIndex, "<span style=\"background-color: " + BACKGROUND_COLOR + "\">");
            }
        } while (highlightTextIndex > 0);

        return m_displayString;
    }
    void RichTextHighlighter::PaintHighlightedRichText(QPainter* painter, QStyleOptionViewItem option, QRect availableRect) const
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        const QRect textRect = option.widget->style()->proxy()->subElementRect(QStyle::SE_ItemViewItemText, &option);

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
        textDoc.setHtml("<body>" + m_displayString + "</body>");
        painter->translate(availableRect.topLeft());
        textDoc.setTextWidth(availableRect.width());
        textDoc.drawContents(painter, QRectF(0, 0, availableRect.width(), availableRect.height()));

        painter->restore();
    }
}

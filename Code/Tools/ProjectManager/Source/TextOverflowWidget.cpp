/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TextOverflowWidget.h>
#include <ExternalLinkDialog.h>
#include <SettingsInterface.h>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QRegularExpression>
#include <QDesktopServices>

namespace O3DE::ProjectManager
{
    static QString overflowLink = "OverflowLink";
    static int maxTextLength = 100;

    // Elides text based on character length with html link tags removed
    QString ElideLinkedText(const QString& text, int maxLength)
    {
        QRegularExpression linksRegex("</?a(|\\s+[^>]+)>");
        QRegularExpressionMatchIterator matches = linksRegex.globalMatch(text);
        int displayLength = 0;
        int truncateEnd = 0;
        int matchIndex = 0;

        QRegularExpressionMatch match;

        if (matches.hasNext())
        {
            int lastMatchLength = 0;
            while (matches.hasNext())
            {
                match = matches.next();
                int matchStartIndex = match.capturedStart();
                int matchLength = match.capturedLength();

                // If at the start of a new link tag mark this index
                // to truncate from here in-case the link text is too long.
                displayLength += matchStartIndex - truncateEnd - lastMatchLength;
                truncateEnd = matchStartIndex;

                // Exit because we've run out of displayed characters and truncate to last avaliable character
                if (displayLength >= maxLength)
                {
                    truncateEnd -= displayLength - maxLength;
                    break;
                }

                lastMatchLength = matchLength;
                ++matchIndex;
            }

            // If there is still room left after we process all matches then
            // offset by the last match length and the remaining display characters
            if (displayLength < maxLength)
            {
                truncateEnd += lastMatchLength + maxLength - displayLength;
                displayLength = maxLength;
            }
        }
        else
        {
            displayLength = text.length();

            if (displayLength > maxLength)
            {
                truncateEnd = maxLength;
            }
        }

        // Truncate and add link to full texts
        if (displayLength >= maxLength)
        {
            // Append closing tag if link text got truncated %2
            return QString("%1%2 <a href=\"%3\">Read More...</a>")
                .arg(text.leftRef(truncateEnd), match.isValid() && matchIndex % 2 == 1 ? match.captured() : "", overflowLink);
        }

        return text;
    }

    TextOverflowDialog::TextOverflowDialog(const QString& title, const QString& text, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(title);
        setModal(true);
        setObjectName("textOverflowDialog");
        setMinimumSize(600, 700);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(5, 5, 5, 5);
        vLayout->setSpacing(0);
        vLayout->setAlignment(Qt::AlignTop);
        setLayout(vLayout);

        QScrollArea* scrollArea = new QScrollArea(this);
        vLayout->addWidget(scrollArea);

        // Set scroll bar policy
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        QWidget* textArea = new QWidget();
        textArea->setContentsMargins(10, 10, 10, 10);

        scrollArea->setWidget(textArea);

        QVBoxLayout* scrollLayout = new QVBoxLayout;
        scrollLayout->setSizeConstraint(QLayout::SetFixedSize);
        textArea->setLayout(scrollLayout);

        QLabel* overflowText = new QLabel(text);
        overflowText->setObjectName("overflowTextDialogLabel");
        overflowText->setWordWrap(true);
        overflowText->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        overflowText->setOpenExternalLinks(true);
        overflowText->setAlignment(Qt::AlignLeft);
        scrollLayout->addWidget(overflowText);
    }

    TextOverflowLabel::TextOverflowLabel(const QString& title, const QString& text, QWidget* parent)
        : QLabel(text, parent)
        , m_title(title)
        , m_fullText(text)
    {
        setWordWrap(true);
        setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        setAlignment(Qt::AlignLeft);

        // Truncate text and display overflow dialog if it is too long
        if (text.length() > maxTextLength)
        {
            setText(ElideLinkedText(text, maxTextLength));
        }

        connect(this, &QLabel::linkActivated, this, &TextOverflowLabel::OnLinkActivated);
    }

    void TextOverflowLabel::OnLinkActivated(const QString& link)
    {
        if (link != overflowLink)
        {
            QDesktopServices::openUrl(QUrl(link));
        }
        else
        {
            TextOverflowDialog* overflowDialog = new TextOverflowDialog(m_title, m_fullText, this);
            overflowDialog->open();
        }
    }
} // namespace O3DE::ProjectManager

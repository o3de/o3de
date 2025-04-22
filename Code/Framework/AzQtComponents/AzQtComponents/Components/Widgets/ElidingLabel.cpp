/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/algorithm.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QTextCursor>
#include <QTextDocument>

namespace AzQtComponents
{
    ElidingLabel::ElidingLabel(const QString& text, QWidget* parent /* = nullptr */)
        : QLabel(parent)
        , m_elideMode(Qt::ElideRight)
        , m_metricsLabel(new QLabel(this))
    {
        // Used to return the default sizeHint for a the un-elided text.
        // Should not be displayed.
        m_metricsLabel->hide();
        setText(text);
    }

    void ElidingLabel::handleElision()
    {
        m_elidedText.clear();

        elide();

        if (m_updateGeomentry)
        {
            updateGeometry();
            m_updateGeomentry = false;
        }
    }

    void ElidingLabel::setText(const QString& text)
    {
        if (text == m_text)
        {
            return;
        }

        m_text = text;
        m_metricsLabel->setText(m_text);

        requestElide(true);
    }

    void ElidingLabel::setDescription(const QString& description)
    {
        m_description = description;
    }

    void ElidingLabel::setElideMode(Qt::TextElideMode mode)
    {
        if (m_elideMode == mode)
        {
            return;
        }

        m_elideMode = mode;
        update();
    }

    void ElidingLabel::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        requestElide(false);
    }

    void ElidingLabel::elide()
    {
        ensurePolished();

        if (Qt::mightBeRichText(m_text))
        {
            // If RichText tags are elided using fontMetrics.elidedText(), they will break.
            // A TextDocument is used to produce elided text that takes this into account.
            static const QString ellipsis(QStringLiteral("..."));
            const int maxLineWidth = TextRect().width();

            static QTextDocument doc;
            doc.setHtml(m_text);
            doc.setDefaultFont(font());
            doc.setDocumentMargin(0.0);

            // Turn off wrapping so the document uses a single line.
            static QTextOption option = doc.defaultTextOption();
            option.setWrapMode(QTextOption::WrapMode::NoWrap);
            doc.setDefaultTextOption(option);
            doc.adjustSize();

            if (doc.size().width() <= maxLineWidth)
            {
                m_elidedText = m_text;
            }
            else
            {
                QTextCursor textCursor(&doc);
                textCursor.movePosition(QTextCursor::End);

                int ellipsisWidth = 0;

                // At the moment only ElideRight and ElideNone are ever used. This will need expanding if other elision modes are used.
                if (m_elideMode == Qt::ElideRight)
                {
                    ellipsisWidth = fontMetrics().horizontalAdvance(ellipsis);
                }

                // Move the cursor back until the text fits or the start of the text is reached.
                while (doc.size().width() + ellipsisWidth > maxLineWidth && !textCursor.atStart())
                {
                    textCursor.deletePreviousChar();
                    doc.adjustSize();
                }

                if (m_elideMode == Qt::ElideRight)
                {
                    textCursor.insertText(ellipsis);
                }

                m_elidedText = doc.toHtml();
            }
        }
        else
        {
            m_elidedText = fontMetrics().elidedText(m_text, m_elideMode, TextRect().width());
        }

        QLabel::setText(m_elidedText);

        if (m_elidedText != m_text)
        {
            if (m_description.isEmpty())
            {
                setToolTip(m_text);
            }
            else
            {
                setToolTip(QStringLiteral("%1\n%2").arg(m_text, m_description));
            }
        }
        else
        {
            setToolTip(m_description);
        }
    }

    QRect ElidingLabel::TextRect()
    {
        QRect textRect = contentsRect();

        // Account for margins when determining how much space we actually have for our text
        if (indent() == -1)
        {
            if (frameWidth())
            {
                int textMargin = fontMetrics().horizontalAdvance('x') / 2;
                textRect.adjust(textMargin, 0, -textMargin, 0);
            }
        }

        return textRect;
    }

    void ElidingLabel::refreshStyle()
    {
        style()->unpolish(this);
        style()->polish(this);
        update();
    }

    void ElidingLabel::setObjectName(const QString& name)
    {
        m_metricsLabel->setObjectName(name);
        QLabel::setObjectName(name);
    }

    QSize ElidingLabel::minimumSizeHint() const
    {
        // Override the QLabel minimumSizeHint width to let other
        // widgets know we can deal with less space.
        return QWidget::minimumSizeHint().boundedTo({ 0, std::numeric_limits<int>::max() });
    }

    QSize ElidingLabel::sizeHint() const
    {
        QSize sh = m_metricsLabel->sizeHint();

        const QMargins margins = contentsMargins();
        sh.rheight() += margins.top() + margins.bottom();
        sh.rwidth() += margins.left() + margins.right();

        return sh;
    }

    void ElidingLabel::setFilter(const QString& filter)
    {
        m_filterString = filter;
        m_filterRegex = QRegExp(m_filterString, Qt::CaseInsensitive);
    }

    bool ElidingLabel::TextMatchesFilter() const
    {
        return m_filterString.isEmpty() || m_text.contains(m_filterRegex);
    }

    void ElidingLabel::paintEvent(QPaintEvent* event)
    {
        if (m_filterString.isEmpty())
        {
            QLabel::paintEvent(event);
            return;
        }
        QPainter painter(this);
        QRect textRect = TextRect();

        int regexIndex = -1;

        painter.save();

        while ((regexIndex = text().indexOf(m_filterRegex, regexIndex + 1)) >= 0)
        {
            QString preSelectedText = text().left(regexIndex);
            int preSelectedTextLength = fontMetrics().horizontalAdvance(preSelectedText);
            QString selectedText = text().mid(regexIndex, m_filterString.length());
            int selectedTextLength = fontMetrics().horizontalAdvance(selectedText);

            int leftSpot = textRect.left() + preSelectedTextLength;

            // Only need to do the draw if we actually are going to be highlighting the text.
            if (leftSpot < textRect.right())
            {
                int visibleLength = AZStd::GetMin(selectedTextLength, textRect.right() - leftSpot);
                QRect highlightRect(textRect.left() + preSelectedTextLength, textRect.top(), visibleLength, textRect.height());

                // paint the highlight rect
                painter.fillRect(highlightRect, backgroundColor);
            }
        }
        painter.restore();

        QLabel::paintEvent(event);
    }

    void ElidingLabel::timerEvent([[maybe_unused]] QTimerEvent* event)
    {
        if (m_elideDeferred)
        {
            // do the elision, but keep the timer running in case another request comes in too quickly
            handleElision();
            m_elideDeferred = false;
        }
        else
        {
            // s_minTimeBetweenUpdates has elapsed since the last elide, and no new requests have come in
            killTimer(m_elideTimerId);
            m_elideTimerId = 0;
        }
    }

    void ElidingLabel::requestElide(bool updateGeometry)
    {
        if (!m_updateGeomentry && updateGeometry)
        {
            m_updateGeomentry = true;
        }
        if (!m_elideTimerId)
        {
            // do the elision immediately, but start a timer to make sure we don't elide again too quickly
            handleElision();
            m_elideTimerId = startTimer(s_minTimeBetweenUpdates);
        }
        else
        {
            // the timer's already running
            m_elideDeferred = true;
        }
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_ElidingLabel.cpp"

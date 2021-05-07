/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "GemItemDelegate.h"
#include "GemModel.h"
#include <QEvent>
#include <QPainter>
#include <QMouseEvent>

namespace O3DE::ProjectManager
{
    GemItemDelegate::GemItemDelegate(GemModel* gemModel, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_gemModel(gemModel)
    {
    }

    void GemItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return;
        }

        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        painter->setRenderHint(QPainter::Antialiasing);

        QRect fullRect, itemRect, contentRect;
        CalcRects(options, modelIndex, fullRect, itemRect, contentRect);

        QFont standardFont(options.font);
        standardFont.setPixelSize(s_fontSize);

        painter->save();
        painter->setClipping(true);
        painter->setClipRect(fullRect);
        painter->setFont(options.font);

        // Draw background
        painter->fillRect(fullRect, m_backgroundColor);

        // Draw item background
        const QColor itemBackgroundColor = options.state & QStyle::State_MouseOver ? m_itemBackgroundColor.lighter(120) : m_itemBackgroundColor;
        painter->fillRect(itemRect, itemBackgroundColor);

        // Draw border
        if (options.state & QStyle::State_Selected)
        {
            painter->save();
            QPen borderPen(m_borderColor);
            borderPen.setWidth(s_borderWidth);
            painter->setPen(borderPen);
            painter->drawRect(itemRect);
            painter->restore();
        }

        // Gem name
        const QString gemName = m_gemModel->GetName(modelIndex);
        QFont gemNameFont(options.font);
        gemNameFont.setPixelSize(s_gemNameFontSize);
        gemNameFont.setBold(true);
        QRect gemNameRect = GetTextRect(gemNameFont, gemName, s_gemNameFontSize);
        gemNameRect.moveTo(contentRect.left(), contentRect.top());

        painter->setFont(gemNameFont);
        painter->setPen(m_textColor);
        painter->drawText(gemNameRect, Qt::TextSingleLine, gemName);

        // Gem creator
        const QString gemCreator = m_gemModel->GetCreator(modelIndex);
        QRect gemCreatorRect = GetTextRect(standardFont, gemCreator, s_fontSize);
        gemCreatorRect.moveTo(contentRect.left(), contentRect.top() + gemNameRect.height());

        painter->setFont(standardFont);
        painter->setPen(m_linkColor);
        painter->drawText(gemCreatorRect, Qt::TextSingleLine, gemCreator);

        // Gem summary
        const QSize summarySize = QSize(contentRect.width() - s_summaryStartX - s_itemMargins.right() * 4, contentRect.height());
        const QRect summaryRect = QRect(/*topLeft=*/QPoint(contentRect.left() + s_summaryStartX, contentRect.top()), summarySize);

        painter->setFont(standardFont);
        painter->setPen(m_textColor);

        const QString summary = m_gemModel->GetSummary(modelIndex);
        painter->drawText(summaryRect, Qt::AlignLeft | Qt::TextWordWrap, summary);

        painter->restore();
    }

    QSize GemItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        int marginsHorizontal = s_itemMargins.left() + s_itemMargins.right() + s_contentMargins.left() + s_contentMargins.right();
        return QSize(marginsHorizontal + s_summaryStartX, s_height);
    }

    bool GemItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex)
    {
        if (!modelIndex.isValid())
        {
            return false;
        }

        return QStyledItemDelegate::editorEvent(event, model, option, modelIndex);
    }

    void GemItemDelegate::CalcRects(const QStyleOptionViewItem& option, const QModelIndex& modelIndex, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const
    {
        const bool isFirst = modelIndex.row() == 0;

        outFullRect = QRect(option.rect);
        outItemRect = QRect(outFullRect.adjusted(s_itemMargins.left(), isFirst ? s_itemMargins.top() * 2 : s_itemMargins.top(), -s_itemMargins.right(), -s_itemMargins.bottom()));
        outContentRect = QRect(outItemRect.adjusted(s_contentMargins.left(), s_contentMargins.top(), -s_contentMargins.right(), -s_contentMargins.bottom()));
    }

    QRect GemItemDelegate::GetTextRect(QFont& font, const QString& text, qreal fontSize) const
    {
        font.setPixelSize(fontSize);
        return QFontMetrics(font).boundingRect(text);
    }
} // namespace O3DE::ProjectManager

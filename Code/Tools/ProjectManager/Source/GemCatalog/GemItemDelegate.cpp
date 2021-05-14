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
        AddPlatformIcon(GemInfo::Android, ":/Resources/Android.svg");
        AddPlatformIcon(GemInfo::iOS, ":/Resources/iOS.svg");
        AddPlatformIcon(GemInfo::Linux, ":/Resources/Linux.svg");
        AddPlatformIcon(GemInfo::macOS, ":/Resources/macOS.svg");
        AddPlatformIcon(GemInfo::Windows, ":/Resources/Windows.svg");
    }

    void GemItemDelegate::AddPlatformIcon(GemInfo::Platform platform, const QString& iconPath)
    {
        QPixmap pixmap(iconPath);
        qreal aspectRatio = static_cast<qreal>(pixmap.width()) / pixmap.height();
        m_platformIcons.insert(platform, QIcon(iconPath).pixmap(s_platformIconSize * aspectRatio, s_platformIconSize));
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
        const QSize summarySize = QSize(contentRect.width() - s_summaryStartX - s_buttonWidth - s_itemMargins.right() * 4, contentRect.height());
        const QRect summaryRect = QRect(/*topLeft=*/QPoint(contentRect.left() + s_summaryStartX, contentRect.top()), summarySize);

        painter->setFont(standardFont);
        painter->setPen(m_textColor);

        const QString summary = m_gemModel->GetSummary(modelIndex);
        painter->drawText(summaryRect, Qt::AlignLeft | Qt::TextWordWrap, summary);


        DrawButton(painter, contentRect, modelIndex);
        DrawPlatformIcons(painter, contentRect, modelIndex);

        painter->restore();
    }

    QSize GemItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        int marginsHorizontal = s_itemMargins.left() + s_itemMargins.right() + s_contentMargins.left() + s_contentMargins.right();
        return QSize(marginsHorizontal + s_buttonWidth + s_summaryStartX, s_height);
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

    QRect GemItemDelegate::CalcButtonRect(const QRect& contentRect) const
    {
        const QPoint topLeft = QPoint(contentRect.right() - s_buttonWidth - s_itemMargins.right(), contentRect.top() + contentRect.height() / 2 - s_buttonHeight / 2);
        const QSize size = QSize(s_buttonWidth, s_buttonHeight);
        return QRect(topLeft, size);
    }

    void GemItemDelegate::DrawPlatformIcons(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const
    {
        const GemInfo::Platforms platforms = m_gemModel->GetPlatforms(modelIndex);
        int startX = 0;

        // Iterate and draw the platforms in the order they are defined in the enum.
        for (int i = 0; i < GemInfo::NumPlatforms; ++i)
        {
            // Check if the platform is supported by the given gem.
            const GemInfo::Platform platform = static_cast<GemInfo::Platform>(1 << i);
            if (platforms & platform)
            {
                // Get the icon for the platform and draw it.
                const auto iterator = m_platformIcons.find(platform);
                if (iterator != m_platformIcons.end())
                {
                    const QPixmap& pixmap = iterator.value();
                    painter->drawPixmap(contentRect.left() + startX, contentRect.bottom() - s_platformIconSize, pixmap);
                    qreal aspectRatio = static_cast<qreal>(pixmap.width()) / pixmap.height();
                    startX += s_platformIconSize * aspectRatio + s_platformIconSize / 2.5;
                }
            }
        }
    }

    void GemItemDelegate::DrawButton(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const
    {
        painter->save();
        const QRect buttonRect = CalcButtonRect(contentRect);
        QPoint circleCenter;
        QString buttonText;

        const bool isAdded = m_gemModel->IsAdded(modelIndex);
        if (isAdded)
        {
            painter->setBrush(m_buttonEnabledColor);
            painter->setPen(m_buttonEnabledColor);

            circleCenter = buttonRect.center() + QPoint(buttonRect.width() / 2 - s_buttonBorderRadius, 1);
            buttonText = "Added";
        }
        else
        {
            circleCenter = buttonRect.center() + QPoint(-buttonRect.width() / 2 + s_buttonBorderRadius + 1, 1);
            buttonText = "Get";
        }

        // Rounded rect
        painter->drawRoundedRect(buttonRect, s_buttonBorderRadius, s_buttonBorderRadius);

        // Text
        QFont font;
        QRect textRect = GetTextRect(font, buttonText, s_buttonFontSize);
        if (isAdded)
        {
            textRect = QRect(buttonRect.left(), buttonRect.top(), buttonRect.width() - s_buttonCircleRadius * 2.0, buttonRect.height());
        }
        else
        {
            textRect = QRect(buttonRect.left() + s_buttonCircleRadius * 2.0, buttonRect.top(), buttonRect.width() - s_buttonCircleRadius * 2.0, buttonRect.height());
        }

        font.setPixelSize(s_buttonFontSize);
        painter->setFont(font);
        painter->setPen(m_textColor);
        painter->drawText(textRect, Qt::AlignCenter, buttonText);

        // Circle
        painter->setBrush(m_textColor);
        painter->drawEllipse(circleCenter, s_buttonCircleRadius, s_buttonCircleRadius);

        painter->restore();
    }
} // namespace O3DE::ProjectManager

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemRequirementDelegate.h>
#include <GemCatalog/GemModel.h>

#include <QPainter>
#include <QMouseEvent>
#include <QDesktopServices>

namespace O3DE::ProjectManager
{
    GemRequirementDelegate::GemRequirementDelegate(QAbstractItemModel* model, QObject* parent)
        : GemItemDelegate(model, parent)
    {
    }

    void GemRequirementDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return;
        }

        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QRect fullRect, itemRect, contentRect;
        CalcRects(options, fullRect, itemRect, contentRect);

        QFont standardFont(options.font);
        standardFont.setPixelSize(static_cast<int>(s_fontSize));
        QFontMetrics standardFontMetrics(standardFont);

        painter->setClipping(true);
        painter->setClipRect(fullRect);
        painter->setFont(options.font);

        // Draw background
        painter->fillRect(fullRect, m_backgroundColor);

        // Draw item background
        const QColor itemBackgroundColor = m_itemBackgroundColor;
        painter->fillRect(itemRect, itemBackgroundColor);

        // Gem name
        QString gemName = GemModel::GetDisplayName(modelIndex);
        QFont gemNameFont(options.font);
        const int firstColumnMaxTextWidth = s_summaryStartX - 30;
        gemName = QFontMetrics(gemNameFont).elidedText(gemName, Qt::TextElideMode::ElideRight, firstColumnMaxTextWidth);
        gemNameFont.setPixelSize(static_cast<int>(s_gemNameFontSize));
        gemNameFont.setBold(true);
        QRect gemNameRect = GetTextRect(gemNameFont, gemName, s_gemNameFontSize);
        gemNameRect.moveTo(contentRect.left(), contentRect.center().y() - static_cast<int>(s_gemNameFontSize));

        painter->setFont(gemNameFont);
        painter->setPen(m_textColor);
        painter->drawText(gemNameRect, Qt::TextSingleLine, gemName);

        // Gem requirement
        const QRect requirementRect = CalcRequirementRect(contentRect);
        const QString requirement = GemModel::GetRequirement(modelIndex);
        DrawText(requirement, painter, requirementRect, standardFont);

        painter->restore();
    }

    QRect GemRequirementDelegate::CalcRequirementRect(const QRect& contentRect) const
    {
        const QSize requirementSize = QSize(contentRect.width() - s_summaryStartX - s_itemMargins.right(), contentRect.height());
        return QRect(QPoint(contentRect.left() + s_summaryStartX, contentRect.top()), requirementSize);
    }

    bool GemRequirementDelegate::editorEvent(
        [[maybe_unused]] QEvent* event,
        [[maybe_unused]] QAbstractItemModel* model,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& modelIndex)
    {
        if (!modelIndex.isValid())
        {
            return false;
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            QRect fullRect, itemRect, contentRect;
            CalcRects(option, fullRect, itemRect, contentRect);

            const QRect requirementsRect = CalcRequirementRect(contentRect);
            if (requirementsRect.contains(mouseEvent->pos()))
            {
                const QString html = GemModel::GetRequirement(modelIndex);
                QString anchor = anchorAt(html, mouseEvent->pos(), requirementsRect);
                if (!anchor.isEmpty())
                {
                    QDesktopServices::openUrl(QUrl(anchor));
                    return true;
                }
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, modelIndex);
    }

} // namespace O3DE::ProjectManager

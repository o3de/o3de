/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemRequirementDelegate.h>
#include <GemCatalog/GemModel.h>

#include <QPainter>

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
        standardFont.setPixelSize(s_fontSize);
        QFontMetrics standardFontMetrics(standardFont);

        painter->save();
        painter->setClipping(true);
        painter->setClipRect(fullRect);
        painter->setFont(options.font);

        // Draw background
        painter->fillRect(fullRect, m_backgroundColor);

        // Draw item background
        const QColor itemBackgroundColor = m_itemBackgroundColor;
        painter->fillRect(itemRect, itemBackgroundColor);

        // Gem name
        QString gemName = GemModel::GetName(modelIndex);
        QFont gemNameFont(options.font);
        const int firstColumnMaxTextWidth = s_summaryStartX - 30;
        gemName = QFontMetrics(gemNameFont).elidedText(gemName, Qt::TextElideMode::ElideRight, firstColumnMaxTextWidth);
        gemNameFont.setPixelSize(s_gemNameFontSize);
        gemNameFont.setBold(true);
        QRect gemNameRect = GetTextRect(gemNameFont, gemName, s_gemNameFontSize);
        gemNameRect.moveTo(contentRect.left(), contentRect.center().y() - s_gemNameFontSize);

        painter->setFont(gemNameFont);
        painter->setPen(m_textColor);
        painter->drawText(gemNameRect, Qt::TextSingleLine, gemName);

        // Gem requirement
        const QSize requirementSize = QSize(contentRect.width() - s_summaryStartX - s_itemMargins.right(), contentRect.height());
        const QRect requirementRect = QRect(QPoint(contentRect.left() + s_summaryStartX, contentRect.top()), requirementSize);

        painter->setFont(standardFont);
        painter->setPen(m_textColor);

        const QString requirement = GemModel::GetRequirement(modelIndex);
        painter->drawText(requirementRect, Qt::AlignLeft | Qt::TextWordWrap, requirement);

        painter->restore();
    }

    bool GemRequirementDelegate::editorEvent(
        [[maybe_unused]] QEvent* event,
        [[maybe_unused]] QAbstractItemModel* model,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& modelIndex)
    {
        // Do nothing here
        return false;
    }
} // namespace O3DE::ProjectManager

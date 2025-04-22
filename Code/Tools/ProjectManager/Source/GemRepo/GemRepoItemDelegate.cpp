/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoItemDelegate.h>
#include <GemRepo/GemRepoModel.h>
#include <ProjectManagerDefs.h>
#include <AdjustableHeaderWidget.h>

#include <QEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QHeaderView>
#include <QLocale>

namespace O3DE::ProjectManager
{
    GemRepoItemDelegate::GemRepoItemDelegate(QAbstractItemModel* model, AdjustableHeaderWidget* header, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_model(model)
        , m_headerWidget(header)
    {
        m_refreshIcon   = QIcon(":/Refresh.svg").pixmap(s_refreshIconSize, s_refreshIconSize);
        m_editIcon      = QIcon(":/Edit.svg").pixmap(s_iconSize, s_iconSize);
        m_deleteIcon    = QIcon(":/Delete.svg").pixmap(s_iconSize, s_iconSize);
        m_hiddenIcon    = QIcon(":/Hidden.svg").pixmap(s_iconSize, s_iconSize);
        m_visibleIcon   = QIcon(":/Visible.svg").pixmap(s_iconSize, s_iconSize);
        m_blueBadge   = QIcon(":/BannerBlue.svg").pixmap(s_badgeWidth, s_badgeHeight);
        m_greenBadge   = QIcon(":/BannerGreen.svg").pixmap(s_badgeWidth, s_badgeHeight);
    }

    void GemRepoItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return;
        }

        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        painter->setRenderHint(QPainter::Antialiasing);

        QRect fullRect, itemRect, contentRect;
        CalcRects(options, fullRect, itemRect, contentRect);

        QFont standardFont(options.font);
        standardFont.setPixelSize(static_cast<int>(s_fontSize));
        QFontMetrics standardFontMetrics(standardFont);

        QFont standardBoldFont(options.font);
        standardBoldFont.setPixelSize(static_cast<int>(s_fontSize));
        standardBoldFont.setBold(true);
        QFontMetrics standardFontBoldMetrics(standardFont);

        painter->save();
        painter->setClipping(true);
        painter->setClipRect(fullRect);
        painter->setFont(standardFont);
        painter->setPen(m_textColor);

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

        int currentHorizontalOffset = CalcColumnXBounds(HeaderOrder::Name).first + s_contentMargins.left() ;

        // Repo name
        QString repoName = GemRepoModel::GetName(modelIndex);
        int sectionSize = m_headerWidget->m_header->sectionSize(static_cast<int>(HeaderOrder::Name)) - s_contentMargins.left();
        repoName = standardFontMetrics.elidedText(repoName, Qt::TextElideMode::ElideRight,
            sectionSize - AdjustableHeaderWidget::s_headerTextIndent);

        QRect repoNameRect = GetTextRect(standardFont, repoName, s_fontSize);
        repoNameRect.moveTo(currentHorizontalOffset,
            contentRect.center().y() - repoNameRect.height() / 2);
        repoNameRect = painter->boundingRect(repoNameRect, Qt::TextSingleLine, repoName);

        painter->drawText(repoNameRect, Qt::TextSingleLine, repoName);

        // Repo creator
        currentHorizontalOffset += sectionSize;
        sectionSize = m_headerWidget->m_header->sectionSize(static_cast<int>(HeaderOrder::Creator));

        QString repoCreator = GemRepoModel::GetCreator(modelIndex);
        repoCreator = standardFontMetrics.elidedText(repoCreator, Qt::TextElideMode::ElideRight,
            sectionSize - AdjustableHeaderWidget::s_headerTextIndent);

        QRect repoCreatorRect = GetTextRect(standardFont, repoCreator, s_fontSize);
        repoCreatorRect.moveTo(currentHorizontalOffset + AdjustableHeaderWidget::s_headerTextIndent,
            contentRect.center().y() - repoCreatorRect.height() / 2);
        repoCreatorRect = painter->boundingRect(repoCreatorRect, Qt::TextSingleLine, repoCreator);

        painter->drawText(repoCreatorRect, Qt::TextSingleLine, repoCreator);

        // Badge
        currentHorizontalOffset += sectionSize;
        sectionSize = m_headerWidget->m_header->sectionSize(static_cast<int>(HeaderOrder::Badge));
        auto badgeType = GemRepoModel::GetBadgeType(modelIndex);
        const QPixmap* badge = nullptr;
        QString badgeText;
        if (badgeType == GemRepoInfo::BadgeType::BlueBadge)
        {
            badge = &m_blueBadge;
            badgeText = tr("O3DE Official");
        }
        else if (badgeType == GemRepoInfo::BadgeType::GreenBadge)
        {
            badge = &m_greenBadge;

            // this text should be made dynamic at some point
            badgeText = tr("O3DF Recommended");
        }

        if (badge)
        {
            const QRect badgeRect = CalcBadgeRect(contentRect);
            painter->drawPixmap(badgeRect, m_blueBadge);

            painter->setFont(standardBoldFont);

            QRect badgeLabelRect = GetTextRect(standardBoldFont, badgeText, s_fontSize);
            badgeLabelRect.moveTo(currentHorizontalOffset + s_badgeLeftMargin,
                contentRect.center().y() - (badgeLabelRect.height() / 2) - 1);
            badgeLabelRect = painter->boundingRect(badgeLabelRect, Qt::TextSingleLine, badgeText);
            painter->drawText(badgeLabelRect, Qt::TextSingleLine, badgeText);

            painter->setFont(standardFont);
        }

        // Last updated
        currentHorizontalOffset += sectionSize;
        sectionSize = m_headerWidget->m_header->sectionSize(static_cast<int>(HeaderOrder::Updated));
        auto lastUpdated = GemRepoModel::GetLastUpdated(modelIndex);

        // get the month day and year in the preferred locale's format (QLocale defaults to the OS locale)
        QString monthDayYear = lastUpdated.toString(QLocale().dateFormat(QLocale::ShortFormat));

        // always show 12 hour + minutes + am/pm
        QString hourMinuteAMPM = lastUpdated.toString("h:mmap");

        QString repoUpdatedDate = QString("%1 %2").arg(monthDayYear, hourMinuteAMPM);
        repoUpdatedDate = standardFontMetrics.elidedText(
            repoUpdatedDate, Qt::TextElideMode::ElideRight,
            sectionSize - AdjustableHeaderWidget::s_headerTextIndent);

        QRect repoUpdatedDateRect = GetTextRect(standardFont, repoUpdatedDate, s_fontSize);
        repoUpdatedDateRect.moveTo(currentHorizontalOffset + AdjustableHeaderWidget::s_headerTextIndent,
            contentRect.center().y() - repoUpdatedDateRect.height() / 2);
        repoUpdatedDateRect = painter->boundingRect(repoUpdatedDateRect, Qt::TextSingleLine, repoUpdatedDate);

        painter->drawText(repoUpdatedDateRect, Qt::TextSingleLine, repoUpdatedDate);

        // Refresh button
        const QRect refreshButtonRect = CalcRefreshButtonRect(contentRect);
        painter->drawPixmap(refreshButtonRect.topLeft(), m_refreshIcon);

        // Visibility button
        const QRect visibilityButtonRect = CalcVisibilityButtonRect(contentRect);
        painter->drawPixmap(visibilityButtonRect, GemRepoModel::IsEnabled(modelIndex) ? m_visibleIcon  : m_hiddenIcon);

        painter->restore();
    }

    QSize GemRepoItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        const int marginsHorizontal = s_itemMargins.left() + s_itemMargins.right() + s_contentMargins.left() + s_contentMargins.right();
        return QSize(marginsHorizontal + s_nameDefaultWidth + s_creatorDefaultWidth + s_buttonsDefaultWidth, s_height);
    }

    bool GemRepoItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex)
    {
        if (!modelIndex.isValid())
        {
            return false;
        }

        if (event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<const QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_X)
            {
                emit RemoveRepo(modelIndex);
                return true;
            }
            else if (keyEvent->key() == Qt::Key_R || keyEvent->key() == Qt::Key_F5)
            {
                emit RefreshRepo(modelIndex);
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            QRect fullRect, itemRect, contentRect;
            CalcRects(option, fullRect, itemRect, contentRect);
            const QRect visibilityButtonRect = CalcVisibilityButtonRect(contentRect);
            const QRect refreshButtonRect = CalcRefreshButtonRect(contentRect);

            if (visibilityButtonRect.contains(mouseEvent->pos()))
            {
                bool isAdded = GemRepoModel::IsEnabled(modelIndex);
                GemRepoModel::SetEnabled(*model, modelIndex, !isAdded);
                return true;
            }
            else if (refreshButtonRect.contains(mouseEvent->pos()))
            {
                emit RefreshRepo(modelIndex);
                return true;
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, modelIndex);
    }

    void GemRepoItemDelegate::CalcRects(const QStyleOptionViewItem& option, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const
    {
        outFullRect = QRect(option.rect);
        outItemRect = QRect(outFullRect.adjusted(s_itemMargins.left(), s_itemMargins.top(), -s_itemMargins.right(), -s_itemMargins.bottom()));
        outContentRect = QRect(outItemRect.adjusted(s_contentMargins.left(), s_contentMargins.top(), -s_contentMargins.right(), -s_contentMargins.bottom()));
    }

    QRect GemRepoItemDelegate::GetTextRect(QFont& font, const QString& text, qreal fontSize) const
    {
        font.setPixelSize(static_cast<int>(fontSize));
        return QFontMetrics(font).boundingRect(text);
    }

    QPair<int, int> GemRepoItemDelegate::CalcColumnXBounds(HeaderOrder header) const
    {
        return m_headerWidget->CalcColumnXBounds(static_cast<int>(header));
    }

    QRect GemRepoItemDelegate::CalcBadgeRect(const QRect& contentRect) const
    {
        const auto bounds = CalcColumnXBounds(HeaderOrder::Badge);
        const QPoint topLeft = QPoint(bounds.first, contentRect.center().y() - s_badgeHeight / 2);
        return QRect(topLeft, QSize(s_badgeWidth, s_badgeHeight));
    }

    QRect GemRepoItemDelegate::CalcVisibilityButtonRect(const QRect& contentRect) const
    {
        const auto bounds = CalcColumnXBounds(HeaderOrder::Buttons);
        const int centerX = (bounds.first + bounds.second) / 2;

        const QPoint topLeft = QPoint(centerX + s_refreshIconSpacing, contentRect.center().y() - s_iconSize / 2);
        return QRect(topLeft, QSize(s_iconSize, s_iconSize));
    }

    QRect GemRepoItemDelegate::CalcRefreshButtonRect(const QRect& contentRect) const
    {
        const auto bounds = CalcColumnXBounds(HeaderOrder::Buttons);
        const int centerX = (bounds.first + bounds.second) / 2;

        const QPoint topLeft = QPoint(centerX - s_refreshIconSpacing - s_refreshIconSize, contentRect.center().y() - s_refreshIconSize / 2 + 1);
        return QRect(topLeft, QSize(s_refreshIconSize, s_refreshIconSize));
    }

} // namespace O3DE::ProjectManager

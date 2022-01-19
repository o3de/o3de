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

        int currentHorizontalOffset = CalcHeaderXPos(HeaderOrder::Name);

        // Repo name
        QString repoName = GemRepoModel::GetName(modelIndex);
        int sectionSize = m_headerWidget->m_header->sectionSize(static_cast<int>(HeaderOrder::Name));
        repoName = standardFontMetrics.elidedText(repoName, Qt::TextElideMode::ElideRight,
            sectionSize - AdjustableHeaderWidget::s_headerTextIndent);

        QRect repoNameRect = GetTextRect(standardFont, repoName, s_fontSize);
        repoNameRect.moveTo(currentHorizontalOffset + AdjustableHeaderWidget::s_headerTextIndent,
            contentRect.center().y() - repoNameRect.height() / 2);
        repoNameRect = painter->boundingRect(repoNameRect, Qt::TextSingleLine, repoName);

        painter->drawText(repoNameRect, Qt::TextSingleLine, repoName);

        // Rem repo creator
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

        // Repo update
        currentHorizontalOffset += sectionSize;
        sectionSize = m_headerWidget->m_header->sectionSize(static_cast<int>(HeaderOrder::Update));

        QString repoUpdatedDate = GemRepoModel::GetLastUpdated(modelIndex).toString(RepoTimeFormat);
        repoUpdatedDate = standardFontMetrics.elidedText(
            repoUpdatedDate, Qt::TextElideMode::ElideRight,
            sectionSize - GemRepoItemDelegate::s_refreshIconSpacing - GemRepoItemDelegate::s_refreshIconSize - AdjustableHeaderWidget::s_headerTextIndent);

        QRect repoUpdatedDateRect = GetTextRect(standardFont, repoUpdatedDate, s_fontSize);
        repoUpdatedDateRect.moveTo(currentHorizontalOffset + AdjustableHeaderWidget::s_headerTextIndent,
            contentRect.center().y() - repoUpdatedDateRect.height() / 2);
        repoUpdatedDateRect = painter->boundingRect(repoUpdatedDateRect, Qt::TextSingleLine, repoUpdatedDate);

        painter->drawText(repoUpdatedDateRect, Qt::TextSingleLine, repoUpdatedDate);

        // Draw refresh button
        const QRect refreshButtonRect = CalcRefreshButtonRect(contentRect);
        painter->drawPixmap(refreshButtonRect.topLeft(), m_refreshIcon);

        if (options.state & QStyle::State_MouseOver)
        {
            DrawEditButtons(painter, contentRect);
        }

        painter->restore();
    }

    QSize GemRepoItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        const int marginsHorizontal = s_itemMargins.left() + s_itemMargins.right() + s_contentMargins.left() + s_contentMargins.right();
        return QSize(marginsHorizontal + s_nameDefaultWidth + s_creatorDefaultWidth + s_updatedDefaultWidth, s_height);
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
            const QRect deleteButtonRect = CalcDeleteButtonRect(contentRect);
            const QRect refreshButtonRect = CalcRefreshButtonRect(contentRect);

            if (deleteButtonRect.contains(mouseEvent->pos()))
            {
                emit RemoveRepo(modelIndex);
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

    int GemRepoItemDelegate::CalcHeaderXPos(HeaderOrder header, bool calcEnd) const
    {
        return m_headerWidget->CalcHeaderXPos(static_cast<int>(header), calcEnd);
    }

    QRect GemRepoItemDelegate::CalcDeleteButtonRect(const QRect& contentRect) const
    {
        const int deleteHeaderEndX = CalcHeaderXPos(HeaderOrder::Delete, /*calcEnd*/true);
        const QPoint topLeft = QPoint(deleteHeaderEndX - s_iconSize - s_contentMargins.right(), contentRect.center().y() - s_iconSize / 2);
        return QRect(topLeft, QSize(s_iconSize, s_iconSize));
    }

    QRect GemRepoItemDelegate::CalcRefreshButtonRect(const QRect& contentRect) const
    {
        const int headerEndX = CalcHeaderXPos(HeaderOrder::Update, /*calcEnd*/ true);
        const int leftX = headerEndX - s_refreshIconSize - s_refreshIconSpacing;
        // Dividing size by 3 centers much better
        const QPoint topLeft = QPoint(leftX, contentRect.center().y() - s_refreshIconSize / 3);
        return QRect(topLeft, QSize(s_refreshIconSize, s_refreshIconSize));
    }

    void GemRepoItemDelegate::DrawEditButtons(QPainter* painter, const QRect& contentRect) const
    {
        const QRect deleteButtonRect = CalcDeleteButtonRect(contentRect);
        painter->drawPixmap(deleteButtonRect, m_deleteIcon);
    }

} // namespace O3DE::ProjectManager

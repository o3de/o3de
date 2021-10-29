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

#include <QEvent>
#include <QPainter>
#include <QMouseEvent>

namespace O3DE::ProjectManager
{
    GemRepoItemDelegate::GemRepoItemDelegate(QAbstractItemModel* model, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_model(model)
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
        QRect buttonRect = CalcButtonRect(contentRect);

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

        // Repo enabled
        DrawButton(painter, buttonRect, modelIndex);

        // Repo name
        QString repoName = GemRepoModel::GetName(modelIndex);
        repoName = QFontMetrics(standardFont).elidedText(repoName, Qt::TextElideMode::ElideRight, s_nameMaxWidth);

        QRect repoNameRect = GetTextRect(standardFont, repoName, s_fontSize);
        int currentHorizontalOffset = buttonRect.left() + s_buttonWidth + s_buttonSpacing;
        repoNameRect.moveTo(currentHorizontalOffset, contentRect.center().y() - repoNameRect.height() / 2);
        repoNameRect = painter->boundingRect(repoNameRect, Qt::TextSingleLine, repoName);

        painter->drawText(repoNameRect, Qt::TextSingleLine, repoName);

        // Rem repo creator
        QString repoCreator = GemRepoModel::GetCreator(modelIndex);
        repoCreator = standardFontMetrics.elidedText(repoCreator, Qt::TextElideMode::ElideRight, s_creatorMaxWidth);

        QRect repoCreatorRect = GetTextRect(standardFont, repoCreator, s_fontSize);
        currentHorizontalOffset += s_nameMaxWidth + s_contentSpacing;
        repoCreatorRect.moveTo(currentHorizontalOffset, contentRect.center().y() - repoCreatorRect.height() / 2);
        repoCreatorRect = painter->boundingRect(repoCreatorRect, Qt::TextSingleLine, repoCreator);

        painter->drawText(repoCreatorRect, Qt::TextSingleLine, repoCreator);

        // Repo update
        QString repoUpdatedDate = GemRepoModel::GetLastUpdated(modelIndex).toString(RepoTimeFormat);
        repoUpdatedDate = standardFontMetrics.elidedText(repoUpdatedDate, Qt::TextElideMode::ElideRight, s_updatedMaxWidth);

        QRect repoUpdatedDateRect = GetTextRect(standardFont, repoUpdatedDate, s_fontSize);
        currentHorizontalOffset += s_creatorMaxWidth + s_contentSpacing;
        repoUpdatedDateRect.moveTo(currentHorizontalOffset, contentRect.center().y() - repoUpdatedDateRect.height() / 2);
        repoUpdatedDateRect = painter->boundingRect(repoUpdatedDateRect, Qt::TextSingleLine, repoUpdatedDate);

        painter->drawText(repoUpdatedDateRect, Qt::TextSingleLine, repoUpdatedDate);

        // Draw refresh button
        painter->drawPixmap(
            repoUpdatedDateRect.left() + s_updatedMaxWidth + s_refreshIconSpacing,
            contentRect.center().y() - s_refreshIconSize / 3, // Dividing size by 3 centers much better
            m_refreshIcon);

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

        int marginsHorizontal = s_itemMargins.left() + s_itemMargins.right() + s_contentMargins.left() + s_contentMargins.right();
        return QSize(marginsHorizontal + s_buttonWidth + s_buttonSpacing + s_nameMaxWidth + s_creatorMaxWidth + s_updatedMaxWidth + s_contentSpacing * 3, s_height);
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
            if (keyEvent->key() == Qt::Key_Space)
            {
                const bool isAdded = GemRepoModel::IsEnabled(modelIndex);
                GemRepoModel::SetEnabled(*model, modelIndex, !isAdded);
                return true;
            }
            else if (keyEvent->key() == Qt::Key_X)
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
            const QRect buttonRect = CalcButtonRect(contentRect);
            const QRect deleteButtonRect = CalcDeleteButtonRect(contentRect);
            const QRect refreshButtonRect = CalcRefreshButtonRect(contentRect, buttonRect);

            if (buttonRect.contains(mouseEvent->pos()))
            {
                const bool isAdded = GemRepoModel::IsEnabled(modelIndex);
                GemRepoModel::SetEnabled(*model, modelIndex, !isAdded);
                return true;
            }
            else if (deleteButtonRect.contains(mouseEvent->pos()))
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

    QRect GemRepoItemDelegate::CalcButtonRect(const QRect& contentRect) const
    {
        const QPoint topLeft = QPoint(contentRect.left(), contentRect.top() + contentRect.height() / 2 - s_buttonHeight / 2);
        const QSize size = QSize(s_buttonWidth, s_buttonHeight);
        return QRect(topLeft, size);
    }

    void GemRepoItemDelegate::DrawButton(QPainter* painter, const QRect& buttonRect, const QModelIndex& modelIndex) const
    {
        painter->save();
        QPoint circleCenter;

        const bool isEnabled = GemRepoModel::IsEnabled(modelIndex);
        if (isEnabled)
        {
            painter->setBrush(m_buttonEnabledColor);
            painter->setPen(m_buttonEnabledColor);

            circleCenter = buttonRect.center() + QPoint(buttonRect.width() / 2 - s_buttonBorderRadius + 1, 1);
        }
        else
        {
            circleCenter = buttonRect.center() + QPoint(-buttonRect.width() / 2 + s_buttonBorderRadius + 1, 1);
        }

        // Rounded rect
        painter->drawRoundedRect(buttonRect, s_buttonBorderRadius, s_buttonBorderRadius);

        // Circle
        painter->setBrush(m_textColor);
        painter->drawEllipse(circleCenter, s_buttonCircleRadius, s_buttonCircleRadius);

        painter->restore();
    }

    QRect GemRepoItemDelegate::CalcDeleteButtonRect(const QRect& contentRect) const
    {
        const QPoint topLeft = QPoint(contentRect.right() - s_iconSize, contentRect.center().y() - s_iconSize / 2);
        return QRect(topLeft, QSize(s_iconSize, s_iconSize));
    }

    QRect GemRepoItemDelegate::CalcRefreshButtonRect(const QRect& contentRect, const QRect& buttonRect) const
    {
        const int topLeftX = buttonRect.left() + s_buttonWidth + s_buttonSpacing + s_nameMaxWidth + s_creatorMaxWidth + s_updatedMaxWidth + s_contentSpacing * 2 + s_refreshIconSpacing;
        const QPoint topLeft = QPoint(topLeftX, contentRect.center().y() - s_refreshIconSize / 3);
        return QRect(topLeft, QSize(s_refreshIconSize, s_refreshIconSize));
    }

    void GemRepoItemDelegate::DrawEditButtons(QPainter* painter, const QRect& contentRect) const
    {
        painter->drawPixmap(contentRect.right() - s_iconSize, contentRect.center().y() - s_iconSize / 2, m_deleteIcon);
    }

} // namespace O3DE::ProjectManager

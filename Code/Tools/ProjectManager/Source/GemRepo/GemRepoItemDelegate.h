/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QStyledItemDelegate>
#include <GemRepo/GemRepoInfo.h>
#endif

QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
QT_FORWARD_DECLARE_CLASS(QEvent)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(AdjustableHeaderWidget)

    class GemRepoItemDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit GemRepoItemDelegate(QAbstractItemModel* model, AdjustableHeaderWidget* header, QObject* parent = nullptr);
        ~GemRepoItemDelegate() = default;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;

        // Colors
        const QColor m_textColor = QColor("#FFFFFF");
        const QColor m_backgroundColor = QColor("#333333"); // Outside of the actual repo item
        const QColor m_itemBackgroundColor = QColor("#404040"); // Background color of the repo item
        const QColor m_borderColor = QColor("#1E70EB");

        // Item
        inline constexpr static int s_height = 72; // Repo item total height
        inline constexpr static qreal s_fontSize = 12.0;

        // Margin and borders
        inline constexpr static QMargins s_itemMargins = QMargins(/*left=*/0, /*top=*/8, /*right=*/0, /*bottom=*/8); // Item border distances
        inline constexpr static QMargins s_contentMargins = QMargins(/*left=*/20, /*top=*/20, /*right=*/20, /*bottom=*/20); // Distances of the elements within an item to the item borders
        inline constexpr static int s_borderWidth = 4;

        // Content TableView is ~842px minimum
        inline constexpr static int s_nameDefaultWidth = 200;
        inline constexpr static int s_creatorDefaultWidth = 240;
        inline constexpr static int s_badgeDefaultWidth = 150;
        inline constexpr static int s_updatedDefaultWidth = 130;
        inline constexpr static int s_buttonsDefaultWidth = 80;

        // Icon
        inline constexpr static int s_iconSize = 20;
        inline constexpr static int s_iconSpacing = 16;
        inline constexpr static int s_refreshIconSize = 16;
        inline constexpr static int s_refreshIconSpacing = 10;

        inline constexpr static int s_badgeWidth = 130;
        inline constexpr static int s_badgeHeight = 30;
        inline constexpr static int s_badgeLeftMargin = 25;

        enum class HeaderOrder
        {
            Name,
            Creator,
            Badge,
            Updated,
            Buttons 
        };

    signals:
        void RemoveRepo(const QModelIndex& modelIndex);
        void RefreshRepo(const QModelIndex& modelIndex);

    protected:
        void CalcRects(const QStyleOptionViewItem& option, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const;
        QRect GetTextRect(QFont& font, const QString& text, qreal fontSize) const;
        QPair<int, int> CalcColumnXBounds(HeaderOrder header) const;
        QRect CalcBadgeRect(const QRect& contentRect) const;
        QRect CalcVisibilityButtonRect(const QRect& contentRect) const;
        QRect CalcRefreshButtonRect(const QRect& contentRect) const;

        QAbstractItemModel* m_model = nullptr;

        AdjustableHeaderWidget* m_headerWidget = nullptr;

        QPixmap m_refreshIcon;
        QPixmap m_editIcon;
        QPixmap m_deleteIcon;
        QPixmap m_hiddenIcon;
        QPixmap m_visibleIcon;
        QPixmap m_blueBadge;
        QPixmap m_greenBadge;
    };
} // namespace O3DE::ProjectManager

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
#include "GemInfo.h"
#include <QAbstractItemModel>
#include <QHash>
#endif

QT_FORWARD_DECLARE_CLASS(QEvent)

namespace O3DE::ProjectManager
{
    class GemItemDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemItemDelegate(QAbstractItemModel* model, QObject* parent = nullptr);
        ~GemItemDelegate() = default;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        virtual QString anchorAt(const QString& html, const QPoint& position, const QRect& rect);

        // Colors
        const QColor m_textColor = QColor("#FFFFFF");
        const QColor m_linkColor = QColor("#94D2FF");
        const QColor m_backgroundColor = QColor("#333333"); // Outside of the actual gem item
        const QColor m_itemBackgroundColor = QColor("#404040"); // Background color of the gem item
        const QColor m_borderColor = QColor("#1E70EB");
        const QColor m_buttonEnabledColor = QColor("#00B931");
        const QColor m_buttonImplicitlyEnabledColor = QColor("#BCBCBE");

        // Item
        inline constexpr static int s_height = 105; // Gem item total height
        inline constexpr static qreal s_gemNameFontSize = 13.0;
        inline constexpr static qreal s_fontSize = 12.0;
        inline constexpr static int s_summaryStartX = 150;

        // Margin and borders
        inline constexpr static QMargins s_itemMargins = QMargins(/*left=*/16, /*top=*/8, /*right=*/16, /*bottom=*/8); // Item border distances
        inline constexpr static QMargins s_contentMargins = QMargins(/*left=*/20, /*top=*/12, /*right=*/20, /*bottom=*/12); // Distances of the elements within an item to the item borders
        inline constexpr static int s_borderWidth = 4;

        // Button
        inline constexpr static int s_buttonWidth = 32;
        inline constexpr static int s_buttonHeight = 16;
        inline constexpr static int s_buttonBorderRadius = s_buttonHeight / 2;
        inline constexpr static int s_buttonCircleRadius = s_buttonBorderRadius - 2;
        inline constexpr static qreal s_buttonFontSize = 10.0;

        // Feature tags
        inline constexpr static int s_featureTagFontSize = 10;
        inline constexpr static int s_featureTagBorderMarginX = 3;
        inline constexpr static int s_featureTagBorderMarginY = 3;
        inline constexpr static int s_featureTagSpacing = 7;

    signals:
        void MovieStartedPlaying(const QMovie* playingMovie) const;

    protected:
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) override;
        bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override;

        void CalcRects(const QStyleOptionViewItem& option, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const;
        QRect GetTextRect(QFont& font, const QString& text, qreal fontSize) const;
        QRect CalcButtonRect(const QRect& contentRect) const;
        QRect CalcSummaryRect(const QRect& contentRect, bool hasTags) const;
        void DrawPlatformIcons(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const;
        void DrawButton(QPainter* painter, const QRect& buttonRect, const QModelIndex& modelIndex) const;
        void DrawFeatureTags(QPainter* painter, const QRect& contentRect, const QStringList& featureTags, const QFont& standardFont, const QRect& summaryRect) const;
        void DrawText(const QString& text, QPainter* painter, const QRect& rect, const QFont& standardFont) const;
        void DrawDownloadStatusIcon(QPainter* painter, const QRect& contentRect, const QRect& buttonRect, const QModelIndex& modelIndex) const;

        QAbstractItemModel* m_model = nullptr;

    private:
        // Platform icons
        void AddPlatformIcon(GemInfo::Platform platform, const QString& iconPath);
        inline constexpr static int s_platformIconSize = 12;
        QHash<GemInfo::Platform, QPixmap> m_platformIcons;

        // Status icons
        void SetStatusIcon(QPixmap& m_iconPixmap, const QString& iconPath);
        inline constexpr static int s_statusIconSize = 16;
        inline constexpr static int s_statusButtonSpacing = 5;

        QPixmap m_unknownStatusPixmap;
        QPixmap m_notDownloadedPixmap;
        QPixmap m_downloadSuccessfulPixmap;
        QPixmap m_downloadFailedPixmap;
        QMovie* m_downloadingMovie = nullptr;
    };
} // namespace O3DE::ProjectManager

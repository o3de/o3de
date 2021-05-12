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

#pragma once

#if !defined(Q_MOC_RUN)
#include <QStyledItemDelegate>
#include "GemInfo.h"
#include "GemModel.h"
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
        explicit GemItemDelegate(GemModel* gemModel, QObject* parent = nullptr);
        ~GemItemDelegate() = default;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;

    private:
        void CalcRects(const QStyleOptionViewItem& option, const QModelIndex& modelIndex, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const;
        QRect GetTextRect(QFont& font, const QString& text, qreal fontSize) const;
        QRect CalcButtonRect(const QRect& contentRect) const;
        void DrawPlatformIcons(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const;
        void DrawButton(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const;

        GemModel* m_gemModel = nullptr;

        // Colors
        const QColor m_textColor = QColor("#FFFFFF");
        const QColor m_linkColor = QColor("#94D2FF");
        const QColor m_backgroundColor = QColor("#333333"); // Outside of the actual gem item
        const QColor m_itemBackgroundColor = QColor("#404040"); // Background color of the gem item
        const QColor m_borderColor = QColor("#1E70EB");
        const QColor m_buttonEnabledColor = QColor("#00B931");

        // Item
        inline constexpr static int s_height = 135; // Gem item total height
        inline constexpr static qreal s_gemNameFontSize = 16.0;
        inline constexpr static qreal s_fontSize = 15.0;
        inline constexpr static int s_summaryStartX = 200;

        // Margin and borders
        inline constexpr static QMargins s_itemMargins = QMargins(/*left=*/20, /*top=*/10, /*right=*/20, /*bottom=*/10); // Item border distances
        inline constexpr static QMargins s_contentMargins = QMargins(/*left=*/15, /*top=*/12, /*right=*/12, /*bottom=*/12); // Distances of the elements within an item to the item borders
        inline constexpr static int s_borderWidth = 4;

        // Button
        inline constexpr static int s_buttonWidth = 70;
        inline constexpr static int s_buttonHeight = 24;
        inline constexpr static int s_buttonBorderRadius = 12;
        inline constexpr static int s_buttonCircleRadius = s_buttonBorderRadius - 3;
        inline constexpr static qreal s_buttonFontSize = 12.0;

        // Platform icons
        void AddPlatformIcon(GemInfo::Platform platform, const QString& iconPath);
        inline constexpr static int s_platformIconSize = 16;
        QHash<GemInfo::Platform, QPixmap> m_platformIcons;
    };
} // namespace O3DE::ProjectManager

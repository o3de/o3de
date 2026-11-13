/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include "GemInfo.h"

#include <QStyledItemDelegate>
#include <QAbstractItemModel>
#include <QColor>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QEvent)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(AdjustableHeaderWidget)

    //! The paint values of the Gem Catalog rows that the flattened GUI owns; the members of GemItemDelegate below take their value from
    //! here. A named namespace, not an anonymous one: a unity build joins the files of a target into one translation unit, where
    //! anonymous namespaces merge.
    namespace GemItemDelegateFlat
    {
        // ------------------------------------------------------------------------------
        // The rows of the Gem Catalog list (GemItemDelegate::paint)
        // ------------------------------------------------------------------------------
        inline constexpr QColor ItemText(0xFF, 0xFF, 0xFF);             // GemItemDelegate.h:36 (text, knob). TOKENIZE: GemItemText
        inline constexpr QColor ItemLink(0x94, 0xD2, 0xFF);             // GemItemDelegate.h:37 (read nowhere). TOKENIZE: GemItemLink
        inline constexpr QColor ListSurface(0x33, 0x33, 0x33);          // GemItemDelegate.h:38 (outside). TOKENIZE: GemItemListSurface
        inline constexpr QColor ItemFill(0x40, 0x40, 0x40);             // GemItemDelegate.h:39 (item). TOKENIZE: GemItemFill
        inline constexpr QColor SelectedBorder(0x1E, 0x70, 0xEB);       // GemItemDelegate.h:40. TOKENIZE: GemItemSelectedBorder
        inline constexpr QColor ToggleTrackAdded(0x00, 0xB9, 0x31);     // GemItemDelegate.h:41. TOKENIZE: GemItemToggleTrackAdded
        inline constexpr QColor ToggleTrackImplicit(0xBC, 0xBC, 0xBE);  // GemItemDelegate.h:42. TOKENIZE: GemItemToggleTrackImplicit
    } // namespace GemItemDelegateFlat

    class GemItemDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit GemItemDelegate(QAbstractItemModel* model, AdjustableHeaderWidget* header, bool readOnly, QObject* parent = nullptr);
        ~GemItemDelegate() = default;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        virtual QString anchorAt(const QString& html, const QPoint& position, const QRect& rect);

        // Colors
        const QColor m_textColor = GemItemDelegateFlat::ItemText;
        const QColor m_linkColor = GemItemDelegateFlat::ItemLink;
        const QColor m_backgroundColor = GemItemDelegateFlat::ListSurface; // Outside of the actual gem item
        const QColor m_itemBackgroundColor = GemItemDelegateFlat::ItemFill; // Background color of the gem item
        const QColor m_borderColor = GemItemDelegateFlat::SelectedBorder;
        const QColor m_buttonEnabledColor = GemItemDelegateFlat::ToggleTrackAdded;
        const QColor m_buttonImplicitlyEnabledColor = GemItemDelegateFlat::ToggleTrackImplicit;

        // Item
        inline constexpr static int s_height = 105; // Gem item total height
        inline constexpr static qreal s_gemNameFontSize = 13.0;
        inline constexpr static qreal s_fontSize = 12.0;
        inline constexpr static int s_defaultSummaryStartX = 270;

        // Margin and borders
        inline constexpr static QMargins s_itemMargins = QMargins(/*left=*/16, /*top=*/5, /*right=*/16, /*bottom=*/5); // Item border distances
        inline constexpr static QMargins s_contentMargins = QMargins(/*left=*/10, /*top=*/12, /*right=*/30, /*bottom=*/12); // Distances of the elements within an item to the item borders
        inline constexpr static int s_borderWidth = 4;
        inline constexpr static int s_extraSummarySpacing = s_itemMargins.right();

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

        // Platform text
        inline constexpr static int s_platformTextleftMarginCorrection = -3;
        inline constexpr static int s_platformTextHeightAdjustment = 25;
        inline constexpr static int s_platformTextWrapAroundMargin = 5;
        inline constexpr static int s_platformTextLineBottomMargin = 5;
        inline constexpr static int s_platformTextWrapAroundLineMaxCount = 2;

        // Version
        inline constexpr static int s_versionSize = 70;
        inline constexpr static int s_versionSizeSpacing = 25;

        // Status icon
        inline constexpr static int s_statusIconSize = 16;
        inline constexpr static int s_statusIconSizeLarge = 20;
        inline constexpr static int s_statusButtonSpacing = 5;

        enum class HeaderOrder
        {
            Preview,
            Name,
            Summary,
            Version,
            Status
        };

    signals:
        void MovieStartedPlaying(const QMovie* playingMovie) const;

    protected:
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) override;
        bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override;

        void CalcRects(const QStyleOptionViewItem& option, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const;
        QRect GetTextRect(QFont& font, const QString& text, qreal fontSize) const;
        QPair<int, int> CalcColumnXBounds(HeaderOrder header) const;
        QRect CalcButtonRect(const QRect& contentRect) const;
        QRect CalcSummaryRect(const QRect& contentRect, bool hasTags) const;
        void DrawPlatformIcons(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const;
        void DrawPlatformText(QPainter* painter, const QRect& contentRect,  const QFont& standardFont, const QModelIndex& modelIndex) const;
        void DrawButton(QPainter* painter, const QRect& buttonRect, const QModelIndex& modelIndex) const;
        void DrawFeatureTags(
            QPainter* painter,
            const QRect& contentRect,
            const QStringList& featureTags,
            const QFont& standardFont,
            const QRect& summaryRect) const;
        void DrawText(const QString& text, QPainter* painter, const QRect& rect, const QFont& standardFont) const;
        void DrawDownloadStatusIcon(
            QPainter* painter, const QRect& contentRect, const QRect& buttonRect, const QModelIndex& modelIndex) const;

        QAbstractItemModel* m_model = nullptr;

    private:
        // Platform icons
        void AddPlatformIcon(GemInfo::Platform platform, const QString& iconPath);
        inline constexpr static int s_platformIconSize = 12;
        QHash<GemInfo::Platform, QPixmap> m_platformIcons;

        // Status icons
        void SetStatusIcon(QPixmap& m_iconPixmap, const QString& iconPath);

        QPixmap m_unknownStatusPixmap;
        QPixmap m_notDownloadedPixmap;
        QPixmap m_downloadedPixmap;
        QPixmap m_downloadSuccessfulPixmap;
        QPixmap m_downloadFailedPixmap;
        QMovie* m_downloadingMovie = nullptr;
        QPixmap m_updatePixmap;
        bool m_readOnly = false;

        AdjustableHeaderWidget* m_headerWidget = nullptr;
    };
} // namespace O3DE::ProjectManager

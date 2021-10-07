/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemItemDelegate.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <QEvent>
#include <QAbstractItemView>
#include <QPainter>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QHoverEvent>

namespace O3DE::ProjectManager
{
    GemItemDelegate::GemItemDelegate(QAbstractItemModel* model, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_model(model)
    {
        AddPlatformIcon(GemInfo::Android, ":/Android.svg");
        AddPlatformIcon(GemInfo::iOS, ":/iOS.svg");
        AddPlatformIcon(GemInfo::Linux, ":/Linux.svg");
        AddPlatformIcon(GemInfo::macOS, ":/macOS.svg");
        AddPlatformIcon(GemInfo::Windows, ":/Windows.svg");
    }

    void GemItemDelegate::AddPlatformIcon(GemInfo::Platform platform, const QString& iconPath)
    {
        QPixmap pixmap(iconPath);
        qreal aspectRatio = static_cast<qreal>(pixmap.width()) / pixmap.height();
        m_platformIcons.insert(platform, QIcon(iconPath).pixmap(static_cast<int>(static_cast<qreal>(s_platformIconSize) * aspectRatio), s_platformIconSize));
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
        CalcRects(options, fullRect, itemRect, contentRect);

        QFont standardFont(options.font);
        standardFont.setPixelSize(static_cast<int>(s_fontSize));
        QFontMetrics standardFontMetrics(standardFont);

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
        QString gemName = GemModel::GetDisplayName(modelIndex);
        QFont gemNameFont(options.font);
        const int firstColumnMaxTextWidth = s_summaryStartX - 30;
        gemNameFont.setPixelSize(static_cast<int>(s_gemNameFontSize));
        gemNameFont.setBold(true);
        gemName = QFontMetrics(gemNameFont).elidedText(gemName, Qt::TextElideMode::ElideRight, firstColumnMaxTextWidth);
        QRect gemNameRect = GetTextRect(gemNameFont, gemName, s_gemNameFontSize);
        gemNameRect.moveTo(contentRect.left(), contentRect.top());
        painter->setFont(gemNameFont);
        painter->setPen(m_textColor);
        gemNameRect = painter->boundingRect(gemNameRect, Qt::TextSingleLine, gemName);
        painter->drawText(gemNameRect, Qt::TextSingleLine, gemName);

        // Gem creator
        QString gemCreator = GemModel::GetCreator(modelIndex);
        gemCreator = standardFontMetrics.elidedText(gemCreator, Qt::TextElideMode::ElideRight, firstColumnMaxTextWidth);
        QRect gemCreatorRect = GetTextRect(standardFont, gemCreator, s_fontSize);
        gemCreatorRect.moveTo(contentRect.left(), contentRect.top() + gemNameRect.height());

        painter->setFont(standardFont);
        gemCreatorRect = painter->boundingRect(gemCreatorRect, Qt::TextSingleLine, gemCreator);
        painter->drawText(gemCreatorRect, Qt::TextSingleLine, gemCreator);

        // Gem summary

        // In case there are feature tags displayed at the bottom, decrease the size of the summary text field.
        const QStringList featureTags = GemModel::GetFeatures(modelIndex);
        const int featureTagAreaHeight = 30;
        const int summaryHeight = contentRect.height() - (!featureTags.empty() * featureTagAreaHeight);

        const int additionalSummarySpacing = s_itemMargins.right() * 3;
        const QSize summarySize = QSize(contentRect.width() - s_summaryStartX - s_buttonWidth - additionalSummarySpacing,
            summaryHeight);
        const QRect summaryRect = QRect(/*topLeft=*/QPoint(contentRect.left() + s_summaryStartX, contentRect.top()), summarySize);

        painter->setFont(standardFont);
        painter->setPen(m_textColor);

        const QString summary = GemModel::GetSummary(modelIndex);
        painter->drawText(summaryRect, Qt::AlignLeft | Qt::TextWordWrap, summary);

        DrawButton(painter, contentRect, modelIndex);
        DrawPlatformIcons(painter, contentRect, modelIndex);
        DrawFeatureTags(painter, contentRect, featureTags, standardFont, summaryRect);

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

        if (event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<const QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Space)
            {
                const bool isAdded = GemModel::IsAdded(modelIndex);
                GemModel::SetIsAdded(*model, modelIndex, !isAdded);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonPress )
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            QRect fullRect, itemRect, contentRect;
            CalcRects(option, fullRect, itemRect, contentRect);
            const QRect buttonRect = CalcButtonRect(contentRect);

            if (buttonRect.contains(mouseEvent->pos()))
            {
                const bool isAdded = GemModel::IsAdded(modelIndex);
                GemModel::SetIsAdded(*model, modelIndex, !isAdded);
                return true;
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, modelIndex);
    }

    QString GetGemNameList(const QVector<QModelIndex> modelIndices)
    {
        QString gemNameList;
        for (int i = 0; i < modelIndices.size(); ++i)
        {
            if (!gemNameList.isEmpty())
            {
                if (i == modelIndices.size() - 1)
                {
                    gemNameList.append(" and ");
                }
                else
                {
                    gemNameList.append(", ");
                }
            }

            gemNameList.append(GemModel::GetDisplayName(modelIndices[i]));
        }

        return gemNameList;
    }

    bool GemItemDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QRect fullRect, itemRect, contentRect;
            CalcRects(option, fullRect, itemRect, contentRect);
            const QRect buttonRect = CalcButtonRect(contentRect);
            if (buttonRect.contains(event->pos()))
            {
                if (!QToolTip::isVisible())
                {
                    if(GemModel::IsAddedDependency(index) && !GemModel::IsAdded(index))
                    {
                        const GemModel* gemModel = GemModel::GetSourceModel(index.model());
                        AZ_Assert(gemModel, "Failed to obtain GemModel");

                        // we only want to display the gems that must be de-selected to automatically
                        // disable this dependency, so don't include any that haven't been selected (added) 
                        constexpr bool addedOnly = true;
                        QVector<QModelIndex> dependents = gemModel->GatherDependentGems(index, addedOnly);
                        QString nameList = GetGemNameList(dependents);
                        if (!nameList.isEmpty())
                        {
                            QToolTip::showText(event->globalPos(), tr("This gem is a dependency of %1.\nTo disable this gem, first disable %1.").arg(nameList));
                        }
                    }
                }
                return true;
            }
            else if (QToolTip::isVisible())
            {
                QToolTip::hideText();
                event->ignore();
                return true;
            }
        }

        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }

    void GemItemDelegate::CalcRects(const QStyleOptionViewItem& option, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const
    {
        outFullRect = QRect(option.rect);
        outItemRect = QRect(outFullRect.adjusted(s_itemMargins.left(), s_itemMargins.top(), -s_itemMargins.right(), -s_itemMargins.bottom()));
        outContentRect = QRect(outItemRect.adjusted(s_contentMargins.left(), s_contentMargins.top(), -s_contentMargins.right(), -s_contentMargins.bottom()));
    }

    QRect GemItemDelegate::GetTextRect(QFont& font, const QString& text, qreal fontSize) const
    {
        font.setPixelSize(static_cast<int>(fontSize));
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
        const GemInfo::Platforms platforms = GemModel::GetPlatforms(modelIndex);
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
                    startX += static_cast<int>(s_platformIconSize * aspectRatio + s_platformIconSize / 2.5);
                }
            }
        }
    }

    void GemItemDelegate::DrawFeatureTags(QPainter* painter, const QRect& contentRect, const QStringList& featureTags, const QFont& standardFont, const QRect& summaryRect) const
    {
        QFont gemFeatureTagFont(standardFont);
        gemFeatureTagFont.setPixelSize(s_featureTagFontSize);
        gemFeatureTagFont.setBold(false);
        painter->setFont(gemFeatureTagFont);

        int x = s_summaryStartX;
        for (const QString& featureTag : featureTags)
        {
            QRect featureTagRect = GetTextRect(gemFeatureTagFont, featureTag, s_featureTagFontSize);
            featureTagRect.moveTo(contentRect.left() + x + s_featureTagBorderMarginX,
                contentRect.top() + 47);
            featureTagRect = painter->boundingRect(featureTagRect, Qt::TextSingleLine, featureTag);

            QRect backgroundRect = featureTagRect;
            backgroundRect = backgroundRect.adjusted(/*left=*/-s_featureTagBorderMarginX,
                /*top=*/-s_featureTagBorderMarginY,
                /*right=*/s_featureTagBorderMarginX,
                /*bottom=*/s_featureTagBorderMarginY);

            // Skip drawing all following feature tags as there is no more space available.
            if (backgroundRect.right() > summaryRect.right())
            {
                break;
            }

            // Draw border.
            painter->setPen(m_textColor);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(backgroundRect);

            // Draw text within the border.
            painter->setPen(m_textColor);
            painter->drawText(featureTagRect, Qt::TextSingleLine, featureTag);

            x += backgroundRect.width() + s_featureTagSpacing;
        }
    }

    void GemItemDelegate::DrawButton(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const
    {
        painter->save();
        const QRect buttonRect = CalcButtonRect(contentRect);
        QPoint circleCenter;

        if (GemModel::IsAdded(modelIndex))
        {
            painter->setBrush(m_buttonEnabledColor);
            painter->setPen(m_buttonEnabledColor);

            circleCenter = buttonRect.center() + QPoint(buttonRect.width() / 2 - s_buttonBorderRadius + 1, 1);
        }
        else if (GemModel::IsAddedDependency(modelIndex))
        {
            painter->setBrush(m_buttonImplicitlyEnabledColor);
            painter->setPen(m_buttonImplicitlyEnabledColor);

            circleCenter = buttonRect.center() + QPoint(buttonRect.width() / 2 - s_buttonBorderRadius + 1, 1);
        }
        else
        {
            circleCenter = buttonRect.center() + QPoint(-buttonRect.width() / 2 + s_buttonBorderRadius, 1);
        }

        // Rounded rect
        painter->drawRoundedRect(buttonRect, s_buttonBorderRadius, s_buttonBorderRadius);

        // Circle
        painter->setBrush(m_textColor);
        painter->drawEllipse(circleCenter, s_buttonCircleRadius, s_buttonCircleRadius);

        painter->restore();
    }
} // namespace O3DE::ProjectManager
